#include "MemoryManager.h"
#include <memory>
#include <atomic>

std::atomic<size_t> g_requestedMemoryUsed = 0;
std::atomic<size_t> g_actualMemoryUsed = 0;
std::atomic<uint32> g_timestamp = 0;

namespace MemoryManagerInternal
{
	struct AllocInfo
	{
		size_t TotalSize;
		size_t RequestedSize;
		char Filename[128];
		uint32 Line;
		uint32 Timestamp;

		static const uint32 PaddingAmount = (32 - (sizeof(size_t) * 2 + 128 + sizeof(uint32) * 2) % 32) % 32;
		char Padding[PaddingAmount];
	};

	void* Alloc(size_t amount)
	{
		return AllocTrack(amount, nullptr, 0);
	}

	void* AllocTrack(size_t amount, const char* filename, int line)
	{
		g_requestedMemoryUsed += amount;

		static_assert(sizeof(AllocInfo) % 32 == 0, "Size of AllocInfo is not a multiple of 32 bytes");

		g_requestedMemoryUsed += amount;

		size_t amountToAllocate = sizeof(AllocInfo) + 64 + amount + 64;
		g_actualMemoryUsed += amountToAllocate;

		void* memory = malloc(amountToAllocate);

		// setup the tracking info
		AllocInfo* info = (AllocInfo*)memory;
		info->TotalSize = amountToAllocate;
		info->RequestedSize = amount;
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
		info->Line = 0;
		info->Timestamp = g_timestamp++;

		// setup the sentinel values
		memset((uint8*)memory + sizeof(AllocInfo), 0xb0, 64);
		memset((uint8*)memory + sizeof(AllocInfo) + 64 + amount, 0xb0, 64);

		return (uint8*)memory + sizeof(AllocInfo) + 64;
	}

	void* Realloc(void* original, size_t amount)
	{
		// TODO: track used memory

		return realloc(original, amount);
	}

	void Free(void* memory)
	{
		FreeTrack(memory, nullptr, 0);
	}

	void FreeTrack(void* memory, const char* filename, int line)
	{
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

		free((uint8*)memory - 64 - sizeof(AllocInfo));
	}

	void GetMemoryUsage(size_t* usage)
	{
		*usage = g_requestedMemoryUsed;
	}
}