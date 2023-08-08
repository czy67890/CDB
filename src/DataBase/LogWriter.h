/*!
 * \file LogWriter.h
 *
 * \author czy
 * \date 2023.08.07
 *
 * 
 */
#pragma once

#include <cstdint>
#include "CDataBase/Slice.h"
#include "CDataBase/Status.h"
#include "DataBase/DBFormat.h"
#include "DataBase/LogFormat.h"
namespace CDB{
	class WritableFile;

	namespace Log{
		class Writer {
		public:
			explicit Writer(WritableFile* dest);

			Writer(WritableFile* dest, uint64_t destLen);

			Writer(const Writer&) = delete;

			Writer& operator=(const Writer&) = delete;

			~Writer() = default;

			Status addRecord(const Slice& slice);

		private:
			Status emitPhysicalRecord(RecordType type, const char* ptr, size_t length);
			
			WritableFile* dest_;

			int blockOffset_;

			uint32_t typeCrc_[KMaxRecordType + 1];

		};
	}

}
