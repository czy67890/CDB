/*!
 * \file MutexLock.h
 *
 * \author czy
 * \date 2023.07.29
 *
 * 
 */
#pragma once
#include "Util/ThreadAnnotations.h"
#include <cassert>
#include <condition_variable>  // NOLINT
#include <cstddef>
#include <cstdint>
#include <mutex>  // NOLINT
#include <string>

namespace CDB{
	class CondVar;

	class LOCKABLE Mutex{
	public:
		Mutex() = default;
		~Mutex() = default;

		Mutex(const Mutex&) = delete;

		Mutex& operator=(const Mutex&) = delete;

		void lock() EXCLUSIVE_LOCK_FUNCTION() { mu_.lock(); }

		void unlock() UNLOCK_FUNCTION() { mu_.unlock(); }

		void assrtHeld() ASSERT_EXCLUSIVE_LOCK() {}
			

	private:
		friend class CondVar;
		std::mutex mu_;
	};

	class CondVar{
	public:
		explicit CondVar(Mutex * mu)
			:mu_(mu)
		{
			assert(mu_ != nullptr);
		}

		~CondVar() = default;

		CondVar(const CondVar&) = delete;

		CondVar& operator=(const CondVar&) = delete;

		void wait(){
			///adopt lock 代表仅仅是收养这个mutex,不再重新上锁
			///但是会正常的解锁
			std::unique_lock<std::mutex> lock(mu_->mu_,std::adopt_lock);
			cv_.wait(lock);
			lock.release();
		}

		void signal(){
			cv_.notify_one();
		}
		void signalAll(){
			cv_.notify_all();
		}

	private:
		Mutex* const mu_;
		std::condition_variable cv_;
	};


	class SCOPED_LOCKABLE MutexLock {
	public:
		explicit MutexLock(Mutex* mu) EXCLUSIVE_LOCK_FUNCTION(mu) : mu_(mu) {
			this->mu_->lock();
		}
		~MutexLock() UNLOCK_FUNCTION() { this->mu_->unlock(); }

		MutexLock(const MutexLock&) = delete;
		MutexLock& operator=(const MutexLock&) = delete;

	private:
		Mutex* const mu_;
	};
}