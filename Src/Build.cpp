#if defined GAME_ENABLE_HOTLOAD && !defined GAME_ENABLE_HOTLOAD_DLL
// loader-only files

#include "GameLibLoader.cpp"
#endif


#if defined GAME_ENABLE_HOTLOAD_DLL || !defined GAME_ENABLE_HOTLOAD
// monolithic and game lib files

#include "GameMain.cpp"
#include "FileSystem.cpp"
#include "SpriteBatchHelper.cpp"
#include "Gui/TextPrinter.cpp"
#include "Graphics/Model.cpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Graphics/tiny_obj_loader.h"

#endif

#if !defined GAME_ENABLE_HOTLOAD || !defined GAME_ENABLE_HOTLOAD_DLL
// loader and monolithic

#include "main.cpp"


#endif
