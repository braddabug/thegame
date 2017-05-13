#ifndef CONTENT_CONTENTMANAGER_H
#define CONTENT_CONTENTMANAGER_H

#include <atomic>
#include <vector>
#include <string.h>
#include "../Common.h"
#include "../Logging.h"
#include "../MyNxna2.h"

namespace Content
{
	enum class LoaderType
	{
		ModelObj,

		Texture2D
	};

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

	template<typename T>
	struct Content
	{
		std::atomic<ContentState> State;
		T* Data;
		
		char Filename[256];
		LoaderType Type;
	};
	
	class ContentManager;

	struct ContentLoader
	{
		LoaderType Type;
		int (*Loader)(ContentManager*, const char*, void*, void*);
		void* LoaderParam;
	};

	struct ContentManagerData
	{
		std::vector<ContentLoader> Loaders;
	};

	class ContentManager
	{
		struct ContentGeneric
		{
			std::atomic<ContentState> State;
			void* Data;
			LoaderType Type;

			char Filename[256];
		};

		static const uint32 m_maxContent = 1000;
		uint32 m_numContent;
		ContentGeneric m_content[m_maxContent];

		static ContentManagerData* m_data;

	public:
		static void SetGlobalData(ContentManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		
		template<typename T>
		static ContentState Load(const char* filename, LoaderType type, T* destination, const char* folder = nullptr)
		{
			WriteLog(nullptr, LogSeverityType::Info, LogChannelType::Content, "Loading %s (untracked)...", filename);

			// we're never really using ContentGeneric directly. It's just storage for Content<T>.
			static_assert(sizeof(ContentGeneric) == sizeof(Content<T>), "ContentGeneric is incorrect size");
			static_assert(alignof(ContentGeneric) == alignof(Content<T>), "ContentGeneric is incorrect alignment");

			if (filename == nullptr || destination == nullptr) return ContentState::UnknownError;

			auto loader = findLoader(type);
			if (loader == nullptr)
			{
				WriteLog(nullptr, LogSeverityType::Error, LogChannelType::Content, "No loader found for %s (type %d)", filename, (int)type);
				return ContentState::NoLoader;
			}

			char buffer[256];
			if (folder != nullptr)
			{
#ifdef _WIN32
				strcpy_s(buffer, folder);
				strcat_s(buffer, filename);
#else
				strncpy(buffer, folder, 256);
				strncat(buffer, filename, 256);
#endif
				filename = buffer;
			}

			typedef int(*StrongLoaderType)(ContentManager*, const char*, T*, void*);
			if (((StrongLoaderType)(loader->Loader))(nullptr, filename, destination, loader->LoaderParam) == 0)
			{
				WriteLog(nullptr, LogSeverityType::Info, LogChannelType::Content, "Loaded %s (untracked)", filename);
				return ContentState::Loaded;
			}

			WriteLog(nullptr, LogSeverityType::Error, LogChannelType::Content, "Error when loading %s (untracked)...", filename);
			return ContentState::UnknownError;
		}

		template<typename T>
		ContentState LoadTracked(const char* filename, LoaderType type, T* destination, const char* folder = nullptr)
		{
			if (filename == nullptr || destination == nullptr) return ContentState::UnknownError;

			auto r = Get(filename, type, &destination);
			if (r == ContentState::NotLoaded)
			{
				for (uint32 i = 0; i < m_maxContent; i++)
				{
					if (m_content[i].State == ContentState::Inactive)
					{
						m_content[i].State = ContentState::Incomplete;
#ifdef _WIN32
						strncpy_s(m_content[i].Filename, filename, 256);
#else
						strncpy(m_content[i].Filename, filename, 256);
#endif
						
						auto loader = findLoader(type);
						if (loader == nullptr)
						{
							m_content[i].State = ContentState::NoLoader;
							return ContentState::NoLoader;
						}

						typedef int(*StrongLoaderType)(ContentManager*, const char*, T*, void*);
						if (((StrongLoaderType)(loader->Loader))(this, filename, destination, loader->LoaderParam) == 0)
							return ContentState::Loaded;
					}
				}
			}
			else if (r == ContentState::Incomplete)
			{
				// the resource is being loaded, so wait for it.
				// TODO: we could probably do something fancy, like steal the job and work it in this thread instead of spinning
				while (r == ContentState::Incomplete)
					Tick();

				// TODO
			}
			else
			{
				// there was probably an error
				return r;
			}
		}

		template<typename T>
		Content<T>* BeginLoad(const char* filename, LoaderType type, T* destination);

		template<typename T>
		Content<T>* WaitForLoad(Content<T>* content);

		template<typename T>
		ContentState Get(const char* filename, LoaderType type, T** destination)
		{
			for (uint32 i = 0; i < m_numContent; i++)
			{
				if (m_content[i].Type == type &&
#ifdef _WIN32
					_stricmp(m_content[i].Filename, filename) == 0)
#else
					strcasecmp(m_content[i].Filename, filename) == 0)
#endif
				{
					*destination = (T*)m_content[i].Data;
					return m_content[i].State;
				}
			}

			return ContentState::NotLoaded;
		}

		void Tick()
		{
			// TODO
		}

	private:
		static ContentLoader* findLoader(LoaderType type);
	};
}

