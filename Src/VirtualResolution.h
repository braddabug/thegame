#ifndef VIRTUALRESOLUTION_H
#define VIRTUALRESOLUTION_H

#include "Common.h"

struct VirtualResolutionData;

#define DEFINE_RESOLUTION(r) R ## r = r,
#define DEFINE_RESOLUTIONS \
	DEFINE_RESOLUTION(1536) \
	DEFINE_RESOLUTION(768) \
	DEFINE_RESOLUTION(640)
	

enum class Resolution
{
	DEFINE_RESOLUTIONS
};

class VirtualResolution
{
	static VirtualResolutionData* m_data;

public:
	static void SetGlobalData(VirtualResolutionData** data);
	static void Shutdown();

	static void Init(int screenHeight);

	static bool InjectNearestResolution(const char* source, char* destination, uint32 destinationLength);
};

#endif // VIRTUALRESOLUTION_H
