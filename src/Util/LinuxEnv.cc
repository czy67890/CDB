/*!
 * \file LinuxEnv.h
 *
 * \author czy
 * \date 2023.07.30
 *
 * 
 */
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifndef __Fuchsia__
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "CDataBase/Env.h"
#include "CDataBase/Slice.h"
#include "CDataBase/Status.h"
#include "Util/ThreadAnnotations.h"
#include "Util/PosixLogger.h"
#include "Util/MutexLock.h"
namespace CDB {
	namespace {
		//can be set by setReadOnlyMapLimit() and maxOpenFileLimit
		int gOpenReadOnlyFileLimit = 1;

		// 64bit or more can mmap 1000 ,none for 32-bits
		constexpr int KDefaultMmapLimit = (sizeof(void*) >= 8 ? 1000 : 0);

		// set by set setReadOnlyMapLimit()
		int gMapLimit = KDefaultMmapLimit;

#if defined(HAVE_O_CLOEXEC)
		
#endif
		constexpr const int kOpenBaseFlags = O_CLOEXEC;
		constexpr size_t KWriteableFileBufferSize = 65536;

		Status LinuxError(const std::string& context, int errorNum) {
			// ENOENT present the case dir or file not be founded
			if (errorNum == ENOENT) {
				return Status::NotFound(context, std::strerror(errorNum));
			}
			else {
				return Status::IOError(context, std::strerror(errorNum));
			}
		}

		class Limiter {
		public:
			Limiter(int maxAcquires)
				:
#if !defined (NDEBUG)
				maxAcquires_(maxAcquires),
#endif
				acquireAllowed_(maxAcquires)
			{
				assert(maxAcquires >= 0);
			}


			bool acquire() {
				//get orignal and then sub atomic
				int oldAcquiresAllowed = acquireAllowed_.fetch_sub(1, std::memory_order_relaxed);

				if (oldAcquiresAllowed > 0) {
					return true;
				}
				// get orignal and then add atomic
				int preIncrementAcquiresAllowed = acquireAllowed_.fetch_add(1, std::memory_order_relaxed);
				(void)preIncrementAcquiresAllowed;
				assert(preIncrementAcquiresAllowed < maxAcquires_);
				return false;
			}

			void release() {
				int oldAcquiresAllowed = acquireAllowed_.fetch_add(1, std::memory_order_relaxed);
				(void)oldAcquiresAllowed;
				assert(oldAcquiresAllowed < maxAcquires_);
			}

		private:
#if !defined (NDEBUG)
			const int maxAcquires_;
#endif
			std::atomic<int> acquireAllowed_;
		};

		class LinuxSequentialFile final :public SequentialFile {
		public:
			LinuxSequentialFile(std::string filename, int fd)
				:fileName_(std::move(filename)), fd_(fd)
			{

			}

			~LinuxSequentialFile()  { close(fd_); }

			Status read(size_t n, Slice* result, char* scratch) override {
				Status status;
				while (true) {
					ssize_t readSize = ::read(fd_, scratch, n);
					if (readSize < 0) {
						//when a slow sys call be called and sig occur return EINTR 
						//in read we can restart it
						if (errno == EINTR) {
							continue;
						}
						status = LinuxError(fileName_, errno);
						break;
					}
					*result = Slice(scratch, readSize);
					break;
				}
				return status;
			}

			/*!
			 *  @brief skip file ptr n bytes
			 *  @param
			 *  @return
			 *
			 */
			Status skip(uint64_t n) override {
				if (::lseek(fd_, n, SEEK_CUR) == static_cast<off_t>(-1)) {
					return LinuxError(fileName_, errno);
				}
				return Status::OK();
			}



		private:
			const int fd_;
			const std::string fileName_;
		};

