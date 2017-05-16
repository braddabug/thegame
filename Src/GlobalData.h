#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include "Common.h"

struct SpriteBatchData;

namespace Gui
{
	struct TextPrinterData;
}

namespace Content
{
	struct ContentManagerData;
}

namespace Graphics
{
	struct ModelData;
	struct TextureLoaderData;
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

struct LogData;
struct JobQueueData;

struct GlobalData
{
	LogData* Log;
	JobQueueData* JobQueue;
	Nxna::Graphics::GraphicsDevice* Device;
	Nxna::Input::InputState* Input;
	SpriteBatchData* SpriteBatch;
	Gui::TextPrinterData* TextPrinter;
	Content::ContentManagerData* ContentData;
	Graphics::ModelData* ModelData;
	Graphics::TextureLoaderData* TextureLoaderData;
};


#endif // GLOBALDATA_H
