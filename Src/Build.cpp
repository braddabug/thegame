#if defined GAME_ENABLE_HOTLOAD && !defined GAME_ENABLE_HOTLOAD_DLL
// loader-only files

#include "GameLibLoader.cpp"
#include "Logging.cpp"

#endif


#if defined GAME_ENABLE_HOTLOAD_DLL || !defined GAME_ENABLE_HOTLOAD
// monolithic and game lib files


#include "Utils.cpp"
#include "GameMain.cpp"
#include "Logging.cpp"
#include "StringManager.cpp"
#include "FileSystem.cpp"
#include "FileFinder.cpp"
#include "SpriteBatchHelper.cpp"
#include "WaitManager.cpp"
#include "JobQueue.cpp"
#include "VirtualResolution.cpp"
#include "HashStringManager.cpp"
#include "Gui/TextPrinter.cpp"
#include "Gui/Console.cpp"
#include "Gui/GuiManager.cpp"
#include "Graphics/Model.cpp"
#include "Graphics/TextureLoader.cpp"
#include "Graphics/ShaderLibrary.cpp"
#include "Graphics/DrawUtils.cpp"
#include "Content/ContentLoader.cpp"
#include "Content/ContentManager.cpp"
#include "Audio/AudioEngine.cpp"
#include "Audio/SoundManager.cpp"
#include "Audio/SongPlayer.cpp"
#include "Game/SceneManager.cpp"
#include "Game/CharacterManager.cpp"
#include "Game/ScriptManager.cpp"
#include "Game/NavMesh.cpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Graphics/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Graphics/stb_image.h"

#define INIPARSE_IMPLEMENTATION
#include "iniparse.h"

#endif

#if !defined GAME_ENABLE_HOTLOAD || !defined GAME_ENABLE_HOTLOAD_DLL
// loader and monolithic

#include "main.cpp"
#include "MemoryManager.cpp"

#endif

MemoryManager* g_memory;
PlatformInfo* g_platform;