		class LinuxRandomAccessFile final :public RandomAccessFile {
		public:
			LinuxRandomAccessFile(std::string filename, int fd, Limiter* fdLimiter)
				:hasPermanentFd_(fdLimiter->acquire()),
				fd_(hasPermanentFd_ ? fd : -1),
				fdLimiter_(fdLimiter),
				fileName_(std::move(filename))
			{
				if (!hasPermanentFd_) {
					assert(fd_ == -1);
					::close(fd);
				}
			}

			~LinuxRandomAccessFile()  {
				if (hasPermanentFd_) {
					assert(fd_ != -1);
					::close(fd_);
					fdLimiter_->release();
				}
			}

			Status read(uint64_t offset, size_t n, Slice* result, char* scratch) const override {
				int fd = fd_;
				if (!hasPermanentFd_) {
					fd = ::open(fileName_.c_str(), O_RDONLY | kOpenBaseFlags);
					if (fd < 0) {
						return LinuxError(fileName_, errno);
					}
				}
				assert(fd != -1);
				Status status;
				/// what is pread diff from read 
				/// read every time will start from file's currnt offset ,then add the offset
				/// pread enable us to indicate offset(always from zero)
				ssize_t readSize = ::pread(fd, scratch, n, static_cast<off_t>(offset));
				*result = Slice(scratch, readSize < 0 ? 0 : readSize);
				if (readSize < 0) {
					status = LinuxError(fileName_, errno);
				}
				if (!hasPermanentFd_) {
					assert(fd != fd_);
					::close(fd);
				}
				return status;
			}

		private:
			const bool hasPermanentFd_; // if this is false ,file is opened on every read
			const int fd_; // -1 if permanent is false
			Limiter* const fdLimiter_;
			const std::string fileName_;
		};

		class LinuxMmapReadableFile final : public RandomAccessFile {
		public:
			LinuxMmapReadableFile(std::string fileName, char* mmapBase, size_t len, Limiter* mmapLimiter)
				:mmapBase_(mmapBase), len_(len), mmapLimiter_(mmapLimiter), fileName_(std::move(fileName))

			{}

			~LinuxMmapReadableFile()  {
				::munmap(static_cast<void*>(mmapBase_),len_);
				mmapLimiter_->release();
			}

			Status read(uint64_t offset, size_t n, Slice* result, char* scratch) const override {
				if (offset + n > len_) {
					*result = Slice();
					///EINVAL 无效的参数
					return LinuxError(fileName_, EINVAL);
				}
				*result = Slice(mmapBase_ + offset, n);
				return Status::OK();
			}


		private:
			char* const mmapBase_;
			const size_t len_;
			Limiter* const mmapLimiter_;
			const std::string fileName_;
		};


		class LinuxWritableFile final :public WritableFile {
		public:
			LinuxWritableFile(std::string fileName, int fd)
				:pos_(0), fd_(fd), isManifest_(isManifest(fileName)),
				fileName_(std::move(fileName)), dirName_(dirname(fileName_))
			{

			}

			~LinuxWritableFile()  {
				if (fd_ >= 0) {
					close();
				}
			}

			Status append(const Slice& data) override {
				size_t writeSize = data.size();
				const char* writeData = data.data();

				size_t copySize = std::min(writeSize, KWriteableFileBufferSize - pos_);
				std::memcpy(buf_ + pos_, writeData, copySize);
				writeData += copySize;
				writeSize -= copySize;
				pos_ += copySize;

				if (writeSize == 0) {
					return Status::OK();
				}

				Status status = flushBuffer();
				if (!status.ok()) {
					return status;
				}
				///if we can put the data to the buffer once,we put
				/// otherwise w directly write to disk with no buffer
				if (writeSize < KWriteableFileBufferSize) {
					std::memcpy(buf_, writeData, writeSize);
					pos_ = writeSize;
					return Status::OK();
				}
				return writeUnBuffered(writeData, writeSize);
			}



			static Slice basename(const std::string& filename) {
				std::string::size_type separator_pos = filename.rfind('/');
				if (separator_pos == std::string::npos) {
					return Slice(filename);
				}
				// The filename component should not contain a path separator. If it does,
				// the splitting was done incorrectly.
				assert(filename.find('/', separator_pos + 1) == std::string::npos);

				return Slice(filename.data() + separator_pos + 1,
					filename.length() - separator_pos - 1);
			}


