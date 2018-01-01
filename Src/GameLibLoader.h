#ifndef GAMELIBLOADER_H
#define GAMELIBLOADER_H

#include "GameMain.h"
#include "Gui/TextPrinter.h"

struct GlobalData;

struct GameLib
{
	// LibLoaded() is called when the game lib is loaded, and also when it's reloaded.
	void(*LibLoaded)(GlobalData* data, bool initial);
	int(*Init)(WindowInfo* window);
	void(*ContentLoaded)();
	void(*TickFixed)(float elapsed);
	void(*Tick)(float elapsed);
	void(*Shutdown)();
	void(*Unloading)();
	void(*HandleExternalEvent)(ExternalEvent e);

#ifdef _WIN32
	void* Lib;
#else

#endif
};

bool LoadGameLib(GameLib* result);
bool UnloadGameLib(GameLib* lib);

#endif // GAMELIBLOADER_H
