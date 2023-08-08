/*!
 * \file LogReader.h
 *
 * \author czy
 * \date 2023.08.08
 *
 * 
 */
#pragma once
#include <cstdint>
#include "DataBase/DBFormat.h"
#include "CDataBase/Slice.h"
#include "CDataBase/Status.h"


namespace CDB{
	class SequentialFile;

	namespace Log{
		class Reader{
		public:
			
			
			class Reporter{
			public:
				virtual ~Reporter();

				virtual void corruption(size_t bytes, const Status& status) = 0;

			};

			Reader(SequentialFile* file, Reporter* reporter, bool checkSum, uint64_t initOffset);


			Reader(const Reader&) = delete;

			Reader& operator=(const Reader&) = delete;

			~Reader();

			bool readRecord(Slice* record, std::string* sratch);

			uint64_t lastRecordOffset();

			
		private:
			enum {
				KEof = KMaxRecordType + 1,
				KBadRecord = KMaxRecordType + 2

			};


			bool skipToInitialBlock();

			unsigned int readPhysicalRecord(Slice *result);

			void reportCorruption(uint64_t bytes,const char *respons);

			void reportDrop(uint64_t bytes,const Status &reason);

			SequentialFile* const file_;
			
			Reporter* const reporter_;
			
			bool const checkSum_;
			
			char* const backingStore_;
			
			Slice buffer_;

			bool eof_;

			uint64_t lastRecordOffset_;

			uint64_t endOfBufferOffset_;

			uint64_t const initOffset_;

			bool resyncing_;

		};

	}
}
