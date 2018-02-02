#ifndef HASHSTRINGMANAGER_H
#define HASHSTRINGMANAGER_H

// Don't ship this! This is just for development.

#include "Common.h"

struct HashStringManagerData;

class HashStringManager
{
	static HashStringManagerData* m_data;

public:

	enum class HashStringType
	{
		File,
		Noun,
		Verb
	};

	static void SetGlobalData(HashStringManagerData** data);
	static void Shutdown();

	static StringRef Set(HashStringType type, const char* valueStart, const char* valueEnd = nullptr);

	static const char* Get(StringRef hash, HashStringType type);
};

#endif // HASHSTRINGMANAGER_H
