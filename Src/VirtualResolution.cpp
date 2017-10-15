#include "VirtualResolution.h"
#include "MemoryManager.h"
#include "MyNxna2.h"
#include <cstring>

const float VirtualWidth = 1024;
const float VirtualHeight = 768;

struct VirtualResolutionData
{
	Resolution NearestResolution;
	float ScreenWidth, ScreenHeight;
	float Scaling;
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

void VirtualResolution::Init(int screenWidth, int screenHeight)
{
#undef DEFINE_RESOLUTION
#define DEFINE_RESOLUTION(r) if (screenHeight >= r) m_data->NearestResolution = Resolution::R ## r; else

	DEFINE_RESOLUTIONS
	{
		// else... the resolution is too small, but we'll give it a shot...
		m_data->NearestResolution = Resolution::R640;
	}

	m_data->ScreenWidth = (float)screenWidth;
	m_data->ScreenHeight = (float)screenHeight;
	m_data->Scaling = m_data->ScreenHeight / VirtualHeight;
}

Nxna::Vector2 VirtualResolution::ConvertVirtualToScreen(Nxna::Vector2 virtualPosition)
{
	Nxna::Vector2 result = {
		(virtualPosition.X + VirtualWidth * 0.5f) * m_data->Scaling,
		(virtualPosition.Y + VirtualHeight * 0.5f) * m_data->Scaling
	};

	return result;
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
			#ifdef _MSC_VER
			_itoa_s(resolution, buffer, 10);
			#else
			snprintf(buffer, 10, "%d", resolution);
			#endif

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