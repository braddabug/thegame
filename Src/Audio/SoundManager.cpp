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

		struct PlayingInfo
		{
			static const uint32 Capacity = 100;
			Source* Sources[Capacity];
			WaitHandle Waits[Capacity];
			Channel Channels[Capacity];
			bool ReleaseWhenStopped[Capacity];

			int Add(Source* source, Channel channel)
			{
				int index = (intptr_t)source % Capacity;

				for (uint32 i = 0; i < Capacity; i++)
				{
					int i2 = (i + index) % Capacity;
					if (Sources[i2] == nullptr)
					{
						Sources[i2] = source;
						Channels[i2] = channel;
						Waits[i2] = WaitManager::INVALID_WAIT;
						return i2;
					}
				}

				// we must be out of space :(
				return -1;
			}

			int Find(Source* source)
			{
				int index = (intptr_t)source % Capacity;

				for (uint32 i = 0; i < Capacity; i++)
				{
					uint32 i2 = (i + index) % Capacity;
					if (Sources[i2] == source)
					{
						return i2;
					}
					else if (Sources[i2] == nullptr)
					{
						// we found an empty source before the matching handle, therefore
						// that sound is not in our collection.
						return -1;
					}
				}

				return -1;
			}
		} Playing;
	};

	void cmdPlay(const char* param)
	{
		uint32 fileHash = Utils::CalcHash(param);
		SoundManager::PlayOnce(fileHash, Channel::SoundEffects);
	}

	void cmdPlayGroup(const char* param)
	{
		SoundManager::PlayGroupOnce(param, Channel::SoundEffects);
	}

	void cmdSoundSourceCount(const char* param)
	{
		auto data = SoundManager::GetData();

		uint32 count = 0;
		for (uint32 i = 0; i < data->Playing.Capacity; i++)
		{
			if (data->Playing.Sources[i] != nullptr)
				count++;
		}

		WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "%u sound sources active (%u free)", count, data->Playing.Capacity - count);
	}

	SoundManagerData* SoundManager::m_data = nullptr;

	void SoundManager::SetGlobalData(SoundManagerData** data)
	{
		if ((*data) == nullptr)
		{
			*data = (SoundManagerData*)g_memory->AllocAndKeep(sizeof(SoundManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(SoundManagerData));
		}

		m_data = *data;
	}

	void SoundManager::Init()
	{
		ConsoleCommand cmd[3] = {
			{ "play", cmdPlay },
			{ "playgroup", cmdPlayGroup },
			{ "sound_source_count", cmdSoundSourceCount }
		};

		Gui::Console::AddCommands(cmd, 3);

		// load sound group info
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

	void SoundManager::Step()
	{
		for (uint32 i = 0; i < m_data->Playing.Capacity; i++)
		{
			if (m_data->Playing.Sources[i] != nullptr)
			{
				if (m_data->Playing.Waits[i] != WaitManager::INVALID_WAIT ||
					m_data->Playing.ReleaseWhenStopped[i])
				{
					// check to see if this is done waiting
					if (AudioEngine::GetState(m_data->Playing.Sources[i]) == SourceState::Stopped)
					{
						if (m_data->Playing.Waits[i] != WaitManager::INVALID_WAIT)
						{
							WaitManager::SetWaitDone(m_data->Playing.Waits[i]);
							m_data->Playing.Waits[i] = WaitManager::INVALID_WAIT;
						}

						if (m_data->Playing.ReleaseWhenStopped[i])
						{
							ReleaseSource(m_data->Playing.Sources[i]);
						}
					}
				}
			}
		}
	}

	void SoundManager::PlayOnce(uint32 nameHash, Channel channel)
	{
		auto src = GetSource(nameHash, channel);
		if (src != nullptr)
		{
			Play(src, false);
			ReleaseSource(src);
		}
	}

	void SoundManager::PlayGroupOnce(const char* groupName, Channel channel)
	{
		auto src = GetSourceFromGroup(groupName, channel);
		if (src != nullptr)
		{
			Play(src, false);
			ReleaseSource(src);
		}
	}

	Source* SoundManager::GetSource(uint32 nameHash, Channel channel)
	{
		auto buffer = Content::ContentManager::Get(nameHash, Content::ResourceType::Audio);
		if (buffer == nullptr)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to find audio file with hash %u", nameHash);
			return nullptr;
		}

		auto src = AudioEngine::GetFreeSource(channel, false);
		if (src == nullptr)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to get a free source");
			return nullptr;
		}

		AudioEngine::SetBuffer(src, (Buffer*)buffer);

		Content::ContentManager::Release(buffer);

		m_data->Playing.Add(src, channel);

		return src;
	}

	Source* SoundManager::GetSourceFromGroup(const char* groupName, Channel channel)
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

				return GetSource(fileHash, channel);
			}
		}

		return nullptr;
	}

	void SoundManager::ReleaseSource(Source* source)
	{
		AudioEngine::ReleaseSource(source);

		auto index = m_data->Playing.Find(source);
		if (index >= 0)
		{
			m_data->Playing.Sources[index] = nullptr;
			
			if (m_data->Playing.Waits[index] != WaitManager::INVALID_WAIT)
				WaitManager::SetWaitDone(m_data->Playing.Waits[index]);
		}
	}

	void SoundManager::ReleaseSourceWhenFinishedPlaying(Source* source)
	{
		auto index = m_data->Playing.Find(source);
		if (index >= 0)
		{
			m_data->Playing.ReleaseWhenStopped[index] = true;
		}
	}

	WaitHandle SoundManager::Play(Source* source, bool wait)
	{
		if (source == nullptr)
			return WaitManager::INVALID_WAIT;

		AudioEngine::Play(source, false);

		if (wait && AudioEngine::GetState(source) == SourceState::Playing)
		{
			auto index = m_data->Playing.Find(source);
			if (index >= 0)
			{
				auto wait = WaitManager::CreateWait();

				m_data->Playing.Waits[index] = wait;

				return wait;
			}
		}

		return WaitManager::INVALID_WAIT;
	}

	void SoundManager::PlayLooped(Source* source)
	{
		AudioEngine::Play(source, true);
	}

	void SoundManager::Pause(Source* source)
	{
		AudioEngine::Pause(source);
	}

	void SoundManager::Resume(Source* source)
	{
		AudioEngine::Resume(source);
	}
	
	void SoundManager::Stop(Source* source)
	{
		AudioEngine::Stop(source);

		auto index = m_data->Playing.Find(source);
		if (index >= 0)
		{
			WaitManager::SetWaitDone(m_data->Playing.Waits[index]);
			m_data->Playing.Waits[index] = WaitManager::INVALID_WAIT;
		}
	}
}
