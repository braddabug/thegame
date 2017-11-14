#include "ContentManager.h"
#include "../Gui/GuiManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Model.h"
#include "../Utils.h"
#include "../MemoryManager.h"
#include "../iniparse.h"

namespace Content
{
	enum class LoadState
	{
		Free,

		QueuedForLoad,
		QueuedForFixup,
		Loaded,
		QueuedForUnload,
		Unloaded,

		Error
	};

	struct ResourceFile
	{
		//uint32 NameHash;
		ResourceType Type;
		LoadState State;
		int RefCount;

		void* Data;
	};

	struct ResourceFileGroup
	{
		uint32 NameHash;
		uint32* FileIndices;
		uint32 NumFiles;
	};

	struct ContentManagerData
	{
		uint32 Resolution;
		LocaleCode Language;
		LocaleCode Region;

		//ResourceFile* Files;
		uint8* LoaderData;
		//uint32 NumFiles;
		//uint32 MaxFiles;

		static const uint32 MaxGroups = 100;
		ResourceFileGroup Groups[MaxGroups];
		uint32 NumGroups;

		struct
		{
			static const uint32 MaxFiles = 1000;
			ResourceFile Files[MaxFiles];
			uint32 Hashes[MaxFiles];
			bool Active[MaxFiles];
		} FileHashTable;

		struct
		{
			uint32 NumFiles;
			uint32 NumGroups;
			static const uint32 MaxGroups = 100;
			struct Group
			{
				uint32 Hash;
				uint32 NumFiles;

				static const uint32 MaxFiles = 1000;
				struct File
				{
					uint32 TrueHash;
					uint32 BasicHash;
					uint32 Resolution;
					LocaleCode Language;
					LocaleCode Region;
				} Files[MaxFiles];
			} Groups[MaxGroups];
		} PreloadTable;
	};

	ContentManagerData* ContentManager::m_data = nullptr;

	void ContentManager::SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = (ContentManagerData*)g_memory->AlignedAllocTrack(sizeof(ContentManagerData), alignof(ContentManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(ContentManagerData));
		}



		m_data = *data;
	}

	bool ContentManager::Init(uint32 screenHeight, LocaleCode language, LocaleCode region)
	{
		m_data->Resolution = screenHeight;
		m_data->Language = language;
		m_data->Region = region;

		return true;
	}

	void ContentManager::Shutdown()
	{
		// TODO
	}


	void* ContentManager::Get(uint32 hash, ResourceType type, ContentLoadFlags flags)
	{
		uint32 finalIndex;
		if (Utils::HashTableUtils::Find<Content::ResourceFile>(hash, m_data->FileHashTable.Hashes, m_data->FileHashTable.Files, m_data->FileHashTable.Active, m_data->FileHashTable.MaxFiles, &finalIndex))
		{
			m_data->FileHashTable.Files[finalIndex].RefCount++;
			return m_data->FileHashTable.Files[finalIndex].Data;
		}

		// the file wasn't found, so try to load it
		if (flags & ContentLoadFlags::ContentLoadFlags_PreloadOnly)
			return nullptr;

		auto loader = ContentLoader::FindLoader(type);

		uint32 size, alignment;
		if (GetResourceInfo(type, &size, &alignment) == false)
			return nullptr;

		uint32 index;
		if (Utils::HashTableUtils::Reserve<ResourceFile>(hash, m_data->FileHashTable.Hashes, m_data->FileHashTable.Files, m_data->FileHashTable.Active, m_data->FileHashTable.MaxFiles, &index) == false)
			return nullptr;

		m_data->FileHashTable.Files[index].Data = g_memory->AllocTrack(size, __FILE__, __LINE__);

		ContentLoaderParams p = {};
		p.Phase = LoaderPhase::AsyncLoad;
		p.Type = type;
		p.Destination = m_data->FileHashTable.Files[index].Data;
		p.FilenameHash = hash;
		p.LoaderParam = loader->LoaderParam;
		p.LocalDataStorage = (uint8*)g_memory->AllocTrack(ContentLoaderParams::LocalDataStorageSize, __FILE__, __LINE__);

		if (loader->LoaderFunc(&p))
		{
			p.Phase = LoaderPhase::MainThread;
			if (loader->LoaderFunc(&p))
			{
				p.Phase = LoaderPhase::Fixup;
				m_data->FileHashTable.Files[index].State = LoadState::QueuedForFixup;

				if (loader->LoaderFunc(&p))
				{
					return p.Destination;
				}
			}
		}

		return nullptr;
	}

	void ContentManager::Release(void* content)
	{
		/*
		// NOTE: this is suseptible to the ABA problem. If we really want to be safe we should probably have some kind of ContentHandle.

		// TODO: we could build a hash table to make lookups faster

		for (uint32 i = 0; i < m_data->NumFiles; i++)
		{
			if (m_data->Files[i].Data == content)
			{
				if (m_data->Files[i].State == LoadState::Loaded ||
					m_data->Files[i].State == LoadState::Unloaded)
				{
					if (m_data->Files[i].RefCount > 0)
					{
						m_data->Files[i].RefCount--;
						return;
					}
					else
					{
						WriteLog(LogSeverityType::Warning, LogChannelType::Content, "Trying to release content, but no refs were detected");
						return;
					}
				}
				else
				{
					WriteLog(LogSeverityType::Warning, LogChannelType::Content, "Trying to release content, but content isn't loaded");
					return;
				}
			}
		}

		WriteLog(LogSeverityType::Warning, LogChannelType::Content, "Trying to release content, but content wasn't found");
		*/
	}

	bool ContentManager::GetResourceInfo(ResourceType type, uint32* size, uint32* alignment)
	{
		if (size == nullptr || alignment == nullptr)
			return false;

#ifdef _MSC_VER
			// make sure all ResourceTypes are explicitly handled
#pragma warning(push, 4)
#pragma warning(error: 4061 4062)
#endif
			switch (type)
			{
			case ResourceType::Cursor:
			{
				*size = sizeof(CursorInfo);
				*alignment = sizeof(CursorInfo);
				return true;
			}
			case ResourceType::Font:
			{
				*size = sizeof(Gui::Font);
				*alignment = alignof(Gui::Font);
				return true;
			}
			case ResourceType::Model:
			{
				*size = sizeof(Graphics::Model);
				*alignment = alignof(Graphics::Model);
				return true;
			}
			case ResourceType::Texture2D:
			{
				*size = sizeof(Nxna::Graphics::Texture2D);
				*alignment = alignof(Nxna::Graphics::Texture2D);
				return true;
			}
			case ResourceType::Bitmap:
			{
				*size = sizeof(Graphics::Bitmap);
				*alignment = alignof(Graphics::Bitmap);
				return true;
			}
			case ResourceType::Audio:
			{
				*size = sizeof(Audio::Buffer);
				*alignment = alignof(Audio::Buffer);
				return true;
			}
			case ResourceType::LAST:
			{
				return false;
			}
			}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		// keep the compiler happy
		return false;
	}
}