#endif // CONTENT_CONTENTMANAGER_H


#if 0
Asynchronous content loading

1) Open and map
2) Fix up data
3) Upload to OpenGL

Basically, there's a pipeline with n asynchronous steps between the time you call BeginLoad() and the time the resource is ready.

BeginLoad() <--starts an async load of a resource, no effect if the content is already loaded
	WaitForLoad() <--waits on an already started load until its ready
	Load() <--synchronous load, basically like calling BeginLoad() then WaitForLoad()
	Tick() <--called on the main thread, does OpenGL uploads and whatnot, anything that needs to happen on the main thread
	Get() <--retrieves the content, or null if it's not loaded

	struct Content
{
	char Filename[256];
	ContentType Type;
	void* Data;
	std::atomic<int> State; // 0 = incomplete, 1 = complete, -1 = error
}

Rules:

1) BeginLoad(), WaitForLoad(), and Load() must all be called from the main thread
2) Stuff is never added or removed from the queue other than from the main thread

How many threads ?

1) Main thread
2) File I / O(may need more than 1 ? )
3) Fixup(may need more than 1 ? )

Basically, it sounds like we need a worker thread pool with a dynamic number of threads.

struct Job
{
	int(*JobFunc)(Job*, bool workerThread);
	void* Destination;
	byte Data[64];
	std::atomic<int> State; // 0 = inactive, 1 = waiting on worker thread, 2 = worker thread in progress, 3 = waiting on main thread, 4 = main thread in progress, 5 = complete, 6 = error
}

And there's a job queue. Worker threads block until there's a job, then it grabs it and does it.

class JobQueue
{
	Job m_jobs[32];

	JobQueue()
	{
		memset(m_jobs, 0, sizeof(m_jobs));
	}

	void AddJob(JobFunc, Content* c)
	{
		int i = 0;
		while (true) // loop forever, waiting on a worker thread to finish
		{
			if (m_jobs[i].State == waiting on main thread)
			{
				m_jobs[i].State = main thread in progress;

				m_jobs[i].JobFunc(&m_jobs[i], false);

				m_jobs[i].State = inactive;
			}

			if (m_jobs[i].State == inactive || m_jobs[i].State == complete || m_jobs[i].State == error)
			{
				m_jobs[i].JobFunc = jobfunc;
				m_jobs[i].Destination = c;

				memory fence
					m_jobs[i].State = waitingOnWorkerThread;
			}

			i = (i + 1) % 32;
		}
	}

	void Tick()
	{
		for (int i = 0; i < 32; i++)
		{
			if (m_jobs[i].State == waitingOnMainThread)
			{
				m_jobs[i].JobFunc(&m_jobs[i], false);
				m_jobs[i].State = inactive;
			}
		}
	}
}

class ContentManager
{
	Content* BeginLoad(const char* file, ContentType type)
	{
		if content with the same name and type is already loaded then
			return it

			Content* c = new Content();
		c->State = 0;
		c->Filename = file;
		c->Type = type;

		add to the list of content

			auto loader = find loader for this file type;

		JobQueue::AddJob(loader, c);

		return c;
	}

	Content* WaitForLoad(Content* c)
	{
		while (c->State != complete && c->State != error)
		{
			Tick();
		}

		return c;
	}

	Content* Load(const char* file, ContentType type)
	{
		if content with the same name as type is alrady loaded then
			return it

			Content* c = new Content();
		c->State = 0;
		c->Filename = file;
		c->Type = type;

		add to the list of content

			auto loader = find loader for this file type;

		Job j = {};
		j.Filename = file;
		j.Destination = c;
		j.State = waiting on worker thread

			loader(&j, true);

		if (j.State == waiting on main thread)
			loader(&j, false);

		return c;
	}

	Content* Get(const char* file, ContentType type)
	{
		if content with the same name as type is alrady loaded then
			return it

			return null
	}
}

Questions:

Is file mapping always the best way to go ? Maybe it should be flexible enough to allow either, depending on the file type.OTOH, just about every file we load will need to be loaded fully.

WaitForLoad() blocks the main thread, but there may be resources that need to run on the main thread in order to load.Maybe it blocks until that particular content is ready for the main thread, then does whatever it needs to do, then returns.

How does the main thread remove items from the queue without screwing up the workers ? Maybe if it ALWAYS adds to the front and ALWAYS removes from the back it'll be ok.

# Sample #

Loading a texture :

struct TextureJobInfo
{
	int w, h;
	byte* pixels;
};

void LoadTextureJob(Job* j, bool workerThread)
{
	TextureJobInfo* ji = (TextureJobInfo*)j->Data;

	if (workerThread)
	{
		auto filename = j->Destination->Filename;
		open and map filename

			decode the file into RGBA data
			ji = decoded RGBA data

			memory fence
			j->State = waiting on main thread
	}
	else // main thread
	{
		create the OpenGL texture
			upload to OpenGL
			j->Destination->State = complete

			memory fence
			j->State = complete
	}
}

Worker thread proc

void ThreadProc(Job* j)
{
	wait until the bell rings

		while (we haven't gotten to the end of the queue)
		{
			if (interlockedCompareExchange(&j->State, 1, 0) == 0)
			{
				// we got the job!
				j->Jobfunc(j, true);
			}
		}
}

#endif
