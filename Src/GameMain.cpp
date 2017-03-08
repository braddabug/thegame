#include "GameMain.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"

int Init(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	Nxna::Graphics::GraphicsDeviceDesc gdesc = {};
	gdesc.Type = Nxna::Graphics::GraphicsDeviceType::OpenGl41;
	gdesc.ScreenWidth = window->ScreenWidth;
	gdesc.ScreenHeight = window->ScreenHeight;
	if (Nxna::Graphics::GraphicsDevice::CreateGraphicsDevice(&gdesc, device) != Nxna::NxnaResult::Success)
		return -1;

	Nxna::Graphics::Viewport vp(0, 0, (float)window->ScreenWidth, (float)window->ScreenHeight);
	device->SetViewport(vp);

	if (Gui::TextPrinter::Init(device, tpd) == false)
		return -1;

	SpriteBatchHelper::Init(device, sbd);

	return 0;
}

int Load()
{
	return 0;
}

int Unload()
{
	return 0;
}

void Shutdown(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	Gui::TextPrinter::Shutdown(tpd);

	SpriteBatchHelper::Destroy(sbd);

	Nxna::Graphics::GraphicsDevice::DestroyGraphicsDevice(device);
}

void Tick(Nxna::Graphics::GraphicsDevice* device, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	device->ClearColor(0, 0, 0, 0);
	device->ClearDepthStencil(true, true, 1.0f, 0);

	SpriteBatchHelper sb;
	SpriteBatchHelper::Init(sbd, &sb);

	sb.Begin();
	Gui::TextPrinter::PrintScreen(&sb, tpd, 120, 120, Gui::FontType::Default, "Hello, world!");
	sb.End();

	device->Present();
}

#define NXNA2_IMPLEMENTATION
#include "MyNxna2.h"
#undef NXNA2_IMPLEMENTATION