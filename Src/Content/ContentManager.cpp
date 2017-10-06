#include "ContentManager.h"
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
		Loaded,
		QueuedForUnload,
		Unloaded,

		Error
	};

	struct ResourceFile
	{
		uint32 NameHash;
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
		ResourceFile* Files;
		uint32 NumFiles;

		static const uint32 MaxGroups = 100;
		ResourceFileGroup Groups[MaxGroups];
		uint32 NumGroups;

		struct
		{
			uint32 Size;
			ResourceFile** Files;
			uint32* Hashes;
		} FileHashTable;
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

	bool ContentManager::LoadManifest(uint32 screenHeight)
	{
		File f;
		if (FileSystem::OpenAndMap("Content/manifest.txt", &f) == false)
			return false;

		ini_context ctx;
		ini_init(&ctx, (const char*)f.Memory, (const char*)f.Memory + f.FileSize);

		ini_item item;

		uint32 fileListCapacity = 0;

	parse:
		if (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "group"))
				{
					char groupName[64];
					int32 minResolution = 0;
					int32 maxResolution = 10000000;
					uint32 numFiles = 0;

					ini_context groupStart = ctx;

					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "name"))
							ini_value_copy(&ctx, &item, groupName, 64);
						else if (ini_key_equals(&ctx, &item, "minResolution"))
						{
							ini_value_int(&ctx, &item, &minResolution);
							if ((int)screenHeight < minResolution)
								goto parse;
						}
						else if (ini_key_equals(&ctx, &item, "maxResolution"))
						{
							ini_value_int(&ctx, &item, &maxResolution);
							if ((int)screenHeight >= maxResolution)
								goto parse;
						}
						else if (ini_key_equals(&ctx, &item, "file"))
							numFiles++;
					}

					m_data->Groups[m_data->NumGroups].NameHash = Utils::CalcHash(groupName);
					m_data->Groups[m_data->NumGroups].NumFiles = 0;
					m_data->Groups[m_data->NumGroups].FileIndices = (uint32*)g_memory->AllocTrack(sizeof(uint32) * numFiles, __FILE__, __LINE__);

					// go back and load all the files
					while (ini_next_within_section(&groupStart, &item) == ini_result_success)
					{
						if (ini_key_equals(&groupStart, &item, "file"))
						{
							uint32 fileHash = Utils::CalcHash((const uint8*)groupStart.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
							auto type = ContentLoader::GetResourceTypeByFilename((const char*)groupStart.source + item.keyvalue.value_start, (const char*)groupStart.source + item.keyvalue.value_end);

							// see if this file already exists in the file list
							bool found = false;
							for (uint32 i = 0; i < m_data->NumFiles; i++)
							{
								if (m_data->Files[i].NameHash == fileHash &&
									m_data->Files[i].Type == type)
								{
									found = true;
									m_data->Groups[m_data->NumGroups].FileIndices[m_data->Groups[m_data->NumGroups].NumFiles] = i;
									m_data->Groups[m_data->NumGroups].NumFiles++;
									break;
								}
							}

							if (found == false)
							{
								if (m_data->NumFiles >= fileListCapacity)
								{
									// expand the file list
									auto newCapacity = fileListCapacity + numFiles;
									m_data->Files = (ResourceFile*)g_memory->ReallocTrack(m_data->Files, sizeof(ResourceFile) * newCapacity, __FILE__, __LINE__);
									memset(m_data->Files + fileListCapacity, 0, sizeof(ResourceFile) * numFiles);
									fileListCapacity = newCapacity;
								}

								m_data->Files[m_data->NumFiles].NameHash = fileHash;
								m_data->Files[m_data->NumFiles].Type = type;
								
								m_data->Groups[m_data->NumGroups].FileIndices[m_data->Groups[m_data->NumGroups].NumFiles] = m_data->NumFiles;
								m_data->Groups[m_data->NumGroups].NumFiles++;
								
								m_data->NumFiles++;
							}
						}
					}

					m_data->NumGroups++;
				}
			}

			goto parse;
		}

		// build a hash table of all the files
		{
			m_data->FileHashTable.Size = m_data->NumFiles * 3 / 2;
			m_data->FileHashTable.Hashes = (uint32*)g_memory->AllocTrack(m_data->FileHashTable.Size * sizeof(uint32), __FILE__, __LINE__);
			m_data->FileHashTable.Files = (ResourceFile**)g_memory->AllocTrack(m_data->FileHashTable.Size * sizeof(ResourceFile*), __FILE__, __LINE__);
			memset(m_data->FileHashTable.Files, 0, sizeof(ResourceFile*) * m_data->FileHashTable.Size);

			for (uint32 i = 0; i < m_data->NumFiles; i++)
			{
				uint32 index = m_data->Files[i].NameHash % m_data->FileHashTable.Size;

				for (uint32 j = 0; j < m_data->FileHashTable.Size; j++)
				{
					uint32 finalIndex = (index + j) % m_data->FileHashTable.Size;
					if (m_data->FileHashTable.Files[finalIndex] == nullptr)
					{
						m_data->FileHashTable.Files[finalIndex] = &m_data->Files[i];
						break;
					}
				}
			}
		}

		FileSystem::Close(&f);

		return true;
	}

	// TODO: figure out the best way to handle errored files

	bool ContentManager::QueueGroupLoad(uint32 nameHash, bool forceReload)
	{
		// find the group
		for (uint32 i = 0; i < m_data->NumGroups; i++)
		{
			if (m_data->Groups[i].NameHash == nameHash)
			{
				// mark every file in the group as pending load if not already loaded
				for (uint32 j = 0; j < m_data->Groups[i].NumFiles; j++)
				{
					uint32 fileIndex = m_data->Groups[i].FileIndices[j];
					assert(fileIndex < m_data->NumFiles);

					if (m_data->Files[fileIndex].State == LoadState::Unloaded ||
						m_data->Files[fileIndex].State == LoadState::QueuedForUnload)
						m_data->Files[fileIndex].State = LoadState::Loaded;
					else if (m_data->Files[fileIndex].State == LoadState::Free)
						m_data->Files[fileIndex].State = LoadState::QueuedForLoad;
				}

				return true;
			}
		}

		WriteLog(LogSeverityType::Warning, LogChannelType::Content, "No content group with the hash %u was found", nameHash);

		return false;
	}

	bool ContentManager::QueueGroupUnload(uint32 nameHash)
	{
		// find the group
		for (uint32 i = 0; i < m_data->NumGroups; i++)
		{
			if (m_data->Groups[i].NameHash == nameHash)
			{
				// mark every file in the group as pending load if not already loaded
				for (uint32 j = 0; j < m_data->Groups[i].NumFiles; j++)
				{
					uint32 fileIndex = m_data->Groups[i].FileIndices[j];
					assert(fileIndex < m_data->NumFiles);

					if (m_data->Files[fileIndex].State == LoadState::Loaded)
						m_data->Files[fileIndex].State = LoadState::QueuedForUnload;
				}

				return true;
			}
		}

		WriteLog(LogSeverityType::Warning, LogChannelType::Content, "No content group with the hash %u was found", nameHash);

		return false;
	}

	bool ContentManager::BeginLoad()
	{
		// unload files that aren't needed anymore
		for (uint32 i = 0; i < m_data->NumFiles; i++)
		{
			if (m_data->Files[i].State == LoadState::QueuedForUnload ||
				m_data->Files[i].State == LoadState::Unloaded)
			{
				if (m_data->Files[i].RefCount > 0)
				{
					m_data->Files[i].State = LoadState::Unloaded;
					WriteLog(LogSeverityType::Warning, LogChannelType::Content, "File %u is queued for unload, but something is still referencing it", m_data->Files[i].NameHash);
				}
				else
				{
					m_data->Files[i].State = LoadState::Free;

					auto loader = ContentLoader::FindLoader(m_data->Files[i].Type);
					if (loader == nullptr)
						return false;

					if (loader->Unloader(m_data->Files[i].Data) == false)
						return false;

					g_memory->FreeTrack(m_data->Files[i].Data, __FILE__, __LINE__);
				}
			}
			else if (m_data->Files[i].State == LoadState::QueuedForLoad)
			{
				uint32 alignment, size;
				GetResourceInfo(m_data->Files[i].Type, &size, &alignment);
				m_data->Files[i].Data = g_memory->AllocTrack(size, __FILE__, __LINE__);

				auto filename = FileSystem::GetFilenameByHash(m_data->Files[i].NameHash);
				if (filename)
					WriteLog(LogSeverityType::Info, LogChannelType::Content, "Queued %s for loading", filename);
				else
					WriteLog(LogSeverityType::Warning, LogChannelType::Content, "Queued file with hash %u for loading, but can't find its filename", m_data->Files[i].NameHash);
			}
		}

		return true;
	}

	bool ContentManager::PendingLoads(uint32* pending, uint32* success, uint32* error)
	{
		// TODO: atm this is completely single-threaded. It could be made asynchronous... someday.

		int pendingCount = 0;
		int completedCount = 0;
		int errorCount = 0;

		Utils::Stopwatch sw;
		sw.Start();

		bool load = true;

		for (uint32 i = 0; i < m_data->NumFiles; i++)
		{
			if (load && m_data->Files[i].State == LoadState::QueuedForLoad)
			{
				auto loader = ContentLoader::FindLoader(m_data->Files[i].Type);

				ContentLoaderParams p = {};
				p.Destination = m_data->Files[i].Data;
				p.FilenameHash = m_data->Files[i].NameHash;
				p.LoaderParam = loader->LoaderParam;

				if (loader->AsyncLoader(&p) && (loader->MainThreadLoader == nullptr || loader->MainThreadLoader(&p)))
				{
					m_data->Files[i].State = LoadState::Loaded;
				}
				else
				{
					m_data->Files[i].State = LoadState::Error;

					auto filename = FileSystem::GetFilenameByHash(p.FilenameHash);
					if (filename != nullptr)
						LOG_ERROR("Error while loading %s", filename);
					else
						LOG_ERROR("Error while loading file with hash %u", p.FilenameHash);
				}

				// have we done enough work for now?
				if (sw.GetElapsedMilliseconds32() > 33)
					load = false;
			}

			if (m_data->Files[i].State == LoadState::Loaded)
				completedCount++;
			else if (m_data->Files[i].State == LoadState::QueuedForLoad)
				pendingCount++;
			else if (m_data->Files[i].State == LoadState::Error)
				errorCount++;
		}

		if (pending) *pending = pendingCount;
		if (success) *success = completedCount;
		if (error) *error = errorCount;

		return pendingCount > 0;
	}

	void* ContentManager::Get(uint32 hash, ResourceType type)
	{
		uint32 index = hash % m_data->FileHashTable.Size;

		for (uint32 i = 0; i < m_data->FileHashTable.Size; i++)
		{
			uint32 finalIndex = (index + i) % m_data->FileHashTable.Size;
			if (m_data->FileHashTable.Files[finalIndex] != nullptr)
			{
				if (m_data->FileHashTable.Files[finalIndex]->NameHash == hash &&
					m_data->FileHashTable.Files[finalIndex]->Type == type)
				{
					m_data->FileHashTable.Files[finalIndex]->RefCount++;
					return m_data->FileHashTable.Files[finalIndex]->Data;
				}
			}
			else
			{
				return nullptr;
			}
		}

		return nullptr;
	}

	void ContentManager::Release(void* content)
	{
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
