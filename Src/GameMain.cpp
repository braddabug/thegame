#include "GameMain.h"
#include "GlobalData.h"
#include "Logging.h"
#include "StringManager.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include "FileSystem.h"
#include "Graphics/Model.h"
#include "Graphics/TextureLoader.h"
#include "Content/ContentManager.h"
#include "Audio/AudioEngine.h"
#include "MemoryManager.h"



Graphics::Model m;
JobInfo mj;

Nxna::Graphics::GraphicsDevice* g_device;
Nxna::Input::InputState* g_inputState;
Content::ContentManager g_content;
extern LogData* g_log;

void msg(Nxna::Graphics::GraphicsDeviceDebugMessage m)
{
	printf("%s\n", m.Message);
}

void LibLoaded(GlobalData* data, bool initial)
{
	g_memory = data->Memory;

	if (data->Device == nullptr)
	{
		g_device = (Nxna::Graphics::GraphicsDevice*)g_memory->AllocTrack(sizeof(Nxna::Graphics::GraphicsDevice), __FILE__, __LINE__);
		data->Device = g_device;
	}
	else
	{
		g_device = data->Device;
	}

	if (data->Input == nullptr)
	{
		g_inputState = (Nxna::Input::InputState*)g_memory->AllocTrack(sizeof(Nxna::Input::InputState), __FILE__, __LINE__);
		data->Input = g_inputState;
	}
	else
	{
		g_inputState = data->Input;
	}

	// we always expect the log data to be created
	g_log = data->Log;

	StringManager::SetGlobalData(&data->StringData);
	JobQueue::SetGlobalData(&data->JobQueue);
	Audio::AudioEngine::SetGlobalData(&data->Audio);
	SpriteBatchHelper::SetGlobalData(&data->SpriteBatch);
	Gui::TextPrinter::SetGlobalData(&data->TextPrinter);
	Content::ContentManager::SetGlobalData(&data->ContentData, g_device);
	Content::ContentLoader::SetGlobalData(&data->ContentLData, g_device);
	Graphics::Model::SetGlobalData(&data->ModelData);
	Graphics::TextureLoader::SetGlobalData(&data->TextureLoaderData, g_device);
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

	if (Audio::AudioEngine::Init() == false)
		return -1;

	//auto c = Content::ContentManager::Load("Content/Models/Shark.obj", Content::LoaderType::ModelObj, &m);
	//if (c != Content::ContentState::Loaded)
	//	return -1;

	auto c = Content::ContentLoader::BeginLoad("Content/Models/Shark.obj", Content::LoaderType::ModelObj, &m, &mj);
	if (c != Content::ContentState::Incomplete)
		return -1;

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
	g_memory->FreeTrack(g_inputState, __FILE__, __LINE__);
	g_memory->FreeTrack(g_device, __FILE__, __LINE__);

	Graphics::Model::Shutdown();
	Graphics::TextureLoader::Shutdown();
	Content::ContentLoader::Shutdown();

	JobQueue::Shutdown(true);

	Audio::AudioEngine::Shutdown();

	Gui::TextPrinter::Shutdown();

	SpriteBatchHelper::Destroy();

	Nxna::Graphics::GraphicsDevice::DestroyGraphicsDevice(g_device);
}

float rotation = 0;
void Tick()
{
	JobQueue::Tick();

	g_device->ClearColor(0, 0, 0, 0);
	g_device->ClearDepthStencil(true, true, 1.0f, 0);

	SpriteBatchHelper sb;

	rotation = g_inputState->MouseX * 0.01f;
	Nxna::Vector2 cameraPos = Nxna::Vector2(0, 10.0f).Rotate(rotation);
	Nxna::Matrix projection = Nxna::Matrix::CreatePerspectiveFieldOfView(60.0f * 0.0174533f, 640 / (float)480, 0.5f, 1000.0f);
	Nxna::Matrix view = Nxna::Matrix::CreateLookAt(Nxna::Vector3(cameraPos.X, 0, cameraPos.Y), Nxna::Vector3(0, 0, 0), Nxna::Vector3(0, 1.0f, 0));
	auto transform = view * projection;
	
	if (mj.Result == JobResult::Completed)
		Graphics::Model::Render(g_device, &transform, &m, 1);

	sb.Begin();
	Gui::TextPrinter::PrintScreen(&sb, 0, 20, Gui::FontType::Default, "Hello, world!");
	sb.End();

	g_device->Present();
}

void HandleExternalEvent(ExternalEvent e)
{
	switch (e.Type)
	{
	case ExternalEventType::FrameStart:
		Nxna::Input::InputState::FrameReset(g_inputState);
		break;
	case ExternalEventType::MouseMove:
		Nxna::Input::InputState::InjectMouseMove(g_inputState, e.MouseMove.X, e.MouseMove.Y);
		break;
	case ExternalEventType::MouseButtonDown:
		Nxna::Input::InputState::InjectMouseButtonEvent(g_inputState, true, e.MouseButton.Button);
		break;
	case ExternalEventType::MouseButtonUp:
		Nxna::Input::InputState::InjectMouseButtonEvent(g_inputState, false, e.MouseButton.Button);
		break;
	case ExternalEventType::KeyboardButtonDown:
		//Nxna::Input::InputState::InjectKeyEvent(g_inputState, true, e.KeyboardButton.PlatformKey);
		break;
	case ExternalEventType::KeyboardButtonUp:
		//Nxna::Input::InputState::InjectKeyEvent(g_inputState, false, e.KeyboardButton.PlatformKey);
		break;
	}
}

#define NXNA2_IMPLEMENTATION
#include "MyNxna2.h"
#undef NXNA2_IMPLEMENTATION
