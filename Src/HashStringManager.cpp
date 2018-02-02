#include "HashStringManager.h"
#include "MemoryManager.h"
#include "strpool.h"

struct HashStringManagerData
{
	strpool_t PoolFiles;
	strpool_t PoolNoun;
	strpool_t PoolVerbs;
};

HashStringManagerData* HashStringManager::m_data = nullptr;

void HashStringManager::SetGlobalData(HashStringManagerData** data)
{
	if (*data == nullptr)
	{
		*data = (HashStringManagerData*)g_memory->AllocTrack(sizeof(HashStringManagerData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(HashStringManagerData));
		m_data = *data;

		strpool_config_t defaultConf = strpool_default_config;
		defaultConf.ignore_case = true;
		strpool_config_t fileConf = strpool_default_config;
#ifdef _WIN32
		fileConf.ignore_case = true;
#endif
		strpool_init(&m_data->PoolNoun, &defaultConf);
		strpool_init(&m_data->PoolVerbs, &defaultConf);
		strpool_init(&m_data->PoolFiles, &fileConf);
	}
	else
	{
		m_data = *data;
	}
}

void HashStringManager::Shutdown()
{
	if (m_data)
	{
		strpool_term(&m_data->PoolNoun);
		strpool_term(&m_data->PoolVerbs);
		strpool_term(&m_data->PoolFiles);

		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}
}

StringRef HashStringManager::Set(HashStringType type, const char* valueStart, const char* valueEnd)
{
	strpool_t* pool = nullptr;
	switch (type)
	{
	case HashStringType::Noun: pool = &m_data->PoolNoun; break;
	case HashStringType::Verb: pool = &m_data->PoolVerbs; break;
	case HashStringType::File: pool = &m_data->PoolFiles; break;
	}

	if (pool != nullptr)
	{
		int len;
		if (valueEnd == nullptr)
			len = strlen(valueStart);
		else
			len = (int)(valueEnd - valueStart);

		return strpool_inject(pool, valueStart, len);
	}

	return 0;
}

const char* HashStringManager::Get(StringRef hash, HashStringType type)
{
	strpool_t* pool = nullptr;
	switch (type)
	{
	case HashStringType::Noun: pool = &m_data->PoolNoun; break;
	case HashStringType::Verb: pool = &m_data->PoolVerbs; break;
	case HashStringType::File: pool = &m_data->PoolFiles; break;
	}

	if (pool != nullptr)
		return strpool_cstr(pool, hash);

	return nullptr;
}

#define STRPOOL_IMPLEMENTATION
#ifdef _MSC_VER
#define STRPOOL_STRNICMP _strnicmp
#endif
#include "strpool.h"