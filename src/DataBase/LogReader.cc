#include "DataBase/LogReader.h"
#include <cstdio>
#include "CDataBase/Env.h"
#include "Util/Coding.h"
#include "Util/Crc32.h"


using namespace CDB;
using namespace Log;

CDB::Log::Reader::Reporter::~Reporter() = default;

CDB::Log::Reader::Reader(SequentialFile* file, Reporter* reporter, bool checkSum, uint64_t initOffset)
	:file_(file),reporter_(reporter),checkSum_(checkSum),backingStore_(new char[KBlockSize]),buffer_(),eof_(false),
	lastRecordOffset_(0),endOfBufferOffset_(0),initOffset_(initOffset),resyncing_(initOffset > 0)
{
}

CDB::Log::Reader::~Reader()
{
	delete[]backingStore_;
}

bool CDB::Log::Reader::readRecord(Slice* record, std::string* sratch)
{
	if (lastRecordOffset_ < initOffset_) {
		if(!skipToInitialBlock()){
			return false;
		}
	}

	sratch->clear();
	record->clear();
	bool inFragmentRecord = false;
	uint64_t prospectiveRecordOffset = 0;
	Slice fragment;
	while(true){
		const unsigned int recordType = readPhysicalRecord(&fragment);
		uint64_t physicalReadOffset = endOfBufferOffset_ - buffer_.size() - KHeaderSize;
		if(resyncing_){
			if(recordType == KMiddleType){
				continue;
			}
			else if(recordType == KLastType){
				resyncing_ = false;
				continue;
			}
			else{
				resyncing_ = false;
			}
		}
		switch(recordType){
		case KFullType:{
			if(inFragmentRecord){
				if(!sratch->empty()){
					reportCorruption(sratch->size(),"partial record without end(1)");
				}
			}
			prospectiveRecordOffset = physicalReadOffset;
			sratch->clear();
			*record = fragment;
			lastRecordOffset_ = prospectiveRecordOffset;
			return true;
		}
		case KFirstType:{
			if(inFragmentRecord){
				if(!sratch->empty()){
					reportCorruption(sratch->size(),"partial record without end(2)");
				}
			}
			prospectiveRecordOffset = physicalReadOffset;
			sratch->assign(fragment.data());
			inFragmentRecord = true;
			break;
		}

		case KMiddleType:{


		}
		}
	}

}

bool CDB::Log::Reader::skipToInitialBlock()
{
	const size_t offsetInBlock = initOffset_ % KBlockSize;
	uint64_t blockStartLocation = initOffset_ - offsetInBlock;
	
	//Don't seacrch a block if we in trailer 
	if(offsetInBlock > KBlockSize - 6){
		blockStartLocation += KBlockSize;
	}
	endOfBufferOffset_ = blockStartLocation;
	if(blockStartLocation > 0){
		Status skipStatus = file_->skip(blockStartLocation);
		if(!skipStatus.ok()){
			reportCorruption(blockStartLocation, skipStatus);
			return false;
		}
	}
	return true;
}

unsigned int CDB::Log::Reader::readPhysicalRecord(Slice* result)
{
	while(true){
		if(buffer_.size() < KHeaderSize){
			if(!eof_){
				buffer_.clear();
				Status status = file_->read(KBlockSize,&buffer_,backingStore_);
				endOfBufferOffset_ += buffer_.size();
				if(!status.ok()){
					buffer_.clear();
					reportCorruption(KBlockSize,status);
					eof_ = true;
					return KEof;
				}
				else if(buffer_.size() < KBlockSize){
					eof_ = true;
				}
				continue;
			}
			else{
				buffer_.clear();
				return KEof;
			}
		}

		//get the temp header
		const char* header = buffer_.data();
		const uint32_t a = static_cast<uint32_t>(header[4]) & 0xff;
		const uint32_t b = static_cast<uint32_t>(header[5]) & 0xff;
		const unsigned int type = header[6];
		const uint32_t len = a | (b << 8);

		if(KHeaderSize + len > buffer_.size()){
			size_t dropSize = buffer_.size();
			buffer_.clear();
			if(!eof_){
				reportCorruption(dropSize,"bad record length ");
				return KBadRecord;
			}
			return KEof;
		}

		if(type == KZeroType && len == 0){
			buffer_.clear();
			return KBadRecord;
		}

		if(checkSum_){
			uint32_t expectedCrc = crc32::Unmask(DecodeFixed32(header));
			uint32_t actualCrc = crc32::Value(header + 6,1 + len);
			if(actualCrc != expectedCrc){
				size_t dropSize = buffer_.size();
				buffer_.clear();
				reportCorruption(dropSize, "checksum mismatch");
				return KBadRecord;
			}
		}

		buffer_.remove_prefix(KHeaderSize + len);

		if(endOfBufferOffset_ - buffer_.size() - KHeaderSize - len < initOffset_){
			result->clear();
			return KBadRecord;
		}
		*result = Slice(header + KHeaderSize,len) ;
		return type;
	}
}
