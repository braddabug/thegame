#ifndef GLOBALDATA_H
#define GLOBALDATA_H

struct SpriteBatchData;

namespace Gui
{
	struct TextPrinterData;
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
};

#endif // GLOBALDATA_H
