#include <cstdio>
#include "MyNxna2.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include SDL_HEADER

#include "GameMain.h"
#include "GlobalData.h"

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
	memset(&gd, 0, sizeof(GlobalData));

#ifdef GAME_ENABLE_HOTLOAD
	if (LoadGameLib(&g_lib) == false)
	{
		printf("Unable to load game library\n");
		return -1;
	}
#endif

	GAME_LIB_CALL(LibLoaded)(&gd, true);

	SDL_Init(SDL_INIT_VIDEO);

#ifdef GAME_ENABLE_HOTLOAD
	const char* title = "The Game (hotload)";
#else
	const char* title = "The Game (monolithic)";
#endif

	auto window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
	if (window == nullptr)
	{
		printf("Unable to create SDL window\n");
		return -1;
	}

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
		return -1;

	// enable vsync
	if (SDL_GL_SetSwapInterval(-1) != 0)
		SDL_GL_SetSwapInterval(1);

	Gui::TextPrinterData td = {};
	SpriteBatchData sbd = {};

	Nxna::Graphics::GraphicsDevice device;
	WindowInfo wi;
	wi.ScreenWidth = 640;
	wi.ScreenHeight = 480;
	if (LocalInit(&device, &wi, &sbd, &td) != 0)
	{
		printf("Unable to initialize. Exiting...\n");
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

	LocalShutdown(&device, &sbd, &td);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

