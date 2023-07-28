#include "CDataBase/Comprator.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include <cassert>
#include "CDataBase/Slice.h"
#include "Util/Logging.h"
#include "Util/NoDestructor.h"


namespace CDB{
	
	Comparator::~Comparator() = default;

namespace {
	class ByteWiseComparatorImpl :public Comparator{
	public:
		ByteWiseComparatorImpl() = default;
		
		const char* name() const override
		{
			return "CDB's ByteWiseComparator";
		}

		int compare(const Slice& a, const Slice& b) const override{
			return a.compare(b);
		}

		void findShortestSeparator(std::string* start, const Slice& limit) const override{
			size_t minLength = std::min(start->size(),limit.size());
			size_t diffIndex = 0;
			while(diffIndex < minLength && ((*start)[diffIndex] == limit[diffIndex])) {
				diffIndex++;
			}
			if(diffIndex == minLength){
				//when start is limit preFix ,then we do nothing
			}
			else{
				uint8_t diff_byte = static_cast<uint8_t>((*start)[diffIndex]);
				if(diff_byte < static_cast<uint8_t>(0xff) && diff_byte + 1 < static_cast<uint8_t>(limit[diffIndex])){
					(*start)[diffIndex]++;
					start->resize(diffIndex + 1);
					assert(compare(*start,limit) < 0);
				}
			}
		}

		void findShortSuccessor(std::string *key) const override{
			size_t n = key->size();
			for (size_t i = 0; i < n;++i) {
				const uint8_t byte = (*key)[i];
				if(byte != static_cast<uint8_t> (0xff)){
					(*key)[i] = byte + 1;
					key->resize(i + 1);
					return;
				}
			}
		}
		



	};

}		

const Comparator * BytewiseComparator(){
	static NoDestructor<ByteWiseComparatorImpl> singleton;
	return singleton.get();
}

}