#ifndef FILEFINDER_H
#define FILEFINDER_H

struct FileFinderData;

#include "Common.h"
#include "FileSystem.h"

struct FoundFile
{
	uint32 FileSize;
	void* Memory;

	File F;


	bool OwnFile;
};

class FileFinder
{
	static FileFinderData* m_data;

public:
	static void SetGlobalData(FileFinderData** data);
	static void Shutdown();

	struct SearchPathInfo
	{
		const char* Path;
	};

	static void SetSearchPaths(SearchPathInfo* paths, uint32 numPaths);

	static bool OpenAndMap(StringRef filename, FoundFile* result);
	static bool OpenAndMap(const char* filename, FoundFile* result);
	static void Close(FoundFile* file);
};

#endif // FILEFINDER_H
