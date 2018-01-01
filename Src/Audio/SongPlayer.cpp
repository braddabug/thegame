#include "SongPlayer.h"
#include "AudioEngine.h"
#include "../MemoryManager.h"
#include "../FileSystem.h"
#include "../ConsoleCommand.h"
#include "../Gui/Console.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#ifdef SOUND_ENGINE_OPENAL
#include "AL/al.h"
#include "AL/alc.h"
#endif

namespace Audio
{
	struct SongPlayerData
	{
		File CurrentFile;
		stb_vorbis* Decoder;
		uint32 FrameSize;
		bool Loop;
		
#ifdef SOUND_ENGINE_OPENAL
		ALenum ALFormat;
		ALsizei ALFrequency;
		Source* ALSource;
		bool RestartSourceIfStopped;
#endif

		static const uint32 MaxBuffers = 3;
		ALuint Buffers[MaxBuffers];

		static const uint32 ScratchBufferSize = 1024 * 16;
		uint8 ScratchBuffer[ScratchBufferSize];
	};

	SongPlayerData* SongPlayer::m_data = nullptr;

	void cmdPlaySong(const char* arg)
	{
		if (SongPlayer::SetCurrentSong(arg) == false)
		{
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to play %s", arg);
		}
		else
		{
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "Playing %s", arg);
			SongPlayer::Play(true);
		}
	}

	void SongPlayer::SetGlobalData(SongPlayerData** data)
	{
		if (*data == nullptr)
		{
			*data = (SongPlayerData*)g_memory->AllocTrack(sizeof(SongPlayerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(SongPlayerData));
		}
		
		m_data = *data;
	}

	void SongPlayer::Init()
	{
#ifdef SOUND_ENGINE_OPENAL
		alGenBuffers(SongPlayerData::MaxBuffers, m_data->Buffers);
#endif

		ConsoleCommand cmd;
		cmd.Callback = cmdPlaySong;
		cmd.Command = "play_song";
		Gui::Console::AddCommands(&cmd, 1);
	}
	
	void SongPlayer::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	bool SongPlayer::SetCurrentSong(const char* name)
	{
		if (m_data->Decoder != nullptr)
		{
			stb_vorbis_close(m_data->Decoder);
			m_data->Decoder = nullptr;
		}
		
		FileSystem::Close(&m_data->CurrentFile);

		if (name != nullptr)
		{
			if (FileSystem::OpenAndMap(name, &m_data->CurrentFile) == nullptr)
			{
				m_data->CurrentFile.Handle = 0;
				m_data->CurrentFile.Memory = nullptr;
				return false;
			}

			int error;
			m_data->Decoder = stb_vorbis_open_memory((const unsigned char*)m_data->CurrentFile.Memory, m_data->CurrentFile.FileSize, &error, nullptr);
			if (m_data->Decoder == nullptr)
			{
				FileSystem::Close(&m_data->CurrentFile);
				m_data->CurrentFile.Handle = 0;
				m_data->CurrentFile.Memory = nullptr;
				return false;
			}

			auto info = stb_vorbis_get_info(m_data->Decoder);
			m_data->FrameSize = info.max_frame_size;

#ifdef SOUND_ENGINE_OPENAL
			m_data->ALFormat = AudioEngine::GetALFormat(info.channels, 16);
			m_data->ALFrequency = info.sample_rate;
			
			m_data->ALSource = AudioEngine::GetFreeStreamingSource(Channel::Music, info.sample_rate, 2);
			
			while (AudioEngine::StreamingSourceNeedsData(m_data->ALSource))
			{
				auto len = stream();
				AudioEngine::StreamToSource(m_data->ALSource, m_data->ScratchBuffer, len);
			}
#endif

			
		}

		return true;
	}

	void SongPlayer::Play(bool loop)
	{
		m_data->Loop = loop;
		
		AudioEngine::Play(m_data->ALSource, false);

#ifdef SOUND_ENGINE_OPENAL
		m_data->RestartSourceIfStopped = true;
#endif
	}

	void SongPlayer::Pause()
	{
		AudioEngine::Pause(m_data->ALSource);

#ifdef SOUND_ENGINE_OPENAL
		m_data->RestartSourceIfStopped = false;
#endif
	}

	void SongPlayer::Stop()
	{
		AudioEngine::Stop(m_data->ALSource);

#ifdef SOUND_ENGINE_OPENAL
		m_data->RestartSourceIfStopped = false;
#endif

		stb_vorbis_seek_start(m_data->Decoder);
	}

	void SongPlayer::Tick()
	{
		// look for available buffers and fill them

		if (m_data->RestartSourceIfStopped)
		{
			auto state = AudioEngine::GetState(m_data->ALSource);
			if (state != SourceState::Playing)
				AudioEngine::Play(m_data->ALSource, false);
		}

		if (AudioEngine::StreamingSourceNeedsData(m_data->ALSource))
		{
			auto len = stream();
			AudioEngine::StreamToSource(m_data->ALSource, m_data->ScratchBuffer, len);
		}
	}

#ifdef SOUND_ENGINE_OPENAL
	uint32 SongPlayer::stream()
	{
		const uint32 numChannels = 2;

		uint32 totalBytes = 0;
		do
		{
			auto numSamples = stb_vorbis_get_frame_short_interleaved(m_data->Decoder, numChannels, (short*)(m_data->ScratchBuffer + totalBytes), (SongPlayerData::ScratchBufferSize - totalBytes) / sizeof(short));

			if (numSamples == 0)
			{
				if (m_data->Loop)
					stb_vorbis_seek_start(m_data->Decoder);
				else
					m_data->RestartSourceIfStopped = false;
			}
				
			totalBytes += numSamples * sizeof(short) * numChannels;
		} while (totalBytes + m_data->FrameSize < SongPlayerData::ScratchBufferSize);

		return totalBytes;
	}
#endif
}
