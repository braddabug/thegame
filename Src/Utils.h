#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <cmath>
#include "Common.h"

namespace Utils
{
	static inline int Wrap(int value, int maxExclusive)
	{
		if (value > 0)
			return value % maxExclusive;
		if (value < 0)
			return (value + 1) % maxExclusive + maxExclusive - 1;
		return 0;
	}

	int CompareI(const char* strA, uint32 strALength, const char* strB);

	float AngleDiff(float angle1, float angle2);

	constexpr uint32 CalcHash(const char *str)
	{
		// this is djb2: http://www.cse.yorku.ca/~oz/hash.html
		uint32 hash = 5381;
		int c = 0;

		while (c = *str++)
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

		return hash;
	}

	constexpr uint32 CalcHashI(const char *str)
	{
		// this is djb2: http://www.cse.yorku.ca/~oz/hash.html
		uint32 hash = 5381;
		int c = 0;

		while (c = *str++)
		{
			if (c >= 'a' && c <= 'z')
				c -= 'a' - 'A';
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		}

		return hash;
	}

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
				if (index >= size)
					return false;

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

		void Clear()
		{
			m_count = 0;
			memset(m_active, 0, sizeof(m_active));
		}

		
	};

	class HashTableUtils
	{
	public:
		template<typename T>
		static bool Reserve(uint32 hash, uint32* hashes, T* values, bool* active, uint32 tableSize, uint32* index)
		{
			if (tableSize == 0)
				return false;

			uint32 ix = hash % tableSize;

			for (uint32 i = 0; i < tableSize; i++)
			{
				uint32 finalIndex = (ix + i) % tableSize;
				if (active[finalIndex] == false)
				{
					hashes[finalIndex] = hash;
					active[finalIndex] = true;

					if (index) *index = finalIndex;

					return true;
				}
			}

			return false;
		}

		template<typename T>
		static bool Find(uint32 hash, const uint32* hashes, const T* values, const bool* active, uint32 tableSize, uint32* index)
		{
			if (tableSize == 0)
				return false;
			 
			uint32 ix = hash % tableSize;

			for (uint32 i = 0; i < tableSize; i++)
			{
				uint32 finalIndex = (ix + i) % tableSize;
				if (hashes[finalIndex] == hash)
				{
					if (active[finalIndex] == true)
					{
						*index = finalIndex;
						return true;
					}
					else
					{
						return false;
					}
				}
			}

			return false;
		}
	};

	struct StopwatchData;

	class Stopwatch
	{
		bool m_running;
		uint64_t m_timeElapsed;
		uint64_t m_timeStarted;
		static StopwatchData* m_data;

	public:
		static void SetGlobalData(StopwatchData** data);
		static uint64_t GetCurrentTicks();

		Stopwatch();
		void Start();
		void Stop();
		void Reset();

		uint64_t GetElapsedTicks();
		uint64_t GetElapsedMilliseconds();
		unsigned int GetElapsedMilliseconds32();

	private:
		void getFrequency();
	};

	void CopyString(char* destination, const char* source, uint32 destLength);
	void CopyString(char* destination, const char* source, uint32 destLength, uint32 sourceLength);
}

#endif // UTILS_H