			Status close() {
				Status status = flushBuffer();
				const int closeResult = ::close(fd_);
				if (closeResult < 0 && status.ok()) {
					status = LinuxError(fileName_, errno);
				}
				fd_ = -1;
				return status;
			}


			static bool isManifest(const std::string& filename) {
				return basename(filename).starts_with("MANIFEST");
			}

			static std::string dirname(const std::string& filename) {
				std::string::size_type separator_pos = filename.rfind('/');
				if (separator_pos == std::string::npos) {
					return std::string(".");
				}
				// The filename component should not contain a path separator. If it does,
				// the splitting was done incorrectly.
				assert(filename.find('/', separator_pos + 1) == std::string::npos);

				return filename.substr(0, separator_pos);
			}



			Status flush() override {
				return flushBuffer();
			}

			Status sync() override {
				Status status = syncDirIfManifest();
				if (!status.ok()) {
					return status;
				}
				status = flushBuffer();
				if (!status.ok()) {
					return status;
				}
				return syncFd(fd_, fileName_);
			}

		private:

			Status writeUnBuffered(const char* data, size_t size) {
				while (size > 0) {
					ssize_t writeResult = ::write(fd_, data, size);
					if (writeResult < 0) {
						///慢调用时被中断触发EINTR
						if (errno == EINTR) {
							continue;
						}
						return LinuxError(fileName_, errno);
					}
					data += writeResult;
					size -= writeResult;
				}
				return Status::OK();
			}

			Status syncDirIfManifest() {
				Status status;
				if (!isManifest_) {
					return status;
				}
				int fd = ::open(dirName_.c_str(), O_RDONLY | kOpenBaseFlags);
				if (fd < 0) {
					status = LinuxError(dirName_, errno);
				}
				else {
					status = syncFd(fd, dirName_);
					::close(fd);
				}
				return status;
			}

			static Status syncFd(int fd, const std::string& fdPath) {
#if HAVE_FULLFSYNC
				if (::fcntl(fd, F_FULLFSYNC) == 0) {
					return Status::OK();
				}
#endif
				///what is the diff between fsync and fdatasync
				///fdatasync only sync the data to disk not the FILE information
				///but fsync will do ,so fsync need at least twice IO while fdatasync once

#if HAVE_FDATASYNC
				bool syncSuc = ::fdatasync(fd) == 0;
#else
				bool syncSuc = ::fsync(fd) == 0;
#endif
				if (syncSuc) {
					return Status::OK();
				}
				return LinuxError(fdPath, errno);

			}


			Status flushBuffer() {
				Status status = writeUnBuffered(buf_, pos_);
				pos_ = 0;
				return status;
			}



		private:
			char buf_[KWriteableFileBufferSize];
			const bool isManifest_;
			size_t pos_;
			int fd_;
			const std::string fileName_;
			const std::string dirName_;
		};


		int lockOrUnLock(int fd, bool lock) {
			errno = 0;
			/// file lock enable us to lock file in the case we concuracy read file
			struct ::flock fileLockInfo;
			std::memset(&fileLockInfo, 0, sizeof(fileLockInfo));
			fileLockInfo.l_type = (lock ? F_WRLCK : F_UNLCK);
			fileLockInfo.l_whence = SEEK_SET;
			fileLockInfo.l_start = 0;
			fileLockInfo.l_len = 0;
			return ::fcntl(fd, F_SETLK, &fileLockInfo);
		}

		class LinuxFileLock :public FileLock {
		public:
			LinuxFileLock(int fd, std::string filename)
				:fd_(fd), filename_(std::move(filename))
			{

			}

			int fd() const { return fd_; }

			const std::string& filename() const {
				return filename_;
			}


		private:
			const int fd_;
			const std::string filename_;
		};

