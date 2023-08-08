#include "DataBase/MemTable.h"
#include "DataBase/DBFormat.h"
#include "CDataBase/Comprator.h"
#include "CDataBase/Env.h"
#include "CDataBase/Iterator.h"
#include "Util/Coding.h"

namespace CDB{

	static Slice getLengthPreFixedSlice(const char *data){
		uint32_t len;
		const char* p = data;
		p = GetVarint32Ptr(p, p + 5, &len);
		return Slice(p,len);
	} 



	MemTable::MemTable(const InternalKeyComparator& cmp)
		:cmp_(cmp),refs_(0),table_(cmp_,&alloc_)
	{

	}

	MemTable::~MemTable()
	{
		assert(refs_ == 0);
	}

	size_t MemTable::approximateMemUsage()
	{
		return allocator_->memUsage();
	}



	int MemTable::KeyComparator::operator()(const char* a, const char* b) const
	{
		Slice aSlice = getLengthPreFixedSlice(a);
		Slice bSlice = getLengthPreFixedSlice(b);
		return cmp_.compare(aSlice,bSlice);
	}

	static const char * encodeKey(std::string *scratch,const Slice &target){
		scratch->clear();
		PutVarint32(scratch, target.size());
		scratch->append(target.data(),target.size());
		return scratch->data();
	}

	class MemTableIterator : public Iterator{
	public:
		explicit MemTableIterator(MemTable::Table *table)
			:iter(table){}	
	
		MemTableIterator(const MemTableIterator&) = delete;

		MemTableIterator& operator=(const MemTableIterator&) = delete;

		~MemTableIterator() = default;

		bool valid() const override { return iter.valid(); }

		bool seek(const Slice& key)  override { return iter_.seek(encodeKey(&tmp_, key)); }

		void seekToFirst() override {
			iter_.seekToFirst();
		};
			
		void seekToLast() override{
			iter_->seekToLast();
		} 

		void next() override
		{
			iter_.next();
		}

		void prev() override {
			iter.prev();
		}

		Slice key() const override{
			return getLengthPreFixedSlice(iter_.key());
		}

		Slice value() const override{
			Slice keySlice = getLengthPreFixedSlice(iter_.key());
			return getLengthPreFixedSlice(keySlice.data(),keySlice.size());
		}

		Status status () const override{
			return Status::OK();
		}
	private:
		MemTable::Table::Iterator iter_;
		std::string tmp_;
	};


	Iterator* MemTable::newIterator() {
		return new MemTableIterator(&table_);
	}

	void MemTable::add(SequenceNumber s,ValueType type,const Slice &key,const Slice &value){
		/// levelDB save the key and value in 
		/// Slice 
		/// the memory like this
		/// keySize :32bit of key length 4 byte
		/// keyBytes :char[keySize]; keySize byte
		/// tag  :uint64t((sequence << 8) | type) 8byte
		/// valueSize :32bit of value size ;4 byte
		/// valueByte : char[valueSize] ;valueSize byte  
		size_t keySize = key.size();
		size_t valSize = value.size();
		size_t internalKeySize = keySize + 8;
		const size_t encodedLen = VarintLength(internalKeySize) + VarintLength(valSize) + valSize + internalKeySize;
		const char* buf = allocator_.allocate(encodedLen);
		char* p = EncodeVarint32(buf, internalKeySize);
		std::memcpy(p,key.data(),keySize);
		p += keySize;
		EncodeFixed64(p, (s<<8) | type);
		p += 8;
		p = EncodeVarint32(p,valSize);
		std::memcpy(p,value.data(),valSize);
		assert(p + valSize == buf + encodedLen);
		table_.insert(buf);
	}


	bool MemTable::get(const LookupKey& key, std::string *value,Status *s){
		Slice memKey = key.memtable_key();
		Table::Iterator iter(&table_);
		iter.seek(memKey.data());
		if(iter.valid()){
			const char* entry = iter.key();
			uint32_t keyLen;
			const char* keyPtr = GetVarint32Ptr(entry, entry + 5, &keyLen);
			if(cmp_.cmp.user_comparator()->compare(Slice(keyPtr,keyLen - 8),key.user_key())){
				const uint64_t tag = DecodeFixed64(keyPtr + keyLen - 8);
				switch(static_cast<ValueType>(tag & 0xff)){
				case kTypeValue:{
					Slice v = getLengthPreFixedSlice(keyPtr + keyLen - 8);
					value->assign(v.data(),v,size());
					return true;
				}
				case kTypeDeletion:{
					*s = Status::NotFound(Slice());
					return true;
				}

				}
			}
			return false;
		}
	}

}

