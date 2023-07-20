#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "Util/Logging.h"
#include "CDataBase/Env.h"
#include "CDataBase/Slice.h"

using namespace CDB;

void appendNumberTo(std::string* str, uint64_t num)
{
	char buf[30];
	std::snprintf(buf,sizeof(buf),"%llu",static_cast<unsigned long long >(num));
	str->append(buf);
}

void CDB::appendEscapedStringTo(std::string* str, const Slice& value)
{
	for(size_t i = 0;i < value.size();++i){
		char c = value[i];
		if(c >= ' ' && c <= '~'){
			str->push_back(c);
		}
		else{
			char buf[10];
			std::snprintf(buf,sizeof(buf),"\\x%02x",static_cast<unsigned int>(c) & 0xff);
			str->append(buf);
		}
	}
}

std::string CDB::numberToString(uint64_t num)
{	
	std::string r;
	appendNumberTo((&r, num));
	return r;
}

std::string CDB::escapeString(const Slice& value)
{
	std::string r;
	appendEscapedStringTo(&r, value);
	return r;
}

bool CDB::consumeDecimalNumber(Slice* in, uint64_t* val)
{
	constexpr  uint64_t KMaxUint64 = std::numeric_limits<uint64_t>::max();
	constexpr  char KLastDigitOfMaxUint64 = '0' + static_cast<char> (KMaxUint64 % 10);
	uint64_t value = 0;
	const uint8_t* start = reinterpret_cast<const uint8_t*>(in->data());
	const uint8_t* end = start + in->size();
	for(;current != end;++current){
		const uint8_t ch = *current;
		if( ch < '0' || ch > '9'){
			break;
		}

		if(value > KMaxUint64 / 10 || (value == KMaxUint64 / 10 && ch > KLastDigitOfMaxUint64)){
			return false;
		}

		value = (value * 10) + ch - '0';

	}

	*val = value;
	const size_t digit_consumed = current - start;
	in->remove_prefix(digit_consumed);
	return digit_consumed != 0;

}
