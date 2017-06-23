#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include "Common.h"

struct SpriteBatchData;
struct StringManagerData;

namespace Gui
{
	struct TextPrinterData;
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
struct FileSystemData;

struct GlobalData
{
	Globals* Global;
	MemoryManager* Memory;
	LogData* Log;
	JobQueueData* JobQueue;
	FileSystemData* FileSystem;
	StringManagerData* StringData;
	Nxna::Graphics::GraphicsDevice* Device;
	Nxna::Input::InputState* Input;
	Audio::AudioEngineData* Audio;
	SpriteBatchData* SpriteBatch;
	Gui::TextPrinterData* TextPrinter;
	Content::ContentManagerData* ContentData;
	Content::ContentLoaderData* ContentLData;
	Graphics::ModelData* ModelData;
	Graphics::TextureLoaderData* TextureLoaderData;
	Graphics::ShaderLibraryData* ShaderLibraryData;
	Graphics::DrawUtilsData* DrawUtilsData;
	Game::SceneManagerData* SceneData;
};


#endif // GLOBALDATA_H
