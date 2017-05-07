#ifndef GAMEMAIN_H
#define GAMEMAIN_H

#include "MyNxna2.h"

enum class ExternalEventType
{
	FrameStart,
	MouseMove,
	MouseWheel,
	MouseButtonDown,
	MouseButtonUp,
	KeyboardButtonDown,
	KeyboardButtonUp
};
struct ExternalEvent
{
	ExternalEventType Type;
	union
	{
		struct
		{
			int X, Y;
		} MouseMove;
		struct
		{
			int Button;
		} MouseButton;
		struct
		{
			int Delta;
		} MouseWheel;
		struct
		{
			unsigned int PlatformKey;
			unsigned int ConvertedKey;
			unsigned int UnicodeCharacter;
		} KeyboardButton;
	};
};

struct WindowInfo
{
	int ScreenWidth;
	int ScreenHeight;
};

namespace Gui
{
	struct TextPrinterData;
}
struct SpriteBatchData;

#ifdef GAME_ENABLE_HOTLOAD
extern "C"
{
#ifdef _WIN32
#define MG_EXPORT __declspec(dllexport)
#else
#define MG_EXPORT __attribute__ ((visibility ("default")))
#endif
#else
#define MG_EXPORT
#endif

struct GlobalData;

MG_EXPORT
void LibLoaded(GlobalData* data, bool initial);

MG_EXPORT
int Init(WindowInfo* window);

MG_EXPORT
void Shutdown();

MG_EXPORT
void Tick();

MG_EXPORT
void HandleExternalEvent(ExternalEvent e);

#ifdef GAME_ENABLE_HOTLOAD
}
#endif

#endif // GAMEMAIN_H
