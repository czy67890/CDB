/*!
 * \file Logging.h
 *
 * \author czy
 * \date 2023.07.13
 *
 * 
 */
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include "CDataBase/Slice.h"
namespace CDB{
	
	class WritableFile;

	void appendNumberTo(std::string *str,uint64_t num);

	void appendEscapedStringTo(std::string* str, const Slice& value);

	std::string numberToString(uint64_t num);

	std::string escapeString(const Slice & value);

	bool consumeDecimalNumber(Slice* in, uint64_t* val);


}
