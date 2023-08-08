/*!
 * \file MemTable.h
 *
 * \author czy
 * \date 2023.08.02
 *
 * 
 */
#pragma once
#include <string>
#include "DataBase/DBFormat.h"
#include "DataBase/SkipList.h"
#include "CDataBase/DB.h"
#include "Util/Allocator.h" 
namespace CDB{
	
	class InternalKeyComparator;
	
	class MemTableIterator;

	class MemTable { 
	public:
		explicit MemTable(const InternalKeyComparator& cmp);

		~MemTable();

		MemTable(const MemTable&) = delete;

		MemTable& operator(const MemTable&) = delete;

		void ref() { ++refs_; }

		void unRef(){
			--refs_;
			assert(refs_ >= 0);
			if(refs_ <= 0){
				delete this;
			}
		}

		size_t approximateMemUsage();

		void add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value);

		bool get(const LookupKey& key, std::string* value, Status* s);




		Iterator* newIterator();


	private:
		friend class MemTableIterator;

		friend class MemTableBackWardIterator;

		struct KeyComparator{
			const InternalKeyComparator cmp;

			explicit KeyComparator(const InternalKeyComparator & c) : 
				:cmp(c){}

			int operator()(const char* a, const char* b) const;
		};

		typedef SkipList<const char*, KeyComparator> Table;



		~MemTable();

		KeyComparator cmp_;
	
		int refs_;

		Allocator allocator_;

		Table table_;
	};

	
} 