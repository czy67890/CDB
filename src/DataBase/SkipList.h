/*!
 * \file SkipList.h
 * \author czy
 * \date 2023.07.01
 */
#pragma once
#include <atomic>
#include <cassert>
#include <cstdlib>
#include "Util/Random.h"
#include "Util/Allocator.h"
namespace CDB {
	template<typename Key, class Cmp>
	class SkipList {

	private:
		struct Node;

	public:
		explicit SkipList(Cmp cmp,Allocator *alloc);

		SkipList(const SkipList&) = delete;

		SkipList& operator=(const SkipList&) = delete;

		void insert(const Key& key);

		bool contians(const Key& key) const;

		class Iterator {
		public:

			explicit Iterator(const SkipList* list);

			bool valid() const;

			const Key& key() const;

			void next();

			void prev();

			void seek(const Key& target);

			void seekToLast();

			void seekToFirst();

		private:
			const SkipList* list_;
			Node* node_;
		};

		

	private:
		static constexpr int KMaxHeight = 12;

		inline int getMaxHeight() const { return maxHeight_.load(std::memory_order_relaxed); }

		Node* newNode(const Key& key, int height);

		int randomHeight();

		bool equal(const Key& a, const Key& b) const { return cmper_(a, b) == 0; }

		bool keyIsAfterNode(const Key& key, Node* n) const{
			return (n != nullptr && (cmper_(n->key, key) < 0));
		}

		Node* findGreaterOrEqual(const Key& key, Node** prev) const;

		Node* findLessThan(const Key& key) const;

		Node* findLast() const;


	private:
		std::atomic<int> maxHeight_;
		Cmp const cmper_;
		Node* const head_;
		Random rand_;
		Allocator* const alloc_;
	};

	template <typename Key, class Cmp>
	struct SkipList<Key, Cmp>::Node {
		explicit Node(const Key& k) :key(k) {}

		Key const key;

		Node* next(int n) {
			assert(n >= 0);
			return next_[n].load(std::memory_order_acquire);
		}

		void setNext(int n, Node* x) {
			assert(n >= 0);
			next_[n].store(x, std::memory_order_release);
		}


		Node* noBarrierNext(int n) {
			assert(n >= 0);
			return next_[n].load(std::memory_order_relaxed);
		}

		void noBarrierSetNext(int n, Node* x) {
			assert(n >= 0);
			next_[n].store(x, std::memory_order_relaxed);
		}

	private:
		std::atomic<Node*> next_[1];
	};


	template<typename Key, class Cmp>
	void CDB::SkipList<Key,Cmp>::Iterator::seekToLast()
	{
		node_ = list_->findLast();
		if (node_ == list_->head_) {
			node_ = nullptr;
		}
	}

	template<typename Key, class Cmp>
	inline void SkipList<Key, Cmp>::Iterator::seekToFirst()
	{
		node_ = list_->head_->next(0);
	}

	template<typename Key, class Cmp>
	void CDB::SkipList<Key, Cmp>::Iterator::seek(const Key& target)
	{
		node_ = list_->findGreaterOrEqual(target, nullptr);
	}

	template<typename Key, class Cmp>
	CDB::SkipList<Key, Cmp>::Iterator::Iterator(const SkipList* list)
		:list_(list), node_(nullptr)
	{

	}

	template<typename Key, class Cmp>
	inline bool SkipList<Key, Cmp>::Iterator::valid() const
	{
		return node_ != nullptr;
	}

	template<typename Key, class Cmp>
	inline const Key& SkipList<Key, Cmp>::Iterator::key() const
	{
		assert(valid());
		return node_->key;
	}

	template<typename Key, class Cmp>
	inline void SkipList<Key, Cmp>::Iterator::next()
	{
		assert(valid());
		node_ = node_->next(0);
	}

