#include "MemoryManager.h"
#include <memory>
#include <atomic>
#include <cstdio>

#ifdef _WIN32
#include "CleanWindows.h"
#endif

#define MEM_BASIC_ALLOC

std::atomic<size_t> g_requestedMemoryUsed = { 0 };
std::atomic<size_t> g_actualMemoryUsed = { 0 };
std::atomic<uint32> g_timestamp = { 0 };

namespace MemoryManagerInternal
{
	struct AllocInfoMini;

	struct AllocInfo
	{
		size_t TotalSize;
		size_t RequestedSize;
		size_t Alignment;
		char Filename[128];
		uint32 Line;
		uint32 Timestamp;
		AllocInfoMini* Mini;

		static const uint32 PaddingAmount = (32 - (sizeof(size_t) * 3 + 128 + sizeof(uint32) * 2 + sizeof(AllocInfoMini*)) % 32) % 32;
		char Padding[PaddingAmount];
	};

	struct AllocInfoMini
	{
		AllocInfo* Info;
		size_t RequestedSize;
	};

	struct AllocInfoPage
	{
		AllocInfoMini Allocations[1000];
		AllocInfoPage* Next;
		std::atomic<uint32> NumAllocations;
	};

	AllocInfoPage* g_firstPage;
	AllocInfoPage* g_currentPage;
#ifdef _WIN32
	CRITICAL_SECTION g_lock;
#else
	pthread_mutex_t g_lock;
#endif

	AllocInfoMini* GetNewAlloc()
	{
#ifdef _WIN32
		EnterCriticalSection(&g_lock);
#else
		pthread_mutex_lock(&g_lock);
#endif

		if (g_currentPage->NumAllocations == 1000)
		{
			auto newPage = (AllocInfoPage*)malloc(sizeof(AllocInfoPage));
			newPage->Next = nullptr;
			newPage->NumAllocations = 0;
			g_currentPage->Next = newPage;
			g_currentPage = newPage;
		}

		auto info = &g_currentPage->Allocations[g_currentPage->NumAllocations++];

#ifdef _WIN32
		LeaveCriticalSection(&g_lock);
#else
		pthread_mutex_unlock(&g_lock);
#endif

		return info;
	}

	void Initialize()
	{
		g_firstPage = g_currentPage = (AllocInfoPage*)malloc(sizeof(AllocInfoPage));
		g_currentPage->Next = nullptr;
		g_currentPage->NumAllocations = 0;

#ifdef _WIN32
		InitializeCriticalSection(&g_lock);
#else
		g_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
	}

	void Shutdown()
	{
		AllocInfoPage* page = g_firstPage;
		while (page != nullptr)
		{
			auto next = page->Next;
			free(page);
			page = next;
		}
	}

	void SetDefaults(MemoryManager* manager)
	{
		manager->Alloc = MemoryManagerInternal::Alloc;
		manager->AllocTrack = MemoryManagerInternal::AllocTrack;
		manager->AlignedAlloc = MemoryManagerInternal::AlignedAlloc;
		manager->AlignedAllocTrack = MemoryManagerInternal::AlignedAllocTrack;
		manager->Realloc = MemoryManagerInternal::Realloc;
		manager->ReallocTrack = MemoryManagerInternal::ReallocTrack;
		manager->Free = MemoryManagerInternal::Free;
		manager->FreeTrack = MemoryManagerInternal::FreeTrack;
		manager->AllocAndKeep = MemoryManagerInternal::AllocAndKeep;
	}

	void* Alloc(size_t amount)
	{
		return AllocTrack(amount, nullptr, 0);
	}
	
	void* AlignedAlloc(size_t amount, size_t alignment)
	{
		return AlignedAllocTrack(amount, alignment, nullptr, 0);
	}

	void* AllocTrack(size_t amount, const char* filename, int line)
	{
		size_t alignment;
		if (amount <= 4)
			alignment = alignof(float);
		else if (amount <= 8)
			alignment = alignof(double);
		else
			alignment = 16;

		return AlignedAllocTrack(amount, alignment, filename, line);
	}

	void* AlignedAllocTrack(size_t amount, size_t alignment, const char* filename, int line)
	{
#ifdef MEM_BASIC_ALLOC
		return malloc(amount);
#else
		g_requestedMemoryUsed += amount;

		static_assert(sizeof(AllocInfo) % 32 == 0, "Size of AllocInfo is not a multiple of 32 bytes");

		g_requestedMemoryUsed += amount;

		size_t preSize = sizeof(AllocInfo) + 64;
		size_t amountToAllocate = preSize + amount + 64;
		g_actualMemoryUsed += amountToAllocate;

		void* memory = malloc(amountToAllocate);

		// setup the tracking info
		AllocInfo* info = (AllocInfo*)memory;
		info->TotalSize = amountToAllocate;
		info->RequestedSize = amount;
		info->Alignment = alignment;
		if (filename != nullptr)
		{
#ifdef _WIN32
			strncpy_s(info->Filename, filename, _TRUNCATE);
#else
			strncpy(info->Filename, filename, 128);
			info->Filename[127] = 0;
#endif
		}
		else
			info->Filename[0] = 0;
		info->Line = line;
		info->Timestamp = g_timestamp++;

		// setup the sentinel values
		memset((uint8*)memory + sizeof(AllocInfo), 0xb0, 64);
		memset((uint8*)memory + sizeof(AllocInfo) + 64 + amount, 0xb0, 64);

		// record the allocation
		auto log = GetNewAlloc();
		log->Info = info;
		log->RequestedSize = amount;
		info->Mini = log;

		return (uint8*)memory + preSize;
#endif
	}

	
	

