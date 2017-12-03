#ifndef AUDIO_AUDIOENGINE_H
#define AUDIO_AUDIOENGINE_H

#include "../Common.h"

namespace Audio
{
	enum class Channel
	{
		SoundEffects,
		Music
	};

	struct BufferDesc
	{
		uint32 NumChannels;
		uint32 SampleRate;
		uint32 SampleBitSize;

		uint8* Data;
		uint32 DataByteLength;
	};

	struct Buffer
	{
		uint32 ALBuffer;

		struct
		{
			uint32 NumChannels;
			uint32 SampleRate;
			uint32 SampleBitSize;
		} Internal;
	};

	struct Source;
	struct AudioEngineData;

	enum class SourceState
	{
		Stopped,
		Playing,
		Paused
	};

	class AudioEngine
	{
		static AudioEngineData* m_data;

	public:
		static void SetGlobalData(AudioEngineData** data);
		static bool Init();
		static void Shutdown();

		static bool CreateBuffer(const BufferDesc* desc, Buffer* buffer);
		static bool WriteToBuffer(Buffer* buffer, const uint8* data, uint32 length);
		static void DestroyBuffer(Buffer* buffer);

		static Source* GetFreeSource(Channel channel);
		static Source* GetFreeStreamingSource(Channel channel, uint32 sampleRate, uint32 numChannels);
		static void ReleaseSource(Source* source);
		static void ReleaseSourceWhenFinishedPlaying(Source* source);

		static void SetBuffer(Source* source, Buffer* buffer);

		static bool StreamingSourceNeedsData(Source* source);
		static void StreamToSource(Source* source, uint8* data, uint32 dataLength);

		void SetPosition(Source* source, float x, float y, float z);
		void SetPosition(Source* source, float* position3f);

		void SetPositionRel(Source* source, float x, float y, float z);
		void SetPositionRel(Source* source, float* position3f);

		static void Play(Source* source, bool loop);
		static void Pause(Source* source);
		static void Resume(Source* source);
		static void Stop(Source* stop);

		static SourceState GetState(Source* source);

#ifdef SOUND_ENGINE_OPENAL
		static uint32 GetALSource(Source* source);
		static int GetALFormat(int numChannels, int bitsPerSample);
#endif

	private:
		static Source* getFreeSource(Channel channel, bool streaming);
	};
}

#endif // AUDIO_AUDIOENGINE_H
