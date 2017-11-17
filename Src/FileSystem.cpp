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

#ifndef LIGHTMAPPER
#include "tinyfiles.h"

struct FileSystemData
{
	static const uint32 MaxFiles = FileSystem::MaxFiles * 4 / 3;
	uint32 FileNameHashes[MaxFiles];
	FileSystem::FileInfo Files[MaxFiles];
	bool FileActive[MaxFiles];
	uint32 NumFiles;
};

FileSystemData* FileSystem::m_data;

void FileSystem::SetGlobalData(FileSystemData** data)
{
#ifndef FILESYSTEM_BASIC_IMPL
	if (*data == nullptr)
	{
		*data = (FileSystemData*)g_memory->AllocTrack(sizeof(FileSystemData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(FileSystemData));
	}
#endif

	m_data = *data;
}

void FileSystem::Shutdown()
{
#ifndef FILESYSTEM_BASIC_IMPL
	g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	m_data = nullptr;
#endif
}

#ifndef FILESYSTEM_BASIC_IMPL
void FileSystem::searchPathRecursive(const char* root, const char* path, uint32 depth)
{
	char rootAndPath[256];
#ifdef _MSC_VER
	strcpy_s(rootAndPath, root);
	strcat_s(rootAndPath, path);
#else
	strcpy(rootAndPath, root);
	strcat(rootAndPath, path);
#endif

	auto pathLen = strlen(path);

	tfDIR dir;
	tfDirOpen(&dir, rootAndPath);

	while (dir.has_next)
	{
		tfFILE file;
		tfReadFile(&dir, &file);

		if (file.is_dir)
		{
			if (file.name[0] != '.' && depth > 0)
			{
				char buffer[256]; buffer[0] = 0;

				if (path[0] != 0)
				{
#ifdef _MSC_VER
					strncpy_s(buffer, path, 256);
					strcat_s(buffer, "/");
#else
					strncpy(buffer, path, 256);
					strcat(buffer, "/");
#endif
				}

#ifdef _WIN32
				strncat_s(buffer, file.name, 256);
#else
				strncat(buffer, file.name, 256);
#endif

				searchPathRecursive(root, buffer, depth - 1);
			}
		}
		else if (file.is_reg)
		{
			auto len = strlen(rootAndPath) + strlen(file.name) + 2;
			auto name = (char*)g_memory->AllocAndKeep(len, __FILE__, __LINE__);
#ifdef _WIN32
			strcpy_s(name, len, rootAndPath);
			strcat_s(name, len, "/");
			strcat_s(name, len, file.name);
#else
			strcpy(name, rootAndPath);
			strcat(name, "/");
			strcat(name, file.name);
#endif
			char pathAndName[256]; pathAndName[0] = 0;
#ifdef _MSC_VER
			strcpy_s(pathAndName, path);
			strcat_s(pathAndName, "/");
			strcat_s(pathAndName, file.name);
#else
			strcpy(pathAndName, path);
			strcat(pathAndName, "/");
			strcat(pathAndName, file.name);
#endif


			uint32 index;
			Utils::HashTableUtils::Reserve(Utils::CalcHash(pathAndName), m_data->FileNameHashes, m_data->FileActive, m_data->MaxFiles, &index);
			m_data->Files[index].DiskFilename = name;
			Utils::CopyString(m_data->Files[index].Filename, pathAndName, 64);
			m_data->NumFiles++;
		}

		tfDirNext(&dir);
	}

	tfDirClose(&dir);
}

void FileSystem::SetSearchPaths(SearchPathInfo* paths, uint32 numPaths)
{
	for (uint32 i = 0; i < numPaths; i++)
	{
		searchPathRecursive(paths[i].Path, "", paths[i].MaxDepth - 1);
	}

	LOG("%u files added to search path", m_data->NumFiles);
}
#endif

uint32 FileSystem::GetNumFiles()
{
	return m_data->NumFiles;
}

const char* FileSystem::GetDiskFilenameByHash(uint32 hash)
{
	uint32 index;
	if (Utils::HashTableUtils::Find(hash, m_data->FileNameHashes, m_data->FileActive, m_data->MaxFiles, &index))
	{
		return m_data->Files[index].DiskFilename;
	}

	return nullptr;
}

const char* FileSystem::GetFilenameByHash(uint32 hash)
{
	uint32 index;
	if (Utils::HashTableUtils::Find(hash, m_data->FileNameHashes, m_data->FileActive, m_data->MaxFiles, &index))
	{
		return m_data->Files[index].Filename;
	}

	return nullptr;
}

const char* FileSystem::GetFilenameByIndex(uint32 index)
{
	uint32 count = 0;
	for (uint32 i = 0; i < m_data->MaxFiles; i++)
	{
		if (m_data->FileActive[i])
		{
			if (count == index)
				return m_data->Files[i].Filename;

			count++;
		}
	}

	return nullptr;
}

const char* FileSystem::GetDiskFilenameByIndex(uint32 index)
{
	uint32 count = 0;
	for (uint32 i = 0; i < m_data->MaxFiles; i++)
	{
		if (m_data->FileActive[i])
		{
			if (count == index)
				return m_data->Files[i].DiskFilename;

			count++;
		}
	}

	return nullptr;
}

#endif // LIGHTMAPPER

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

#ifndef LIGHTMAPPER

bool FileSystem::OpenKnown(const char* name, File* file)
{
    uint32 hash = Utils::CalcHash(name);
    return OpenKnown(hash, file);
}

bool FileSystem::OpenKnown(uint32 hash, File* file)
{
	uint32 index;
	if (Utils::HashTableUtils::Find(hash, m_data->FileNameHashes, m_data->FileActive, m_data->MaxFiles, &index))
	{
		return Open(m_data->Files[index].DiskFilename, file);
	}

	return false;
}

#endif // LIGHTMAPPER

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

#define TINYFILES_IMPL
#include <cerrno>
#include "tinyfiles.h"
#undef TINYFILES_IMPL
