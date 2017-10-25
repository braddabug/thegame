#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include "Common.h"

struct SpriteBatchData;
struct StringManagerData;

namespace Gui
{
	struct TextPrinterData;
	struct ConsoleData;
	struct GuiManagerData;
}

namespace Content
{
	struct ContentManagerData;
	struct ContentLoaderData;
}

namespace Graphics
{
	struct ModelData;
	struct TextureLoaderData;
	struct ShaderLibraryData;
	struct DrawUtilsData;
}

namespace Audio
{
	struct AudioEngineData;
}

namespace Game
{
	struct SceneManagerData;
	struct CharacterManagerData;
	struct ScriptManagerData;
}

namespace Nxna
{
namespace Graphics
{
	class GraphicsDevice;
}
namespace Input
{
	class InputState;
}
}

struct Globals;
struct LogData;
struct JobQueueData;
struct MemoryManager;
struct PlatformInfo;
struct FileSystemData;
struct VirtualResolutionData;
struct WaitManagerData;

namespace Utils
{
	struct StopwatchData;
}

struct CursorInfo
{
	void* pCursor;
	void* pSurface;
};

struct PlatformInfo
{
	const char* (*GetClipboardText)();
	void(*FreeClipboardText)(const char*);

	bool (*CreateCursor)(uint8 width, uint8 height, uint32 hotX, uint32 hotY, uint8* pixels, CursorInfo* result);
	void (*FreeCursor)(CursorInfo* cursor);
	void (*SetCursor)(CursorInfo* cursor);
};

namespace Audio
{
	struct SoundManagerData;
}

struct GlobalData
{
	Globals* Global;
	MemoryManager* Memory;
	PlatformInfo* Platform;
	LogData* Log;
	JobQueueData* JobQueue;
	FileSystemData* FileSystem;
	StringManagerData* StringData;
	VirtualResolutionData* ResolutionData;
	WaitManagerData* WaitData;
	Nxna::Graphics::GraphicsDevice* Device;
	Nxna::Input::InputState* Input;
	Audio::AudioEngineData* Audio;
	SpriteBatchData* SpriteBatch;
	Gui::TextPrinterData* TextPrinter;
	Gui::ConsoleData* ConsoleData;
	Gui::GuiManagerData* GuiData;
	Content::ContentManagerData* ContentData;
	Content::ContentLoaderData* ContentLData;
	Graphics::ModelData* ModelData;
	Graphics::TextureLoaderData* TextureLoaderData;
	Graphics::ShaderLibraryData* ShaderLibraryData;
	Graphics::DrawUtilsData* DrawUtilsData;
	Game::SceneManagerData* SceneData;
	Game::CharacterManagerData* CharacterData;
	Game::ScriptManagerData* ScriptData;
	Utils::StopwatchData* StopwatchData;
	Audio::SoundManagerData* SoundData;
};


#endif // GLOBALDATA_H
