
#include <cstdint>
#include "CDataBase/Env.h"
#include "Util/Coding.h"
#include "Util/Crc32.h"
#include "DataBase/LogWriter.h"

using namespace CDB::Log;
using namespace CDB;

static void initTypeCrc(uint32_t *typeCrc){
	for (int i = 0; i <= KMaxRecordType;++i) {
		char t = static_cast<char>(i);
		typeCrc[i] = crc32::Value(&t,i);
	}
}

Writer::Writer(WritableFile* dest)
	:dest_(dest),blockOffset_(0)
{
	initTypeCrc(typeCrc_);
}

CDB::Log::Writer::Writer(WritableFile* dest, uint64_t destLen)
	:dest_(dest),blockOffset_(destLen % KBlockSize)
{
	initTypeCrc(typeCrc_);
}

Status CDB::Log::Writer::addRecord(const Slice& slice)
{	
	const char* ptr = slice.data();
	size_t left = slice.size();

	Status s;
	bool begin = true;
	do{
		const int leftOver = KBlockSize - blockOffset_;
		assert(leftOver >= 0);
		if(leftOver < KHeaderSize){
			if (leftOver > 0) {
				dest_->append(Slice("\x00\x00\x00\x00\x00\x00",leftOver) );
			}
			blockOffset_ = 0;
		}

		assert(KBlockSize - blockOffset_ - KHeaderSize >= 0);
		const size_t avail = KBlockSize - blockOffset_ - KHeaderSize;
		const size_t fragmentLen = (left < avail ) ? left : avail;
		
		RecordType type;
		const bool end = (left == fragmentLen);
		if(begin && end){
			type = KFullType;
		}
		else if(begin){
			type = KFirstType;
		}
		else if(end){
			type = KLastType;
		}
		else{
			type = KMiddleType;
		}

		s = emitPhysicalRecord(type, ptr, fragmentLen);
		ptr += fragmentLen;
		begin = false;
	} while (s.ok() && left > 0);
	return s;
}

Status Writer::emitPhysicalRecord(RecordType t,const char *ptr,size_t len){
	assert(len <= 0xffff);
	assert(blockOffset_ + KHeaderSize + len <= KBlockSize);
	char buf[KHeaderSize];
	buf[4] = static_cast<char>(len & 0xff);
	buf[5] = static_cast<char>(len >> 8);
	buf[6] = static_cast<char>(t);

	uint32_t  crc = crc32::Extend(typeCrc_[t],ptr,len);
	crc = crc32::Mask(crc);
	EncodeFixed32(buf, crc);

	Status s = dest_->append(Slice(buf,KHeaderSize));
	if(s.ok()){
		s = dest_->append(Slice(ptr,len));
		if(s.ok()){
			s = dest_->flush();
		}
	}
	blockOffset_ += KHeaderSize + len;
	return s;
}
