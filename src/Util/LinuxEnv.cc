/*!
 * \file LinuxEnv.h
 *
 * \author czy
 * \date 2023.07.30
 *
 * 
 */
#pragma once
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
namespace CDB{
	namespace{
		//can be set by setReadOnlyMapLimit() and maxOpenFileLimit
		int gOpenReadOnlyFileLimit = 1;

		// 64bit or more can mmap 1000 ,none for 32-bits
		constexpr int KDefaultMmapLimit = (sizeof(void*) >= 8 ? 1000 : 0);
		
		// set by set setReadOnlyMapLimit()
		int gMapLimit = KDefaultMmapLimit;

#if defined(HAVE_O_CLOEXEC)
		constexpr const int kOpenBaseFlags = O_CLOEXEC;
#endif

		constexpr size_t KWriteableFileBufferSize = 65536;

		Status LinuxError(const std::string & context,int errorNum){
			// ENOENT present the case dir or file not be founded
			if(errorNum == ENOENT){
				return Status::NotFound(context,std::strerror(errorNum));
			}
			else{
				return Status::IOError(context, std::strerror(errorNum));
			}
		}

		class Limiter{
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


			bool acquire(){
				//get orignal and then sub atomic
				int oldAcquiresAllowed = acquireAllowed_.fetch_sub(1,std::memory_order_relaxed);
				
				if(oldAcquiresAllowed > 0){
					return true;
				}
				// get orignal and then add atomic
				int preIncrementAcquiresAllowed = acquireAllowed_.fetch_add(1,std::memory_order_relaxed);
				(void)preIncrementAcquiresAllowed;
				assert(preIncrementAcquiresAllowed < maxAcquires_);
				return false;
			}

			void release(){
				int oldAcquiresAllowed = acquireAllowed_.fetch_add(1,std::memory_order_relaxed);
				(void)oldAcquiresAllowed;
				assert(oldAcquiresAllowed < maxAcquires_);
			}

		private:
#if !defined (NDEBUG)
			const int maxAcquires_;
#endif
			std::atomic<int> acquireAllowed_;
		};

		class LinuxSequentialFile final :public SequentialFile{
		public:
			LinuxSequentialFile(std::string filename, int fd)
				:fileName_(std::move(filename)), fd_(fd)
			{

			}

			~LinuxSequentialFile() override { close(fd_); }

			Status read(size_t n,Slice *result,char *scratch) override{
				Status status;
				while(true){
					ssize_t readSize = ::read(fd_,scratch,n);
					if(readSize < 0){
						//when a slow sys call be called and sig occur return EINTR 
						//in read we can restart it
						if(errno == EINTR){
							continue;
						}
						status = LinuxError(fileName_, errno);
						break;
					}
					*result = Slice(scratch,readSize);
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
			Status skip(uint64_t n) override{
				if (::lseek(fd_,n,SEEK_CUR) == static_cast<off_t>(-1)) {
					return LinuxError(fileName_,errno);
				}
				return Status::OK();
			}



		private:
			const int fd_;
			const std::string fileName_;
		};

		class LinuxRandomAccessFile final :public RandomAccessFile{
		public:
			LinuxRandomAccessFile(std::string filename ,int fd,Limiter *fdLimiter)
				:hasPermanentFd_(fdLimiter->acquire()),
				fd_(hasPermanentFd_ ? fd : -1),
				fdLimiter_(fdLimiter),
				fileName_(std::move(filename))
			{
				if(!hasPermanentFd_){
					assert(fd_ == -1);
					::close(fd);
				}
			}

			~LinuxRandomAccessFile() override{
				if(hasPermanentFd_){
					assert(fd_ != -1);
					::close(fd_);
					fdLimiter_->release();
				}
			}

			Status read(uint64_t offset,size_t n,Slice *result,char * scratch) const override{
				int fd = fd_;
				if(!hasPermanentFd_){
					fd = ::open(fileName_.c_str(),O_RDONLY | kOpenBaseFlags);
					if(fd < 0){
						return LinuxError(fileName_, errno);
					}
				}
				assert(fd != -1);
				Status status;
				/// what is pread diff from read 
				/// read every time will start from file's currnt offset ,then add the offset
				/// pread enable us to indicate offset(always from zero)
				ssize_t readSize = ::pread(fd,scratch,n,static_cast<off_t>(offset));
				*result = Slice((scratch,readSize < 0) ? 0 :readSize);
				if(readSize < 0){
					status = LinuxError(fileName_, errno);
				}
				if(!hasPermanentFd_){
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

	}
}