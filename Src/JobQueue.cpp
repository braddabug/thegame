#include "JobQueue.h"

JobQueueData* JobQueue::m_data;

#ifdef _WIN32
#define CRITICALSECTION_LOCK(lock) EnterCriticalSection(lock)
#define CRITICALSECTION_UNLOCK(lock) LeaveCriticalSection(lock)
#else
#define CRITICALSECTION_LOCK(lock) pthread_mutex_lock(lock)
#define CRITICALSECTION_UNLOCK(lock) pthread_mutex_unlock(lock)
#endif

void JobQueue::SetGlobalData(JobQueueData** data)
{
	if (*data == nullptr)
	{
		*data = new JobQueueData();
		memset(*data, 0, sizeof(JobQueueData));

#ifdef _WIN32
		InitializeCriticalSection(&(*data)->Lock);
		InitializeConditionVariable(&(*data)->Signal);
#else
		(*data)->Signal = PTHREAD_COND_INITIALIZER;
		(*data)->Lock = PTHREAD_MUTEX_INITIALIZER;
#endif
	}

	m_data = *data;
	m_data->Active = true;

	// create the threads
	for (uint32 i = 0; i < JobQueueData::MaxThreads; i++)
	{
#ifdef _WIN32
		m_data->Threads[i] = CreateThread(nullptr, 0, threadProc, nullptr, 0, nullptr);
#else
		if (pthread_create(&m_data->Threads[i], nullptr, threadProc, nullptr) != 0)
		{
			// TODO: report the error
		}
#endif
	}
}

void JobQueue::Shutdown(bool complete)
{
	m_data->Active = false;

#ifdef _WIN32
	WakeAllConditionVariable(&m_data->Signal);
#else
	pthread_cond_broadcast(&m_data->Signal);
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
					m_data->Jobs[i].MainThreadFunc(m_data->Jobs[i].Data);
					m_data->JobStates[i] = JobState::Inactive;
				}
			}
		}
	}

#ifdef _WIN32
	WaitForMultipleObjects(JobQueueData::MaxThreads, m_data->Threads, TRUE, INFINITE);
#else
	for (uint32 i = 0; i < JobQueueData::MaxThreads; i++)
	{
		pthread_join(m_data->Threads[i], nullptr);
	}
#endif

	for (uint32 i = 0; i < JobQueueData::MaxThreads; i++)
	{
#ifdef _WIN32
		CloseHandle(m_data->Threads[i]);
		m_data->Threads[i] = INVALID_HANDLE_VALUE;
#else
		// nothing
#endif
	}

	if (complete)
		delete m_data;
}

uint32 JobQueue::AddJob(JobFunc asyncFunc, JobFunc mainThreadFunc, P_OUT_OPTIONAL JobInfo* result, void* data, size_t dataSize)
{
	return AddDependantJob((uint32)-1, asyncFunc, mainThreadFunc, result, data, dataSize);
}

uint32 JobQueue::AddDependantJob(uint32 parentJobHandle, JobFunc asyncFunc, JobFunc mainThreadFunc, P_OUT_OPTIONAL JobInfo* result, void* data, size_t dataSize)
{
	const uint32 invalidJob = (uint32)-1;
	
	if (result)
	{
		(*result).Result = JobResult::Pending;
		(*result).Job = invalidJob;
	}
	if (dataSize > Job::MaxDataSize)
	{
		if (result) (*result).Result = JobResult::Error;
		return invalidJob;
	}

	CRITICALSECTION_LOCK(&m_data->Lock);

	uint32 r = invalidJob;
	for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
	{
		if (m_data->JobStates[i] == JobState::Inactive)
		{
			r = i;
			if (result) (*result).Job = i;
			m_data->Jobs[i].Result = result;
			m_data->Jobs[i].AsyncFunc = asyncFunc;
			m_data->Jobs[i].MainThreadFunc = mainThreadFunc;
			memcpy(m_data->Jobs[i].Data, data, dataSize);
			m_data->DependantJobCount[i] = 0;

			if (parentJobHandle != invalidJob)
			{
				m_data->Jobs[i].ParentIndex = parentJobHandle;
				m_data->DependantJobCount[parentJobHandle]++;
			}
			else
			{
				m_data->Jobs[i].ParentIndex = (uint32)-1;
			}

			m_data->JobStates[i] = JobState::WaitingForAsyncPickup;
			m_data->NumJobs++;

			break;
		}
	}

	CRITICALSECTION_UNLOCK(&m_data->Lock);

	if (r != invalidJob)
#ifdef _WIN32
		WakeConditionVariable(&m_data->Signal);
#else
		pthread_cond_signal(&m_data->Signal);
#endif
	else
		if (result) (*result).Result = JobResult::Error;

	return r;
}

