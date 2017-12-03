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

		static const uint32 MaxSources = 120;
		static const uint32 MaxStreamingSources = 8;
		static const uint32 MaxStreamingBuffers = 3;
		uint32 NumSources;
		uint32 NumStreamingSources;
		Source Sources[MaxSources];
		Source StreamingSources[MaxStreamingSources];
		SourceOwnership SourceOwnershipState[MaxSources];
		SourceOwnership StreamingSourceOwnershipState[MaxStreamingSources];

		struct
		{
			uint32 SampleRate;
			uint32 NumChannels;
			ALuint Buffers[MaxStreamingBuffers];
		} StreamingSourceBufferInfo[MaxStreamingSources];
	};

	AudioEngineData* AudioEngine::m_data = nullptr;

	void AudioEngine::SetGlobalData(AudioEngineData** data)
	{
		if (*data == nullptr)
		{
			*data = (AudioEngineData*)g_memory->AllocTrack(sizeof(AudioEngineData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(AudioEngineData));
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

		uint32 totalSourceCount = 0;
		const uint32 maxTotalSources = AudioEngineData::MaxSources + AudioEngineData::MaxStreamingSources;
		ALuint sources[maxTotalSources];

		for (uint32 i = 0; i < maxTotalSources; i++)
		{
			alGenSources(1, &sources[i]);
			if (alGetError() == AL_NO_ERROR)
				totalSourceCount++;
			else
				break;
		}

		if (totalSourceCount < 10)
		{
			LOG_ERROR("OpenAL was only able to create %d sources, which is not enough", m_data->NumSources);
			return false;
		}

		m_data->NumStreamingSources = totalSourceCount / 10;
		if (m_data->NumStreamingSources > AudioEngineData::MaxStreamingSources) m_data->NumStreamingSources = AudioEngineData::MaxStreamingSources;
		m_data->NumSources = totalSourceCount - m_data->NumStreamingSources;
		
		assert(m_data->NumSources <= AudioEngineData::MaxSources);
		for (uint32 i = 0; i < m_data->NumSources; i++)
		{
			m_data->Sources[i].Index = i;
			m_data->Sources[i].Streaming = false;
			m_data->Sources[i].ALSource = sources[i];
		}

		assert(m_data->NumStreamingSources <= AudioEngineData::MaxStreamingSources);
		for (uint32 i = 0; i < m_data->NumStreamingSources; i++)
		{
			m_data->StreamingSources[i].Index = i;
			m_data->StreamingSources[i].Streaming = true;
			m_data->StreamingSources[i].ALSource = sources[m_data->NumSources + i];

			alGenBuffers(AudioEngineData::MaxStreamingBuffers, m_data->StreamingSourceBufferInfo[i].Buffers);
		}

		LOG("OpenAL initialized with %d sources (%d streaming)", totalSourceCount, m_data->NumStreamingSources);

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

		buffer->Internal.NumChannels = desc->NumChannels;
		buffer->Internal.SampleRate = desc->SampleRate;
		buffer->Internal.SampleBitSize = desc->SampleBitSize;

		if (desc->Data != nullptr)
			return WriteToBuffer(buffer, desc->Data, desc->DataByteLength);

		return true;
	}

	bool AudioEngine::WriteToBuffer(Buffer* buffer, const uint8* data, uint32 length)
	{
		if (data == nullptr)
			return false;

		ALenum format = 0;
		if (buffer->Internal.SampleBitSize == 8)
		{
			if (buffer->Internal.NumChannels == 1)
				format = AL_FORMAT_MONO8;
			else if (buffer->Internal.NumChannels == 2)
				format = AL_FORMAT_STEREO8;
		}
		else if (buffer->Internal.SampleBitSize == 16)
		{
			if (buffer->Internal.NumChannels == 1)
				format = AL_FORMAT_MONO16;
			else if (buffer->Internal.NumChannels == 2)
				format = AL_FORMAT_STEREO16;
		}
		if (format == 0)
			return false;

		alBufferData(buffer->ALBuffer, format, data, length, buffer->Internal.SampleRate);

		return true;
	}

	void AudioEngine::DestroyBuffer(Buffer* buffer)
	{
		alDeleteBuffers(1, &buffer->ALBuffer);
	}

	Source* AudioEngine::GetFreeSource(Channel channel)
	{
		return getFreeSource(channel, false);
	}

	Source* AudioEngine::GetFreeStreamingSource(Channel channel, uint32 sampleRate, uint32 numChannels)
	{
		auto source = getFreeSource(channel, true);

		// remember the sample rate and channel count
		if (source != nullptr)
		{
			m_data->StreamingSourceBufferInfo[source->Index].NumChannels = numChannels;
			m_data->StreamingSourceBufferInfo[source->Index].SampleRate = sampleRate;
		}

		return source;
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

	bool AudioEngine::StreamingSourceNeedsData(Source* source)
	{
		if (source == nullptr || source->Streaming == false)
			return false;

		ALint state, total, processed;

		alGetSourcei(source->ALSource, AL_SOURCE_STATE, &state);
		if (state == AL_INITIAL)
		{
			// if initial then no buffers are considered processed, which isn't helpful, is it?
			alGetSourcei(source->ALSource, AL_BUFFERS_QUEUED, &total);

			return total < AudioEngineData::MaxStreamingBuffers;
		}

		alGetSourcei(source->ALSource, AL_BUFFERS_PROCESSED, &processed);

		return processed > 0;
	}

	void AudioEngine::StreamToSource(Source* source, uint8* data, uint32 dataLength)
	{
		if (source == nullptr || source->Streaming == false || source->Index >= m_data->NumStreamingSources)
			return;

		ALint state;
		ALuint buffer;

		alGetSourcei(source->ALSource, AL_SOURCE_STATE, &state);
		if (state == AL_INITIAL)
		{
			ALint total;
			alGetSourcei(source->ALSource, AL_BUFFERS_QUEUED, &total);
			if (total < AudioEngineData::MaxStreamingBuffers)
			{
				buffer = m_data->StreamingSourceBufferInfo[source->Index].Buffers[total];
			}
		}
		else
		{
			alSourceUnqueueBuffers(source->ALSource, 1, &buffer);
		}

		// fill the buffer
		auto format = GetALFormat(m_data->StreamingSourceBufferInfo[source->Index].NumChannels, 16);
		alBufferData(buffer, format, data, dataLength, m_data->StreamingSourceBufferInfo[source->Index].SampleRate);

		alSourceQueueBuffers(source->ALSource, 1, &buffer);
	}

	void AudioEngine::Play(Source* source, bool loop)
	{
		alSourcei(source->ALSource, AL_LOOPING, loop ? 1 : 0);
		alSourcePlay(source->ALSource);
	}

	void AudioEngine::Pause(Source* source)
	{
		alSourcePause(source->ALSource);
	}

	void AudioEngine::Resume(Source* source)
	{
		alSourcePlay(source->ALSource);
	}

	void AudioEngine::Stop(Source* source)
	{
		alSourceStop(source->ALSource);
	}

	SourceState AudioEngine::GetState(Source* source)
	{
		int value;
		alGetSourcei(source->ALSource, AL_SOURCE_STATE, &value);

		switch (value)
		{
		case AL_PLAYING: return SourceState::Playing;
		case AL_PAUSED: return SourceState::Paused;
		default: return SourceState::Stopped;
		}
	}

#ifdef SOUND_ENGINE_OPENAL
	uint32 AudioEngine::GetALSource(Source* source)
	{
		return source->ALSource;
	}

	int AudioEngine::GetALFormat(int numChannels, int bitsPerSample)
	{
		ALenum format = 0;
		if (bitsPerSample == 8)
		{
			if (numChannels == 1)
				format = AL_FORMAT_MONO8;
			else if (numChannels == 2)
				format = AL_FORMAT_STEREO8;
		}
		else if (bitsPerSample == 16)
		{
			if (numChannels == 1)
				format = AL_FORMAT_MONO16;
			else if (numChannels == 2)
				format = AL_FORMAT_STEREO16;
		}

		return format;
	}
#endif

	Source* AudioEngine::getFreeSource(Channel channel, bool streaming)
	{
		uint32 length = streaming ? m_data->NumStreamingSources : m_data->NumSources;

		SourceOwnership* ownership = streaming ? m_data->StreamingSourceOwnershipState : m_data->SourceOwnershipState;
		Source* sources = streaming ? m_data->StreamingSources : m_data->Sources;

		uint32 source = (uint32)-1;

		for (uint32 i = 0; i < length; i++)
		{
			if (ownership[i] == SourceOwnership::Released)
			{
				source = i;
				break;
			}
			else if (ownership[i] == SourceOwnership::ReleaseWhenFinished)
			{
				ALint state;
				alGetSourcei(sources[i].ALSource, AL_SOURCE_STATE, &state);

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

			// unqueue all buffers
			alSourceStop(sources[source].ALSource);

			ALint processed;
			alGetSourcei(sources[source].ALSource, AL_BUFFERS_PROCESSED, &processed);
			for (int i = 0; i < processed; i++)
			{
				ALuint buffer;
				alSourceUnqueueBuffers(sources[source].ALSource, 1, &buffer);
			}

#ifndef NDEBUG
			// make sure that NO buffers are in the queue
			ALint total;
			alGetSourcei(sources[source].ALSource, AL_BUFFERS_QUEUED, &total);
			assert(total == 0);
#endif

			alSourceRewind(sources[source].ALSource); // make sure it's always AL_INITIAL
			alSourcei(sources[source].ALSource, AL_LOOPING, 0);

			ownership[source] = SourceOwnership::Owned;
			return &sources[source];
		}

		return nullptr;
	}
}
