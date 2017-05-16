#include "WaitManager.h"

WaitManagerData* WaitManager::m_data = nullptr;

int WaitManager::CreateWait()
{
	for (size_t i = 0; i < m_data->Checksums.size(); i++)
	{
		if ((m_data->Checksums[i] & 1) == 0)
		{
			// it's dead, so we can grab it
			m_data->Checksums[i]++;
			int wait = (m_data->Checksums[i] << 16) | (i & 0xffff);

			return wait;
		}
	}

	// make sure we don't create too many
	if (m_data->Checksums.size() == 0xffff - 1)
		return INVALID_WAIT;

	// still here? insert a new spot
	m_data->Checksums.push_back(1);
	int wait = (1 << 16) | ((m_data->Checksums.size() - 1) & 0xffff);
	return wait;
}

WaitManager::WaitStatus WaitManager::GetWaitStatus(int wait)
{
	int index = wait & 0xffff;

	if (index >= (int)m_data->Checksums.size())
		return WaitStatus::Done;

	int checksum = (wait & 0xffff0000) >> 16;

	if (m_data->Checksums[index] == checksum)
		return WaitStatus::Waiting;

	return WaitStatus::Done;
}

void WaitManager::SetWaitDone(int wait)
{
	int index = wait & 0xffff;

	if (index >= (int)m_data->Checksums.size())
		return;

	int checksum = (wait & 0xffff0000) >> 16;

	if (m_data->Checksums[index] == checksum)
		m_data->Checksums[index]++;
}
