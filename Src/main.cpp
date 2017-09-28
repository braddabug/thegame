#include <cstdio>
#include <atomic>
#include "MyNxna2.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include SDL_HEADER
#include "utf8.h"

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

#undef None

extern PlatformInfo* g_platform;

const char* LocalGetClipboardText()
{
	return SDL_GetClipboardText();
}

void LocalFreeClipboardText(const char* text)
{
	SDL_free((char*)text);
}

bool LocalCreateCursor(uint8 width, uint8 height, uint32 hotX, uint32 hotY, uint8* pixels, CursorInfo* cursor)
{
	if (cursor == nullptr)
		return false;

	cursor->pSurface = SDL_CreateRGBSurfaceFrom(pixels, (int)width, (int)height, 32, (int)(width * 4), 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	if (cursor->pSurface == nullptr)
		return false;

	cursor->pCursor = SDL_CreateColorCursor((SDL_Surface*)cursor->pSurface, (int)hotX, (int)hotY);
	if (cursor->pCursor == nullptr)
		return false;

	return true;
}

void LocalFreeCursor(CursorInfo* cursor)
{
	SDL_FreeCursor((SDL_Cursor*)cursor->pCursor);
	SDL_FreeSurface((SDL_Surface*)cursor->pSurface);
}

void LocalSetCursor(CursorInfo* cursor)
{
	SDL_SetCursor((SDL_Cursor*)cursor->pCursor);
}

int LocalInit(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	return GAME_LIB_CALL(Init)(window);
}

void LocalShutdown(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	GAME_LIB_CALL(Shutdown)();
}

void LocalTick(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd, float elapsed)
{
	GAME_LIB_CALL(Tick)(elapsed);
}

void LocalHandleEvent(ExternalEvent e)
{
	GAME_LIB_CALL(HandleExternalEvent)(e);
}

Nxna::Input::Key ConvertSDLKey(SDL_Keycode key)
{
	switch (key)
	{
	case SDLK_BACKSPACE:
		return Nxna::Input::Key::Back;
	case SDLK_RETURN:
		return Nxna::Input::Key::Enter;
	case SDLK_ESCAPE:
		return Nxna::Input::Key::Escape;
	case SDLK_SPACE:
		return Nxna::Input::Key::Space;
	case SDLK_BACKQUOTE:
		return Nxna::Input::Key::OemTilde;
	case SDLK_LSHIFT:
		return Nxna::Input::Key::LeftShift;
	case SDLK_RSHIFT:
		return Nxna::Input::Key::RightShift;
	case SDLK_LCTRL:
		return Nxna::Input::Key::LeftControl;
	case SDLK_RCTRL:
		return Nxna::Input::Key::RightControl;
	case SDLK_UP:
		return Nxna::Input::Key::Up;
	case SDLK_DOWN:
		return Nxna::Input::Key::Down;
	case SDLK_RIGHT:
		return Nxna::Input::Key::Right;
	case SDLK_LEFT:
		return Nxna::Input::Key::Left;
	case SDLK_PERIOD:
		return Nxna::Input::Key::OemPeriod;
	case SDLK_SLASH:
		return Nxna::Input::Key::OemQuestion;
	case SDLK_PLUS:
		return Nxna::Input::Key::OemPlus;
	case SDLK_MINUS:
		return Nxna::Input::Key::OemMinus;
	case SDLK_EQUALS:
		return Nxna::Input::Key::OemPlus;
	case SDLK_QUOTE:
		return Nxna::Input::Key::OemQuotes;
	case SDLK_SEMICOLON:
		return Nxna::Input::Key::OemSemicolon;
	case SDLK_BACKSLASH:
		return Nxna::Input::Key::OemBackslash;
	case SDLK_F1:
		return Nxna::Input::Key::F1;
	case SDLK_F2:
		return Nxna::Input::Key::F2;
	case SDLK_F3:
		return Nxna::Input::Key::F3;
	case SDLK_F4:
		return Nxna::Input::Key::F4;
	case SDLK_F5:
		return Nxna::Input::Key::F5;
	case SDLK_F6:
		return Nxna::Input::Key::F6;
	case SDLK_F7:
		return Nxna::Input::Key::F7;
	case SDLK_F8:
		return Nxna::Input::Key::F8;
	case SDLK_F9:
		return Nxna::Input::Key::F9;
	case SDLK_F10:
		return Nxna::Input::Key::F10;
	case SDLK_F11:
		return Nxna::Input::Key::F11;
	case SDLK_F12:
		return Nxna::Input::Key::F12;
	case SDLK_DELETE:
		return Nxna::Input::Key::Delete;
	case SDLK_INSERT:
		return Nxna::Input::Key::Insert;
	default:
		if (key >= SDLK_a && key <= SDLK_z)
			return (Nxna::Input::Key)((int)Nxna::Input::Key::A + (key - SDLK_a));
		if (key >= SDLK_0 && key <= SDLK_9)
			return (Nxna::Input::Key)((int)Nxna::Input::Key::D0 + (key - SDLK_0));

		return Nxna::Input::Key::None;
	}
}

struct CommandLineOptions
{
	uint32 ScreenWidth;
	uint32 ScreenHeight;
	uint32 MultisampleLevel;
};

void ParseCommandLineOptions(int argc, char* argv[], CommandLineOptions* result)
{
	// first set the defaults
	result->ScreenWidth = 640;
	result->ScreenHeight = 480;
	result->MultisampleLevel = 16;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-w") == 0)
		{
			i++;

			if (i < argc)
			{
				result->ScreenWidth = (uint32)strtol(argv[i], nullptr, 10);
			}
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			i++;

			if (i < argc)
			{
				result->ScreenHeight = (uint32)strtol(argv[i], nullptr, 10);
			}
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			i++;

			if (i < argc)
			{
				result->MultisampleLevel = (uint32)strtol(argv[i], nullptr, 10);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	CommandLineOptions options;
	ParseCommandLineOptions(argc, argv, &options);

	MemoryManager mem;
	g_memory = &mem;

	MemoryManagerInternal::SetDefaults(g_memory);
	/*mem.Alloc = MemoryManagerInternal::Alloc;
	mem.AllocTrack = MemoryManagerInternal::AllocTrack;
	mem.Realloc = MemoryManagerInternal::Realloc;
	mem.ReallocTrack = MemoryManagerInternal::ReallocTrack;
	mem.Free = MemoryManagerInternal::Free;
	mem.FreeTrack = MemoryManagerInternal::FreeTrack;
	mem.AllocAndKeep = MemoryManagerInternal::AllocAndKeep;*/

	memset(&gd, 0, sizeof(GlobalData));
	gd.Memory = g_memory;

	MemoryManagerInternal::Initialize();

	PlatformInfo platform;
	platform.GetClipboardText = LocalGetClipboardText;
	platform.FreeClipboardText = LocalFreeClipboardText;
	platform.CreateCursor = LocalCreateCursor;
	platform.FreeCursor = LocalFreeCursor;
	platform.SetCursor = LocalSetCursor;
	g_platform = &platform;
	gd.Platform = g_platform;

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

	int screenWidth = options.ScreenWidth;
	int screenHeight = options.ScreenHeight;

	if (options.MultisampleLevel > 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
	}

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
			WriteLog(gd.Log, LogSeverityType::Warning, LogChannelType::Unknown, "Unable to enable vsync");
		else
			WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Vsync enabled");
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

	uint32 prevTicks = SDL_GetTicks();
	while (true)
	{
		uint32 elapsedTicks = SDL_GetTicks();
		float elapsedTime = (elapsedTicks - prevTicks) / 1000.0f;
		prevTicks = elapsedTicks;

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
				ee.Type = ExternalEventType::MouseButtonUp;
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
				ee.KeyboardButton.Key = ConvertSDLKey(e.key.keysym.sym);
				LocalHandleEvent(ee);
			}
				break;
			case SDL_KEYUP:
			{
				ExternalEvent ee = {};
				ee.Type = ExternalEventType::KeyboardButtonUp;
				ee.KeyboardButton.PlatformKey = e.key.keysym.scancode;
				ee.KeyboardButton.ConvertedKey = e.key.keysym.sym;
				ee.KeyboardButton.Key = ConvertSDLKey(e.key.keysym.sym);
				LocalHandleEvent(ee);
			}
				break;
			case SDL_TEXTINPUT:
			{
				int unicode;
				utf8codepoint(e.text.text, &unicode);

				ExternalEvent ee = {};
				ee.Type = ExternalEventType::TextInput;
				ee.TextInput.Unicode = unicode;
				LocalHandleEvent(ee);
			}
				break;
			}
		}

		LocalTick(&device, &sbd, &td, elapsedTime);

		SDL_GL_SwapWindow(window);
	}

end:
	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Shutting down...");

	LocalShutdown(&device, &sbd, &td);

	SDL_DestroyWindow(window);
	SDL_Quit();

	WriteLog(gd.Log, LogSeverityType::Normal, LogChannelType::Unknown, "Bye! Hope you had fun!");

	for (uint32 i = 0; i < LogData::NumLinePages; i++)
		g_memory->FreeTrack(gd.Log->LineDataPages[i], __FILE__, __LINE__);
	g_memory->FreeTrack(gd.Log, __FILE__, __LINE__);

	size_t memoryUsage;
	MemoryManagerInternal::GetMemoryUsage(&memoryUsage);
	MemoryManagerInternal::DumpReport("memory.json");
	MemoryManagerInternal::Shutdown();

	printf("Memory usage at exit: %u\n", (uint32)memoryUsage);

	return 0;
}

