#include "ContentLoader.h"
#include "../MemoryManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Model.h"
#include "../Audio/AudioLoader.h"

namespace Content
{
	struct ContentLoaderData
	{
		std::vector<Loader> Loaders;
	};

	ContentLoaderData* ContentLoader::m_data = nullptr;

	void ContentLoader::SetGlobalData(ContentLoaderData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
			*data = NewObject<ContentLoaderData>(__FILE__, __LINE__);

		m_data = *data;

		// function pointers within the lib have to be reset
		m_data->Loaders.clear();
		m_data->Loaders.push_back(Loader{ ResourceType::Model, (JobFunc)Graphics::Model::LoadObj, (JobFunc)Graphics::Model::FinalizeLoadObj, nullptr, nullptr, device, sizeof(Graphics::Model), alignof(Graphics::Model) });
		m_data->Loaders.push_back(Loader{ ResourceType::Texture2D, (JobFunc)Graphics::TextureLoader::LoadPixels, (JobFunc)Graphics::TextureLoader::ConvertPixelsToTexture, nullptr });
		m_data->Loaders.push_back(Loader{ ResourceType::Bitmap, (JobFunc)Graphics::TextureLoader::LoadPixels, nullptr, nullptr });
		m_data->Loaders.push_back(Loader{ ResourceType::Audio, (JobFunc)Audio::AudioLoader::LoadWav, nullptr, nullptr });
	}

	void ContentLoader::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	ResourceType ContentLoader::GetLoaderResourceType(LoaderType type)
	{
#ifdef _MSC_VER
// make sure all LoaderTypes are explicitly handled
#pragma warning(push, 4)
#pragma warning(error: 4061 4062)
#endif
		switch (type)
		{
		case LoaderType::ModelObj: return ResourceType::Model;
		case LoaderType::Texture2D: return ResourceType::Texture2D;
		case LoaderType::Audio: return ResourceType::Audio;
		case LoaderType::LAST: return ResourceType::LAST; // this shouldn't happen!
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		// keep the compiler happy
		return ResourceType::LAST;
	}

	LoaderType ContentLoader::GetLoaderByNameHash(uint32 hash)
	{
#undef DEFINE_LOADER_TYPE
#define DEFINE_LOADER_TYPE(t) Utils::CalcHash(#t),
		uint32 hashes[] = {
			DEFINE_LOADER_TYPES
		};
#undef DEFINE_LOADER_TYPE

		for (int i = 0; i < (int)LoaderType::LAST; i++)
		{
			if (hashes[i] == hash)
				return (LoaderType)i;
		}

		return LoaderType::LAST;
	}

	Loader* ContentLoader::findLoader(ResourceType type)
	{
		for (size_t i = 0; i < m_data->Loaders.size(); i++)
		{
			if (m_data->Loaders[i].Type == type)
				return &m_data->Loaders[i];
		}

		return nullptr;
	}
}
