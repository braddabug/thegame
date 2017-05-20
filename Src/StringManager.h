#ifndef STRINGMANAGER_H
#define STRINGMANAGER_H

#include "Common.h"

struct StringManagerData;

class StringManager
{
	static StringManagerData* m_data;

public:
	static void SetGlobalData(StringManagerData** data);

	static PersistantString Create(const char* source);
	static void Acquire(PersistantString string);
	static void Release(PersistantString string);
};

#endif // STRINGMANAGER_H
