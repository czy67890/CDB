#include "DataBase/LogReader.h"
#include <cstdio.h>
#include "CDataBase/Env.h"
#include "Util/Coding.h"
#include "Util/Crc32.h"


using namespace CDB;
using namespace Log;

CDB::Log::Reader::Reporter::~Reporter() = default;

CDB::Log::Reader::Reader(SequentialFile* file, Reporter* reporter, bool checkSum, uint64_t initOffset)
	:file_(file),reporter_(reporter),checkSum_(checkSum),backingStore_(new char[KBlockSize]),buffer_(),eof_(false),
	lastRecordOffset_(0),endOfBufferOffset_(0),initOffset_(initOffset),resyncing_(initOffset > 0)
{
}

CDB::Log::Reader::~Reader()
{
	delete[]backingStore_;
}
