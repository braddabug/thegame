#include "ContentLoader.h"
#include "../MemoryManager.h"
#include "../Gui/GuiManager.h"
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
		m_data->Loaders.push_back(Loader{ ResourceType::Model, (JobFunc)Graphics::Model::LoadObj, nullptr, device, sizeof(Graphics::Model), alignof(Graphics::Model) });
		m_data->Loaders.push_back(Loader{ ResourceType::Texture2D, (JobFunc)Graphics::TextureLoader::LoadPixels, nullptr });
		m_data->Loaders.push_back(Loader{ ResourceType::Bitmap, (JobFunc)Graphics::TextureLoader::LoadPixels, nullptr });
		m_data->Loaders.push_back(Loader{ ResourceType::Audio, (JobFunc)Audio::AudioLoader::LoadWav, nullptr });
		m_data->Loaders.push_back(Loader{ ResourceType::Cursor, (JobFunc)Gui::GuiManager::LoadCursor, nullptr });
	}

	void ContentLoader::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	ResourceType ContentLoader::GetResourceTypeByFilename(const char* filename, const char* filenameEnd)
	{
		if (filename == nullptr) return ResourceType::LAST;
		if (filenameEnd == nullptr) filenameEnd = filename + strlen(filename);

		const char* ext = filenameEnd - 1;
		while (ext != filename && *ext != '.')
			ext--;

		return GetResourceTypeByFileExtension(ext, filenameEnd);
	}

	ResourceType ContentLoader::GetResourceTypeByFileExtension(const char* ext, const char* extEnd)
	{
		if (ext == nullptr || ext[0] == 0)
			return ResourceType::LAST;

		int extLen;
		if (extEnd == nullptr)
			extLen = strlen(ext);
		else
			extLen = (int)(extEnd - ext);

		const uint32 maxExtLength = 6;
		char extBuffer[maxExtLength + 1];
#ifdef _MSC_VER
		strncpy_s(extBuffer, ext, maxExtLength < extLen ? maxExtLength : extLen);
#else
		strncpy(extBuffer, ext, maxExtLength < extLen ? maxExtLength : extLen);
#endif

		if (strcmp(extBuffer, ".ttf") == 0)
			return ResourceType::Font;
		if (strcmp(extBuffer, ".obj") == 0)
			return ResourceType::Model;
		if (strcmp(extBuffer, ".bmp") == 0 ||
			strcmp(extBuffer, ".png") == 0 ||
			strcmp(extBuffer, ".tga") == 0)
			return ResourceType::Texture2D;
		if (strcmp(extBuffer, ".wav") == 0)
			return ResourceType::Audio;

		return ResourceType::LAST;
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

	ResourceType ContentLoader::GetResourceTypeByNameHash(uint32 hash)
	{
#undef DEFINE_RESOURCE_TYPE
#define DEFINE_RESOURCE_TYPE(t) Utils::CalcHash(#t),
		uint32 hashes[] = {
			DEFINE_RESOURCE_TYPES
		};
#undef DEFINE_RESOURCE_TYPE

		for (int i = 0; i < (int)ResourceType::LAST; i++)
		{
			if (hashes[i] == hash)
				return (ResourceType)i;
		}

		return ResourceType::LAST;
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
