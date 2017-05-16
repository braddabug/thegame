#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <atomic>

#include "Common.h"
#ifdef _WIN32
#include "CleanWindows.h"
#else
#error TODO
#endif

typedef bool(*JobFunc)(uint32, void*);

enum class JobState : uint8
{
	Inactive,
	WaitingForAsyncPickup,
	AsyncActive,
	WaitingForMainThreadPickup,
	MainThreadActive,

	DoneWaitingOnChildren
};

struct Job
{
	JobFunc AsyncFunc;
	JobFunc MainThreadFunc;
	void* Data;
	uint32 ParentIndex;
};

struct JobQueueData
{
	static const uint32 MaxThreads = 8;
#ifdef _WIN32
	HANDLE Threads[MaxThreads];
	CONDITION_VARIABLE Signal;
	CRITICAL_SECTION CS;
#else
#error TODO
#endif

	static const uint32 MaxJobs = 100;
	Job Jobs[MaxJobs];
	std::atomic<JobState> JobStates[MaxJobs];
	std::atomic<uint32> DependantJobCount[MaxJobs];
	uint32 NumJobs;

	std::atomic<bool> Active;
};

class JobQueue
{
	static JobQueueData* m_data;

public:
	static void SetGlobalData(JobQueueData** data);

	static void Shutdown(bool complete);

	static bool AddJob(JobFunc asyncFunc, JobFunc mainThreadFunc, void* data);
	static bool AddDependantJob(uint32 parentJobHandle, JobFunc asyncFunc, JobFunc mainThreadFunc, void* data);

	static void Tick();

private:
#ifdef _WIN32
	static DWORD WINAPI threadProc(void* param);
#endif
};

#endif // JOBQUEUE_H
