#include "ContentManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Model.h"

namespace Content
{
	ContentManagerData* ContentManager::m_data = nullptr;
	typedef int(*LoaderFunc)(ContentManager*, const char*, void*, void*);

	void ContentManager::SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
			*data = new ContentManagerData();

		m_data = *data;

		// function pointers within the lib have to be reset
		m_data->Loaders.clear();
		m_data->Loaders.push_back(ContentLoader { LoaderType::ModelObj, (LoaderFunc)Graphics::Model::LoadObj, device });
		m_data->Loaders.push_back(ContentLoader { LoaderType::Texture2D, (LoaderFunc)Graphics::TextureLoader::Load, device });
	}

	/*
	Content* ContentManager::Load(const char* filename, ContentType type)
	{
		Content* result = Get(filename, type);
		if (result != nullptr)
			return result;

		result = new Content();
		result->State = ContentState::Incomplete;
#ifdef _WIN32
		strncpy_s(result->Filename, filename, 256);
#else
		strncpy(result->Filename, filename, 256);
#endif
		result->Type = type;

		// add to the list of content
		m_content.push_back(result);

		auto loader = findLoader(type);
		if (loader == nullptr)
		{
			result->State = ContentState::NoLoader;
			return result;
		}

		loader->Loader(result, loader->LoaderParam);

		return result;
	}*/


	ContentLoader* ContentManager::findLoader(LoaderType type)
	{
		for (size_t i = 0; i < m_data->Loaders.size(); i++)
		{
			if (m_data->Loaders[i].Type == type)
				return &m_data->Loaders[i];
		}

		return nullptr;
	}
}
