#ifndef GRAPHICS_TEXTURELOADER_H
#define GRAPHICS_TEXTURELOADER_H

#include "../MyNxna2.h"
#include "../Content/ContentManager.h"
#include "stb_image.h"

namespace Graphics
{
	class TextureLoader
	{
	public:
		static int Load(Content::ContentManager* content, const char* filename, Nxna::Graphics::Texture2D* destination, void* device)
		{
			Nxna::Graphics::GraphicsDevice* gd = (Nxna::Graphics::GraphicsDevice*)device;

			int w, h, d;
			auto img = stbi_load(filename, &w, &h, &d, 4);
			if (img == nullptr)
			{
				return -1;
			}

			Nxna::Graphics::TextureCreationDesc desc = {};
			desc.Width = w;
			desc.Height = h;
			desc.ArraySize = 1;
			Nxna::Graphics::SubresourceData srd = {};
			srd.Data = img;
			srd.DataPitch = w * 4;
			if (gd->CreateTexture2D(&desc, &srd, destination) != Nxna::NxnaResult::Success)
			{
				stbi_image_free(img);
				return -1;
			}

			return 0;
		}
	};
}

#endif // GRAPHICS_TEXTURELOADER_H