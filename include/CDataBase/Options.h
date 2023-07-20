/*!
 * \file Options.h
 *
 * \author czy
 * \date 2023.07.11
 *
 * 
 */
#pragma once
#include <cstddef>
namespace CDB
{
	class Cache;
	class Comparator;
	class Env;
	class FilterPolicy;
	class Logger;
	class Snapshot;

	enum CompressionType {
		KNoCompression = 0x0,
		KSnappyCompression = 0x1,
		KZstdCompression = 0x2
	};


	struct Options {
		Options();

		const Comparator* comparator;

		bool create_if_missing = false;

		bool error_if_exists = false;

		bool paranoid_checks = false;

		Env* env;

		Logger* infoLog = nullptr;

		size_t write_buffer_size = 4 * 1024 * 1024;

		int max_open_files = 1000;

		Cache* block_cache = nullptr;

		size_t block_size = 4 * 1024;

		int block_restart_interval = 16;

		size_t max_file_size = 2 * 1024 * 1024;

		CompressionType  compression = KSnappyCompression;

		int zstd_compression_level = 1;

		bool reuse_logs = false;

		const FilterPolicy* filter_policy = nullptr;


	};

	struct ReadOptions {
		bool verify_checksums = false;
		
		bool fill_cache = true;
		
		const Snapshot* snapshot = nullptr;
	};


	struct WriteOptions {
		WriteOptions() = default;

		bool sync = false;
	};





}


