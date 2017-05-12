#ifndef GEOMETRY_H
#define GEOMETRY_H

namespace Geo
{

	struct InputMesh
	{
		unsigned int MaterialIndex;

		unsigned int StartIndex;
		unsigned int NumIndices;

		unsigned int StartVertex;
		unsigned int NumVertices;
	};

	enum class VertexElementFormat
	{
		/// A single 32-bit floating point number
		Single = 1,

		/// Two 32-bit floating point numbers
		Vector2,

		/// Three 32-bit floating point numbers
		Vector3,

		/// Four 32-bit floating point numbers
		Vector4,

		/// Four bytes of color info, packed as RGBA
		Color
	};

	enum class VertexElementUsage
	{
		Position = 0,
		Normal = 1,
		TextureCoordinate = 2,
		Color = 3
	};

	struct InputElement
	{
		unsigned int Offset;
		VertexElementFormat ElementFormat;
		VertexElementUsage ElementUsage;
		unsigned int UsageIndex;
		unsigned int InputSlot;
	};

	struct Info
	{
		void* Vertices;
		unsigned int NumVertices;
		unsigned int VertexStride;

		void* Indices;
		unsigned int NumIndices;
		unsigned int IndexSize; // 2 = 16-bit, 4 = 32-bit

		char** DiffuseTextureNames;
		unsigned int NumTextures;

		InputMesh* Meshes;
		unsigned int NumMeshes;

		InputElement* VertexElements;
		unsigned int NumVertexElements;
		
	};

	bool Write(const char* path, Info* info);
	int Merge(const Geo::Info* texture1Mesh, const Geo::Info* texture2Mesh, Geo::Info* result);

#if 0
	struct Vertex
	{
		float X, Y, Z;
		float TU, TV;
		float LU, LV;
	};

	struct Mesh
	{
		unsigned int TextureIndex;

		unsigned int NumVertices;
		Vertex* Vertices;

		unsigned int NumIndices;
		unsigned short* Indices;
	};

	bool Write(const char* path, Mesh* meshes, unsigned int numMeshes, const char** textureNames, unsigned int numTextures);
#endif
}

#endif // GEOMETRY_H
