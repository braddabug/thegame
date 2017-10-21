#include "WaitManager.h"

struct WaitManagerData
{
	static const uint32 Capacity = 1000;

	// when checksum is even (or 0) that means it's dead, otherwise alive (in use)
	uint16 Checksums[Capacity];
};

WaitManagerData* WaitManager::m_data = nullptr;

void WaitManager::SetGlobalData(WaitManagerData** data)
{
	if (*data == nullptr)
	{
		*data = (WaitManagerData*)g_memory->AllocAndKeep(sizeof(WaitManagerData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(WaitManagerData));
	}
	m_data = *data;
}

void WaitManager::Shutdown()
{
	g_memory->FreeTrack(m_data->Checksums, __FILE__, __LINE__);
	g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	m_data = nullptr;
}

void WaitManager::Reset()
{
	memset(m_data, 0, sizeof(WaitManagerData));
}

WaitHandle WaitManager::CreateWait()
{
	for (size_t i = 0; i < m_data->Capacity; i++)
	{
		if ((m_data->Checksums[i] & 1) == 0)
		{
			// it's dead, so we can grab it
			m_data->Checksums[i]++;
			WaitHandle wait = (m_data->Checksums[i] << 16) | (i & 0xffff);

			return wait;
		}
	}

	return INVALID_WAIT;
}

WaitManager::WaitStatus WaitManager::GetWaitStatus(WaitHandle wait)
{
	uint32 index = WAIT_GET_INDEX(wait);

	if (index >= WaitManagerData::Capacity)
		return WaitStatus::Done;

	uint32 checksum = (wait & 0xffff0000) >> 16;

	if (m_data->Checksums[index] == checksum)
		return WaitStatus::Waiting;

	return WaitStatus::Done;
}

void WaitManager::SetWaitDone(WaitHandle wait)
{
	uint32 index = WAIT_GET_INDEX(wait);

	if (index >= WaitManagerData::Capacity)
		return;

	uint32 checksum = (wait & 0xffff0000) >> 16;

	if (m_data->Checksums[index] == checksum)
		m_data->Checksums[index]++;
}