	void* Realloc(void* original, size_t amount)
	{
		return ReallocTrack(original, amount, nullptr, 0);
	}

	void* ReallocTrack(void* original, size_t amount, const char* filename, int line)
	{
		if (original == nullptr)
			return AllocTrack(amount, filename, line);

#ifdef MEM_BASIC_ALLOC
		return realloc(original, amount);
#else
		size_t amountToAllocate = sizeof(AllocInfo) + 64 + amount + 64;

		AllocInfo* info = (AllocInfo*)((uint8*)original - 64 - sizeof(AllocInfo));

		// check the sentinels
		for (uint8* mem = (uint8*)original - 64; mem < original; mem++)
			if (*mem != 0xb0)
				throw "Memory is corrupt";
		for (uint8* mem = (uint8*)original + info->RequestedSize; mem < (uint8*)original + info->RequestedSize + 64; mem++)
			if (*mem != 0xb0)
				throw "Memory is corrupt";


		void* memory = realloc(info, amountToAllocate);

		AllocInfo* newinfo = (AllocInfo*)memory;

		// track memory usage
		g_actualMemoryUsed += amountToAllocate - newinfo->TotalSize;
		g_requestedMemoryUsed += amount - newinfo->RequestedSize;
		
		newinfo->RequestedSize = amount;
		newinfo->TotalSize = amountToAllocate;
		
		if (filename != nullptr)
		{
#ifdef _WIN32
			strncpy_s(newinfo->Filename, filename, _TRUNCATE);
#else
			strncpy(newinfo->Filename, filename, 128);
			newinfo->Filename[127] = 0;
#endif
		}
		else
			newinfo->Filename[0] = 0;

		newinfo->Line = line;
		newinfo->Mini->RequestedSize = amount;

		// set the sentinel values
		memset((uint8*)memory + sizeof(AllocInfo) + 64 + amount, 0xb0, 64);

		

		return (uint8*)memory + sizeof(AllocInfo) + 64;
#endif
	}

	void Free(void* memory)
	{
		FreeTrack(memory, nullptr, 0);
	}

	void FreeTrack(void* memory, const char* filename, int line)
	{
#ifdef MEM_BASIC_ALLOC
		free(memory);
#else
		AllocInfo* info = (AllocInfo*)((uint8*)memory - 64 - sizeof(AllocInfo));

		// check the sentinels
		for (uint8* mem = (uint8*)memory - 64; mem < memory; mem++)
			if (*mem != 0xb0)
				throw "Memory is corrupt";
		for (uint8* mem = (uint8*)memory + info->RequestedSize; mem < (uint8*)memory + info->RequestedSize + 64; mem++)
			if (*mem != 0xb0)
				throw "Memory is corrupt";

		// TODO: is there a better way to cause an abort and still possibly get an error message? Almost definitely.

		g_actualMemoryUsed -= info->TotalSize;
		g_requestedMemoryUsed -= info->RequestedSize;

		info->Mini->Info = nullptr;

		free((uint8*)memory - 64 - sizeof(AllocInfo));
#endif
	}

	void* AllocAndKeep(size_t amount, const char* filename, int line)
	{
		// TODO: this is for memory that will be allocated and held until the game exits.
		// It should be excluded from memory-leakage detection.
		// This will allow the memory manager to potentially do some optimizations, like allocate
		// a big buffer and dole it out
		return AllocTrack(amount, filename, line);
	}

	void GetMemoryUsage(size_t* usage)
	{
		*usage = g_requestedMemoryUsed;
	}

	void DumpReport(const char* filename)
	{
		FILE* fp;
#ifdef _WIN32
		fopen_s(&fp, filename, "w");
#else
		fp = fopen(filename, "w");
#endif

		fprintf(fp, "[\n");

		auto page = g_firstPage;
		while (page != nullptr)
		{
			for (uint32 i = 0; i < page->NumAllocations; i++)
			{
				fprintf(fp, "\t{\n");

				fprintf(fp, "\t\t\"requestedSize\": %u,\n", (uint32)page->Allocations[i].RequestedSize);

				if (page->Allocations[i].Info == nullptr)
				{
					fprintf(fp, "\t\t\"freed\": true,\n");
					fprintf(fp, "\t\t\"filename\": null,\n");
					fprintf(fp, "\t\t\"line\": null\n");
				}
				else
				{
#ifdef _WIN32
					// Windows puts \ in paths, which aren't legal in json without escaping them, so replace them
					for (char* c = page->Allocations[i].Info->Filename; *c != 0; c++)
						if (*c == '\\') *c = '/';
#endif

					fprintf(fp, "\t\t\"freed\": false,\n");
					fprintf(fp, "\t\t\"filename\": \"%s\",\n", page->Allocations[i].Info->Filename);
					fprintf(fp, "\t\t\"line\": %u\n", page->Allocations[i].Info->Line);
				}

				if (i < page->NumAllocations - 1)
					fprintf(fp, "\t},\n");
				else
					fprintf(fp, "\t}\n");
			}

			page = page->Next;
		}


		fprintf(fp, "]\n");

		fclose(fp);
	}
}