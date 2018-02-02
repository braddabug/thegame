#include "TextureLoader.h"
#include "Bitmap.h"
#include "../MemoryManager.h"
#include "../StringManager.h"

namespace Graphics
{
	TextureLoaderData* TextureLoader::m_data;

	void TextureLoader::SetGlobalData(TextureLoaderData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = NewObject<TextureLoaderData>(__FILE__, __LINE__);
			(*data)->Device = device;
			(*data)->Initialized = false;
		}

		m_data = *data;
	}

	void TextureLoader::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
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

	bool TextureLoader::LoadPixels(Content::ContentLoaderParams* params)
	{
		static_assert(sizeof(Bitmap) <= Content::ContentLoaderParams::LocalDataStorageSize, "Bitmap is too big");

		if (params->Phase == Content::LoaderPhase::AsyncLoad)
		{
			int w, h, d;
			auto filename = HashStringManager::Get(params->FilenameHash, HashStringManager::HashStringType::File);
			if (filename == nullptr)
			{
				LOG_ERROR("Unable to get filename for texture with hash %u", params->FilenameHash);

				params->State = Content::ContentState::NotFound;
				return false;
			}

			FoundFile f;
			if (FileFinder::OpenAndMap(filename, &f) == false)
			{
				LOG_ERROR("Unable to open texture %s", filename);

				params->State = Content::ContentState::NotFound;
				return false;
			}

			auto img = stbi_load_from_memory((unsigned char*)f.Memory, f.FileSize, &w, &h, &d, 4);

			FileFinder::Close(&f);

			if (img == nullptr)
			{
				LOG_ERROR("Unable to load texture %s", filename);

				params->State = Content::ContentState::NotFound;
				return false;
			}

			Bitmap* storage = (Bitmap*)params->LocalDataStorage;

			storage->Width = (uint32)w;
			storage->Height = (uint32)h;
			storage->Pixels = img;

			return true;
		}
		else if (params->Phase == Content::LoaderPhase::MainThread)
		{
			if (params->Type == Content::ResourceType::Bitmap)
			{
				memcpy(params->Destination, params->LocalDataStorage, sizeof(Bitmap));
				params->State = Content::ContentState::Loaded;
				return true;
			}
			else
			{
				Bitmap* storage = (Bitmap*)params->LocalDataStorage;
				Nxna::Graphics::Texture2D* destination = (Nxna::Graphics::Texture2D*)params->Destination;

				if (ConvertBitmapToTexture(storage, destination))
				{
					stbi_image_free(storage->Pixels);
					params->State = Content::ContentState::Loaded;
					return true;
				}

				stbi_image_free(storage->Pixels);
				params->State = Content::ContentState::UnknownError;
				return false;
			}
		}

		return true;
	}

	bool TextureLoader::ConvertBitmapToTexture(Bitmap* bitmap, Nxna::Graphics::Texture2D* result)
	{
		Nxna::Graphics::TextureCreationDesc desc = {};
		desc.Width = bitmap->Width;
		desc.Height = bitmap->Height;
		desc.ArraySize = 1;
		Nxna::Graphics::SubresourceData srd = {};
		srd.Data = bitmap->Pixels;
		srd.DataPitch = bitmap->Width * 4;
		if (m_data->Device->CreateTexture2D(&desc, &srd, result) != Nxna::NxnaResult::Success)
		{
			return false;
		}

		return true;
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
		return m_data->Device->CreateTexture2D(&desc, &srd, result) == Nxna::NxnaResult::Success;
	}
}
