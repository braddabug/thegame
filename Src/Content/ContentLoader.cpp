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
		m_data->Loaders.push_back(Loader{ LoaderType::ModelObj, (JobFunc)Graphics::Model::LoadObj, (JobFunc)Graphics::Model::FinalizeLoadObj, device });
		m_data->Loaders.push_back(Loader{ LoaderType::Texture2D, (JobFunc)Graphics::TextureLoader::LoadPixels, (JobFunc)Graphics::TextureLoader::ConvertPixelsToTexture, nullptr });
		m_data->Loaders.push_back(Loader{ LoaderType::Audio, (JobFunc)Audio::AudioLoader::LoadWav, nullptr, nullptr });
	}

	void ContentLoader::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	Loader* ContentLoader::findLoader(LoaderType type)
	{
		for (size_t i = 0; i < m_data->Loaders.size(); i++)
		{
			if (m_data->Loaders[i].Type == type)
				return &m_data->Loaders[i];
		}

		return nullptr;
	}
}
