#include "ContentManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Model.h"

namespace Content
{
	ContentManagerData* ContentManager::m_data = nullptr;

	void ContentManager::SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
			*data = new ContentManagerData();

		m_data = *data;
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
}
