#ifndef GAMELIBLOADER_H
#define GAMELIBLOADER_H

#include "GameMain.h"
#include "Gui/TextPrinter.h"

struct SpriteBatchData;

struct GameLib
{
	int(*Init)(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);
	void(*Loaded)(void* data, bool initial);
	void(*ContentLoaded)(void* data);
	void(*Tick)(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);
	void(*Shutdown)(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd);
	void(*Unloading)(void* data);
	void(*HandleExternalEvent)(void* data, ExternalEvent e);

#ifdef _WIN32
	void* Lib;
#else

#endif
};

bool LoadGameLib(GameLib* result);
bool UnloadGameLib(GameLib* lib);

#endif // GAMELIBLOADER_H
