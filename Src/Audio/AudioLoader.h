#ifndef AUDIO_AUDIOLOADER_H
#define AUDIO_AUDIOLOADER_H

#include "AudioEngine.h"
#include "../FileSystem.h"

namespace Audio
{
	class AudioLoader
	{
	public:
		static bool LoadWav(const char* filename, Buffer* destination)
		{
			File* f;
			if (FileSystem::OpenAndMap(filename, f) == nullptr)
				return false;

			struct RiffHeader
			{
				uint32 Riff;
				uint32 ChunkSize;
				uint32 WaveID;
				uint32 RiffType;
			};
			static_assert(sizeof(RiffHeader) == sizeof(uint32) * 4, "RiffHeader is unexpected size");

			uint8* cursor = (uint8*)f->Memory;

			RiffHeader* header = (RiffHeader*)cursor;
			cursor += sizeof(uint32) * 4;

			// TODO: validate the riff header

			return false;
		}
	};
}

#endif // AUDIO_AUDIOLOADER_H

