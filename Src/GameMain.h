#ifndef GAMEMAIN_H
#define GAMEMAIN_H

#include "MyNxna2.h"

enum class ExternalEventType
{
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

MG_EXPORT
int Init(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);

MG_EXPORT
void Shutdown(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);

MG_EXPORT
void Tick(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);

#ifdef GAME_ENABLE_HOTLOAD
}
#endif

#endif // GAMEMAIN_H
