#ifndef CONTENT_CONTENTLOADER_H
#define CONTENT_CONTENTLOADER_H

#include <vector>
#include "../MyNxna2.h"
#include "../JobQueue.h"

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

	enum class LoaderType
	{
		ModelObj,

		Texture2D,

		Audio
	};

	struct Loader
	{
		LoaderType Type;
		JobFunc AsyncLoader;
		JobFunc MainThreadLoader;
		void* LoaderParam;
	};

	struct ContentLoaderParams
	{
		uint8 LocalDataStorage[32];

		// input
		PersistantString Filename;
		void* LoaderParam;
		bool Asynchronous;
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

		template<typename T>
		static ContentState Load(const char* filename, LoaderType type, T* destination)
		{
			auto loader = findLoader(type);
			if (loader == nullptr)
			{
				WriteLog(LogSeverityType::Error, LogChannelType::Content, "No loader found for %s (type %d)", filename, (int)type);
				return ContentState::NoLoader;
			}

			ContentLoaderParams p = {};
			p.Destination = destination;
			p.Filename = filename;
			p.LoaderParam = loader->LoaderParam;

			if (loader->AsyncLoader(&p) == true && (loader->MainThreadLoader == nullptr || loader->MainThreadLoader(&p)))
			{
				WriteLog(LogSeverityType::Info, LogChannelType::Content, "Loaded %s (untracked)", filename);
				return ContentState::Loaded;
			}

			WriteLog(LogSeverityType::Error, LogChannelType::Content, "Error when loading %s (untracked)...", filename);
			return ContentState::UnknownError;
		}

		template<typename T>
		static ContentState BeginLoad(PersistantString filename, LoaderType type, T* destination, P_OUT_OPTIONAL JobInfo* result, JobHandle parent = (JobHandle)-1)
		{
			static_assert(sizeof(ContentLoaderParams) < Job::MaxDataSize, "ContentLoaderParams is too big");

			auto loader = findLoader(type);
			if (loader == nullptr)
			{
				WriteLog(LogSeverityType::Error, LogChannelType::Content, "No loader found for %s (type %d)", filename, (int)type);
				return ContentState::NoLoader;
			}

			ContentLoaderParams p = {};
			p.Destination = destination;
			p.Filename = filename;
			p.LoaderParam = loader->LoaderParam;
			p.Asynchronous = true;

			StringManager::Acquire(filename);
			
			JobHandle job = JobQueue::AddDependantJob(parent, loader->AsyncLoader, loader->MainThreadLoader, result, &p, sizeof(ContentLoaderParams));

			if (job == (uint32)-1)
			{
				StringManager::Release(filename);
				return ContentState::UnknownError;
			}

			return ContentState::Incomplete;
		}

	private:
		static Loader* findLoader(LoaderType type);
	};
}

#endif // CONTENT_CONTENTLOADER_H
