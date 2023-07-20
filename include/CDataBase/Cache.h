/*!
 * \file Cache.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include <cstdint>
#include "CDataBase/Slice.h"

namespace CDB{

	class Cache;


	Cache* newLRUCache(size_t  capacity);

	class Cache{
	public:

		Cache() = default;

		Cache& (const Cache&) = delete;

		Cache& operator=(const Cache&) = delete;

		virtual ~Cache();

		struct Handle {};

		virtual Handle* insert(const Slice &key,void *value,size_t charge,void (*deleter)(const Slice &key,void *value)) = 0;
		
		virtual Handle* lookUp(const Slice& key) = 0;

		virtual void release(Handle *handle) = 0;

		virtual void* value(Handle* handle) = 0;

		virtual void erase(const Slice& key) = 0;

		virtual uint64_t newId() = 0;

		virtual void prune();

		virtual size_t totalCharge() const = 0;

	};
}

