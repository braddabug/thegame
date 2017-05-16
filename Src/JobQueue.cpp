#include "JobQueue.h"

JobQueueData* JobQueue::m_data;

void JobQueue::SetGlobalData(JobQueueData** data)
{
	if (*data == nullptr)
	{
		*data = new JobQueueData();
		memset(*data, 0, sizeof(JobQueueData));

#ifdef _WIN32
		InitializeCriticalSection(&(*data)->CS);
		InitializeConditionVariable(&(*data)->Signal);
#endif
	}

	m_data = *data;
	m_data->Active = true;

	// create the threads
#ifdef _WIN32
	for (uint32 i = 0; i < JobQueueData::MaxThreads; i++)
	{
		m_data->Threads[i] = CreateThread(nullptr, 0, threadProc, nullptr, 0, nullptr);
	}
#endif
}

void JobQueue::Shutdown(bool complete)
{
	m_data->Active = false;

#ifdef _WIN32
	WakeAllConditionVariable(&m_data->Signal);
#endif
	// wait until all jobs are done
	bool waiting = true;
	while (waiting)
	{
		waiting = false;

		for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
		{
			if (m_data->JobStates[i] != JobState::Inactive)
			{
				waiting = true;

				if (m_data->JobStates[i] == JobState::WaitingForMainThreadPickup)
				{
					m_data->Jobs[i].MainThreadFunc(i, m_data->Jobs[i].Data);
					m_data->JobStates[i] = JobState::Inactive;
				}
			}
		}
	}

#ifdef _WIN32
	WaitForMultipleObjects(JobQueueData::MaxThreads, m_data->Threads, TRUE, INFINITE);

	for (uint32 i = 0; i < JobQueueData::MaxThreads; i++)
	{
		CloseHandle(m_data->Threads[i]);
		m_data->Threads[i] = INVALID_HANDLE_VALUE;
	}
#endif

	if (complete)
		delete m_data;
}

bool JobQueue::AddJob(JobFunc asyncFunc, JobFunc mainThreadFunc, void* data)
{
	return AddDependantJob((uint32)-1, asyncFunc, mainThreadFunc, data);
}

bool JobQueue::AddDependantJob(uint32 parentJobHandle, JobFunc asyncFunc, JobFunc mainThreadFunc, void* data)
{
	const uint32 invalidJob = (uint32)-1;

#ifdef _WIN32
	EnterCriticalSection(&m_data->CS);

	bool result = false;
	for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
	{
		if (m_data->JobStates[i] == JobState::Inactive)
		{
			result = true;
			m_data->Jobs[i].AsyncFunc = asyncFunc;
			m_data->Jobs[i].MainThreadFunc = mainThreadFunc;
			m_data->Jobs[i].Data = data;

			if (parentJobHandle == invalidJob)
			{
				m_data->Jobs[i].Parent = nullptr;
				m_data->Jobs[i].FirstChild = nullptr;
				m_data->Jobs[i].Sibling = nullptr;
			}
			else
			{
				m_data->Jobs[i].Parent = &m_data->Jobs[parentJobHandle];
				if (m_data->Jobs[i].Parent->FirstChild == nullptr)
					m_data->Jobs[i].Parent->FirstChild = &m_data->Jobs[i];
				else
				{
					m_data->Jobs[i].Sibling = m_data->Jobs[i].Parent->FirstChild;
					m_data->Jobs[i].Parent->FirstChild = &m_data->Jobs[i];
				}
			}

			m_data->JobStates[i] = JobState::WaitingForAsyncPickup;
			break;
		}
	}

	LeaveCriticalSection(&m_data->CS);

	if (result)
		WakeConditionVariable(&m_data->Signal);

	return result;
#else
#error TODO
#endif
}

void JobQueue::Tick()
{
	// remember, only call this function from the main thread!

#ifdef _WIN32
	for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
	{
		if (m_data->JobStates[i] == JobState::WaitingForMainThreadPickup)
		{
			m_data->Jobs[i].MainThreadFunc(i, m_data->Jobs[i].Data);
			m_data->JobStates[i] = JobState::Inactive;
		}
	}
#endif
}


#ifdef _WIN32
DWORD WINAPI JobQueue::threadProc(void* param)
#endif
{
#ifdef _WIN32
	while (m_data->Active)
	{
		EnterCriticalSection(&m_data->CS);

		while (m_data->NumJobs == 0 && m_data->Active)
		{
			SleepConditionVariableCS(&m_data->Signal, &m_data->CS, 1000);
		}

		const uint32 invalidJob = (uint32)-1;
		uint32 jobIndex = invalidJob;
		for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
		{
			if (m_data->JobStates[i] == JobState::WaitingForAsyncPickup)
			{
				jobIndex = i;
				m_data->JobStates[i] = JobState::AsyncActive;
				m_data->NumJobs--;
				break;
			}
		}

		LeaveCriticalSection(&m_data->CS);

		// now we have a job to do
		if (jobIndex != invalidJob)
		{
			if (m_data->Jobs[jobIndex].AsyncFunc(jobIndex, m_data->Jobs[jobIndex].Data))
			{
				if (m_data->Jobs[jobIndex].MainThreadFunc != nullptr)
					m_data->JobStates[jobIndex] = JobState::WaitingForMainThreadPickup;
				else
					m_data->JobStates[jobIndex] = JobState::Inactive;
			}
			else
				m_data->JobStates[jobIndex] = JobState::Inactive;
		}
	}

	return 0;
#else
#error TODO
#endif

}