		/*!
		 * \class LinuxLockTable
		 *
		 * \brief actually a protected set
		 *
		 * \author czy
		 * \date 2023.07.31
		 */
		class LinuxLockTable {
		public:

			bool insert(const std::string& fname) LOCKS_EXCLUDED(mu_) {
				mu_.lock();
				bool suc = lockedFiles_.insert(fname).second;
				mu_.unlock();
				return suc;
			}

			void remove(const std::string& fname) LOCKS_EXCLUDED(mu_) {
				mu_.lock();
				lockedFiles_.erase(fname);
				mu_.lock();
			}

		private:
			Mutex mu_;
			std::set <std::string> lockedFiles_ GUARDED_BY(mu_);
		};

		using BackWorkGroundFunc = std::function<void(void*)>;
		class LinuxEnv :public Env {
		public:
			LinuxEnv() ;

			~LinuxEnv()  {
				static const char msg[] = "LinuxEnv singleton destroyed. Unsupported behavior!\n";
				std::fwrite(msg, 1, sizeof(msg), stderr);
				std::abort();
			}

			Status newSequentialFile(const std::string& filename, SequentialFile** result) override {
				int fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
				if (fd < 0) {
					*result = nullptr;
					return LinuxError(filename, errno);
				}
				*result = new LinuxSequentialFile(filename, fd);
				return Status::OK();
			}
			/*!
			 *  @brief  newRandomAccessFile
			 *	create a random accessFile and try to mmap it (in ordered to prove io perfermance)
			 *  @param filename,  result
			 *  @return
			 *
			 */
			Status newRandomAccessFile(const std::string& filename, RandomAccessFile** result) override {
				*result = nullptr;
				int fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
				if (fd < 0) {
					return LinuxError(filename, errno);
				}

				if (!mmapLimiter_.acquire()) {
					*result = new LinuxRandomAccessFile(filename, fd, &fdLimiter_);
					return Status::OK();
				}

				uint64_t fileSize;

				Status status = getFileSize(filename, &fileSize);

				if (status.ok()) {
					void* mmapBase = ::mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0);
					if (mmapBase != MAP_FAILED) {
						*result = new LinuxMmapReadableFile(filename, static_cast<char*>(mmapBase), fileSize, &mmapLimiter_);
					}
					else {
						status = LinuxError(filename, errno);
					}
				}
				::close(fd);
				if (!status.ok()) {
					mmapLimiter_.release();
				}
				return status;
			}

			Status newWritableFile(const std::string& filename,
				WritableFile** result) override
			{
				int fd = ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
				if (fd < 0) {
					*result = nullptr;
					return LinuxError(filename, errno);
				}
				*result = new LinuxWritableFile(filename, fd);
				return Status::OK();
			}

