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

		struct _SubstitutionTable
		{
			static const uint32 MaxFiles = 1000;
			uint32 SourceHash[MaxFiles];
			uint32 DestHash[MaxFiles];
			bool Active[MaxFiles];
		} SubstitutionTable;

		struct _FileHashTable
		{
			static const uint32 MaxFiles = 1000;
			ResourceFile Files[MaxFiles];
			uint32 Hashes[MaxFiles];
			StringRef Filenames[MaxFiles];
			bool Active[MaxFiles];
		} FileHashTable;

		struct _PreloadTable
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

	void cmdReload(const char* arg)
	{
		if (ContentManager::ReloadAll() == false)
		{
			LOG_ERROR("Fatal error while trying to reload content");
			exit(-1);
		}
	}

	bool ContentManager::Init(uint32 screenHeight, LocaleCode language, LocaleCode region)
	{
		m_data->Resolution = screenHeight;
		m_data->Language = language;
		m_data->Region = region;

#if 0
		// look for localized or res-dependant versions of files
		{
			char res[6];
			if (VirtualResolution::InjectNearestResolution("r%", res, 6) == false)
				return false;

			char locale[6];
			memcpy(locale, language.Code, 2);
			if (region.NumericCode != 0)
			{
				locale[2] = '-';
				memcpy(locale + 3, region.Code, 2);
				locale[5] = 0;
			}
				
			const char* filename;
			uint32 i = 0;
			while ((filename = FileSystem::GetFilenameByIndex(i++)) != nullptr)
			{
				uint32 originalHash = Utils::CalcHash(filename);
				
				uint32 index;
				if (Utils::HashTableUtils::Reserve(originalHash, m_data->SubstitutionTable.SourceHash, m_data->SubstitutionTable.Active, m_data->SubstitutionTable.MaxFiles, &index) == false)
						return false;
				m_data->SubstitutionTable.DestHash[index] = originalHash;

				char buffer[256];

				auto dot = strrchr(filename, '.');
				auto end = Utils::CopyString(buffer, filename, 256, dot - filename + 1);

				// look for res-dependant
				Utils::ConcatString(buffer, res, 256);
				Utils::ConcatString(buffer, dot, 256);

				uint32 hash = Utils::CalcHash(buffer);
				if (FileSystem::GetFilenameByHash(hash) != nullptr)
				{
					// add to the list of substitutions
					m_data->SubstitutionTable.DestHash[index] = hash;
				}
				end[0] = 0;

				// look for localized
				Utils::ConcatString(buffer, locale, 256);
				Utils::ConcatString(buffer, dot, 256);

				hash = Utils::CalcHash(buffer);
				if (FileSystem::GetFilenameByHash(hash) != nullptr)
				{
					// add to the list of substitutions
					m_data->SubstitutionTable.DestHash[index] = hash;
				}
				end[0] = 0;

				// look for res-dependant and localized
				Utils::ConcatString(buffer, res, 256);
				Utils::ConcatString(buffer, ".", 256);
				Utils::ConcatString(buffer, locale, 256);
				Utils::ConcatString(buffer, dot, 256);

				hash = Utils::CalcHash(buffer);
				if (FileSystem::GetFilenameByHash(hash) != nullptr)
				{
					// add to the list of substitutions
					m_data->SubstitutionTable.DestHash[index] = hash;
				}
				end[0] = 0;
			}
		}
#endif
		// add the "reload" console command
		ConsoleCommand cmd = {};
		cmd.Command = "reload";
		cmd.Callback = cmdReload;
		Gui::Console::AddCommands(&cmd, 1);

		return true;
	}

	void ContentManager::Shutdown()
	{
		// TODO
	}


	void* ContentManager::Get(StringRef filename, ResourceType type, ContentLoadFlags flags)
	{
		uint32 finalIndex;

#if 0
		// see if we need to look for a substituted (localized or res-dependant) version
		if ((flags & ContentLoadFlags_DontFixup) == 0)
		{
			if (Utils::HashTableUtils::Find(hash, m_data->SubstitutionTable.SourceHash, m_data->SubstitutionTable.Active, m_data->SubstitutionTable.MaxFiles, &finalIndex))
			{
				hash = m_data->SubstitutionTable.DestHash[finalIndex];
			}
		}
#endif
		auto path = HashStringManager::Get(filename, HashStringManager::HashStringType::File);
		if (path == nullptr) return nullptr;

		auto hash = Utils::CalcHash(path);

		if (Utils::HashTableUtils::Find(hash, m_data->FileHashTable.Hashes, m_data->FileHashTable.Active, m_data->FileHashTable.MaxFiles, &finalIndex))
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
		if (Utils::HashTableUtils::Reserve(hash, m_data->FileHashTable.Hashes, m_data->FileHashTable.Active, m_data->FileHashTable.MaxFiles, &index) == false)
			return nullptr;

		m_data->FileHashTable.Filenames[index] = filename;
		m_data->FileHashTable.Files[index].Data = g_memory->AllocTrack(size, __FILE__, __LINE__);

		if (load(filename, type, loader, index))
			return m_data->FileHashTable.Files[index].Data;

		// still here? Well, we tried.
		g_memory->FreeTrack(m_data->FileHashTable.Files[index].Data, __FILE__, __LINE__);
		m_data->FileHashTable.Active[index] = false;

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

	bool ContentManager::ReloadAll()
	{
		for (uint32 i = 0; i < ContentManagerData::_FileHashTable::MaxFiles; i++)
		{
			if (m_data->FileHashTable.Active[i])
			{
				auto type = m_data->FileHashTable.Files[i].Type;
				auto loader = ContentLoader::FindLoader(type);

				if (loader == nullptr)
				{
					LOG_ERROR("Unable to find loader for file with hash %u during reload. Ignoring.", m_data->FileHashTable.Hashes[i]);
					continue;
				}

				if (loader->UnloaderFunc(m_data->FileHashTable.Files[i].Data) == false)
				{
					LOG_ERROR("Unable to unload file with hash %u during reload. Ignoring.", m_data->FileHashTable.Hashes[i]);
					continue;
				}

				if (load(m_data->FileHashTable.Filenames[i], type, loader, i) == false)
				{
					// uh oh, we can't really recover from this!
					LOG_ERROR("Unable to reload file with hash %u!", m_data->FileHashTable.Hashes[i]);
					return false;
				}
			}
		}

		return true;
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

	bool ContentManager::load(StringRef filename, ResourceType type, Loader* loader, uint32 destIndex)
	{
		ContentLoaderParams p = {};
		p.Phase = LoaderPhase::AsyncLoad;
		p.Type = type;
		p.Destination = m_data->FileHashTable.Files[destIndex].Data;
		p.FilenameHash = filename;
		p.LoaderParam = loader->LoaderParam;
		p.LocalDataStorage = (uint8*)g_memory->AllocTrack(ContentLoaderParams::LocalDataStorageSize, __FILE__, __LINE__);

		if (loader->LoaderFunc(&p))
		{
			p.Phase = LoaderPhase::MainThread;
			if (loader->LoaderFunc(&p))
			{
				p.Phase = LoaderPhase::Fixup;
				m_data->FileHashTable.Files[destIndex].State = LoadState::QueuedForFixup;

				if (loader->LoaderFunc(&p))
				{
					m_data->FileHashTable.Files[destIndex].State = LoadState::Loaded;
					return true;
				}
			}
		}

		m_data->FileHashTable.Files[destIndex].State = LoadState::Error;
		return false;
	}
}
