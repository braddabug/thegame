#if defined GAME_ENABLE_HOTLOAD && !defined GAME_ENABLE_HOTLOAD_DLL
// loader-only files

#include "GameLibLoader.cpp"
#include "Logging.cpp"

#endif


#if defined GAME_ENABLE_HOTLOAD_DLL || !defined GAME_ENABLE_HOTLOAD
// monolithic and game lib files

#include "GameMain.cpp"
#include "Logging.cpp"
#include "FileSystem.cpp"
#include "SpriteBatchHelper.cpp"
#include "WaitManager.cpp"
#include "JobQueue.cpp"
#include "Gui/TextPrinter.cpp"
#include "Graphics/Model.cpp"
#include "Graphics/TextureLoader.cpp"
#include "Content/ContentManager.cpp"
#include "Audio/AudioEngine.cpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Graphics/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Graphics/stb_image.h"

#endif

#if !defined GAME_ENABLE_HOTLOAD || !defined GAME_ENABLE_HOTLOAD_DLL
// loader and monolithic

#include "main.cpp"

#endif
