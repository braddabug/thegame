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

	enum class ResourceType
	{
		Font,
		Model,
		Texture2D,
		Bitmap,
		Audio,

		LAST
	};

	struct Loader
	{
		ResourceType Type;
		JobFunc AsyncLoader;
		JobFunc MainThreadLoader;
		JobFunc Unloader;
		JobFunc RefreshRefs;
		void* LoaderParam;

		uint32 ResourceSize;
		uint32 ResourceAlignment;
	};

	struct ContentLoaderParams
	{
		uint8 LocalDataStorage[32];

		// input
		uint32 FilenameHash;
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

	private:
		static Loader* findLoader(ResourceType type);
	};
}

#endif // CONTENT_CONTENTLOADER_H
