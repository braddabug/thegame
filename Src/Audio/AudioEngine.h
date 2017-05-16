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

	struct Buffer;
	struct Source;
	struct AudioEngineData;

	class AudioEngine
	{
		static AudioEngineData* m_data;

	public:
		static void SetGlobalData(AudioEngineData** data);
		static bool Init();
		static void Shutdown();

		bool CreateBuffer(const BufferDesc* desc, Buffer* buffer);
		void DestroyBuffer(Buffer* buffer);

		Source* GetFreeSource(Channel channel, bool streaming);
		void ReleaseSource(Source* source);
		void ReleaseSourceWhenFinishedPlaying(Source* source);

		void SetBuffer(Source* source, Buffer* buffer);
		void StreamBuffer(Source* source, Buffer* buffer);

		void SetPosition(Source* source, float x, float y, float z);
		void SetPosition(Source* source, float* position3f);

		void SetPositionRel(Source* source, float x, float y, float z);
		void SetPositionRel(Source* source, float* position3f);

		void Play(Source* source);
		void Pause(Source* source);
		void Stop(Source* stop);
	};
}

#endif // AUDIO_AUDIOENGINE_H
