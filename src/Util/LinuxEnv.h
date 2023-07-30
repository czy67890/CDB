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
			LinuxSequentialFile()
		};
	}
}