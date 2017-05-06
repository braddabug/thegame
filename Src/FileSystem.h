#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "Common.h"

struct File
{
	uint32 FileSize;
	void* Memory;

#ifdef _WIN32
	void* Handle;
	void* Mapping;
#else
	int Handle;
#endif
};

class FileSystem
{
public:
	static bool Open(const char* path, File* file);
	static void Close(File* file);
	static bool IsOpen(File* file);

	static void* MapFile(File* file);
	static void UnmapFile(File* file);

	static void* OpenAndMap(const char* path, File* file)
	{
		if (FileSystem::Open(path, file) == false)
			return nullptr;

		if (FileSystem::MapFile(file) == nullptr)
		{
			FileSystem::Close(file);
			return nullptr;
		}

		return file->Memory;
	}
};

#endif // FILESYSTEM_H
