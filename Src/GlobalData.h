#ifndef GLOBALDATA_H
#define GLOBALDATA_H

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

struct GlobalData
{
	Nxna::Graphics::GraphicsDevice* Device;
	Nxna::Input::InputState* Input;
	SpriteBatchData* SpriteBatch;
	Gui::TextPrinterData* TextPrinter;
	Content::ContentManagerData* ContentData;
	Graphics::ModelData* ModelData;
};

#endif // GLOBALDATA_H
