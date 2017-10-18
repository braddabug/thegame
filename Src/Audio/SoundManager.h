#ifndef AUDIO_SOUNDMANAGER_H
#define AUDIO_SOUNDMANAGER_H

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

		static uint32 Play(uint32 nameHash, Channel channel, PlayFlag flags);
		static uint32 PlayGroup(const char* groupName, Channel channel, PlayFlag flags);
		static void ReleaseHandle(uint32 handle);
		
		static void Resume(uint32 handle);
		static void Pause(uint32 handle);
		static void Stop(uint32 handle);

	};
}

#endif // AUDIO_SOUNDMANAGER_H
