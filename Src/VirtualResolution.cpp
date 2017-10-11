#include "VirtualResolution.h"
#include "MemoryManager.h"
#include <cstring>

struct VirtualResolutionData
{
	Resolution NearestResolution;
};

VirtualResolutionData* VirtualResolution::m_data = nullptr;

void VirtualResolution::SetGlobalData(VirtualResolutionData** data)
{
	if (*data == nullptr)
	{
		*data = (VirtualResolutionData*)g_memory->AllocTrack(sizeof(VirtualResolutionData), __FILE__, __LINE__);
		memset(*data, 0, sizeof(VirtualResolutionData));
	}

	m_data = *data;
}

void VirtualResolution::Shutdown()
{
	g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	m_data = nullptr;
}

void VirtualResolution::Init(int screenHeight)
{
#undef DEFINE_RESOLUTION
#define DEFINE_RESOLUTION(r) if (screenHeight >= r) m_data->NearestResolution = Resolution::R ## r; else

	DEFINE_RESOLUTIONS
	{
		// else... the resolution is too small, but we'll give it a shot...
		m_data->NearestResolution = Resolution::R640;
	}
}

bool VirtualResolution::InjectNearestResolution(const char* source, char* destination, uint32 destinationLength)
{
	uint32 sourceCursor = 0;
	uint32 destCursor = 0;

	destination[0] = 0;

	while (destCursor < destinationLength - 1)
	{
		if (source[sourceCursor] == 0)
		{
			return true;
		}

		if (source[sourceCursor] == '%')
		{
			int resolution = (int)m_data->NearestResolution;
			char buffer[10];
			_itoa_s(resolution, buffer, 10);

			char* bufferCursor = buffer;
			while (*bufferCursor != 0 && destCursor < destinationLength - 1)
			{
				destination[destCursor++] = *bufferCursor;
				bufferCursor++;
			}

			sourceCursor++;
		}
		else
		{
			destination[destCursor++] = source[sourceCursor++];
			destination[destCursor] = 0;
		}
	}

	return false;
}