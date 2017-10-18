#ifndef AUDIO_AUDIOENGINE_H
#define AUDIO_AUDIOENGINE_H

#include "../Common.h"

namespace Audio
{
	enum class Channel
	{
		SoundEffects
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
	};

	struct Source;
	struct AudioEngineData;

	class AudioEngine
	{
		static AudioEngineData* m_data;

	public:
		static void SetGlobalData(AudioEngineData** data);
		static bool Init();
		static void Shutdown();

		static bool CreateBuffer(const BufferDesc* desc, Buffer* buffer);
		static void DestroyBuffer(Buffer* buffer);

		static Source* GetFreeSource(Channel channel, bool streaming);
		static void ReleaseSource(Source* source);
		static void ReleaseSourceWhenFinishedPlaying(Source* source);

		static void SetBuffer(Source* source, Buffer* buffer);
		static void StreamBuffer(Source* source, Buffer* buffer);

		void SetPosition(Source* source, float x, float y, float z);
		void SetPosition(Source* source, float* position3f);

		void SetPositionRel(Source* source, float x, float y, float z);
		void SetPositionRel(Source* source, float* position3f);

		static void Play(Source* source, bool loop);
		void Pause(Source* source);
		void Stop(Source* stop);
	};
}

#endif // AUDIO_AUDIOENGINE_H
