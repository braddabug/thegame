#ifndef AUDIO_SONGPLAYER_H
#define AUDIO_SONGPLAYER_H

#include "../Common.h"

namespace Audio
{
	struct SongPlayerData;

	class SongPlayer
	{
		static SongPlayerData* m_data;

	public:
		static void SetGlobalData(SongPlayerData** data);

		static void Init();
		static void Shutdown();

		static bool SetCurrentSong(const char* name);

		static void Play(bool loop);
		static void Pause();
		static void Stop();

		static void Tick();

	private:

#ifdef SOUND_ENGINE_OPENAL
		static uint32 stream();
#endif
	};
}

#endif // AUDIO_SONGPLAYER_H

