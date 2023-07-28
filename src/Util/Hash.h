/*!
 * \file Hash.h
 *	A Func With Hash Func
 * \author czy
 * \date 2023.07.28
 *
 * 
 */

#pragma  once
#include <cstddef>
#include <cstdint>


namespace CDB{
	uint32_t Hash(const char *data,size_t n,uint32_t seed);
}
