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
		uint32 TextureIndex;
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
		Nxna::Graphics::Texture2D* Textures;

		uint32 VertexStride;
		Nxna::Graphics::VertexBuffer Vertices;
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
	};
}

#endif // GRAPHICS_MODEL_H

