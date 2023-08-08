#include "Util/Allocator.h"
namespace CDB{
	/// <summary>
	/// default block size
	/// when we allocate  mem huge than 1KB
	/// we derectly allocate
	/// otherwise we allocate a block as buffer
	/// </summary>
	constexpr int KBlockSize = 4096;
}


CDB::Allocator::Allocator()
	:allocPtr_(nullptr),allocBytesRemain_(0),memUsage_(0)
{

}

CDB::Allocator::~Allocator()
{
	for (size_t i = 0; i < blocks_.size();++i) {
		delete[] blocks_[i];
	}
}

char* CDB::Allocator::allocateAligned(size_t bytes)
{
	constexpr int align = (sizeof(void *) > 8 ? sizeof(void*) : 8);
	///use & to accecelerate calculate
	size_t currentMod = reinterpret_cast<uintptr_t>(allocPtr_) & (align - 1);
	///byte needed to fill 
	size_t slop = align -  currentMod;
	int needed = bytes + slop;
	if(needed <= allocBytesRemain_){
		result = allocPtr_ + slop;
		allocPtr_ += needed;
		allocBytesRemain_ -= needed;
	}
	else{
		result = allocateFallBack(bytes);
	}
	assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
	return result;
}

char* CDB::Allocator::allocateFallBack(size_t bytes)
{	
	/// a huge object will self contian a memory
	if (bytes > KBlockSize / 4) {
		char* result = allocateNewBlock(bytes);
		return result;
	}
	allocPtr_ = allocateNewBlock(KBlockSize);
	allocBytesRemain_ = KBlockSize;
	char* result = allocPtr_;
	allocPtr_ += bytes;
	allocBytesRemain_ -= bytes;
	return result;
}

char* CDB::Allocator::allocateNewBlock(size_t blockBytes)
{	
	assert(blockBytes > 0);
	char* result = new char[blockBytes];
	blocks_.push_back(result);
	memUsage_.fetch_add(blockBytes + sizeof(char*),std::memory_order_relaxed );
	return result;
}
