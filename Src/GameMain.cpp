#include "GameMain.h"
#include "GlobalData.h"
#include "Logging.h"
#include "StringManager.h"
#include "SpriteBatchHelper.h"
#include "Gui/TextPrinter.h"
#include "Gui/Console.h"
#include "FileSystem.h"
#include "Graphics/Model.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/ShaderLibrary.h"
#include "Graphics/DrawUtils.h"
#include "Content/ContentManager.h"
#include "Audio/AudioEngine.h"
#include "Game/SceneManager.h"
#include "MemoryManager.h"



Graphics::Model m;
JobInfo mj;

Globals* g_globals;
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

	if (data->Global == nullptr)
	{
		data->Global = NewObject<Globals>(__FILE__, __LINE__);
		memset(data->Global, 0, sizeof(Globals));
	}
	g_globals = data->Global;

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
		memset(g_inputState, 0, sizeof(Nxna::Input::InputState));
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
	Gui::Console::SetGlobalData(&data->ConsoleData);
	Content::ContentManager::SetGlobalData(&data->ContentData, g_device);
	Content::ContentLoader::SetGlobalData(&data->ContentLData, g_device);
	Graphics::Model::SetGlobalData(&data->ModelData);
	Graphics::TextureLoader::SetGlobalData(&data->TextureLoaderData, g_device);
	Graphics::ShaderLibrary::SetGlobalData(&data->ShaderLibraryData, g_device);
	Graphics::DrawUtils::SetGlobalData(&data->DrawUtilsData, g_device);
	Game::SceneManager::SetGlobalData(&data->SceneData, g_device);
}

int Init(WindowInfo* window)
{
	g_globals->DevMode = true;

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

	if (Graphics::ShaderLibrary::LoadCoreShaders() == false)
		return -1;

	if (Audio::AudioEngine::Init() == false)
		return -1;

	//auto c = Content::ContentManager::Load("Content/Models/Shark.obj", Content::LoaderType::ModelObj, &m);
	//if (c != Content::ContentState::Loaded)
	//	return -1;

	//auto c = Content::ContentLoader::BeginLoad("Content/Models/Shark.obj", Content::LoaderType::ModelObj, &m, &mj);
	//auto c = Content::ContentLoader::BeginLoad("Content/Models/out.obj", Content::LoaderType::ModelObj, &m, &mj);
	//if (c != Content::ContentState::Incomplete)
	//	return -1;

	/*if (Content::ContentLoader::Load("Content/Models/out.obj", Content::LoaderType::ModelObj, &m) != Content::ContentState::Loaded)
		return -1;
	Game::SceneDesc scene = {};
	Game::SceneModelDesc models[] = { &m, nullptr, {0,0,0}, {0,0,0} };
	scene.NumModels = 1;
	scene.Models = models;

	Game::SceneLightDesc lights[1];
	lights[0].Type = Game::LightType::Point;
	lights[0].Point.Position[0] = 0;
	lights[0].Point.Position[1] = 100.0f;
	lights[0].Point.Position[2] = 0;
	lights[0].Point.Color[0] = 1.0f;
	lights[0].Point.Color[1] = 1.0f;
	lights[0].Point.Color[2] = 0;
	scene.NumLights = 1;
	scene.Lights = lights;

	Game::SceneManager::CreateScene(&scene);*/

	Game::SceneManager::CreateScene("Content/Scenes/scene.txt");

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

	Game::SceneManager::Shutdown();
	Graphics::Model::Shutdown();
	Graphics::TextureLoader::Shutdown();
	Content::ContentLoader::Shutdown();

	JobQueue::Shutdown(true);

	Audio::AudioEngine::Shutdown();

	Gui::TextPrinter::Shutdown();

	SpriteBatchHelper::Destroy();

	Nxna::Graphics::GraphicsDevice::DestroyGraphicsDevice(g_device);
}


Nxna::Vector3 cameraPosition(0, 2.0f, 100);
float cameraPitch = 0;
float cameraYaw = 0;


