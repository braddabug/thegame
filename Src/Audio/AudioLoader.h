#ifndef AUDIO_AUDIOLOADER_H
#define AUDIO_AUDIOLOADER_H

#include "AudioEngine.h"
#include "../FileSystem.h"

namespace Audio
{
	class AudioLoader
	{
	public:
		static bool LoadWav(Content::ContentLoaderParams* params)
		{
			if (params->Phase == Content::LoaderPhase::AsyncLoad)
			{
				File f;
				auto filename = FileSystem::GetDiskFilenameByHash(params->FilenameHash);
				if (FileSystem::OpenAndMap(filename, &f) == nullptr)
				{
					params->State = Content::ContentState::NotFound;
					return false;
				}

				struct RiffHeader
				{
					uint32 Riff;
					uint32 ChunkSize;
					uint32 WaveID;;
					uint32 RiffType;
				};
				static_assert(sizeof(RiffHeader) == sizeof(uint32) * 4, "RiffHeader is unexpected size");

				uint8* cursor = (uint8*)f.Memory;

				RiffHeader* header = (RiffHeader*)cursor;
				cursor += sizeof(uint32) * 4;

				// TODO: validate the riff header

				uint32 formatSize;
				memcpy(&formatSize, cursor, sizeof(uint32));
				cursor += sizeof(uint32);

				struct WAVEFORMATEX
				{
					uint16 FormatTag;
					uint16 Channels;
					uint32 SamplesPerSec;
					uint32 AvgBytesPerSec;
					uint16 BlockAlign;
					uint16 BitsPerSample;
					uint16 Size;
				};

				WAVEFORMATEX* wavHeader = (WAVEFORMATEX*)cursor;
				cursor += formatSize;

				struct DataHeader
				{
					uint32 Data;
					uint32 Size;
				};

				DataHeader* data = (DataHeader*)cursor;
				cursor += sizeof(uint32) * 2;

				BufferDesc d = {};
				d.NumChannels = wavHeader->Channels;
				d.SampleBitSize = wavHeader->BitsPerSample;
				d.SampleRate = wavHeader->SamplesPerSec;
				d.Data = cursor;
				d.DataByteLength = data->Size;
				return AudioEngine::CreateBuffer(&d, (Buffer*)params->Destination);
			}

			return true;
		}
	};
}

#endif // AUDIO_AUDIOLOADER_H

