/*!
 * \file Env.h
 *
 * \author czy
 * \date 2023.07.07
 *
 * 
 */
#pragma once
#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>
#include "CDataBase/Status.h"
#include "CDataBase/Slice.h"
#include <functional>
namespace CDB {

	class FileLock;
	class Logger;
	class RandomAccessFile;
	class SequentialFile;
	class WritableFile;

	class Env {
		using ArrageFunc = std::function<void(void* arg)>;
	public:
		Env();

		Env(const Env&) = delete;

		virtual ~Env();

		static Env* Default();

		virtual Status newSequentialFile(const std::string& name, SequentialFile** result) = 0;

		virtual Status newRandomAccessFile(const std::string& fname,
			RandomAccessFile** result) = 0;

		virtual Status newWriteableFile(const std::string& name, WritableFile** result) = 0;

		virtual Status newAppendableFile(const std::string& name, WritableFile** result) = 0;

		virtual bool fileExists(const std::string& fname);

		virtual Status getChildren(const std::string& dir, std::vector<std::string>* result) = 0;

		virtual Status removeFile(const std::string& fname);

		virtual Status deleteFile(const std::string& fname);

		virtual Status createDir(const std::string& dirname) = 0;

		virtual Status removeDir(const std::string& dirname);

		virtual Status deleteDir(const std::string& dirname);

		virtual Status getFileSize(const std::string& fname, uint64_t* fileSize) = 0;

		virtual Status renameFile(const std::string& src, const std::string& target) = 0;

		virtual Status lockFile(const std::string& fname, FileLock** lock) = 0;

		virtual Status unlockFile(FileLock* lock) = 0;

		virtual void schedule(ArrageFunc func, void* arg) = 0;


		virtual void startThread(ArrageFunc func, void* arg) = 0;

		virtual Status getTestDir(std::string* path) = 0;

		virtual Status newLogger(const std::string& fname, Logger** result) = 0;

		virtual uint64_t nowMicros() = 0;

		virtual void sleepMicroSeconds(int micros) = 0;

	};

	class  SequentialFile {

	public:
		SequentialFile() = default;

		SequentialFile(const SequentialFile&) = delete;

		virtual ~SequentialFile();

		virtual Status read(size_t n, Slice* result, char* scratch) = 0;

		virtual Status skip(uint64_t n) = 0;
	};

	class RandomAccessFile {
	public:
		RandomAccessFile() = default;

		RandomAccessFile(const RandomAccessFile&) = delete;

		RandomAccessFile& operator=(const RandomAccessFile&) = delete;

		virtual ~RandomAccessFile();

		virtual Status read(uint64_t offset, size_t n, Slice* result, char* scratch) const = 0;

	};

	class WritableFile {
	public:

		WritableFile() = default;

		WritableFile(const WritableFile&) = delete;

		WritableFile& operator=(const WritableFile&) = delete;

		virtual ~WritableFile();

		virtual Status append(const Slice&data ) = 0;

		virtual Status close() = 0;
		
		virtual Status flush() = 0;

		virtual Status sync() = 0;


	};

	class Logger{
	public:
		Logger() = default;
		
		Logger(const Logger&) = delete;

		Logger& operator=(const Logger & ) = delete;

		virtual ~Logger();

		virtual void Logv(const char* format, std::va_list ap) = 0;

	};

	class FileLock{
	public:
		
		FileLock() = default;

		FileLock(const FileLock&) = delete;

		FileLock& operator= (const FileLock&) = delete;

		virtual ~FileLock();
	};

	void Log(Logger* infoLog, const char* format, ...);

	Status WriteStringToFile(Env* env, const Slice& data, const std::string& fname);

	Status ReadFileToString(Env* env, const std::string& fname, std::string* data);

	class EnvWrapper :public Env{
	public:
		explicit EnvWrapper(Env * t):
			target_(t)
		{

		}

		virtual ~EnvWrapper();

		Status newSequentialFile(const std ::string &f,SequentialFile ** r) override{
			return target_->newSequentialFile(f, r);
		}

		Status newRandomAccessFile(const std::string& f,
			RandomAccessFile** r) override {
			return target_->newRandomAccessFile(f, r);
		}

		Status newWriteableFile(const std::string& name, WritableFile** result) override{
			return target_->newWriteableFile(name,result);
		}

		Status newAppendableFile(const std::string& f, WritableFile** r) override {
			return target_->newAppendableFile(f, r);
		}
		bool fileExists(const std::string& f) override {
			return target_->fileExists(f);
		}
		Status getChildren(const std::string& dir,
			std::vector<std::string>* r) override {
			return target_->getChildren(dir, r);
		}
		Status removeFile(const std::string& f) override {
			return target_->removeFile(f);
		}
		Status createDir(const std::string& d) override {
			return target_->createDir(d);
		}
		Status removeDir(const std::string& d) override {
			return target_->removeDir(d);
		}
		Status getFileSize(const std::string& f, uint64_t* s) override {
			return target_->getFileSize(f, s);
		}
		Status renameFile(const std::string& s, const std::string& t) override {
			return target_->renameFile(s, t);
		}
		Status lockFile(const std::string& f, FileLock** l) override {
			return target_->lockFile(f, l);
		}
		Status unlockFile(FileLock* l) override { return target_->unlockFile(l); }
		void schedule(ArrageFunc f, void* a) override {
			return target_->schedule(f, a);
		}
		void startThread(ArrageFunc f, void* a) override {
			return target_->startThread(f, a);
		}
		Status getTestDir(std::string* path) override {
			return target_->getTestDir(path);
		}
		Status newLogger(const std::string& fname, Logger** result) override {
			return target_->newLogger(fname, result);
		}
		uint64_t nowMicros() override { return target_->nowMicros(); }
		void sleepMicroSeconds(int micros) override {
			target_->sleepMicroSeconds(micros);
		}

	private:
		Env* target_;
	};
}