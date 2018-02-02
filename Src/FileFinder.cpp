#include "FileFinder.h"
#include "FileSystem.h"
#include "MemoryManager.h"
#include "HashStringManager.h"

struct FileFinderData
{
	static const uint32 MaxPaths = 10;
	static const uint32 MaxPathLen = 256;
	char Paths[MaxPaths][MaxPathLen];
	uint32 NumPaths;
};

FileFinderData* FileFinder::m_data;

void FileFinder::SetGlobalData(FileFinderData** data)
{
#ifndef FILESYSTEM_BASIC_IMPL
	if (*data == nullptr)
	{
		*data = (FileFinderData*)g_memory->AllocTrack(sizeof(FileFinderData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(FileFinderData));
	}
#endif

	m_data = *data;
}

void FileFinder::Shutdown()
{
#ifndef FILESYSTEM_BASIC_IMPL
	g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	m_data = nullptr;
#endif
}

void FileFinder::SetSearchPaths(SearchPathInfo* paths, uint32 numPaths)
{
	m_data->NumPaths = numPaths;

	for (uint32 i = 0; i < numPaths; i++)
	{
		strncpy_s(m_data->Paths[i], paths[i].Path, FileFinderData::MaxPathLen);
		m_data->Paths[i][FileFinderData::MaxPathLen - 1] = 0;

		//searchPathRecursive(paths[i].Path, "", paths[i].MaxDepth - 1);
	}

	//LOG("%u files added to search path", m_data->NumFiles);
}

bool FileFinder::OpenAndMap(StringRef filename, FoundFile* result)
{
	auto f = HashStringManager::Get(filename, HashStringManager::HashStringType::File);
	if (f != nullptr)
	{
		return OpenAndMap(f, result);
	}

	return false;
}

bool FileFinder::OpenAndMap(const char* filename, FoundFile* result)
{
	if (filename == nullptr || result == nullptr)
		return false;

	char path[512];

	for (uint32 i = 0; i < m_data->MaxPathLen; i++)
	{
		path[0] = 0;
		strncpy_s(path, m_data->Paths[i], 512);
		strncat_s(path, "/", 512);
		strncat_s(path, filename, 512);

		if (FileSystem::OpenAndMap(path, &result->F) != nullptr)
		{
			result->Memory = result->F.Memory;
			result->FileSize = result->F.FileSize;
			result->OwnFile = true;
			return true;
		}
	}

	return false;
}

void FileFinder::Close(FoundFile* file)
{
	if (file != nullptr)
	{
		if (file->OwnFile)
			FileSystem::Close(&file->F);

		file->Memory = nullptr;
		file->FileSize = 0;
	}
}

#define TINYFILES_IMPL
#include <cerrno>
//#include "tinyfiles.h"
#undef TINYFILES_IMPL