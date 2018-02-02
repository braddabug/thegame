#ifndef GRAPHICS_SHADERLIBRARY_H
#define GRAPHICS_SHADERLIBRARY_H

#include "../MyNxna2.h"
#include "../Common.h"

namespace Graphics
{
	struct ShaderLibraryData;

	enum class ShaderType
	{
		BasicWhite,         // position-only, no texture, just white
		BasicTextured,      // position and tex coords, 1 texture

		LAST
	};

	enum class BlendType
	{
		Opaque,
		Transparent,

		LAST
	};

	class ShaderLibrary
	{
		static ShaderLibraryData* m_data;

	public:
		static void SetGlobalData(ShaderLibraryData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static bool LoadCoreShaders();

		static Nxna::Graphics::ShaderPipeline* GetShader(ShaderType type);
		static Nxna::Graphics::BlendState* GetBlending(BlendType type);

	private:
		static bool createShader(const uint8* vertexBytecode, size_t sizeOfVertexBytecode, const uint8* pixelBytecode, size_t sizeOfPixelBytecode, Nxna::Graphics::InputElement* elements, int numElements, Nxna::Graphics::ShaderPipeline* result);
	};
}

#endif // GRAPHICS_SHADERLIBRARY_H
