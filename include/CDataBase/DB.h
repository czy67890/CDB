/*!
 * \file DB.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include<cstdint>
#include<cstdio>
#include "CDataBase/Iterator.h"
#include "CDataBase/Options.h"

namespace CDB{

	static const int KMajirVersion = 1;
	static const int KMinorVersion = 23;

	struct Options;
	struct ReadOptions;
	struct WriteOptions;
	struct WriteBatch;

	class Snapshot{
	protected:
		virtual ~Snapshot();
	};

	struct Range {
		Range() = default;
		Range(const Slice& s, const Slice& l)
			:start(s),limit(l)
		{
			
		}
		Slice start;
		Slice limit;
	};

class DB{
public:
	static Status open(const Options& options, const std::string& name, DB** dbptr);
	
	DB() = default;
	
	DB(const DB&) = delete;
	
	DB& operator=(const DB&) = delete;
	
	virtual ~DB();

	virtual Status put(const WriteOptions& options, const Slice& key, const Slice& value) = 0;
	
	virtual Status deleteK(const WriteOptions& opeions, const Slice& key, std::string* value) = 0;

	virtual Status get(const ReadOptions& options, const Slice& key, std::string* value) = 0;

	virtual Iterator* newIterator(const ReadOptions& options) = 0;

	virtual const Snapshot* getSnapshot() = 0;

	virtual void releaseSnapshot(const Snapshot* snapshot) = 0;

	virtual bool getProperty(const Slice& property, std::string* value) = 0;

	virtual void getApproximateSizes(const Range* rrange, int n, uint64_t* size) = 0;

	virtual void compactRange(const Slice* begin, const Slice* end) = 0;

};

Status destoryDB(const std::string& name, const Options& options);

Status repairDB(const std::string& dbname, const Options& options);



}
