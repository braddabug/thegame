#include "ContentManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Model.h"
#include "../Utils.h"
#include "../MemoryManager.h"

namespace Content
{
	struct ContentInfo
	{
		uint32 NameHash;
		uint32 PathHash;
		void* Data;
	};

	enum class LoadState
	{
		Unloaded,

		PendingWorkerThread,
		PendingMainThread,
		PendingUnload,

		Completed,
		Error
	};

	enum class FileState
	{
		Dead,

		Loaded,
		PendingUnload
	};

	struct ContentManagerData
	{
		uint32 ResourceTypeCount[(int)ResourceType::LAST];
		void* FirstResource[(int)ResourceType::LAST];

		static const uint32 MaxManifestFiles = 100;

		struct
		{
			uint32 Count;
			Utils::HashTable<uint32, ContentManager::MaxLoadedFiles * 2> Files;
			uint32 Name[ContentManager::MaxLoadedFiles];
			LoadState State[ContentManager::MaxLoadedFiles];
			ResourceType Type[ContentManager::MaxLoadedFiles];
			void* Data[ContentManager::MaxLoadedFiles];

			void* ResourceData[(int)ResourceType::LAST];
		} Loaded;
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

	bool ContentManager::BeginLoad(uint32* nameHashes, ResourceType* types, uint32 numFiles)
	{
		// mark all existing files as "to unload"
		for (uint32 i = 0; i < ContentManager::MaxLoadedFiles; i++)
		{
			if (m_data->Loaded.State[i] == LoadState::Completed)
			{
				m_data->Loaded.State[i] = LoadState::PendingUnload;
			}
		}

		// go through each new file and see if it exists in the already loaded files.
		for (uint32 i = 0; i < numFiles; i++)
		{
			uint32* index;
			if (m_data->Loaded.Files.GetPtr(nameHashes[i], &index))
			{
				// abort the unload and keep it
				if (m_data->Loaded.State[*index] == LoadState::PendingUnload)
					m_data->Loaded.State[*index] = LoadState::Completed;
			}
		}

		// unload old unused files
		for (uint32 i = 0; i < ContentManager::MaxLoadedFiles; i++)
		{
			if (m_data->Loaded.State[i] == LoadState::PendingUnload)
			{
				auto type = m_data->Loaded.Type[i];

				auto loader = ContentLoader::FindLoader(type);
				if (loader == nullptr)
					return false;

				if (loader->Unloader(m_data->Loaded.Data[i]) == false)
					return false;

				m_data->Loaded.Files.Remove(m_data->Loaded.Name[i]);
				m_data->Loaded.State[i] = LoadState::Unloaded;
			}
		}
		
		uint32 totalResourceCount[(int)ResourceType::LAST] = {};

		// count how many of each resource type we have
		for (uint32 i = 0; i < numFiles; i++)
		{
			totalResourceCount[(int)types[i]]++;
		}

		// allocate storage
		void* resourceData[(int)ResourceType::LAST];
		uint32 resourceSize[(int)ResourceType::LAST];
		uint32 resourceCount[(int)ResourceType::LAST];
		for (uint32 i = 0; i < (uint32)ResourceType::LAST; i++)
		{
			uint32 alignment;
			GetResourceInfo((ResourceType)i, &resourceSize[i], &alignment);
			resourceData[i] = g_memory->AllocTrack(resourceSize[i] * totalResourceCount[i], __FILE__, __LINE__);
			resourceCount[i] = 0;
		}

		// copy the old resources
		for (uint32 i = 0; i < ContentManager::MaxLoadedFiles; i++)
		{
			if (m_data->Loaded.State[i] == LoadState::Completed)
			{
				auto type = m_data->Loaded.Type[i];

				uint32 size, alignment;
				GetResourceInfo(type, &size, &alignment);

				uint32 index = m_data->Loaded.Count;
				m_data->Loaded.Data[index] = (char*)resourceData[(int)type] + resourceCount[(int)type] * size;
				memcpy(m_data->Loaded.Data[index], m_data->Loaded.Data[i], size);
				resourceCount[(int)type]++;
				m_data->Loaded.Count++;
			}
		}

		// find places for the new files
		for (uint32 i = 0; i < numFiles; i++)
		{
			uint32* index;
			if (m_data->Loaded.Files.GetPtr(nameHashes[i], &index) == false)
			{
				// verify that this filename is known
				if (FileSystem::GetFilenameByHash(nameHashes[i]) == nullptr)
				{
					LOG_ERROR("Unable to find filename associated with hash %u. Cannot load file.", nameHashes[i]);
				}
				else
				{
					// this is a new file, so find a new spot for it
					uint32 index = m_data->Loaded.Count;
					m_data->Loaded.Name[index] = nameHashes[i];
					m_data->Loaded.Type[index] = types[i];
					m_data->Loaded.State[index] = LoadState::PendingWorkerThread;
					m_data->Loaded.Files.Add(nameHashes[i], index);

					auto resourceType = types[i];
					m_data->Loaded.Data[index] = (char*)resourceData[(int)resourceType] + resourceCount[(int)resourceType] * resourceSize[(int)resourceType];
					assert(m_data->Loaded.Data[index] < (char*)resourceData[(int)resourceType] + totalResourceCount[(int)resourceType] * resourceSize[(int)resourceType]);
					resourceCount[(int)resourceType]++;
					m_data->Loaded.Count++;
				}
			}
		}

		// delete the old memory
		for (uint32 i = 0; i < (int)ResourceType::LAST; i++)
		{
			if (m_data->Loaded.ResourceData[i] != nullptr)
				g_memory->FreeTrack(m_data->Loaded.ResourceData[i], __FILE__, __LINE__);

			m_data->Loaded.ResourceData[i] = resourceData[i];
		}
		
		return true;
	}

