#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <atomic>

#include "Common.h"
#ifdef _WIN32
#include "CleanWindows.h"
#else
#error TODO
#endif

typedef bool(*JobFunc)(void*);

enum class JobState : uint8
{
	Inactive,
	WaitingForAsyncPickup,
	AsyncActive,
	WaitingForMainThreadPickup,
	MainThreadActive,

	DoneWaitingOnChildren
};

enum class JobResult
{
	Pending,
	WaitingOnChildren,
	Completed,
	Error
};

typedef uint32 JobHandle;

struct JobInfo
{
	volatile JobResult Result;
	JobHandle Job;
};

struct Job
{
	JobFunc AsyncFunc;
	JobFunc MainThreadFunc;
	uint32 ParentIndex;
	JobInfo* Result;

	static const uint32 MaxDataSize = 128;
	alignas(16) uint8 Data[MaxDataSize];
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
	std::atomic<int32> DependantJobCount[MaxJobs];
	std::atomic<uint32> NumJobs;

	std::atomic<bool> Active;
};

class JobQueue
{
	static JobQueueData* m_data;

public:
	static void SetGlobalData(JobQueueData** data);

	static void Shutdown(bool complete);

	static JobHandle AddJob(JobFunc asyncFunc, JobFunc mainThreadFunc, JobInfo* result, void* data, size_t dataSize);
	static JobHandle AddDependantJob(uint32 parentJobHandle, JobFunc asyncFunc, JobFunc mainThreadFunc, JobInfo* result, void* data, size_t dataSize);

	static void WaitForJob(volatile JobResult* result);

	static void Tick();

private:
#ifdef _WIN32
	static DWORD WINAPI threadProc(void* param);
#endif

	static inline void cleanupJob(uint32 index, JobState state);
};

#endif // JOBQUEUE_H
