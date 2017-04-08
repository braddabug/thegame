#ifndef GRAPHICS_MODEL_H
#define GRAPHICS_MODEL_H

#include "../Common.h"
#include "../MyNxna2.h"

namespace Graphics
{
	struct ModelMesh
	{	
		uint32 NumTriangles;
		uint32 FirstIndex;
		uint32 TextureIndex;
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
		Nxna::Graphics::ShaderPipeline Pipeline;
		Nxna::Graphics::ConstantBuffer Constants;

		static bool LoadObj(Nxna::Graphics::GraphicsDevice* device, uint8* data, uint32 length, Model* result);
		static void Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* modelview, Model* models, uint32 numModels);
	};
}

#endif // GRAPHICS_MODEL_H

