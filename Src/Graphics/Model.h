#ifndef GRAPHICS_MODEL_H
#define GRAPHICS_MODEL_H

#include "../Common.h"
#include "../Content/ContentManager.h"
#include "../MyNxna2.h"

namespace Graphics
{
	struct ModelMesh
	{	
		uint32 NumTriangles;
		uint32 FirstIndex;
		uint32 DiffuseTextureIndex;
		uint32 LightmapTextureIndex;
	};

	struct ModelData
	{
		Nxna::Graphics::ConstantBuffer Constants;
		Nxna::Graphics::SamplerState SamplerState;

		bool Initialized;
	};

	struct Model
	{
		uint32 NumMeshes;
		ModelMesh* Meshes;

		uint32 NumTextures;
		static const uint32 MAX_TEXTURES = 10;
		uint32 Textures[MAX_TEXTURES];

		uint32 VertexStride;
		uint32 NumVertices;
		uint32 NumIndices;
		Nxna::Graphics::VertexBuffer Vertices;
		Nxna::Graphics::IndexBuffer Indices;
		Nxna::Graphics::RasterizerState RasterState;

		float BoundingBox[6];
		
		static ModelData* m_data;

		static void SetGlobalData(ModelData** data);
		static void Shutdown();

		static bool LoadObj(Content::ContentLoaderParams* params);
		static bool FinalizeLoadObj(Content::ContentLoaderParams* params);

		enum RenderFlags
		{
			None = 0,
			DrawBoundingBoxes = 1
		};

		static void BeginRender(Nxna::Graphics::GraphicsDevice* device);
		static void Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* transform, Model* model);

		static void UpdateAABB(float* boundingBox, Nxna::Matrix* transform, float* result);
	};
}

#endif // GRAPHICS_MODEL_H

