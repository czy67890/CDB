#pragma once
#include <string_view>
/*!
 * \file FilterPolicy.h
 *
 * \author czy
 * \date 2023.07.11
 *
 * 
 */
namespace CDB{
class Slice;


class FilterPolicy{
public:
	
	virtual ~FilterPolicy();

	virtual const char* name() const;

	virtual void createFilter(const Slice* keys, int n, std::string* dst) const = 0;

	virtual bool keyMayMatch(const Slice& key, const Slice& filter) const = 0;

};

const FilterPolicy* newBloomFilterPolicy(int bitsPerKey);

}