#ifndef CONTENT_CONTENTLOADER_H
#define CONTENT_CONTENTLOADER_H

#include <vector>
#include "../MyNxna2.h"
#include "../JobQueue.h"
#include "../StringManager.h"

namespace Content
{
	enum class ContentState
	{
		Inactive = 0,
		Incomplete = 1,
		Loaded = 2,

		UnknownError = -1,
		NotFound = -2,
		InvalidFormat = -3,
		NoLoader = -4,
		NotLoaded = -5
	};

#define DEFINE_LOADER_TYPE(t) t,
#define DEFINE_LOADER_TYPES \
	DEFINE_LOADER_TYPE(ModelObj) \
	DEFINE_LOADER_TYPE(Texture2D) \
	DEFINE_LOADER_TYPE(Audio)

	enum class LoaderType
	{
		DEFINE_LOADER_TYPES

		LAST
	};

#define DEFINE_RESOURCE_TYPE(t) t,
#define DEFINE_RESOURCE_TYPES \
	DEFINE_RESOURCE_TYPE(Font) \
	DEFINE_RESOURCE_TYPE(Cursor) \
	DEFINE_RESOURCE_TYPE(Model) \
	DEFINE_RESOURCE_TYPE(Texture2D) \
	DEFINE_RESOURCE_TYPE(Bitmap) \
	DEFINE_RESOURCE_TYPE(Audio)

	enum class ResourceType
	{
		DEFINE_RESOURCE_TYPES

		LAST
	};

	struct Loader
	{
		ResourceType Type;
		JobFunc LoaderFunc;
		JobFunc UnloaderFunc;
		void* LoaderParam;

		uint32 ResourceSize;
		uint32 ResourceAlignment;
	};

	enum class LoaderPhase
	{
		AsyncLoad,
		MainThread,
		Fixup
	};

	struct ContentLoaderParams
	{
		static const uint32 LocalDataStorageSize = 32;
		uint8* LocalDataStorage;

		// input
		LoaderPhase Phase;
		ResourceType Type;
		StringRef FilenameHash;
		void* LoaderParam;
		JobHandle Job;

		// output
		void* Destination;
		ContentState State;
	};

	struct ContentLoaderData;

	class ContentLoader
	{
		static ContentLoaderData* m_data;

	public:
		static void SetGlobalData(ContentLoaderData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static Loader* FindLoader(ResourceType type)
		{
			return findLoader(type);
		}

		static ResourceType GetResourceTypeByFilename(const char* filename, const char* filenameEnd = nullptr);
		static ResourceType GetResourceTypeByFileExtension(const char* ext, const char* extEnd = nullptr);

		static ResourceType GetLoaderResourceType(LoaderType type);
		static LoaderType GetLoaderByNameHash(uint32 hash);
		static ResourceType GetResourceTypeByNameHash(uint32 hash);

	private:
		static Loader* findLoader(ResourceType type);
	};
}

#endif // CONTENT_CONTENTLOADER_H
