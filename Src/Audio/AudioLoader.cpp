#include "AudioLoader.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace Audio
{
    AudioLoader::OggInfo AudioLoader::OggOpen(const uint8* data, uint32 length)
    {
        OggInfo result;
        result.Handle = stb_vorbis_open_memory(data, length, nullptr, nullptr);
        if (result.Handle == nullptr)
            return result;

        auto info = stb_vorbis_get_info((stb_vorbis*)result.Handle);

        result.NumChannels = info.channels;
        result.SampleRate = info.sample_rate;

        return result;
    }

    uint32 AudioLoader::OggRead(OggInfo* handle, const uint8* outputBuffer, uint32 outputBufferLength)
    {
        auto size = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)handle->Handle, handle->NumChannels, (short*)outputBuffer, outputBufferLength / sizeof(short));

        return size * handle->NumChannels * sizeof(short);
    }
    
    void AudioLoader::OggRewind(OggInfo* handle)
    {
        stb_vorbis_seek_start((stb_vorbis*)handle->Handle);
    }

    void AudioLoader::OggClose(OggInfo* handle)
    {
        stb_vorbis_close((stb_vorbis*)handle->Handle);
        handle->Handle = nullptr;
    }
}

