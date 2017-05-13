#include "TextureLoader.h"

namespace Graphics
{
	TextureLoaderData* TextureLoader::m_data;

	void TextureLoader::SetGlobalData(TextureLoaderData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = new TextureLoaderData();
			(*data)->Device = device;
			(*data)->Initialized = false;
		}

		m_data = *data;
	}

	Nxna::Graphics::Texture2D TextureLoader::GetErrorTexture(bool needOwnership)
	{
		if (needOwnership)
		{
			// generate a brand new error texture since the caller is taking responsibility of destroying the texture when they're done
			Nxna::Graphics::Texture2D result;
			generateErrorTexture(&result);
			return result;
		}
		else
		{
			// return a cached version of the texture
			if (m_data->Initialized == false)
			{
				generateErrorTexture(&m_data->ErrorTexture);
				m_data->Initialized = true;
			}

			return m_data->ErrorTexture;
		}
	}

	int TextureLoader::Load(Content::ContentManager* content, const char* filename, Nxna::Graphics::Texture2D* destination, void* data)
	{
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
		if (m_data->Device->CreateTexture2D(&desc, &srd, destination) != Nxna::NxnaResult::Success)
		{
			stbi_image_free(img);
			return -1;
		}

		return 0;
	}

	bool TextureLoader::generateErrorTexture(Nxna::Graphics::Texture2D* result)
	{
		uint8 pixels[] = {
			255, 153, 00, 255,
			255, 204, 00, 255,
			255, 153, 00, 255,
			255, 204, 00, 255
		};

		Nxna::Graphics::TextureCreationDesc desc = {};
		desc.Width = 2;
		desc.Height = 2;
		desc.ArraySize = 1;
		Nxna::Graphics::SubresourceData srd = {};
		srd.Data = pixels;
		srd.DataPitch = 2 * 4;
		Nxna::Graphics::Texture2D errorTex;
		return m_data->Device->CreateTexture2D(&desc, &srd, result) == Nxna::NxnaResult::Success;
	}
}