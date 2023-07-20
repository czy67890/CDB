/*!
 * \file Table.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include <cstdint>
#include "CDataBase/Iterator.h"

namespace CDB{

class Block;
class BlockHandle;
class Footer;
struct Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;
		

class Table{
public:
	static Status open(const Options &options,RandomAccessFile *file,uint64_t fileSize,Table **table);
	
	Table(const Table&) = delete;

	Table& operator=(const Table&) = delete;

	~Table();

	Iterator* newIterator(const ReadOptions&) const;

	uint64_t ApproximateOffsetOf(const Slice& key) const;

private:
	friend class TableCache;
	
	struct Rep;
	
	struct Iterator* BlockReader(void*, const ReadOptions&, const Slice&);
	
	explicit Table(Rep * rep):rep_(rep){}
	
	Status InternalGet(const ReadOptions&, const Slice& key, void* arg,
		void (*handle_result) (void* arg, const Slice& k, const Slice& v));

	void readMeta(const Footer & footer);

	void readFilter(const Slice& filterHandleValue);

private:
	Rep* const rep_;
};



}
