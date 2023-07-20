#include <atomic>
#include <set>
#include <gtest/gtest.h>
#include <stdint.h>
#include "DataBase/SkipList.h"
namespace CDB {
	using Key = uint64_t;
	struct TestCmp {

		int operator()(const Key& a, const Key &b) const{
			if(a < b){
				return -1;
			}
			else if(a > b){
				return 1;
			}
			else{
				return 0;
			}
		}
	};

	TEST(SkipListTest,Empty){
		TestCmp cmp;
		SkipList<Key, TestCmp> list(cmp);
		ASSERT_TRUE(!list.contians(10));
		SkipList<Key, TestCmp>::Iterator iter(&list);
		ASSERT_TRUE(!iter.valid());
		iter.seekToFirst();
		ASSERT_TRUE(!iter.valid());
		iter.seek(100);
		ASSERT_TRUE(!iter.valid());
		iter.seekToLast();
		ASSERT_TRUE(!iter.valid());
	}

	TEST(SkipListTest, InsertAndLookUp) {
		const int N = 2000;
		const int R = 5000;
		Random rnd(1000);
		std::set<Key> keys;
		TestCmp cmp;
		SkipList<Key, TestCmp> list(cmp);
		for(int i = 0;i < N;++i){
			Key key = rnd.Next();
			if(keys.insert(key).second){
				list.insert(key);
			}
		}
		for (int i = 0; i < R; i++) {
			if (list.contians(i)) {
				ASSERT_EQ(keys.count(i), 1);
			}
			else {
				ASSERT_EQ(keys.count(i), 0);
			}
		}
		{
			SkipList<Key, TestCmp>::Iterator iter(&list);
			ASSERT_TRUE(!iter.valid());

			iter.seek(0);
			ASSERT_TRUE(iter.valid());
			ASSERT_EQ(*(keys.begin()), iter.key());

			iter.seekToFirst();
			ASSERT_TRUE(iter.valid());
			ASSERT_EQ(*(keys.begin()), iter.key());

			iter.seekToLast();
			ASSERT_TRUE(iter.valid());
			ASSERT_EQ(*(keys.rbegin()), iter.key());
		}

		// Forward iteration test
		for (int i = 0; i < R; i++) {
			SkipList<Key, TestCmp>::Iterator iter(&list);
			iter.seek(i);

			// Compare against model iterator
			std::set<Key>::iterator model_iter = keys.lower_bound(i);
			for (int j = 0; j < 3; j++) {
				if (model_iter == keys.end()) {
					ASSERT_TRUE(!iter.valid());
					break;
				}
				else {
					ASSERT_TRUE(iter.valid());
					ASSERT_EQ(*model_iter, iter.key());
					++model_iter;
					iter.next();
				}
			}
		}
	}
	



}