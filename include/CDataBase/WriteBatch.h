/*!
 * \file WriteBatch.h
 *
 * \author czy
 * \date 2023.07.12
 *
 * 
 */
#pragma once
#include <string>
#include "CDataBase/Status.h"

namespace CDB{
	class Slice;


	class WriteBatch{
	public:
		class Handler{
		public:
		
		virtual ~Handler();

		virtual void put(const Slice& key, const Slice& value) = 0;

		virtual void delete(const Slice& key) = 0;



		};

		WriteBatch();

		WriteBatch(const WriteBatch&) = default;

		WriteBatch& operator=(const WriteBatch&) = default;

		~WriteBatch();

		void put(const Slice& key, const Slice& value);

		void deleteK(const Slice& key);

		void clear();

		size_t approximateSize() const;

		void append(const WriteBatch& source);

		Status iterate(Handler* handler) const;
	
	private:
		friend class WriteBatchInternal;

		std::string rep_;
	};

}
