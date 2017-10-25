#ifndef STRINGMANAGER_H
#define STRINGMANAGER_H

#include "Common.h"

struct StringManagerData;

class StringManager
{
	static StringManagerData* m_data;

public:
	static void SetGlobalData(StringManagerData** data);
	static void Shutdown();

	static void Init(const char* languageCode, const char* regionCode);

	static const char* GetLocalizedText(const char* key);
	static const char* GetLocalizedText(uint32 keyHash);
	static const char* GetLocalizedTextFromHandle(uint32 handle);
	static uint32 GetHandle(const char* key);
	static uint32 GetHandle(uint32 keyHash);
};

#endif // STRINGMANAGER_H
