/*!
 * \file Allocator.h
 *
 * \author czy
 * \date 2023.08.04
 *
 * 
 */
#pragma once


#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace CDB{
	class Allocator{
	public:
		
		Allocator();

		Allocator(const Allocator&) = delete;

		Allocator& operator=(const Allocator&) = delete;

		~Allocator();

		char* allocate(size_t bytes);

		char* allocateAligned(size_t bytes);

		size_t memUsage() const { return memUsage_.load(std::memory_order_relaxed); }
	
	private:
		char* allocateFallBack(size_t bytes);
		char* allocateNewBlock(size_t blockBytes);
		char* allocPtr_;
		size_t allocBytesRemain_;
		std::vector<char*> blocks_;
		std::atomic<size_t> memUsage_;
	};

	inline char* Allocator::allocate(size_t bytes){
		assert(bytes > 0);
		if(bytes <= allocBytesRemain_){
			char* result = allocPtr_;
			allocPtr_ += bytes;
			allocBytesRemain_ -= bytes;
			return result;
		}
		return allocateFallBack(bytes);
	}
}