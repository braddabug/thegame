#include "SoundManager.h"
#include "AudioEngine.h"
#include "../FileSystem.h"
#include "../Gui/Console.h"
#include "../ConsoleCommand.h"
#include "../Content/ContentManager.h"
#include "../iniparse.h"

namespace Audio
{
	struct SoundManagerData
	{
		uint32 NumGroups;

		uint32* FileHashes;
		struct Group
		{
			uint32 NameHash;
			uint32 FirstFileIndex;
			uint32 NumFiles;
		}* Groups;
	};

	void cmdPlay(const char* param)
	{
		uint32 fileHash = Utils::CalcHash(param);
		SoundManager::Play(fileHash, Channel::SoundEffects, PLAYFLAG_NONE);
	}

	void cmdPlayGroup(const char* param)
	{
		SoundManager::PlayGroup(param, Channel::SoundEffects, PLAYFLAG_NONE);
	}

	SoundManagerData* SoundManager::m_data = nullptr;

	void SoundManager::SetGlobalData(SoundManagerData** data)
	{
		if ((*data) == nullptr)
		{
			*data = (SoundManagerData*)g_memory->AllocAndKeep(sizeof(SoundManagerData), __FILE__, __LINE__);
		}

		m_data = *data;
	}

	void SoundManager::Init()
	{
		ConsoleCommand cmd[2] = {
			{ "play", cmdPlay },
			{ "playgroup", cmdPlayGroup }
		};

		Gui::Console::AddCommands(cmd, 2);

		// TODO: load sound group info
		ini_context ctx;
		ini_item item;

		File f;
		if (FileSystem::OpenAndMap("Content/Audio/groups.txt", &f) == nullptr)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::Content, "Unable to open Audio/groups.txt. Audio will be disabled.");
			return;
		}

		ini_init(&ctx, (const char*)f.Memory, (const char*)f.Memory + f.FileSize);
		
		// count the groups
		uint32 numGroups = 0;
		uint32 numFiles = 0;

		while (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "group"))
				{
					numGroups++;

					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "file"))
							numFiles++;
					}
				}
			}
		}

		m_data->NumGroups = numGroups;
		m_data->FileHashes = (uint32*)g_memory->AllocTrack(sizeof(uint32) * numFiles, __FILE__, __LINE__);
		m_data->Groups = (SoundManagerData::Group*)g_memory->AllocTrack(sizeof(SoundManagerData::Group) * numGroups, __FILE__, __LINE__);

		ini_init(&ctx, (const char*)f.Memory, (const char*)f.Memory + f.FileSize);

		uint32 groupCursor = 0;
		uint32 fileCursor = 0;

		while (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "group"))
				{
					assert(groupCursor < numGroups);

					m_data->Groups[groupCursor].FirstFileIndex = fileCursor;
					m_data->Groups[groupCursor].NumFiles = 0;

					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "file"))
						{
							assert(fileCursor < numFiles);

							m_data->Groups[groupCursor].NumFiles++;
							m_data->FileHashes[fileCursor] = Utils::CalcHash((const uint8*)ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);

							fileCursor++;
						}
						else if (ini_key_equals(&ctx, &item, "name"))
						{
							m_data->Groups[groupCursor].NameHash = Utils::CalcHash((const uint8*)ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						}
					}

					groupCursor++;
				}
			}
		}

		FileSystem::Close(&f);
	}

	void SoundManager::Shutdown()
	{
		g_memory->FreeTrack(m_data->FileHashes, __FILE__, __LINE__);
		g_memory->FreeTrack(m_data->Groups, __FILE__, __LINE__);

		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	uint32 SoundManager::Play(uint32 nameHash, Channel channel, PlayFlag flags)
	{
		auto buffer = Content::ContentManager::Get(nameHash, Content::ResourceType::Audio);
		if (buffer == nullptr)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to find audio file with hash %u", nameHash);
			return (uint32)-1;
		}

		auto src = AudioEngine::GetFreeSource(channel, false);
		if (src == nullptr)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to get a free source");
			return (uint32)-1;
		}

		bool loop = (flags & PLAYFLAG_LOOP) == PLAYFLAG_LOOP;
		bool getHandle = (flags & PLAYFLAG_GET_HANDLE) == PLAYFLAG_GET_HANDLE;
		bool startPaused = (flags & PLAYFLAG_START_PAUSED) == PLAYFLAG_START_PAUSED;

		AudioEngine::SetBuffer(src, (Buffer*)buffer);
		AudioEngine::ReleaseSourceWhenFinishedPlaying(src);
		AudioEngine::Play(src, loop);

		Content::ContentManager::Release(buffer);

		// TODO: return a handle that the consumer can use to control the sound.
		// It'll be a WaitHandle that we can use as a key to a hash table to get the
		// original source. Once the source is done then we trigger the wait handle so
		// all observers will know the sound has finished.
		return (uint32)-1;
	}

	uint32 SoundManager::PlayGroup(const char* groupName, Channel channel, PlayFlag flags)
	{
		uint32 groupHash = Utils::CalcHash(groupName);

		// look for the group
		for (uint32 i = 0; i < m_data->NumGroups; i++)
		{
			if (m_data->Groups[i].NameHash == groupHash)
			{
				// pick a random file within the group and play it
				uint32 dice = rand() % m_data->Groups[i].NumFiles;
				uint32 fileHash = m_data->FileHashes[m_data->Groups[i].FirstFileIndex + dice];

				return Play(fileHash, channel, flags);
			}
		}

		// still here? we failed :(
		return (uint32)-1;
	}
}
