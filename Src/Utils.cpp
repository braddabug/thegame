#include "Utils.h"
#include <cstdlib>
#include <cstring>
#include "MemoryManager.h"

#ifdef _WIN32
#include "CleanWindows.h"
#else
#include <time.h>
#endif

namespace Utils
{
	int CompareI(const char* strA, uint32 strALength, const char* strB)
	{
		int strBLength = (int)strlen(strB);
		int lenDiff = strALength - strBLength;

		if (lenDiff != 0)
			return lenDiff;

#ifdef _WIN32
		return _strnicmp(strA, strB, strALength);
#else
		return strncasecmp(strA, strB, strALength);
#endif
	}

	float AngleDiff(float angle1, float angle2)
	{
		// based on https://stackoverflow.com/questions/1878907/the-smallest-difference-between-2-angles
		float a = angle1 - angle2;
		float a1 = (a + 3.14159265359f);
		const float twopi = 3.14159265359f * 2.0f;

		return (a1 - floor(a1 / twopi) * twopi) - 3.14159265359f;
	}

	uint32 CalcHash(const uint8* bytes, size_t length)
	{
		// this is djb2: http://www.cse.yorku.ca/~oz/hash.html
		uint32 hash = 5381;

		for (size_t i = 0; i < length; i++)
			hash = ((hash << 5) + hash) + bytes[i]; /* hash * 33 + c */

		return hash;
	}


	struct StopwatchData
	{
#ifdef _WIN32
		int64 Frequency;
#endif
	};


	StopwatchData* Stopwatch::m_data = nullptr;

	void Stopwatch::SetGlobalData(StopwatchData** data)
	{
		if (*data == nullptr)
		{
			*data = (StopwatchData*)g_memory->AllocTrack(sizeof(StopwatchData), __FILE__, __LINE__);

#ifdef _WIN32
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			(*data)->Frequency = frequency.QuadPart;
#elif defined NXNA_PLATFORM_APPLE
			mach_timebase_info(&(*data)->Info);
#endif
		}

		m_data = *data;
	}

	uint64_t Stopwatch::GetCurrentTicks()
	{
#ifdef _WIN32
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);
		return (uint64_t)ticks.QuadPart;
#elif defined NXNA_PLATFORM_APPLE
		return mach_absolute_time();
#else
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		return now.tv_sec * 1000000000 + now.tv_nsec;
#endif
	}

	Stopwatch::Stopwatch()
	{
		m_running = false;
		m_timeElapsed = 0;
	}

	void Stopwatch::Start()
	{
		if (m_running == false)
		{
			m_running = true;
			m_timeStarted = GetCurrentTicks();
		}
	}

	void Stopwatch::Stop()
	{
		if (m_running == true)
		{
			m_running = false;
			m_timeElapsed += GetCurrentTicks() - m_timeStarted;
		}
	}

	void Stopwatch::Reset()
	{
		m_running = false;
		m_timeElapsed = 0;
	}

	uint64_t Stopwatch::GetElapsedTicks()
	{
		if (m_running == false)
			return m_timeElapsed;

		return GetCurrentTicks() - m_timeStarted;
	}

	uint64_t Stopwatch::GetElapsedMilliseconds()
	{
#ifdef _WIN32
		return GetElapsedTicks() * 1000 / m_data->Frequency;
#elif defined NXNA_PLATFORM_APPLE
		return GetElapsedTicks() * m_info.numer / m_info.denom / 1000000;
#else
		return GetElapsedTicks() / 1000000;
#endif
	}

	unsigned int Stopwatch::GetElapsedMilliseconds32()
	{
#ifdef _WIN32
		return (unsigned int)(GetElapsedTicks() * 1000 / m_data->Frequency);
#elif defined NXNA_PLATFORM_APPLE
		return (unsigned int)(GetElapsedTicks() * m_info.numer / m_info.denom / 1000000);
#else
		return (unsigned int)(GetElapsedTicks() / 1000000);
#endif
	}
}