void Tick()
{
	JobQueue::Tick();

	g_device->ClearColor(0, 0, 0, 0);
	g_device->ClearDepthStencil(true, true, 1.0f, 0);

	SpriteBatchHelper sb;
	Nxna::Matrix cameraTransform;

	// setup the camera
	cameraPitch = Nxna::MathHelper::Clamp(cameraPitch, -Nxna::Pi, Nxna::Pi);

	Nxna::Quaternion yawq = Nxna::Quaternion::CreateFromYawPitchRoll(cameraYaw, cameraPitch, 0);

	Nxna::Vector3 forward = Nxna::Vector3::Forward;
	Nxna::Vector3 up = Nxna::Vector3::Up;
	Nxna::Vector3 right = Nxna::Vector3::Right;

	Nxna::Quaternion::Multiply(yawq, forward, forward);
	Nxna::Quaternion::Multiply(yawq, up, up);
	Nxna::Quaternion::Multiply(yawq, right, right);
	

	if (Gui::Console::IsVisible())
	{
		cameraTransform = Nxna::Matrix::Identity;

		Gui::Console::HandleInput(g_inputState);

		if (NXNA_BUTTON_STATE(g_inputState->KeyboardKeysData[(int)Nxna::Input::Key::F1]) &&
			NXNA_BUTTON_TRANSITION_COUNT(g_inputState->KeyboardKeysData[(int)Nxna::Input::Key::F1]) == 1)
		{
			Gui::Console::IsVisible(false);
		}
	}
	else
	{
		if (NXNA_BUTTON_STATE(g_inputState->KeyboardKeysData[(int)Nxna::Input::Key::F1]) &&
			NXNA_BUTTON_TRANSITION_COUNT(g_inputState->KeyboardKeysData[(int)Nxna::Input::Key::F1]) == 1)
		{
			Gui::Console::IsVisible(true);
		}

		// update camera
		bool shiftb = Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::LeftShift);
		bool upb = Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Up) ||
			Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::W);
		bool downb = Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Down) ||
			Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::S);
		bool leftb = Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Left) ||
			Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::A);
		bool rightb = Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Right) ||
			Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::D);
		bool mlb = Nxna::Input::InputState::IsMouseButtonDown(g_inputState, 1);
		bool mrb = Nxna::Input::InputState::IsMouseButtonDown(g_inputState, 3);
		
		if (mrb && mlb)
		{
			// strafe (straif? straife? strayph? streigh?)
			cameraPosition.Y -= g_inputState->RelMouseY;
			cameraPosition += right * g_inputState->RelMouseX;
		}
		else if (mrb)
		{
			// move
			cameraPosition -= forward * g_inputState->RelMouseY;
			cameraYaw -= g_inputState->RelMouseX * 0.01f;
		}
		else if (mlb)
		{
			// look around
			cameraPitch -= g_inputState->RelMouseY * 0.01f;
			cameraYaw -= g_inputState->RelMouseX * 0.01f;
		}

		if (shiftb)
		{
			if (upb)
				cameraPosition += forward;
			if (downb)
				cameraPosition -= forward;
			if (leftb || rightb)
			{
				if (leftb)
					cameraPosition -= right;
				if (rightb)
					cameraPosition += right;
			}
		}
	}


	// setup the camera
	{
		auto lookAt = Nxna::Matrix::CreateLookAt(cameraPosition, cameraPosition + forward, up);
		Nxna::Matrix projection = Nxna::Matrix::CreatePerspectiveFieldOfView(60.0f * 0.0174533f, 640 / (float)480, 0.5f, 1000.0f);
		cameraTransform = lookAt * projection;
	}

	/*
	rotation = g_inputState->MouseX * 0.01f;
	distance = 10.0f + g_inputState->MouseY * 0.1f;
	Nxna::Vector2 cameraPos = Nxna::Vector2(0, distance).Rotate(rotation);
	
	Nxna::Matrix view = Nxna::Matrix::CreateLookAt(Nxna::Vector3(cameraPos.X, 0, cameraPos.Y), Nxna::Vector3(0, 0, 0), Nxna::Vector3(0, 1.0f, 0));
	auto transform = view * projection;*/
	
	Game::SceneManager::Process();
	Game::SceneManager::Render(&cameraTransform);
	//if (mj.Result == JobResult::Completed)
		//Graphics::Model::Render(g_device, &transform, &m, 1);

	sb.Begin();
	auto font = Gui::TextPrinter::GetFont(Gui::FontType::Default);
	Gui::TextPrinter::PrintScreen(&sb, 0, 20, font, "Hello, world!");

	Gui::Console::Draw(&sb, g_log);

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
		Nxna::Input::InputState::InjectKeyEvent(g_inputState, true, e.KeyboardButton.Key);
		break;
	case ExternalEventType::KeyboardButtonUp:
		Nxna::Input::InputState::InjectKeyEvent(g_inputState, false, e.KeyboardButton.Key);
		break;
	case ExternalEventType::TextInput:
		g_inputState->BufferedKeys[g_inputState->NumBufferedKeys++] = e.TextInput.Unicode;
		break;
	}
}

#define NXNA2_IMPLEMENTATION
#include "MyNxna2.h"
#undef NXNA2_IMPLEMENTATION
