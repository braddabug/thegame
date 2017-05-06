#include "GameLibLoader.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifdef _WIN32
#define LOAD(ptr, lib, name) ptr = (decltype(ptr))GetProcAddress((HMODULE)lib, name)
#endif

bool LoadGameLib(GameLib* result)
{
#ifdef _WIN32
	char moduleName[MAX_PATH];
	if (GetModuleFileName(nullptr, moduleName, MAX_PATH) == 0)
		return false;

	// find the filename and replace it with the dll name
	char* slash = strrchr(moduleName, '\\');
	slash[0] = 0;
	strcat_s(moduleName, "\\GameLib.dll");

	result->Lib = LoadLibraryA(moduleName);
	if (result->Lib == nullptr)
		return false;

	LOAD(result->Init, result->Lib, "Init");
	LOAD(result->LibLoaded, result->Lib, "LibLoaded");
	LOAD(result->ContentLoaded, result->Lib, "ContentLoaded");
	LOAD(result->Tick, result->Lib, "Tick");
	LOAD(result->Shutdown, result->Lib, "Shutdown");
	LOAD(result->Unloading, result->Lib, "Unloading");
	LOAD(result->HandleExternalEvent, result->Lib, "HandleExternalEvent");

	if (result->Init == nullptr ||
		result->LibLoaded == nullptr ||
		//result->ContentLoaded == nullptr ||
		result->Tick == nullptr ||
		result->Shutdown == nullptr //||
	//	result->Unloading == nullptr ||
	//	result->HandleExternalEvent == nullptr
		)
		return false;

	return true;

#else
	return false;
#endif
}

bool UnloadGameLib(GameLib* lib)
{
#ifdef _WIN32
	return FreeLibrary((HMODULE)lib->Lib) ? true : false;
#else
	return false;
#endif
}

