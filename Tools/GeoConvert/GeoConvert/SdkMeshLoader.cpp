#include "SdkMeshLoader.h"
#include "Geometry.h"
#include <stdint.h>
#include <cstdio>
#include <memory>
#include <vector>

namespace SdkMeshLoader
{
	typedef unsigned char uint8;
	typedef short int16;
	typedef unsigned short uint16;
	typedef int int32;
	typedef unsigned int uint32;
	typedef uint64_t uint64;

	int Load(const char* name, Geo::Info* result)
	{
#pragma pack(push,1)
		struct SdkMeshHeader
		{
			//Basic Info and sizes
			uint32 Version;
			uint32 IsBigEndian;
			uint64 HeaderSize;
			uint64 NonBufferDataSize;
			uint64 BufferDataSize;

			//Stats
			uint32 NumVertexBuffers;
			uint32 NumIndexBuffers;
			uint32 NumMeshes;
			uint32 NumTotalSubsets;
			uint32 NumFrames;
			uint32 NumMaterials;

			//Offsets to Data
			uint64 VertexStreamHeadersOffset;
			uint64 IndexStreamHeadersOffset;
			uint64 MeshDataOffset;
			uint64 SubsetDataOffset;
			uint64 FrameDataOffset;
			uint64 MaterialDataOffset;
		};

		struct VertexElement
		{
			uint16 Stream;
			uint16 Offset;
			uint8 Type;
			uint8 Method;
			uint8 Usage;
			uint8 UsgaeIndex;
		};

		struct SdkMeshVertexBufferHeader
		{
			uint64 NumVertices;
			uint64 SizeBytes;
			uint64 StrideBytes;
			VertexElement Decl[32];
			uint64 DataOffset;
		};

		struct SdkMeshIndexBufferHeader
		{
			uint64 NumIndices;
			uint64 SizeBytes;
			uint32 IndexType;
			uint32 Unused;
			uint64 DataOffset;
		};

		struct SdkMeshMesh
		{
			char Name[100];
			uint32 NumVertexBuffers;
			uint32 VertexBuffers[16];
			uint32 IndexBuffer;
			uint32 NumSubsets;
			uint32 NumFrameInfluences; //aka bones

			float BoundingBoxCenter[3];
			float BoundingBoxExtents[3];

			uint32 Unused;

			union
			{
				uint64 SubsetOffset;	//Offset to list of subsets (This also forces the union to 64bits)
				uint32*  pSubsets;	    //Pointer to list of subsets
			};

			union
			{
				uint64 FrameInfluenceOffset;  //Offset to list of frame influences (This also forces the union to 64bits)
				uint32*  pFrameInfluences;      //Pointer to list of frame influences
			};
		};

		struct SdkMeshSubset
		{
			char Name[100];
			uint32 MaterialID;
			uint32 PrimitiveType;
			uint32 Unused;
			uint64 IndexStart;
			uint64 IndexCount;
			uint64 VertexStart;
			uint64 VertexCount;
		};

		struct SdkMeshFrame
		{
			char Name[100];
			uint32 Mesh;
			uint32 ParentFrame;
			uint32 ChildFrame;
			uint32 SiblingFrame;
			float Matrix[16];
			uint32 AnimationDataIndex;		//Used to index which set of keyframes transforms this frame
		};

		struct SdkMeshMaterial
		{
			char Name[100];

			// Use MaterialInstancePath
			char MaterialInstancePath[260];

			// Or fall back to d3d8-type materials
			char DiffuseTexture[260];
			char NormalTexture[260];
			char SpecularTexture[260];

			float Diffuse[4];
			float Ambient[4];
			float Specular[4];
			float Emissive[4];
			float Power;

			uint64 Unused[6];
		};

#pragma pack(pop)

		static_assert(sizeof(VertexElement) == 8, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshHeader) == 104, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshVertexBufferHeader) == 288, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshIndexBufferHeader) == 32, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshMesh) == 224, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshSubset) == 144, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshFrame) == 184, "SDK Mesh structure size incorrect");
		static_assert(sizeof(SdkMeshMaterial) == 1256, "SDK Mesh structure size incorrect");

		FILE* fp;
		if (fopen_s(&fp, name, "rb") != 0)
		{
			return -1;
		}

		SdkMeshHeader header;
		fread(&header, sizeof(SdkMeshHeader), 1, fp);

		// validate
		if (header.NumVertexBuffers != 1 || header.NumMeshes != 1)
		{
			fclose(fp);
			return -1;
		}

		SdkMeshVertexBufferHeader vbHeader;
		fread(&vbHeader, sizeof(SdkMeshVertexBufferHeader), 1, fp);

		memset(result, 0, sizeof(Geo::Info));

		// count the # of vertex elements
		Geo::InputElement* elements = new Geo::InputElement[32];
		uint32 numElements = 0;
		for (int i = 0; i < 32; i++)
		{
			if (vbHeader.Decl[i].Usage == 0xFF ||
				vbHeader.Decl[i].Type == 17 /* D3DDECLTYPE_UNUSED */)
				break;

			elements[i].Offset = vbHeader.Decl[i].Offset;
			elements[i].InputSlot = vbHeader.Decl[i].Stream;
			elements[i].UsageIndex = vbHeader.Decl[i].UsgaeIndex;

			switch (vbHeader.Decl[i].Type)
			{
			case 0:
				elements[i].ElementFormat = Geo::VertexElementFormat::Single;
				break;
			case 1:
				elements[i].ElementFormat = Geo::VertexElementFormat::Vector2;
				break;
			case 2:
				elements[i].ElementFormat = Geo::VertexElementFormat::Vector3;
				break;
			case 3:
				elements[i].ElementFormat = Geo::VertexElementFormat::Vector4;
				break;
			default:
				return -1; // not supported
			}

			switch (vbHeader.Decl[i].Usage)
			{
			case 0:
				elements[i].ElementUsage = Geo::VertexElementUsage::Position;
				break;
			case 3:
				elements[i].ElementUsage = Geo::VertexElementUsage::Normal;
				break;
			case 5:
				elements[i].ElementUsage = Geo::VertexElementUsage::TextureCoordinate;
				break;
			default:
				return -1; // not supported
			}

			numElements++;
		}

		result->VertexElements = elements;
		result->NumVertexElements = numElements;
		
		result->NumVertices = vbHeader.NumVertices;
		result->VertexStride = vbHeader.StrideBytes;
		

		SdkMeshIndexBufferHeader ibHeader;
		fread(&ibHeader, sizeof(SdkMeshIndexBufferHeader), 1, fp);

		if (ibHeader.IndexType == 0)
			result->IndexSize = 2;
		else if (ibHeader.IndexType == 1)
			result->IndexSize = 2;
		else
			return -1;

		result->NumIndices = ibHeader.NumIndices;


		// load the mesh
		SdkMeshMesh mesh;
		fread(&mesh, sizeof(SdkMeshMesh), 1, fp);

		if (mesh.IndexBuffer != 0 ||
			mesh.NumVertexBuffers != 1 ||
			mesh.VertexBuffers[0] != 0)
			return -1;

		result->NumMeshes = mesh.NumSubsets;

		// load the subsets
		result->Meshes = new Geo::InputMesh[mesh.NumSubsets];
		for (int i = 0; i < mesh.NumSubsets; i++)
		{
			SdkMeshSubset subset;
			fread(&subset, sizeof(SdkMeshSubset), 1, fp);

			Geo::InputMesh* m = &result->Meshes[i];
			m->MaterialIndex = subset.MaterialID;
			m->NumIndices = subset.IndexCount;
			m->NumVertices = subset.VertexCount;
			m->StartIndex = subset.IndexStart;
			m->StartVertex = subset.VertexStart;
		}

		// skip the frames
		for (int i = 0; i < header.NumFrames; i++)
		{
			SdkMeshFrame frame;
			fread(&frame, sizeof(SdkMeshFrame), 1, fp);
		}

		// read the materials
		result->NumTextures = header.NumMaterials;
		result->DiffuseTextureNames = new char*[header.NumMaterials];
		for (int i = 0; i < header.NumMaterials; i++)
		{
			SdkMeshMaterial mat;
			fread(&mat, sizeof(SdkMeshMaterial), 1, fp);

			result->DiffuseTextureNames[i] = new char[260];
			strcpy_s(result->DiffuseTextureNames[i], 260, mat.DiffuseTexture);
		}

		// TODO: load the vertice
		fseek(fp, vbHeader.DataOffset, SEEK_SET);
		uint8* vbData = new uint8[vbHeader.SizeBytes];
		fread(vbData, 1, vbHeader.SizeBytes, fp);

		if (vbHeader.SizeBytes != vbHeader.StrideBytes * vbHeader.NumVertices)
			return -1;

		result->Vertices = vbData;

		// TODO: load the indices
		fseek(fp, ibHeader.DataOffset, SEEK_SET);
		uint8* ibData = new uint8[ibHeader.SizeBytes];
		fread(ibData, 1, ibHeader.SizeBytes, fp);

		if (ibHeader.SizeBytes != ibHeader.NumIndices * result->IndexSize)
			return -1;

		result->Indices = ibData;

		fclose(fp);

		return 0;
	}
}