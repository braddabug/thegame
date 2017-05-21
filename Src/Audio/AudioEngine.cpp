#include "AudioEngine.h"
#include "../Logging.h"
#include "../MemoryManager.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace Audio
{
	struct Source
	{
		ALuint ALSource;
		uint32 Index;
		bool Streaming;
	};

	enum class SourceOwnership : uint8
	{
		Released = 0,
		ReleaseWhenFinished = 1,
		Owned = 2
	};

	struct AudioEngineData
	{
		ALCdevice* Device;
		ALCcontext* Context;

		static const uint32 MaxSources = 128;
		uint32 NumSources;
		uint32 NumStreamingSources;
		Source Sources[MaxSources];
		SourceOwnership SourceOwnershipState[MaxSources];
	};

	AudioEngineData* AudioEngine::m_data = nullptr;

	void AudioEngine::SetGlobalData(AudioEngineData** data)
	{
		if (*data == nullptr)
		{
			*data = NewObject<AudioEngineData>(__FILE__, __LINE__);
		}

		m_data = *data;
	}

	bool AudioEngine::Init()
	{
		m_data->Device = alcOpenDevice(nullptr);
		if (m_data->Device == nullptr)
		{
			LOG_ERROR("Unable to open OpenAL device");
			return false;
		}

		m_data->Context = alcCreateContext(m_data->Device, nullptr);
		alcMakeContextCurrent(m_data->Context);
		if (m_data->Context == nullptr)
		{
			alcCloseDevice(m_data->Device);
			LOG_ERROR("Unable to create OpenAL context");
			return false;
		}

		m_data->NumSources = 0;
		m_data->NumStreamingSources = 0;
		memset(m_data->SourceOwnershipState, 0, sizeof(m_data->SourceOwnershipState));
		alGetError();
		for (uint32 i = 0; i < AudioEngineData::MaxSources; i++)
		{
			m_data->Sources[i].Index = i;
			m_data->Sources[i].Streaming = false;

			alGenSources(1, &m_data->Sources[i].ALSource);
			if (alGetError() == AL_NO_ERROR)
				m_data->NumSources++;
			else
				break;
		}

		if (m_data->NumSources < 10)
		{
			LOG_ERROR("OpenAL was only able to create %d sources, which is not enough", m_data->NumSources);
			return false;
		}

		// designate some of the sources we just created as streaming ones
		m_data->NumStreamingSources = m_data->NumSources / 10;

		LOG("OpenAL initialized with %d sources", m_data->NumSources);

		return true;
	}

	void AudioEngine::Shutdown()
	{
		alcMakeContextCurrent(nullptr);

		if (m_data->Context != nullptr)
		{
			alcDestroyContext(m_data->Context);
			m_data->Context = nullptr;
		}

		if (m_data->Device != nullptr)
		{
			alcCloseDevice(m_data->Device);
			m_data->Device = nullptr;
		}

		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	bool AudioEngine::CreateBuffer(const BufferDesc* desc, Buffer* buffer)
	{
		ALenum format = 0;
		if (desc->SampleBitSize == 8)
		{
			if (desc->NumChannels == 1)
				format = AL_FORMAT_MONO8;
			else if (desc->NumChannels == 2)
				format = AL_FORMAT_STEREO8;
		}
		else if (desc->SampleBitSize == 16)
		{
			if (desc->NumChannels == 1)
				format = AL_FORMAT_MONO16;
			else if (desc->NumChannels == 2)
				format = AL_FORMAT_STEREO16;
		}
		if (format == 0)
			return false;
		
		alGenBuffers(1, &buffer->ALBuffer);

		alBufferData(buffer->ALBuffer, format, desc->Data, desc->DataByteLength, desc->SampleRate);

		return true;
	}

	void AudioEngine::DestroyBuffer(Buffer* buffer)
	{
		alDeleteBuffers(1, &buffer->ALBuffer);
	}

	Source* AudioEngine::GetFreeSource(Channel channel, bool streaming)
	{
		uint32 start = streaming ? 0 : m_data->NumStreamingSources;
		uint32 length = streaming ? m_data->NumStreamingSources : m_data->NumSources - m_data->NumStreamingSources;

		uint32 source = (uint32)-1;

		for (uint32 i = start; i < length; i++)
		{
			if (m_data->SourceOwnershipState[i] == SourceOwnership::Released)
			{
				source = i;
				break;
			}
			else if (m_data->SourceOwnershipState[i] == SourceOwnership::ReleaseWhenFinished)
			{
				ALint state;
				alGetSourcei(m_data->Sources[i].ALSource, AL_SOURCE_STATE, &state);

				if (state != AL_PLAYING)
				{
					source = i;
					break;
				}
			}
		}

		if (source != (uint32)-1)
		{
			// TODO: set some defaults on the source

			m_data->SourceOwnershipState[source] = SourceOwnership::Owned;
			return &m_data->Sources[source];
		}

		return nullptr;
	}

	void AudioEngine::ReleaseSource(Source* source)
	{
		if (source != nullptr)
		{
			m_data->SourceOwnershipState[source->Index] = SourceOwnership::Released;
		}
	}

	void AudioEngine::ReleaseSourceWhenFinishedPlaying(Source* source)
	{
		if (source != nullptr)
		{
			m_data->SourceOwnershipState[source->Index] = SourceOwnership::ReleaseWhenFinished;
		}
	}

	void AudioEngine::SetBuffer(Source* source, Buffer* buffer)
	{
		if (source != nullptr && buffer != nullptr)
		{
			alSourcei(source->ALSource, AL_BUFFER, buffer->ALBuffer);
		}
	}

	void AudioEngine::StreamBuffer(Source* source, Buffer* buffer)
	{
		if (source != nullptr && buffer != nullptr)
		{
			alSourceQueueBuffers(source->ALSource, 1, &buffer->ALBuffer);
		}
	}

	void AudioEngine::Play(Source* source)
	{
		alSourcePlay(source->ALSource);
	}
}
