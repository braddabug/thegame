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

	float AngleDiff(float angle1, float angle2)
	{
		// based on https://stackoverflow.com/questions/1878907/the-smallest-difference-between-2-angles
		float a = angle1 - angle2;
		float a1 = (a + 3.14159265359f);
		const float twopi = 3.14159265359f * 2.0f;

		return (a1 - floor(a1 / twopi) * twopi) - 3.14159265359f;
	}

	constexpr uint32 CalcHash(const char *str);

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
}

#endif // UTILS_H
