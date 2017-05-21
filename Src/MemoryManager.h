#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

struct MemoryManager
{
	void* (*Alloc)(size_t amount);
	void* (*AllocTrack)(size_t amount, const char* filename, int line);
	
	void* (*AlignedAlloc)(size_t amount, size_t alignment);
	void* (*AlignedAllocTrack)(size_t amount, size_t alignment, const char* filename, int line);

	void* (*Realloc)(void* original, size_t amount);
	void* (*ReallocTrack)(void* original, size_t amount, const char* filename, int line);

	void (*Free)(void* memory);
	void (*FreeTrack)(void* memory, const char* filename, int line);
};

extern MemoryManager* g_memory;

#define ALLOC_NEW(type) new (g_memory->Alloc(sizeof(type))) type

template<typename T>
T* NewObject(const char* filename, int line)
{
	void* memory = g_memory->AllocTrack(sizeof(T), filename, line);
	return new(memory) T();
}

#if !defined GAME_ENABLE_HOTLOAD || !defined GAME_ENABLE_HOTLOAD_DLL

namespace MemoryManagerInternal
{
	void* Alloc(size_t amount);
	void* AllocTrack(size_t amount, const char* filename, int line);

	void* AlignedAlloc(size_t amount, size_t alignment);
	void* AlignedAllocTrack(size_t amount, size_t alignment, const char* filename, int line);

	void* Realloc(void* original, size_t amount);
	void* ReallocTrack(void* original, size_t amount, const char* filename, int line);

	void Free(void* memory);
	void FreeTrack(void* memory, const char* filename, int line);

	void GetMemoryUsage(size_t* usage);
}
#endif

#endif // MEMORYMANAGER_H
