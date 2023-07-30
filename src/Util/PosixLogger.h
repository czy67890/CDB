/*!
 * \file PosixLogger.h
 *
 * \author czy
 * \date 2023.07.29
 *
 * 
 */
#pragma once

#include <sys/time.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <thread>
#include "CDataBase/Env.h"
namespace CDB{
	class PosixLogger final : public :Logger{
	public:
		explicit PosixLogger(std::FILE *fp)
			:fp_(fp)
		{
			assert(fp_ != nullptr);
		}
		/// <summary>
		/// RAII handler override To enable muti 
		/// </summary>
		~PosixLogger() override { std::fclose(fp_); }
		
		/// if a 512B stack buffer is enough we use that , otherwise ,we create a buffer in heap

		void Logv(const char* format,std::va_list argument) override {
			struct ::timeval nowTime;
			::gettimeofday(&nowTime,nullptr);
			const std::time_t nowSeconds = nowTime.tv_sec;
			struct std::tm nowCompoments;
			///将time_t类型的转换成本地类型
			localtime_r(&nowSeconds, &nowCompoments);
			
			constexpr int KMaxThreadIdSize = 32;
			std::ostringstream threadStream;
			threadStream << std::this_thread::get_id();
			auto threadIDStr{ threadStream.str() };
			if (threadIDStr.size() > KMaxThreadIdSize) {
				threadIDStr.resize(KMaxThreadIdSize);
			}
			
			constexpr int kStackBufferSize = 512;
			char stkBuffer[kStackBufferSize];

			int dynamicBufferSize = 0;
			for (int index = 0; index < 2;++index) {
				const int bufferSize = (index == 0) ? kStackBufferSize:dynamicBufferSize;
				char* const buffer = (index == 0) ? stkBuffer : new char[dynamicBufferSize];
				int buffer_offset = std::snprintf(
					buffer, bufferSize, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %s ",
					nowCompoments.tm_year + 1900, nowCompoments.tm_mon + 1,
					nowCompoments.tm_mday, nowCompoments.tm_hour, nowCompoments.tm_min,
					nowCompoments.tm_sec, static_cast<int>(nowTime.tv_usec),
					threadIDStr.c_str());
				assert(buffer_offset <= 28 + KMaxThreadIdSize);
				std::va_list argumentCopy;
				va_copy(argumentCopy,argument);
				
				buffer_offset += std::vsnprintf(buffer + buffer_offset,bufferSize - buffer_offset,format,argumentCopy);
				
				va_end(argumentCopy);

				if(buffer_offset >= bufferSize - 1){
					if (index == 0) {
						dynamicBufferSize = buffer_offset + 2;
						continue;
					}
					assert(false);
					buffer_offset = bufferSize - 1;
				}
				///add a newLine if necessary
				if(buffer[buffer_offset - 1] != '\n'){
					buffer[buffer_offset] = '\n';
					++buffer_offset;
				}

				assert(buffer_offset <= bufferSize);

				std::fwrite(buffer,1,buffer_offset,fp_);
				std::fflush(fp_);
				/// in the case stkBuffer not engouh
				if(index != 0){
					delete[] buffer;
				}
				break;
			}

		}


	private:
		std::FILE* const fp_;
	};
}