#include "GeoLoader.h"
#include "FileSystem.h"
#include "tiny_obj_loader.h"

bool LoadModelObj(const char* path, ModelGeometry* geometry)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	char* pmatlpath = nullptr;
	char matlPath[256];
	strncpy_s(matlPath, path, 256);
	matlPath[255] = 0;
	auto lastSlash = strrchr(matlPath, '/');
	if (lastSlash != nullptr)
	{
		lastSlash[1] = 0;
		pmatlpath = matlPath;
	}

	std::string err;
	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path, pmatlpath) == false)
		return false;

	if (err.empty() == false)
		return false;

	std::vector<float> positions;
	std::vector<float> texcoords;
	std::vector<uint32> indices;

	geometry->Meshes = HeapArray<ModelGeometryMesh>::Init(shapes.size());

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++)
	{
		ModelGeometryMesh m;
		m.StartIndex = positions.size() / 3;
		m.TextureIndex = 0;
		m.NumTriangles = 0;

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			int fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 3)
				return false;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++)
			{
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				float vx = attrib.vertices[3 * idx.vertex_index + 0];
				float vy = attrib.vertices[3 * idx.vertex_index + 1];
				float vz = attrib.vertices[3 * idx.vertex_index + 2];
				/*
				float nx = attrib.normals[3 * idx.normal_index + 0];
				float ny = attrib.normals[3 * idx.normal_index + 1];
				float nz = attrib.normals[3 * idx.normal_index + 2];
				*/

				float tx, ty;
				if (attrib.texcoords.empty())
				{
					tx = 0;
					ty = 0;
				}
				else
				{
					tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					ty = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
				}

				auto index = positions.size() / 3;
				indices.push_back(index);

				positions.push_back(vx);
				positions.push_back(vy);
				positions.push_back(vz);
				texcoords.push_back(tx);
				texcoords.push_back(ty);

				
			}
			m.NumTriangles++;

			index_offset += fv;
		}

		geometry->Meshes.A[s] = m;
	}

	geometry->Indices = HeapArray<uint32>::Init(indices.size());
	geometry->Positions = HeapArray<float>::Init(positions.size());
	geometry->TextureCoords = HeapArray<float>::Init(positions.size());

	memcpy(geometry->Indices.A, indices.data(), sizeof(uint32) * indices.size());
	memcpy(geometry->Positions.A, positions.data(), sizeof(float) * positions.size());
	memcpy(geometry->TextureCoords.A, texcoords.data(), sizeof(float) * texcoords.size());

	return true;
}

bool LoadModelGeo(const char* path, ModelGeometry* geometry)
{
	File f;
	if (FileSystem::Open(path, &f) == false)
		return false;

	uint8* memory = (uint8*)FileSystem::MapFile(&f);
	if (memory == nullptr)
		return false;

	struct GeoHeader
	{
		uint32 Magic;
		uint32 Version;

		uint32 NumMeshes;
		uint32 NumMaterials;

		uint32 NumVertexElements;
		uint32 VertexStride;
		uint32 NumVertices;
		uint32 NumIndices;
		uint32 IndexSize; // (2 = 16 bit, 4 = 32 - bit)

		uint32 Unused;
	};
	static_assert(sizeof(GeoHeader) == 40, "Header is unexpected size");

	struct GeoVertexElement
	{
		uint16 Offset;
		uint16 Format;
		uint16 Usage;
		uint8 UsageIndex;
		uint8 InputSlot;
	};
	static_assert(sizeof(GeoVertexElement) == 8, "GeoVertexElement is unexpected size");

	struct GeoMaterial
	{
		char DiffuseTexture[260];
		char NormalTexture[260];
		char SpecularTexture[260];

		float Diffuse[4];
		float Ambient[4];
		float Specular[4];
		float Emissive[4];
		float Power;
	};
	static_assert(sizeof(GeoMaterial) == 848, "Material is unexpected size");

	struct GeoMesh
	{
		uint32 MaterialIndex;
		uint32 NumIndices;
		uint32 NumVertices;
		uint32 IndexStart;
		uint32 VertexStart;

		uint32 Unused;
	};
	static_assert(sizeof(GeoMesh) == 24, "GeoMesh is unexpected size");

	struct Vertex
	{
		float X, Y, Z;
		float TU, TV;
		float LU, LV;
	};
	static_assert(sizeof(Vertex) == 28, "Vertex is unexpected size");

	uint8* cursor = memory;
	uint8* end = memory + f.FileSize;

	GeoHeader* h = (GeoHeader*)cursor;
	if (h->Magic != 0x5b198900 || h->Version != 1)
	{
		FileSystem::Close(&f);
		return false;
	}
	cursor += sizeof(GeoHeader);
	if (cursor > end) return false;

	GeoVertexElement* elements = (GeoVertexElement*)cursor;
	cursor += sizeof(GeoVertexElement) * h->NumVertexElements;
	if (cursor > end) return false;

	GeoMaterial* materials = (GeoMaterial*)cursor;
	cursor += sizeof(GeoMaterial) * h->NumMaterials;
	if (cursor > end) return false;

	GeoMesh* meshes = (GeoMesh*)cursor;
	cursor += sizeof(GeoMesh) * h->NumMeshes;
	if (cursor > end) return false;

	void* vertices = cursor;
	cursor += h->NumVertices * h->VertexStride;
	if (cursor > end) return false;

	// padding?
	auto padding = (8 - ((cursor - memory) % 8)) % 8;
	cursor += padding;

	void* indices = cursor;
	cursor += h->NumIndices * h->IndexSize;
	if (cursor > end) return false;

	if (geometry != nullptr)
	{
		geometry->Indices = HeapArray<uint32>::Init(h->NumIndices);
		geometry->Positions = HeapArray<float>::Init(h->NumVertices * 3);
		geometry->TextureCoords = HeapArray<float>::Init(h->NumVertices * 2);
		geometry->Meshes = HeapArray<ModelGeometryMesh>::Init(h->NumMeshes);

		for (uint32 i = 0; i < h->NumIndices; i++)
		{
			if (h->IndexSize == 2)
				geometry->Indices.A[i] = ((uint16*)indices)[i];
			else
				geometry->Indices.A[i] = ((uint32*)indices)[i];
		}

		for (uint32 i = 0; i < h->NumVertices; i++)
		{
			// TODO: this assumes the vertices will be layed out like (X,Y,Z) (NX,NY,NZ) (U,V)
			geometry->Positions.A[i * 3 + 0] = ((float*)((uint8*)vertices + i * h->VertexStride))[0];
			geometry->Positions.A[i * 3 + 1] = ((float*)((uint8*)vertices + i * h->VertexStride))[1];
			geometry->Positions.A[i * 3 + 2] = ((float*)((uint8*)vertices + i * h->VertexStride))[2];

			geometry->TextureCoords.A[i * 2 + 0] = ((float*)((uint8*)vertices + i * h->VertexStride))[6];
			geometry->TextureCoords.A[i * 2 + 1] = ((float*)((uint8*)vertices + i * h->VertexStride))[7];
		}
	}

	for (unsigned int i = 0; i < h->NumMeshes; i++)
	{
		auto src = &meshes[i];

		if (geometry != nullptr)
		{
			geometry->Meshes.A[i].NumTriangles = src->NumIndices / 3;
			geometry->Meshes.A[i].StartIndex = src->IndexStart;
			geometry->Meshes.A[i].TextureIndex = src->MaterialIndex;
		}
	}

	FileSystem::Close(&f);

	return true;
}

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"