			Status newAppendableFile(const std::string& fname, WritableFile** result) override {
				int fd = ::open(fname.c_str(), O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
				if (fd < 0) {
					*result = nullptr;
					return LinuxError(fname, errno);
				}
				*result = new LinuxWritableFile(fname, fd);
				return Status::OK();
			}

			bool fileExists(const std::string& filename) override {
				return ::access(filename.c_str(), F_OK) == 0;
			}

			Status getChildren(const std::string& dirStr, std::vector<std::string>* result) override
			{
				result->clear();
				::DIR* dir = ::opendir(dirStr.c_str());
				if (dir == nullptr) {
					return LinuxError(dirStr, errno);
				}
				struct ::dirent* entry;
				// get all subdir name
				while ((entry = ::readdir(dir)) != nullptr) {
					result->emplace_back(entry->d_name);
				}
				::closedir(dir);
				return Status::OK();
			}

			Status removeFile(const std::string& fname) override {
				if (::unlink(fname.c_str()) != 0) {
					return LinuxError(fname, errno);
				}
				return Status::OK();
			}

			Status createDir(const std::string& dirname) override {
				if (::mkdir(dirname.c_str(), 0755) != 0) {
					return LinuxError(dirname, errno);
				}
				return  Status::OK();
			}

			Status removeDir(const std::string& dirname) override {
				if (::rmdir(dirname.c_str()) != 0) {
					return LinuxError(dirname, errno);
				}
				return Status::OK();
			}

			Status getFileSize(const std::string& fname, uint64_t* fileSize) override {
				struct ::stat fileStat;
				if (::stat(fname.c_str(), &fileStat) != 0) {
					*fileSize = 0;
					return LinuxError(fname, errno);
				}
				*fileSize = fileStat.st_size;
				return Status::OK();
			}

			Status renameFile(const std::string& src, const std::string& target) override {
				if (std::rename(src.c_str(), target.c_str()) != 0) {
					return LinuxError(src, errno);
				}
				return Status::OK();
			}

			Status lockFile(const std::string& fname, FileLock** lock) override {
				*lock = nullptr;

				int fd = ::open(fname.c_str(), O_RDWR | O_CREAT | kOpenBaseFlags, 0644);

				if (fd < 0) {
					return LinuxError(fname, errno);
				}

				if (!locks_.insert(fname)) {
					::close(fd);
					return Status::IOError("lock" + fname, "already held by process");
				}

				if (lockOrUnLock(fd, true) == -1) {
					int lockerrNo = errno;
					::close(fd);
					locks_.remove(fname);
					return LinuxError("lock " + fname, lockerrNo);
				}
				*lock = new LinuxFileLock(fd, fname);
				return Status::OK();
			}

			Status unlockFile(FileLock* lock)override {
				LinuxFileLock* fileLock = static_cast<LinuxFileLock*> (lock);
				if (lockOrUnLock(fileLock->fd(), false) == -1) {
					return LinuxError("unlock " + fileLock->filename(), errno);
				}
				locks_.remove(fileLock->filename());
				::close(fileLock->fd());
				delete fileLock;
				return Status::OK();
			}

			void schedule(ArrageFunc func, void* arg) override;


			void startThread(ArrageFunc func, void* arg) override {

				std::thread newThread(func, arg);
				newThread.detach();
			}

			Status getTestDir(std::string* result) override {
				const char* env = std::getenv("TEST_TMPDIR");
				if (env && env[0] != '\0') {
					*result = env;
				}
				else {
					char buf[100];
					std::snprintf(buf, sizeof(buf), "/tmp/CDB-%d",
						static_cast<int>(::geteuid()));
					*result = buf;
				}

				// The CreateDir status is ignored because the directory may already exist.
				createDir(*result);
				return Status::OK();
			}

			Status newLogger(const std::string& fname, Logger** result) override {
				int fd = ::open(fname.c_str(),
					O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
				if (fd < 0) {
					*result = nullptr;
					return LinuxError(fname, errno);
				}

				std::FILE* fp = ::fdopen(fd, "w");
				if (fp == nullptr) {
					::close(fd);
					*result = nullptr;
					return LinuxError(fname, errno);
				}
				else {
					*result = new PosixLogger(fp);
					return Status::OK();
				}

			}


			uint64_t nowMicros() override {
				static constexpr uint64_t kUsecondsPerSecond = 1000000;
				struct ::timeval tv;
				::gettimeofday(&tv, nullptr);
				return static_cast<uint64_t>(tv.tv_sec) * kUsecondsPerSecond + tv.tv_usec;
			}

			void sleepMicroSeconds(int micros) override {
				std::this_thread::sleep_for(std::chrono::microseconds(micros));
			}
			
		private:
			void backgroundThreadMain();


			static void BackgroundThreadEntryPoint(LinuxEnv* env) {
				env->backgroundThreadMain();
			}
			struct BackGroundWorkItem {
				explicit BackGroundWorkItem(BackWorkGroundFunc func, void* arg)
					:func(std::move(func)), arg(arg)
				{
				}
				const BackWorkGroundFunc func;
				void* const arg;
			};

			Mutex backgroundWorkMutex_;
			CondVar backGroundWorkCV_ GUARDED_BY(backgroundWorkMutex_);
			bool startedBackGroundThread_ GUARDED_BY(backgroundWorkMutex_);

			std::queue<BackGroundWorkItem> backGroundWorkQueue_ GUARDED_BY(backgroundWorkMutex_);

			LinuxLockTable locks_;
			Limiter mmapLimiter_;
			Limiter fdLimiter_;
		};
		// Return the maximum number of concurrent mmaps.
		int maxMmaps() { return gMapLimit; }


		// Return the maximum number of read-only files to keep open.
		int maxOpenFiles() {
			// we can use 1/5 file descriptor
			if (gOpenReadOnlyFileLimit >= 0) {
				return gOpenReadOnlyFileLimit;
			}
#ifdef __Fuchsia__
			// Fuchsia doesn't implement getrlimit.
			gOpenReadOnlyFileLimit = 50;
#else
			struct ::rlimit rlim;
			if (::getrlimit(RLIMIT_NOFILE, &rlim)) {
				// getrlimit failed, fallback to hard-coded default.
				gOpenReadOnlyFileLimit = 50;
			}
			else if (rlim.rlim_cur == RLIM_INFINITY) {
				gOpenReadOnlyFileLimit = std::numeric_limits<int>::max();
			}
			else {
				// Allow use of 20% of available file descriptors for read-only files.
				gOpenReadOnlyFileLimit = rlim.rlim_cur / 5;
			}
#endif
			return gOpenReadOnlyFileLimit;
		}
	}

