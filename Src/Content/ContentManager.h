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
	
	struct ContentManagerData;

	class ContentManager
	{
		static ContentManagerData* m_data;

	public:
		static void SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		
		static bool LoadManifest(uint32 screenHeight);
		static int QueueGroupLoad(uint32 nameHash, bool forceReload);
		static int QueueGroupUnload(uint32 nameHash);

		static bool BeginLoad();
		static bool PendingLoads(uint32* pending, uint32* success, uint32* error);

		static void* Get(uint32 hash, ResourceType type);
		static void Release(void* content);
		
		static bool GetResourceInfo(ResourceType type, uint32* size, uint32* alignment);

	private:
		static ContentLoader* findLoader(LoaderType type);
	};
}

#endif // CONTENT_CONTENTMANAGER_H

