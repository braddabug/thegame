#ifndef CONTENT_CONTENTMANAGER_H
#define CONTENT_CONTENTMANAGER_H

#include <atomic>
#include <vector>
#include <string.h>
#include "../Common.h"
#include "../Logging.h"
#include "../MyNxna2.h"
#include "../JobQueue.h"
#include "ContentLoader.h"

namespace Content
{
	
	template<typename T>
	struct Content
	{
		std::atomic<ContentState> State;
		T* Data;
		
		char Filename[256];
		LoaderType Type;
	};
	
	class ContentManager;


	struct ContentManagerData;
	
	enum class LoadResult
	{
		Pending,
		Completed,
		Error
	};

	template<typename T>
	struct ContentHandle
	{
		uint32 Index;
		uint32 Magic;
	};

	class ContentManager
	{
		static ContentManagerData* m_data;

	public:
		static const uint32 MaxLoadedFiles = 100;

		static void SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		

		static bool BeginLoad(uint32* nameHashes, ResourceType* types, uint32 numFiles);
		static LoadResult PendingLoads(bool progress, float* percentProgress);

		static void* Get(uint32 hash, ResourceType type);
		static void* GetPending(uint32 hash, ResourceType type, LoadResult* status);
		
		template<typename T>
		static T* GetX(ContentHandle<T> handle)
		{
			return (T*)get(handle.Index, handle.Magic, nullptr);
		}

		template<typename T>
		static T* GetX(ContentHandle<T> handle, LoadResult* status)
		{
			return (T*)get(handle.Index, handle.Magic, status);
		}

		static bool GetResourceInfo(ResourceType type, uint32* size, uint32* alignment);


		void Tick()
		{
			JobQueue::Tick();
		}

	private:
		static ContentLoader* findLoader(LoaderType type);

		static void* get(uint32 index, uint32 handle, LoadResult* status);
	};
}

#endif // CONTENT_CONTENTMANAGER_H