	LinuxEnv::LinuxEnv()
		:backGroundWorkCV_(&backgroundWorkMutex_), startedBackGroundThread_(false), mmapLimiter_(maxMmaps())
		, fdLimiter_(maxOpenFiles())
	{
	}

	void LinuxEnv::schedule(ArrageFunc func, void* arg) {
		backgroundWorkMutex_.lock();
		if (!startedBackGroundThread_) {
			startedBackGroundThread_ = true;
			std::thread bkThread(LinuxEnv::BackgroundThreadEntryPoint, this);
			bkThread.detach();
		}
		backGroundWorkQueue_.emplace(func, arg);
		backGroundWorkCV_.signal();
		backgroundWorkMutex_.unlock();
	}

	void LinuxEnv::backgroundThreadMain() {
		while (true) {
			backgroundWorkMutex_.lock();
			while (backGroundWorkQueue_.empty()) {
				backGroundWorkCV_.wait();
			}
			assert(!backGroundWorkQueue_.empty());
			auto func = backGroundWorkQueue_.front().func;
			void* arg = backGroundWorkQueue_.front().arg;
			backGroundWorkQueue_.pop();
			backgroundWorkMutex_.unlock();
			func(arg);
		}
	}

	namespace {
		template<typename EnvType>
		class SingletonEnv {
		public:
			SingletonEnv() {

#if !defined(NDEBUG)
				envInited_.store(true, std::memory_order_relaxed);
#endif
				new (&envStorage_) EnvType();
			}

			~SingletonEnv() = default;

			SingletonEnv(const SingletonEnv&) = delete;

			SingletonEnv& operator=(const SingletonEnv&) = delete;

			Env* env() { return reinterpret_cast<Env*>(&envStorage_); }

			static void assertEnvNotInited() {

#if !defined(NDEBUG)
				assert(!envInited_.load(std::memory_order_relaxed));
#endif
			}


		private:
			typename std::aligned_storage<sizeof(EnvType), alignof(EnvType)>::type envStorage_;
#if !defined(NDEBUG)
			static std::atomic<bool> envInited_;
#endif
		};
#if !defined(NDEBUG)
		template<typename EnvType>
		std::atomic<bool> SingletonEnv<EnvType>::envInited_;
#endif
		using LinuxDefaultEnv = SingletonEnv<LinuxEnv>;


	}
	Env* Env::Default()
	{
		static LinuxDefaultEnv envContainer;
		return envContainer.env();
	}

}


