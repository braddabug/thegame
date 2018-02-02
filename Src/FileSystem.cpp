#include "FileSystem.h"

#ifndef FILESYSTEM_BASIC_IMPL
#include "MemoryManager.h"
#include "Logging.h"
#endif

#ifdef _WIN32
#include "CleanWindows.h"
#else
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// some stupid garbage is #defining "Success", which conflicts with NxnaResult::Success
#undef Success
#endif

#include <cstdio>
#include <cstring>

bool FileSystem::Open(const char* path, File* file)
{
	file->Memory = nullptr;
	file->FileSize = 0;

#ifdef _WIN32
	file->Handle = CreateFile(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
	if (file->Handle == INVALID_HANDLE_VALUE)
	{
		file->Handle = nullptr;
		return false;
	}

	return true;
#else
	file->Handle = open(path, O_RDONLY);
	if (file->Handle == -1)
		return false;

	return true;
#endif
}

void FileSystem::Close(File* file)
{
#ifdef _WIN32
	if (file == nullptr || file->Handle == nullptr)
		return;

	if (file->Memory != nullptr)
	{
		UnmapViewOfFile(file->Memory);
		file->Memory = nullptr;
	}

	if (file->Mapping != nullptr)
	{
		CloseHandle(file->Mapping);
		file->Mapping = nullptr;
	}

	CloseHandle(file->Handle);
	file->Handle = nullptr;

#else
	if (file == nullptr || file->Handle == -1)
		return;

	if (file->Memory != nullptr)
	{
		munmap(file->Memory, file->FileSize);
		file->Memory = nullptr;
	}

	close(file->Handle);
	file->Handle = -1;

#endif

	file->FileSize = 0;
}

bool FileSystem::IsOpen(File* file)
{
#ifdef _WIN32
	return file->Handle != nullptr;
#else
	return file->Handle == -1;
#endif
}

void* FileSystem::MapFile(File* file)
{
#ifdef _WIN32
	file->Mapping = CreateFileMapping(file->Handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (file->Mapping == nullptr)
	{
		return nullptr;
	}

	file->Memory = (uint8*)MapViewOfFile(file->Mapping, FILE_MAP_READ, 0, 0, 0);
	if (file->Memory == nullptr)
	{

		CloseHandle(file->Mapping);
		file->Mapping = nullptr;

		return nullptr;
	}

	file->FileSize = GetFileSize(file->Handle, nullptr);

	return file->Memory;
#else
	struct stat sb;
	if (fstat(file->Handle, &sb) == -1)
	{
		return nullptr;
	}
		
	file->Memory = (unsigned char*)mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, file->Handle, 0);
	if (file->Memory == MAP_FAILED)
	{
		file->Memory = nullptr;

		return nullptr;
	}

	file->FileSize = sb.st_size;

	return file->Memory;
#endif
}

void FileSystem::UnmapFile(File* file)
{
	if (file == nullptr || file->Memory == nullptr)
		return;

#ifdef _WIN32
	UnmapViewOfFile(file->Memory);
	CloseHandle(file->Mapping);

	file->Memory = nullptr;
	file->Mapping = nullptr;
#else

	munmap(file->Memory, file->FileSize);
	file->Memory = nullptr;
	
#endif
}