	LoadResult ContentManager::PendingLoads(bool progress, float* percentProgress)
	{
		// TODO: atm this is completely single-threaded. It could be made asynchronous... someday.

		int pendingCount = 0;
		int completedCount = 0;
		int errorCount = 0;

		Utils::Stopwatch sw;
		sw.Start();

		for (uint32 i = 0; i < ContentManager::MaxLoadedFiles; i++)
		{
			if (m_data->Loaded.State[i] == LoadState::PendingWorkerThread ||
				m_data->Loaded.State[i] == LoadState::PendingMainThread)
			{
				if (progress)
				{
					m_data->Loaded.State[i] = LoadState::Error;
					auto loader = ContentLoader::FindLoader(m_data->Loaded.Type[i]);
					
					if (loader != nullptr)
					{
						ContentLoaderParams p = {};
						p.Destination = m_data->Loaded.Data[i];
						p.FilenameHash = m_data->Loaded.Name[i];
						p.LoaderParam = loader->LoaderParam;

						if (loader->AsyncLoader(&p) && (loader->MainThreadLoader == nullptr || loader->MainThreadLoader(&p)))
						{
							m_data->Loaded.State[i] = LoadState::Completed;
						}
						else
						{
							auto filename = FileSystem::GetFilenameByHash(p.FilenameHash);
							if (filename != nullptr)
								LOG_ERROR("Error while loading %s", filename);
							else
								LOG_ERROR("Error while loading file with hash %u", p.FilenameHash);
						}
					}

					// have we done enough work for now?
					if (sw.GetElapsedMilliseconds32() > 33)
						progress = false;
				}
			}

			if (m_data->Loaded.State[i] == LoadState::PendingMainThread ||
				m_data->Loaded.State[i] == LoadState::PendingWorkerThread)
				pendingCount++;
			else if (m_data->Loaded.State[i] == LoadState::Error)
				errorCount++;
			else if (m_data->Loaded.State[i] != LoadState::Unloaded)
				completedCount++;
		}

		if (percentProgress != nullptr)
		{
			int total = pendingCount + completedCount + errorCount;
			*percentProgress = (completedCount + errorCount) / (float)total;
		}

		if (pendingCount == 0)
		{
			if (errorCount > 0)
				return LoadResult::Error;
			else
				return LoadResult::Completed;
		}

		return LoadResult::Pending;
	}

	void* ContentManager::Get(uint32 hash, ResourceType type)
	{
		uint32* index;
		if (m_data->Loaded.Files.GetPtr(hash, &index) == false)
			return nullptr;

		if (m_data->Loaded.Type[*index] != type)
			return nullptr;

		if (m_data->Loaded.State[*index] != LoadState::Completed)
			return nullptr;

		assert(m_data->Loaded.Name[*index] == hash);

		return m_data->Loaded.Data[*index];
	}

	void* ContentManager::GetPending(uint32 hash, ResourceType type, LoadResult* status)
	{
		uint32* index;
		if (m_data->Loaded.Files.GetPtr(hash, &index) == false)
		{
			if (status) *status = LoadResult::Error;
			return nullptr;
		}

		if (m_data->Loaded.Type[*index] != type)
		{
			if (status) *status = LoadResult::Error;
			return nullptr;
		}

		if (status)
		{
			if (m_data->Loaded.State[*index] == LoadState::Completed)
				*status = LoadResult::Completed;
			else if (m_data->Loaded.State[*index] == LoadState::Error)
				*status = LoadResult::Error;
			else
				*status = LoadResult::Pending;
		}
		return m_data->Loaded.Data[*index];
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
	}

#if 0
	void* ContentManager::get(uint32 index, uint32 magic, LoadResult* status)
	{
		if (index >= ContentManagerData::MaxFiles || m_data->Alive[index] == false || m_data->Check[index] != magic || m_data->State[index] == LoadState::Error)
		{
			if (status) *status = LoadResult::Error;
			return nullptr;
		}

		if (status)
		{
			if (m_data->State[index] == LoadState::Completed)
				*status = LoadResult::Completed;
			else
				*status = LoadResult::Pending;
		}

		return m_data->Data[index];
	}
#endif

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
