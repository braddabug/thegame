#include "GameMain.h"
#include "GlobalData.h"
#include "Logging.h"
#include "StringManager.h"
#include "SpriteBatchHelper.h"
#include "VirtualResolution.h"
#include "Gui/TextPrinter.h"
#include "Gui/Console.h"
#include "Gui/GuiManager.h"
#include "FileSystem.h"
#include "Graphics/Model.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/ShaderLibrary.h"
#include "Graphics/DrawUtils.h"
#include "Content/ContentManager.h"
#include "Audio/AudioEngine.h"
#include "Game/SceneManager.h"
#include "Game/CharacterManager.h"
#include "Game/ScriptManager.h"
#include "MemoryManager.h"

Graphics::Model m;
JobInfo mj;

Globals* g_globals;
Nxna::Graphics::GraphicsDevice* g_device;
Nxna::Input::InputState* g_inputState;
Content::ContentManager g_content;
extern LogData* g_log;
extern PlatformInfo* g_platform;

void msg(Nxna::Graphics::GraphicsDeviceDebugMessage m)
{
	printf("%s\n", m.Message);
}

void LibLoaded(GlobalData* data, bool initial)
{
	g_memory = data->Memory;
	g_platform = data->Platform;

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

	Utils::Stopwatch::SetGlobalData(&data->StopwatchData);
	FileSystem::SetGlobalData(&data->FileSystem);
	StringManager::SetGlobalData(&data->StringData);
	JobQueue::SetGlobalData(&data->JobQueue);
	Gui::GuiManager::SetGlobalData(&data->GuiData, g_platform);
	VirtualResolution::SetGlobalData(&data->ResolutionData);
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
	Game::CharacterManager::SetGlobalData(&data->CharacterData, g_device);
	Game::ScriptManager::SetGlobalData(&data->ScriptData);
}

int Init(WindowInfo* window)
{
	g_globals->DevMode = true;

	FileSystem::SearchPathInfo searchPaths[] = {
		{ "Content/", 10 }
	};
	FileSystem::SetSearchPaths(searchPaths, 1);

	if (Content::ContentManager::LoadManifest(window->ScreenHeight) == false)
	{
		WriteLog(LogSeverityType::Error, LogChannelType::Unknown, "Unable to load manifest file");
		return -1;
	}

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
	{
		WriteLog(LogSeverityType::Error, LogChannelType::Unknown, "Unable to load one or more fonts");
		return -1;
	}

	VirtualResolution::Init(window->ScreenWidth, window->ScreenHeight);
	SpriteBatchHelper::Init(g_device);

	if (Graphics::ShaderLibrary::LoadCoreShaders() == false)
		return -1;

	if (Audio::AudioEngine::Init() == false)
		return -1;

	Content::ContentManager::QueueGroupLoad(Utils::CalcHash("global"), false);
	Content::ContentManager::QueueGroupLoad(Utils::CalcHash("scene"), false);
	Content::ContentManager::BeginLoad();
	while (Content::ContentManager::PendingLoads(nullptr, nullptr, nullptr)) {}

	Gui::GuiManager::SetCursor(Gui::CursorType::Pointer);
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
	Game::CharacterManager::Shutdown();
	Game::SceneManager::Shutdown();
	Graphics::Model::Shutdown();
	Graphics::TextureLoader::Shutdown();
	Content::ContentLoader::Shutdown();

	VirtualResolution::Shutdown();
	JobQueue::Shutdown(true);

	Audio::AudioEngine::Shutdown();

	Gui::TextPrinter::Shutdown();

	SpriteBatchHelper::Destroy();

	Nxna::Graphics::GraphicsDevice::DestroyGraphicsDevice(g_device);

	g_memory->FreeTrack(g_inputState, __FILE__, __LINE__);
	g_memory->FreeTrack(g_device, __FILE__, __LINE__);
}


Nxna::Vector3 cameraPosition(0, 50.0f, 100);
float cameraPitch = -0.5f;
float cameraYaw = 0;


void Tick(float elapsed)
{
	JobQueue::Tick();

	g_device->ClearColor(0, 0, 0, 0);
	g_device->ClearDepthStencil(true, true, 1.0f, 0);

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
		
		if (shiftb)
		{
			if (mrb && mlb)
			{
				// strafe (straif? straife? strayph? streigh?)
				cameraPosition.Y -= (float)g_inputState->RelMouseY;
				cameraPosition += right * (float)g_inputState->RelMouseX;
			}
			else if (mrb)
			{
				// move
				cameraPosition -= forward * (float)g_inputState->RelMouseY;
				cameraYaw -= (float)g_inputState->RelMouseX * 0.01f;
			}
			else if (mlb)
			{
				// look around
				cameraPitch -= (float)g_inputState->RelMouseY * 0.01f;
				cameraYaw -= (float)g_inputState->RelMouseX * 0.01f;
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
	
	Game::SceneManager::Process(&cameraTransform, elapsed);
	Game::SceneManager::Render(&cameraTransform);
	//if (mj.Result == JobResult::Completed)
		//Graphics::Model::Render(g_device, &transform, &m, 1);


	Gui::Console::Draw(Gui::GuiManager::GetSprites(), g_log);
	Gui::GuiManager::DrawSpeech(Nxna::Vector2::Zero, "This is a test of speech text");

	Gui::GuiManager::Render();

	g_device->Present();
}

void HandleExternalEvent(ExternalEvent e)
{
	switch (e.Type)
	{
	case ExternalEventType::FrameStart:
		Nxna::Input::InputState::FrameReset(g_inputState);
		g_inputState->RelWheel = 0;
		break;
	case ExternalEventType::MouseMove:
		Nxna::Input::InputState::InjectMouseMove(g_inputState, e.MouseMove.X, e.MouseMove.Y);
		break;
	case ExternalEventType::MouseWheel:
		g_inputState->RelWheel += e.MouseWheel.Delta;
		g_inputState->Wheel += e.MouseWheel.Delta;
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

// TODO: figure out what in Nxna2 is defining "None" because it's breaking the g++ build
#undef None