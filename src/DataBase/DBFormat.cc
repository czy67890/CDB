#include "DataBase/DBFormat.h"

#include <cstdio>
#include <sstream>
#include "Util/Coding.h"
using namespace CDB;

/*!
 *  @brief static function only be seed in this file
		   so that other file ont be affetcted
 *  @param
 *  @return 
 *
 */
static uint64_t packSequenceAndType(uint64_t seq,ValueType t){
	assert(seq < kMaxSequenceNumber);
	assert(t <= kValueTypeForSeek);
	return (seq << 8) | t;
}

void AppendInternalKey(std::string* result, const ParsedInternalKey& key){
	result->append(key.user_key.data(),key.user_key.size());
	PutFixed64(result,packSequenceAndType(key.sequence,key.type));
}

std::string ParsedInternalKey::DebugString() const{
	std::ostringstream ss;
	ss << '\'' << escapeString(user_key) << "' @ " << sequence << " : " << static_cast<int>(type);
	return ss.str();
}

std::string InternalKey::DebugString() const{
	ParsedInternalKey parsed;
	if(ParseInternalKey(rep_,&parsed)){
		return parsed.DebugString();
	}
	std::ostringstream ss;
	ss << "(bad)" << escapeString(rep_);
	return ss.str();
}

const char * InternalKeyComparator::name() const{
	return "this is a comparator writeen by CZY";
}

/*!
 *  @brief函数比较两个slice的大小,如果slice的内容一样大,那么返回序列号较小的那一个
 *  @param
 *  @return 
 *
 */

int InternalKeyComparator::compare(const Slice& a, const Slice& b) const{
	
	int r = user_comparator_->compare(ExtractUserKey(a), ExtractUserKey(b));
	if(r == 0){
		const uint64_t anum = DecodeFixed64(a.data() + a.size() - 8);
		const uint64_t bnum = DecodeFixed64(b.data() + b.size() - 8);
		if(anum > bnum){
			r = -1;
		}
		else if(anum < bnum){
			r = +1;
		}
	}
	return r;
}

void CDB::InternalKeyComparator::findShortestSeparator(std::string* start, const Slice& limit) const
{
	Slice userStart = ExtractUserKey(*start);
	Slice userLimit = ExtractUserKey(limit);
	std::string tmp(userStart);
	user_comparator_->findShortestSeparator(&tmp, userLimit);
	if(tmp.size() < userStart.size() && user_comparator_->compare(userStart,tmp) < 0){
		PutFixed64(&tmp,packSequenceAndType(kMaxSequenceNumber,kValueTypeForSeek));
		assert(this->compare(*start,tmp) < 0);
		assert(this->compare(tmp, limit) < 0);
		start->swap(tmp);
	}
}

void CDB::InternalKeyComparator::findShortSuccessor(std::string* key) const
{
	Slice userKey = ExtractUserKey(*key);
	std::string tmp(userKey);
	user_comparator_->findShortSuccessor(&tmp);
	if(tmp.size() < userKey.size() && user_comparator_->compare(userKey,tmp) < 0){
		PutFixed64(&tmp,packSequenceAndType(kMaxSequenceNumber,kValueTypeForSeek));
		assert(this->compare(*key,tmp) < 0);
		key->swap(tmp);
	}
}






const char* InternalFilterPolicy::name() const
{
	return user_policy_->name();
}

void CDB::InternalFilterPolicy::createFilter(const Slice* keys, int n, std::string* dst) const
{
	Slice* mkey = const_cast<Slice*> (keys);
	for (int i = 0; i < n; ++i) {
		mkey[i] = ExtractUserKey(keys[i]);
	}
	user_policy_->createFilter(keys, n, dst);
}

bool CDB::InternalFilterPolicy::keyMayMatch(const Slice& key, const Slice& filter) const
{
	return user_policy_->keyMayMatch(ExtractUserKey(key),f);
}

LookupKey::LookupKey(const Slice& user_key, SequenceNumber sequence)
{
	size_t usize = user_key.size();
	size_t needed = usize + 13;
	char* dst;
	if(needed <= sizeof(space_)){
		dst = space_;
	}
	else{
		dst = new char[needed];
	}
	start_ = dst;
	dst = EncodeFixed32(dst, usize + 8);
	kstart_ = dst;
	memcpy(dst,user_key.data(),usize);
	dst += usize;
	EncodeFixed64(dst, packSequenceAndType(s, kValueTypeForSeek));
	dst += 8;
	end_ = start_;
}
