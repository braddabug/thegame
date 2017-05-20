#include "StringManager.h"
#include <atomic>
#include <cassert>

struct StringManagerData
{
	static const uint32 MaxStrings = 1000;
	std::atomic<int32> RefCounts[MaxStrings];
	char Strings[MaxStrings][256];
};

StringManagerData* StringManager::m_data = nullptr;

void StringManager::SetGlobalData(StringManagerData** data)
{
	if (*data == nullptr)
	{
		*data = new StringManagerData();
		memset(*data, 0, sizeof(StringManagerData));
	}

	m_data = *data;
}

PersistantString StringManager::Create(const char* source)
{
	uint32 len = (uint32)strlen(source); // TODO: support UTF8
	if (len > 255) return nullptr;

	for (uint32 i = 0; i < StringManagerData::MaxStrings; i++)
	{
		if (m_data->RefCounts[i] == 0)
		{
			int expected = 0;
			if (m_data->RefCounts[i].compare_exchange_weak(expected, 1))
			{
#ifdef _WIN32
				strcpy_s(m_data->Strings[i], source);
#else
				strcpy(m_data->Strings[i], source);
#endif
				return m_data->Strings[i];
			}
		}
	}

	return nullptr;
}

void StringManager::Acquire(PersistantString string)
{
	int32 index = (int32)((string - m_data->Strings[0]) / 256);

	// early exit if string isn't even one of ours
	if (index < 0 || index >= StringManagerData::MaxStrings)
		return;

	assert("Refcount cannot be zero. Something is broken." && m_data->RefCounts[index] > 0);

	m_data->RefCounts[index]++;

}

void StringManager::Release(PersistantString string)
{
	int32 index = (int32)((string - m_data->Strings[0]) / 256);

	// early exit if string isn't even one of ours
	if (index < 0 || index >= StringManagerData::MaxStrings)
		return;

	m_data->RefCounts[index]--;

	assert("Refcount cannot be negative. Something is broken." && m_data->RefCounts[index] >= 0);
}