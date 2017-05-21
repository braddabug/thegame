#include <cstdio>
#include <atomic>
#include "MyNxna2.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include SDL_HEADER

#include "GameMain.h"
#include "GlobalData.h"
#include "Logging.h"
#include "MemoryManager.h"

#ifdef GAME_ENABLE_HOTLOAD
#include "GameLibLoader.h"

GameLib g_lib;
#define GAME_LIB_CALL(n) g_lib.n
#else
#define GAME_LIB_CALL(n) n
#endif

GlobalData gd;

int LocalInit(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	return GAME_LIB_CALL(Init)(window);
}

void LocalShutdown(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	GAME_LIB_CALL(Shutdown)();
}

void LocalTick(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	GAME_LIB_CALL(Tick)();
}

void LocalHandleEvent(ExternalEvent e)
{
	GAME_LIB_CALL(HandleExternalEvent)(e);
}

int main(int argc, char* argv[])
{
	MemoryManager mem;
	g_memory = &mem;

	mem.Alloc = MemoryManagerInternal::Alloc;
	mem.AllocTrack = MemoryManagerInternal::AllocTrack;
	mem.Realloc = MemoryManagerInternal::Realloc;
	mem.Free = MemoryManagerInternal::Free;
	mem.FreeTrack = MemoryManagerInternal::FreeTrack;

	memset(&gd, 0, sizeof(GlobalData));
	gd.Memory = g_memory;

	// create the log data
	gd.Log = (LogData*)mem.AllocTrack(sizeof(LogData), __FILE__, __LINE__);
	memset(gd.Log, 0, sizeof(LogData));
	for (uint32 i = 0; i < LogData::NumLinePages; i++)
		gd.Log->LineDataPages[i] = (char*)g_memory->AllocTrack(LogData::LineDataSize, __FILE__, __LINE__);
	gd.Log->Lock.clear(); // TODO: the docs say to set this to ATOMIC_FLAG_INIT, but that won't compile in VS2015

	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Welcome to The Game!");
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Built %s %s", __DATE__, __TIME__);

#ifdef GAME_ENABLE_HOTLOAD
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Loading game library...");
	if (LoadGameLib(&g_lib) == false)
	{
		WriteLog(gd.Log, LogSeverityType::Error, LogChannelType::Unknown, "Unable to load game library");
		return -1;
	}
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Game library loaded");
#endif

	GAME_LIB_CALL(LibLoaded)(&gd, true);

	SDL_Init(SDL_INIT_VIDEO);

#ifdef GAME_ENABLE_HOTLOAD
	const char* title = "The Game (hotload)";
#else
	const char* title = "The Game (monolithic)";
#endif

	int screenWidth = 640;
	int screenHeight = 480;

	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Creating window...");
	auto window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
	if (window == nullptr)
	{
		WriteLog(gd.Log, LogSeverityType::Error, LogChannelType::Unknown, "Unable to create SDL window");
		return -1;
	}
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Window created. Size: %d x %d", screenWidth, screenHeight);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	auto context = SDL_GL_CreateContext(window);
	if (context == nullptr)
	{
		WriteLog(gd.Log, LogSeverityType::Error, LogChannelType::Unknown, "Unable to create OpenGL context");
		return -1;
	}

	// enable vsync
	if (SDL_GL_SetSwapInterval(-1) != 0)
	{
		if (SDL_GL_SetSwapInterval(1) != 0)
			WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Unable to enable vsync");
		else
			WriteLog(gd.Log, LogSeverityType::Error, LogChannelType::Unknown, "Vsync enabled");
	}
	else
	{
		WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Adaptive vsync enabled");
	}

	Gui::TextPrinterData td = {};
	SpriteBatchData sbd = {};

	Nxna::Graphics::GraphicsDevice device;
	WindowInfo wi;
	wi.ScreenWidth = screenWidth;
	wi.ScreenHeight = screenHeight;
	if (LocalInit(&device, &wi, &sbd, &td) != 0)
	{
		WriteLog(gd.Log, LogSeverityType::Error, LogChannelType::Unknown, "Unable to initialize. Exiting...");
		return -1;
	}

	while (true)
	{
		{
			ExternalEvent ee = {};
			ee.Type = ExternalEventType::FrameStart;
			LocalHandleEvent(ee);
		}

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				goto end;
			case SDL_MOUSEMOTION:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::MouseMove;
				ee.MouseMove.X = e.motion.x;
				ee.MouseMove.Y = e.motion.y;
				LocalHandleEvent(ee);
			}
				break;
			case SDL_MOUSEBUTTONDOWN:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::MouseButtonDown;
				ee.MouseButton.Button = e.button.button;
				LocalHandleEvent(ee);
			}
				break;
			case SDL_MOUSEBUTTONUP:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::MouseButtonDown;
				ee.MouseButton.Button = e.button.button;
				LocalHandleEvent(ee);
			}
				break;
			case SDL_MOUSEWHEEL:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::MouseWheel;
				ee.MouseWheel.Delta = e.wheel.y;
				LocalHandleEvent(ee);
			}
				break;
			case SDL_KEYDOWN:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::KeyboardButtonDown;
				ee.KeyboardButton.PlatformKey = e.key.keysym.scancode;
				ee.KeyboardButton.ConvertedKey = e.key.keysym.sym;
				LocalHandleEvent(ee);
			}
				break;
			case SDL_KEYUP:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::KeyboardButtonUp;
				ee.KeyboardButton.PlatformKey = e.key.keysym.scancode;
				ee.KeyboardButton.ConvertedKey = e.key.keysym.sym;
				LocalHandleEvent(ee);
			}
			break;
			}
		}

		LocalTick(&device, &sbd, &td);

		SDL_GL_SwapWindow(window);
	}

end:
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Shutting down...");

	LocalShutdown(&device, &sbd, &td);

	SDL_DestroyWindow(window);
	SDL_Quit();

	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Bye! Hope you had fun!");

	size_t memoryUsage;
	MemoryManagerInternal::GetMemoryUsage(&memoryUsage);

	printf("Memory usage at exit: %u\n", (uint32)memoryUsage);

	return 0;
}

