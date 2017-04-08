#include "GameMain.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include "FileSystem.h"
#include "Graphics/Model.h"

Graphics::Model m;

void msg(Nxna::Graphics::GraphicsDeviceDebugMessage m)
{
	printf("%s\n", m.Message);
}

int Init(Nxna::Graphics::GraphicsDevice* device, WindowInfo* window, SpriteBatchData* sbd, Gui::TextPrinterData* tpd)
{
	Nxna::Graphics::GraphicsDeviceDesc gdesc = {};
	gdesc.Type = Nxna::Graphics::GraphicsDeviceType::OpenGl41;
	gdesc.ScreenWidth = window->ScreenWidth;
	gdesc.ScreenHeight = window->ScreenHeight;
	if (Nxna::Graphics::GraphicsDevice::CreateGraphicsDevice(&gdesc, device) != Nxna::NxnaResult::Success)
		return -1;

	device->SetMessageCallback(msg);

	Nxna::Graphics::Viewport vp(0, 0, (float)window->ScreenWidth, (float)window->ScreenHeight);
	device->SetViewport(vp);

	if (Gui::TextPrinter::Init(device, tpd) == false)
		return -1;

	SpriteBatchHelper::Init(device, sbd);


	File f;
	if (FileSystem::Open("Content/Models/Shark.obj", &f) == false)
	{
		printf("Unable to laod shark\n");
		return -1;
	}
	auto obj = FileSystem::MapFile(&f);
	if (Graphics::Model::LoadObj(device, (uint8*)obj, f.FileSize, &m) == false)
		return -1;
	FileSystem::Close(&f);

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

	Nxna::Matrix projection = Nxna::Matrix::CreatePerspectiveFieldOfView(60.0f * 0.0174533f, 640 / (float)480, 0.5f, 1000.0f);
	Nxna::Matrix view = Nxna::Matrix::CreateLookAt(Nxna::Vector3(0, 0, 10.0f), Nxna::Vector3(0, 0, 0), Nxna::Vector3(0, 1.0f, 0));
	auto transform = view * projection;
	

	Graphics::Model::Render(device, &transform, &m, 1);

	sb.Begin();
	Gui::TextPrinter::PrintScreen(&sb, tpd, 120, 120, Gui::FontType::Default, "Hello, world!");
	sb.End();

	device->Present();
}

#define NXNA2_IMPLEMENTATION
#include "MyNxna2.h"
#undef NXNA2_IMPLEMENTATION
