#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "Common.h"
#include "Utils.h"

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

struct FileSystemData;

class FileSystem
{
	friend struct FileSystemData;

	static const uint32 MaxFiles = 1000;
	static const char* m_filenames;

	struct FileInfo
	{
		const char* Filename;
	};

	static FileSystemData* m_data;

public:

	static void SetGlobalData(FileSystemData** data);
	static void Shutdown();

	struct SearchPathInfo
	{
		const char* Path;
		uint32 MaxDepth;
	};

	static void SetSearchPaths(SearchPathInfo* paths, uint32 numPaths);
	static const char* GetFilenameByHash(uint32 hash);

	static bool Open(const char* path, File* file);
	static bool Open(uint32 hash, File* file);
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

	static void* OpenAndMap(uint32 hash, File* file)
	{
		if (FileSystem::Open(hash, file) == false)
			return nullptr;

		if (FileSystem::MapFile(file) == nullptr)
		{
			FileSystem::Close(file);
			return nullptr;
		}

		return file->Memory;
	}

private:
	static void searchPathRecursive(const char* root, const char* path, uint32 depth);
};

#endif // FILESYSTEM_H