	template<typename Key, class Cmp>
	inline void SkipList<Key, Cmp>::Iterator::prev()
	{
		assert(valid());
		node_ = list_->findLessThan(node_->key);
		if (node_ == list_->head_) {
			node_ = nullptr;
		}
	}


	template<typename Key, class Cmp>
	typename CDB::SkipList<Key, Cmp>::Node* CDB::SkipList<Key, Cmp>::findLast() const
	{
		Node* x = head_;
		int level = getMaxHeight() - 1;
		while(true){
			Node* next = x->next(level);
			if(next == nullptr){
				if(level == 0){
					return x;
				}
				else{
					level--;
				}
			}
			else{
				x = next;
			}
		}
	}

	template<typename Key, class Cmp>
	typename CDB::SkipList<Key, Cmp>::Node* CDB::SkipList<Key, Cmp>::findLessThan(const Key& key) const
	{
		Node* x = head_;
		int level = getMaxHeight() - 1;
		while(true){
			assert(x == head_ || cmper_(x->key,key) < 0);
			Node* next = x->next(level);
			if(next == nullptr || cmper_(next->key,key) >= 0){
				if(level == 0){
					return x;
				}
				else{
					level--;
				}
			}
			else{
				x = next;
			}
		}
	}

	template<typename Key, class Cmp>
	CDB::SkipList<Key, Cmp>::SkipList(Cmp cmp,Allocator *alloc)
		:cmper_(cmp),head_(newNode(0,KMaxHeight)),
		maxHeight_(1),rand_(0xdeadbeef),alloc_(alloc)
	{
		for (int i = 0; i < KMaxHeight;++i)	 {
			head_->setNext(i,nullptr);
		}
	}

	template<typename Key, class Cmp>
	inline void SkipList<Key, Cmp>::insert(const Key& key)
	{
		Node* prev [KMaxHeight];
		Node* x = findGreaterOrEqual(key,prev);
		///这里阻止了多重插入
		///多重插入会导致整个程序崩溃
		assert(x == nullptr || !equal(key, x->key));

		int height = randomHeight();

		if(height > getMaxHeight()){
			for (int i = getMaxHeight(); i < height; ++i) {
				prev[i] = head_;
			}
			maxHeight_.store(height,std::memory_order_relaxed);
		
		}

		x = newNode(key,height);

		for(int i = 0 ;i < height; ++i){
			x->noBarrierSetNext(i, prev[i]->noBarrierNext(i));
			///这里保证next的有效性
			prev[i]->setNext(i, x);
		}
	}

	template<typename Key, class Cmp>
	inline bool SkipList<Key, Cmp>::contians(const Key& key) const
	{
		Node *x = findGreaterOrEqual(key,nullptr);
		if(x != nullptr && equal(key,x->key)){
			return true;
		}
		else{
			return false;
		}
	}




template<typename Key, class Cmp>
inline typename CDB::SkipList<Key, Cmp>::Node* SkipList<Key, Cmp>::newNode(const Key& key, int height)
{
	char* const nodeMem = alloc_->allocateAligned(sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
	return new (nodeMem) Node(key);
}

template<typename Key, class Cmp>
inline int SkipList<Key, Cmp>::randomHeight()
{
	static const unsigned int KBranching = 4;
	int height = 1;
	while(height < KMaxHeight && rand_.OneIn(KBranching)){
		height++;
	}
	assert(height > 0);
	assert(height <= KMaxHeight);
	return height;
}

template<typename Key, class Cmp>
inline typename CDB::SkipList<Key, Cmp>::Node* SkipList<Key, Cmp>::findGreaterOrEqual(const Key& key, Node** prev) const
{
	Node* x = head_;
	int level = getMaxHeight() - 1;
	while(true){
		Node* next = x->next(level);
		if(keyIsAfterNode(key,next)){
			x = next;
		}
		else{
			if(prev != nullptr){
				prev[level] = x;
			}
			if(level == 0){
				return next;
			}
			else{
				level--;
			}
		}
	}
}

}