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

	enum ContentLoadFlags
	{
		ContentLoadFlags_None = 0,

		ContentLoadFlags_Scene = 1,        // marks this content as associated with the current scene. Content is "global" otherwise.

		ContentLoadFlags_DontFixup = 2,    // don't try to load a resolution or local-specific version of the file. Load exactly what is specified.

		ContentLoadFlags_PreloadOnly = 4,  // only get the resource if it's already loaded. Don't try to load from disk.
		ContentLoadFlags_DontPreload = 8,  // don't add the file to the "preload" list.
	};

	class ContentManager
	{
		static ContentManagerData* m_data;

	public:
		static void SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		static bool Init(uint32 screenHeight, LocaleCode language, LocaleCode region);
		static void Shutdown();

		// TODO: implement these
		static int PreloadGlobal();
		static int PreloadScene(uint32 sceneID);

		static void* Get(uint32 hash, ResourceType type, ContentLoadFlags flags = ContentLoadFlags::ContentLoadFlags_None);
		static void Release(void* content);
		
		static bool GetResourceInfo(ResourceType type, uint32* size, uint32* alignment);

	private:
		static ContentLoader* findLoader(LoaderType type);
	};
}

#endif // CONTENT_CONTENTMANAGER_H