void JobQueue::WaitForJob(volatile JobResult* result)
{
	if (result)
	{
		while (*result == JobResult::Pending || *result == JobResult::WaitingOnChildren)
			Tick();
	}
}

void JobQueue::Tick()
{
	// remember, only call this function from the main thread!

	for (uint32 i = 0; i < JobQueueData::MaxJobs; i++)
	{
		if (m_data->JobStates[i] == JobState::WaitingForMainThreadPickup)
		{
			m_data->Jobs[i].MainThreadFunc(m_data->Jobs[i].Data);

			if (m_data->Jobs[i].ParentIndex != (uint32)-1)
			{
				m_data->DependantJobCount[m_data->Jobs[i].ParentIndex]--;

				if (m_data->DependantJobCount[m_data->Jobs[i].ParentIndex] == 0 &&
					m_data->JobStates[m_data->Jobs[i].ParentIndex] == JobState::DoneWaitingOnChildren)
				{
					cleanupJob(m_data->Jobs[i].ParentIndex, JobState::Inactive);
				}
			}

			cleanupJob(i, JobState::Inactive);
		}
	}
}


#ifdef _WIN32
DWORD WINAPI JobQueue::threadProc(void* param)
#else
void* JobQueue::threadProc(void* param)
#endif
{

	while (m_data->Active)
	{
		CRITICALSECTION_LOCK(&m_data->Lock);

		while (m_data->NumJobs == 0 && m_data->Active)
		{
#ifdef _WIN32
			SleepConditionVariableCS(&m_data->Signal, &m_data->Lock, 1000);
#else
			pthread_cond_wait(&m_data->Signal, &m_data->Lock);
#endif
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

		CRITICALSECTION_UNLOCK(&m_data->Lock);

		// now we have a job to do
		if (jobIndex != invalidJob)
		{
			if (m_data->Jobs[jobIndex].AsyncFunc(m_data->Jobs[jobIndex].Data))
			{
				if (m_data->Jobs[jobIndex].MainThreadFunc != nullptr)
					m_data->JobStates[jobIndex] = JobState::WaitingForMainThreadPickup;
				else
				{
					if (m_data->Jobs[jobIndex].ParentIndex != (uint32)-1)
					{
						CRITICALSECTION_LOCK(&m_data->Lock);

						m_data->DependantJobCount[m_data->Jobs[jobIndex].ParentIndex]--;
						assert(m_data->DependantJobCount[m_data->Jobs[jobIndex].ParentIndex] >= 0);

						if (m_data->DependantJobCount[m_data->Jobs[jobIndex].ParentIndex] == 0 &&
							m_data->JobStates[m_data->Jobs[jobIndex].ParentIndex] == JobState::DoneWaitingOnChildren)
							cleanupJob(m_data->Jobs[jobIndex].ParentIndex, JobState::Inactive);

						CRITICALSECTION_UNLOCK(&m_data->Lock);
					}
					
					if (m_data->DependantJobCount[jobIndex] == 0)
					{
						cleanupJob(jobIndex, JobState::Inactive);
					}
					else
					{
						cleanupJob(jobIndex, JobState::DoneWaitingOnChildren);
					}
				}
			}
			else
			{
				cleanupJob(jobIndex, JobState::Inactive);
			}
		}
	}

	return 0;

}

void JobQueue::cleanupJob(uint32 index, JobState state)
{
	if (m_data->Jobs[index].Result)
	{
		JobResult result;
		switch (state)
		{
		case JobState::DoneWaitingOnChildren:
			result = JobResult::WaitingOnChildren;
			break;
		case JobState::Inactive:
			result = JobResult::Completed;
			break;
		default:
			result = JobResult::Pending;
			break;
		}

		(*m_data->Jobs[index].Result).Result = result;
		(*m_data->Jobs[index].Result).Job = (JobHandle)-1;
		std::atomic_thread_fence(std::memory_order_release);
	}

	m_data->JobStates[index] = state;
}
