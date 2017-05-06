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
}

struct GlobalData
{
	Nxna::Graphics::GraphicsDevice* Device;
	SpriteBatchData* SpriteBatch;
	Gui::TextPrinterData* TextPrinter;
};

#endif // GLOBALDATA_H
