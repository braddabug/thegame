#ifndef UTILS_H
#define UTILS_H

#include "Common.h"

namespace Utils
{
	int CompareI(const char* strA, uint32 strALength, const char* strB);

	uint32 CalcHash(const uint8 *str);

	template<typename T, size_t size>
	class HashTable
	{
		uint32 m_hashes[size];
		T m_data[size];
		bool m_active[size];
		uint32 m_count;

	public:

		enum class Result
		{
			Success,

			ErrorFull,
			ErrorDuplicateHash
		};

		Result Add(uint32 hash, const T& data)
		{
			for (uint32 i = 0; i < size; i++)
			{
				uint32 index = (hash + i) % size;

				if (m_active[index] == false)
				{
					m_active[index] = true;
					m_hashes[index] = hash;
					m_data[index] = data;
					m_count++;

					return Result::Success;
				}
				else if (m_hashes[index] == hash)
				{
					return Result::ErrorDuplicateHash;
				}
			}

			return Result::ErrorFull;
		}

		bool Remove(uint32 hash)
		{
			for (uint32 i = 0; i < size; i++)
			{
				uint32 index = (hash + i) % size;

				if (m_active[index] == false)
					return false;

				if (m_hashes[index] == hash)
				{
					m_active[index] = false;
					m_count--;
					return true;
				}
			}

			return false;
		}

		bool GetPtr(uint32 hash, T** result)
		{
			for (uint32 i = 0; i < size; i++)
			{
				uint32 index = (hash + i) % size;

				if (m_active[index] == false)
					return false;

				if (m_hashes[index] == hash)
				{
					*result = &m_data[index];
					return true;
				}
			}

			return false;
		}

		uint32 GetCount() { return m_count; }
	};
}

#endif // UTILS_H
