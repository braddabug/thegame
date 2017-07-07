#ifndef GEOLOADER_H
#define GEOLOADER_H

#include "Common.h"

struct ModelGeometryMesh
{
	uint32 StartIndex;
	uint32 NumTriangles;
	uint32 TextureIndex;
};

struct ModelGeometry
{
	HeapArray<float> Positions;
	HeapArray<float> TextureCoords;
	HeapArray<uint32> Indices;
	HeapArray<ModelGeometryMesh> Meshes;
};

bool LoadModelObj(const char* path, ModelGeometry* geometry);
bool LoadModelGeo(const char* path, ModelGeometry* geometry);

#endif // GEOLOADER_H

