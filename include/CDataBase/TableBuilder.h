/*!
 * \file TableBuilder.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 * 
 */
#pragma once
#include <cstdint>
#include "CDataBase/Options.h"
#include "CDataBase/Status.h"
namespace CDB{
	class BlockBuilder;

	class BlockHandle;

	class WritableFile;

class TableBuilder {

public:
	TableBuilder(const Options& options, WritableFile* file);

	TableBuilder(const TableBuilder&) = delete;

	TableBuilder& operator=(const TableBuilder&) = delete;

	~TableBuilder();

	Status changeOptions(const Options& options);

	void add(const Slice& key, const Slice& value);

	void flush();

	Status status() const;

	Status finish();

	void abandon();

	uint64_t numEntires() const;

	uint64_t fileSize() const;

private:
	
	bool ok() const { return status().ok(); }

	void writeBlock(BlockBuilder* block, BlockHandle* handle);

	void writeRawBlock(const Slice& data, CompressionType, BlockHandle* handle);

	struct Rep;

	Rep* rep_;
};
		

}
