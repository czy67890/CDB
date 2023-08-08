/*!
 * \file LogFormat.h
 *
 * \author czy
 * \date 2023.08.07
 *
 * 
 */
#pragma once
namespace CDB{
	namespace Log{
		enum RecordType{
			KZeroType = 0,
			
			KFullType = 1,
			
			KFirstType =2,

			KMiddleType = 3,

			KLastType = 4
		};

		static constexpr int KMaxRecordType = KLastType;

		//32KB
		static constexpr int KBlockSize = 32*1024;
		
		static const int KHeaderSize = 4 + 2 + 1;

	}

}