#ifndef GAME_NAVMESH_H
#define GAME_NAVMESH_H

#include "../Common.h"

namespace Game
{
	class NavMesh
	{
		static const uint32 MaxPolys = 10;
		static const uint32 MaxEdges = 5;

		struct
		{
			struct
			{
				float X, Y, Z;
			} Verts[MaxEdges + 1];

			uint32 EdgePolyConnections[MaxEdges];

			uint32 NumEdges;
		} Polys[MaxPolys];

		uint32 NumPolys;

		struct
		{
			Nxna::Graphics::VertexBuffer VBuffer;
			Nxna::Graphics::IndexBuffer IBuffer;
			Nxna::Graphics::ConstantBuffer MatrixCBuffer;
			Nxna::Graphics::ConstantBuffer ColorCBuffer;
			uint32 NumVertices;
			uint32 NumIndices;
			bool DrawBuffersInitialized;
		} Drawing;

	public:
		static bool Load(StringRef filename, NavMesh* result);

		void Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* modelview);

	};
}

#endif // GAME_NAVMESH_H
