#ifndef AUDIO_SOUNDMANAGER_H
#define AUDIO_SOUNDMANAGER_H

#include "../WaitManager.h"

namespace Audio
{
	struct SoundManagerData;

	enum PlayFlag
	{
		PLAYFLAG_NONE = 0,
		PLAYFLAG_LOOP = 0x01,
		PLAYFLAG_GET_HANDLE = 0x02,
		PLAYFLAG_IGNORE_PLAY_LIMIT = 0x04,
		PLAYFLAG_START_PAUSED = 0x08,
		PLAYFLAG_PRESERVE_HANDLE_AFTER_STOP = 0x10
	};

	class SoundManager
	{
		static SoundManagerData* m_data;

	public:
		static void SetGlobalData(SoundManagerData** data);
		static void Init();
		static void Shutdown();
		static void Step();

		static void PlayOnce(uint32 nameHash, Channel channel);
		static void PlayGroupOnce(const char* groupName, Channel channel);

		static Source* GetSource(uint32 nameHash, Channel channel);
		static Source* GetSourceFromGroup(const char* groupName, Channel channel);
		static void ReleaseSource(Source* source);
		
		static WaitHandle Play(Source* source, bool wait);
		static void PlayLooped(Source* source);
		static void Pause(Source* source);
		static void Resume(Source* source);
		static void Stop(Source* source);

		static SoundManagerData* GetData() { return m_data; }

		
	};
}

#endif // AUDIO_SOUNDMANAGER_H
