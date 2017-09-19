#ifndef GRAPHICS_TEXTURELOADER_H
#define GRAPHICS_TEXTURELOADER_H

#include "../MyNxna2.h"
#include "../Content/ContentManager.h"
#include "stb_image.h"

namespace Graphics
{
	struct Bitmap;

	struct TextureLoaderData
	{
		Nxna::Graphics::GraphicsDevice* Device;
		Nxna::Graphics::Texture2D ErrorTexture;
		bool Initialized;
	};

	class TextureLoader
	{
		static TextureLoaderData* m_data;

	public:

		static void SetGlobalData(TextureLoaderData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static Nxna::Graphics::Texture2D GetErrorTexture(bool needOwnership);

		static bool LoadPixels(Content::ContentLoaderParams* params);
		static bool ConvertPixelsToTexture(Content::ContentLoaderParams* params);

		static bool ConvertBitmapToTexture(Bitmap* bitmap, Nxna::Graphics::Texture2D* result);

	private:

		static bool generateErrorTexture(Nxna::Graphics::Texture2D* result);
	};
}

#endif // GRAPHICS_TEXTURELOADER_H