#include "StringManager.h"
#include "iniparse.h"
#include "Logging.h"
#include "ConsoleCommand.h"
#include <cassert>

void cmdReloadStrings(const char* param)
{
	StringManager::Init("en", "us");

	WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "Reloaded string files");
}

struct StringManagerData
{
	static const uint32 Capacity = 1000;
	StringHandle Handles[Capacity];
	const char* Strings[Capacity];

	char* StringMemory;

	int Find(uint32 hash)
	{
		uint32 index = hash % Capacity;
		for (uint32 i = 0; i < Capacity; i++)
		{
			uint32 newIndex = (index + i) % Capacity;
			if (Strings[newIndex] == nullptr)
				return -1;

			if (Handles[newIndex] == hash)
				return (int)newIndex;
		}

		return -1;
	}

	int Add(uint32 hash, const char* str)
	{
		uint32 index = hash % Capacity;
		for (uint32 i = 0; i < Capacity; i++)
		{
			uint32 newIndex = (index + i) % Capacity;
			if (Strings[newIndex] == nullptr)
			{
				Handles[newIndex] = hash;
				Strings[newIndex] = str;
				return (int)newIndex;
			}
		}

		return -1;
	}
};

StringManagerData* StringManager::m_data = nullptr;

void StringManager::SetGlobalData(StringManagerData** data)
{
	if (*data == nullptr)
	{
		*data = (StringManagerData*)g_memory->AllocAndKeep(sizeof(StringManagerData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(StringManagerData));
	}

	m_data = *data;
}

void StringManager::Shutdown()
{
	g_memory->FreeTrack(m_data->StringMemory, __FILE__, __LINE__);
	g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	m_data = nullptr;
}

void StringManager::Init(const char* languageCode, const char* regionCode)
{
	// release old strings
	memset(m_data->Strings, 0, sizeof(const char*) * StringManagerData::Capacity);

	char globalPath[256];
	char languagePath[256];
	char regionPath[256];

	snprintf(globalPath, 256, "Content/text.txt");
	snprintf(languagePath, 256, "Content/text_%s.txt", languageCode);
	snprintf(regionPath, 256, "Content/text_%s_%s.txt", languageCode, regionCode);
	globalPath[255] = languagePath[255] = regionPath[255] = 0;

	const char* source[StringManagerData::Capacity];
	uint32 length[StringManagerData::Capacity];
	memset(source, 0, sizeof(source));

	ini_context ctx;
	ini_item item;
	File globalFile;
	File languageFile;
	File regionFile;
	const char* path = globalPath;
	File* file = &globalFile;

doit:
	if (FileSystem::OpenAndMap(path, file))
	{
		ini_init(&ctx, (const char*)file->Memory, (const char*)file->Memory + file->FileSize);
		
		while (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				while (ini_next_within_section(&ctx, &item) == ini_result_success)
				{
					uint32 hash = Utils::CalcHashI(ctx.source + item.keyvalue.key_start, item.keyvalue.key_end - item.keyvalue.key_start);
					int index = m_data->Find(hash);

					if (index < 0)
					{
						index = m_data->Add(hash, ctx.source);
						if (index < 0)
						{
							WriteLog(LogSeverityType::Error, LogChannelType::Content, "Too many strings!");
							goto done;
						}
					}

					source[index] = ctx.source + item.keyvalue.value_start;
					length[index] = item.keyvalue.value_end - item.keyvalue.value_start;
				}
			}
		}
	}

	if (path == globalPath)
	{
		path = languagePath;
		file = &languageFile;
		goto doit;
	}
	else if (path == languagePath)
	{
		path = regionPath;
		file = &regionFile;
		goto doit;
	}

	{
	// calculate the total length of all the strings
	uint32 stringsLength = 0;
	for (uint32 i = 0; i < StringManagerData::Capacity; i++)
	{
		if (m_data->Strings[i] != nullptr)
		{
			stringsLength += length[i] + 1;
		}
	}

	// allocate memory for the strings
	m_data->StringMemory = (char*)g_memory->ReallocTrack(m_data->StringMemory, stringsLength, __FILE__, __LINE__);
	char* stringCursor = m_data->StringMemory;
	
	// now copy all the strings
	for (uint32 i = 0; i < StringManagerData::Capacity; i++)
	{
		if (m_data->Strings[i] != nullptr)
		{
			m_data->Strings[i] = stringCursor;

			assert(stringCursor + length[i] + 1 <= m_data->StringMemory + stringsLength);
			memcpy(stringCursor, source[i], length[i]);
			stringCursor[length[i]] = 0;
			stringCursor += length[i] + 1;
		}
	}
	}

done:
	FileSystem::Close(&globalFile);
	FileSystem::Close(&languageFile);
	FileSystem::Close(&regionFile);

	ConsoleCommand cmd;
	cmd.Command = "reload_strings";
	cmd.Callback = cmdReloadStrings;
	Gui::Console::AddCommands(&cmd, 1);
}

const char* StringManager::GetLocalizedText(const char* key)
{
	auto hash = Utils::CalcHashI(key);
	return GetLocalizedText(hash);
}

const char* StringManager::GetLocalizedText(uint32 keyHash)
{
	int index = m_data->Find(keyHash);
	if (index >= 0)
		return m_data->Strings[index];

	return nullptr;
}

const char* StringManager::GetLocalizedTextFromHandle(StringHandle handle)
{
	if (handle < StringManagerData::Capacity)
		return m_data->Strings[handle];

	return nullptr;
}

StringHandle StringManager::GetHandle(const char* key)
{
	auto hash = Utils::CalcHashI(key);
	return GetHandle(hash);
}

StringHandle StringManager::GetHandle(uint32 keyHash)
{
	return (uint32)m_data->Find(keyHash);
}
