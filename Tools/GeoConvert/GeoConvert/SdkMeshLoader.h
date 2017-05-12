#ifndef SDKMESHLOADER_H
#define SDKMESHLOADER_H

namespace Geo
{
	struct Info;
}

namespace SdkMeshLoader
{
	int Load(const char* name, Geo::Info* result);
}

#endif // SDKMESHLOADER_H
