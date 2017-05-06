#include "GameMain.h"
#include "GlobalData.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include "FileSystem.h"
#include "Graphics/Model.h"

Graphics::Model m;

Nxna::Graphics::GraphicsDevice* g_device;

void msg(Nxna::Graphics::GraphicsDeviceDebugMessage m)
{
	printf("%s\n", m.Message);
}

void LibLoaded(GlobalData* data, bool initial)
{
	if (data->Device == nullptr)
	{
		g_device = new Nxna::Graphics::GraphicsDevice();
		data->Device = g_device;
	}
	else
	{
		g_device = data->Device;
	}

	SpriteBatchHelper::SetGlobalData(&data->SpriteBatch);
	Gui::TextPrinter::SetGlobalData(&data->TextPrinter);
}

int Init(WindowInfo* window)
{
	Nxna::Graphics::GraphicsDeviceDesc gdesc = {};
	gdesc.Type = Nxna::Graphics::GraphicsDeviceType::OpenGl41;
	gdesc.ScreenWidth = window->ScreenWidth;
	gdesc.ScreenHeight = window->ScreenHeight;
	if (Nxna::Graphics::GraphicsDevice::CreateGraphicsDevice(&gdesc, g_device) != Nxna::NxnaResult::Success)
		return -1;

	g_device->SetMessageCallback(msg);

	Nxna::Graphics::Viewport vp(0, 0, (float)window->ScreenWidth, (float)window->ScreenHeight);
	g_device->SetViewport(vp);

	if (Gui::TextPrinter::Init(g_device) == false)
		return -1;

	SpriteBatchHelper::Init(g_device);


	File f;
	if (FileSystem::OpenAndMap("Content/Models/Shark.obj", &f) == nullptr)
	{
		printf("Unable to laod shark\n");
		return -1;
	}
	if (Graphics::Model::LoadObj(g_device, (uint8*)f.Memory, f.FileSize, &m) == false)
	{
		FileSystem::Close(&f);
		return -1;
	}
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

void Shutdown()
{
	Gui::TextPrinter::Shutdown();

	SpriteBatchHelper::Destroy();

	Nxna::Graphics::GraphicsDevice::DestroyGraphicsDevice(g_device);
}

void Tick()
{
	g_device->ClearColor(0, 0, 0, 0);
	g_device->ClearDepthStencil(true, true, 1.0f, 0);

	SpriteBatchHelper sb;

	Nxna::Matrix projection = Nxna::Matrix::CreatePerspectiveFieldOfView(60.0f * 0.0174533f, 640 / (float)480, 0.5f, 1000.0f);
	Nxna::Matrix view = Nxna::Matrix::CreateLookAt(Nxna::Vector3(0, 0, 10.0f), Nxna::Vector3(0, 0, 0), Nxna::Vector3(0, 1.0f, 0));
	auto transform = view * projection;
	

	Graphics::Model::Render(g_device, &transform, &m, 1);

	sb.Begin();
	Gui::TextPrinter::PrintScreen(&sb, 120, 120, Gui::FontType::Default, "Hello, world!");
	sb.End();

	g_device->Present();
}

#define NXNA2_IMPLEMENTATION
#include "MyNxna2.h"
#undef NXNA2_IMPLEMENTATION
