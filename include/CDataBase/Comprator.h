/*!
 * \file Comprator.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include <string>


namespace CDB{
	class Slice;


class Comparator{
public:
	virtual ~Comparator();

	virtual int compare(const Slice& a, const Slice& b)const = 0;

	virtual const char* name() const = 0;

	virtual void findShortestSeparator(const std::string* start, const Slice& limit) const = 0;
	
	virtual void findShortSuccessor(const std::string* key) const = 0;

};

const Comparator* byteWiseComparator();
}
