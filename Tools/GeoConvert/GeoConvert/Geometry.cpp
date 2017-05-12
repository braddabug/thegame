#include "Geometry.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef uint64_t uint64;

namespace Geo
{
	void UpdateOffset(FILE* fp, unsigned int targetOffset)
	{
		unsigned int pos = ftell(fp);
		fseek(fp, targetOffset, SEEK_SET);
		fwrite(&pos, 4, 1, fp);
		fseek(fp, pos, SEEK_SET);
	}

	bool Write(const char* path, Info* info)
	{
		if (path == nullptr || info == nullptr)
			return false;
		if (info->NumVertexElements > 16)
			return false;

		struct Header
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

		struct VertexElement
		{
			uint16 Offset;
			uint16 Format;
			uint16 Usage;
			uint8 UsageIndex;
			uint8 InputSlot;
		};

		struct Material
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

		struct Mesh
		{
			uint32 MaterialIndex;
			uint32 NumIndices;
			uint32 NumVertices;
			uint32 IndexStart;
			uint32 VertexStart;

			uint32 Unused;
		};

		static_assert(sizeof(Header) == 40, "Header is unexpected size");
		static_assert(sizeof(VertexElement) == 8, "VertexElement is unexpected size");
		static_assert(sizeof(Material) == 848, "Material is unexpected size");
		static_assert(sizeof(Mesh) == 24, "Mesh is unexpected size");

#ifdef _WIN32
		FILE* fp;
		if (fopen_s(&fp, path, "wb") != 0)
			return false;
#endif

		Header h = {};
		h.Magic = 0x5b198900; // just random bytes
		h.Version = 1;
		h.NumMeshes = info->NumMeshes;
		h.NumMaterials = info->NumTextures;
		h.NumVertexElements = info->NumVertexElements;
		h.VertexStride = info->VertexStride;
		h.NumVertices = info->NumVertices;
		h.NumIndices = info->NumIndices;
		h.IndexSize = info->IndexSize;

		fwrite(&h, sizeof(Header), 1, fp);

		// vertex elements
		VertexElement elements[16] = {};
		for (int i = 0; i < info->NumVertexElements; i++)
		{
			elements[i].Offset = info->VertexElements[i].Offset;
			elements[i].Format = (uint16)info->VertexElements[i].ElementFormat;
			elements[i].Usage = (uint16)info->VertexElements[i].ElementUsage;
			elements[i].UsageIndex = info->VertexElements[i].UsageIndex;
			elements[i].InputSlot = info->VertexElements[i].InputSlot;
		}
		fwrite(elements, sizeof(VertexElement), info->NumVertexElements, fp);

		// materials
		for (int i = 0; i < info->NumTextures; i++)
		{
			Material m = {};
			strncpy_s(m.DiffuseTexture, info->DiffuseTextureNames[i], 260);
			m.Diffuse[0] = 1.0f;
			m.Diffuse[1] = 1.0f;
			m.Diffuse[2] = 1.0f;
			m.Diffuse[3] = 1.0f;

			fwrite(&m, sizeof(Material), 1, fp);
		}

		// meshes
		for (int i = 0; i < info->NumMeshes; i++)
		{
			Mesh m = {};
			m.MaterialIndex = info->Meshes[i].MaterialIndex;
			m.NumIndices = info->Meshes[i].NumIndices;
			m.NumVertices = info->Meshes[i].NumVertices;
			m.IndexStart = info->Meshes[i].StartIndex;
			m.VertexStart = info->Meshes[i].StartVertex;

			fwrite(&m, sizeof(Mesh), 1, fp);
		}

		// vertices
		fwrite(info->Vertices, 1, info->NumVertices * info->VertexStride, fp);

		// insert padding if necessary
		auto pos = ftell(fp);
		auto padding = (8 - (pos % 8)) % 8;
		if (padding > 0)
		{
			uint64 dummy = 0;
			fwrite(&dummy, 1, padding, fp);
		}

		// indices
		fwrite(info->Indices, 1, info->NumIndices * info->IndexSize, fp);

		fclose(fp);

		return true;
	}

	int Merge(const Geo::Info* texture1Mesh, const Geo::Info* texture2Mesh, Geo::Info* result)
	{
		if (texture1Mesh->NumIndices != texture2Mesh->NumIndices ||
			texture1Mesh->NumVertices != texture2Mesh->NumVertices)
			return -1;

		return -1;
	}

#if 0
	bool Write(const char* path, Mesh* meshes, unsigned int numMeshes, const char** textureNames, unsigned int numTextures)
	{
#ifdef _WIN32
		FILE* fp;
		if (fopen_s(&fp, path, "wb") != 0)
			return false;
#endif
		unsigned int dummy = 0;

		// write the header
		struct Header
		{
			unsigned int Magic;
			unsigned int Version;
			unsigned int NumMeshes;
			unsigned int NumTextures;
			unsigned int OffsetToTextures;
			unsigned int Unused;
		} header;
		static_assert(sizeof(header) == 24, "header is unexpected size");

		header.Magic = 0x5b198900; // just random bytes
		header.Version = 1;
		header.NumMeshes = numMeshes;
		header.NumTextures = numTextures;
		header.OffsetToTextures = 0;
		header.Unused = 0;

		fwrite(&header, sizeof(header), 1, fp);

		// write the offset placeholders
		for (unsigned int i = 0; i < numMeshes; i++)
		{
			unsigned int dummy;
			fwrite(&dummy, 4, 1, fp);
		}

		// write the texture names
		UpdateOffset(fp, offsetof(Header, OffsetToTextures));
		for (unsigned int i = 0; i < numTextures; i++)
		{
			unsigned char len = (unsigned char)strlen(textureNames[i]);
			fwrite(&len, 1, 1, fp);

			fwrite(textureNames[i], len + 1, 1, fp);
		}

		// write each mesh
		for (unsigned int i = 0; i < numMeshes; i++)
		{
			// TODO: make sure we're on an 8-byte boundary

			// write the offset to the mesh
			UpdateOffset(fp, sizeof(Header) + i * 4);

			fwrite(&meshes[i].TextureIndex, 4, 1, fp);
			fwrite(&meshes[i].NumIndices, 4, 1, fp);
			fwrite(&meshes[i].NumVertices, 4, 1, fp);
			fwrite(&dummy, 4, 1, fp);
			
			// write the vertices
			fwrite(meshes[i].Vertices, sizeof(float) * 7, meshes[i].NumVertices, fp);

			// write the indices
			fwrite(meshes[i].Indices, sizeof(unsigned short), meshes[i].NumIndices, fp);
		}

		// done!
		fclose(fp);

		return true;
	}
#endif
}