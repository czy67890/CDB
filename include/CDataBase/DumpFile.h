/*!
 * \file DumpFile.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include <string>
namespace CDB{
	
	Status dumpFile(Env* env, const std::string& fname, WritableFile* dst);

}