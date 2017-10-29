#ifndef STRINGMANAGER_H
#define STRINGMANAGER_H

#include "Common.h"

struct StringManagerData;

typedef uint32 StringHandle;

class StringManager
{
	static StringManagerData* m_data;

public:
	static const StringHandle InvalidHandle = (StringHandle)-1;

	static void SetGlobalData(StringManagerData** data);
	static void Shutdown();

	static void Init(const char* languageCode, const char* regionCode);

	static const char* GetLocalizedText(const char* key);
	static const char* GetLocalizedText(uint32 keyHash);
	static const char* GetLocalizedTextFromHandle(StringHandle handle);
	static StringHandle GetHandle(const char* key);
	static StringHandle GetHandle(uint32 keyHash);
};

#endif // STRINGMANAGER_H
