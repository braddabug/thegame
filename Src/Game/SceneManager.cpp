#include "SceneManager.h"
#include "../FileSystem.h"
#include "../Graphics/Model.h"
#include "../Graphics/DrawUtils.h"
#include "../MemoryManager.h"
#include "../Utils.h"
#include "../iniparse.h"

namespace Game
{
	struct SceneManagerData
	{
		uint32 NumModels;
		uint32 ModelNameHash[SceneDesc::MaxModels];
		Nxna::Matrix ModelTransforms[SceneDesc::MaxModels];
		Graphics::Model Models[SceneDesc::MaxModels];

		SceneLightDesc Lights[SceneDesc::MaxLights];
		uint32 NumLights;

		int SelectedModelIndex;
	};

	SceneManagerData* SceneManager::m_data = nullptr;
	Nxna::Graphics::GraphicsDevice* SceneManager::m_device = nullptr;

	void SceneManager::SetGlobalData(SceneManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = (SceneManagerData*)g_memory->AllocTrack(sizeof(SceneManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(SceneManagerData));

			(*data)->SelectedModelIndex = 0;
		}

		m_device = device;
		m_data = *data;
	}

	void SceneManager::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	void SceneManager::CreateScene(SceneDesc* desc)
	{
		assert(desc->NumModels <= SceneDesc::MaxModels);
		assert(desc->NumLights <= SceneDesc::MaxLights);

		// TODO: do we own the models or not?

		m_data->NumModels = desc->NumModels;
		for (uint32 i = 0; i < desc->NumModels; i++)
		{
			if (desc->Models[i].Name == nullptr)
				m_data->ModelNameHash[i] = 0;
			else
				m_data->ModelNameHash[i] = Utils::CalcHash((const uint8*)desc->Models[i].Name);

			Content::ContentLoader::Load<Graphics::Model>(desc->Models[i].Name, Content::LoaderType::ModelObj, &m_data->Models[i]);

			//m_data->Models[i] = *desc->Models[i].Model;
			m_data->ModelTransforms[i] = Nxna::Matrix::Identity;
		}

		m_data->NumLights = desc->NumLights;
		for (uint32 i = 0; i < desc->NumLights; i++)
		{
			m_data->Lights[i] = desc->Lights[i];
		}
	}

	bool SceneManager::CreateScene(const char* sceneFile)
	{
		SceneDesc desc;
		if (LoadSceneDesc(sceneFile, &desc) == false)
			return false;

		CreateScene(&desc);
	}

	bool SceneManager::LoadSceneDesc(const char* sceneFile, SceneDesc* result)
	{
		memset(result, 0, sizeof(SceneDesc));

		File f;
		if (FileSystem::OpenAndMap(sceneFile, &f) == false)
			return false;

		const char* txt = (char*)f.Memory;

		ini_context ctx;
		ini_init(&ctx, txt, txt + f.FileSize);

		ini_item item;

	parse:
		if (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "light"))
				{
					float x = 0, y = 0, z = 0;
					float r = 1.0f, g = 1.0f, b = 1.0f;

					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "x"))
							ini_value_float(&ctx, &item, &x);
						else if (ini_key_equals(&ctx, &item, "y"))
							ini_value_float(&ctx, &item, &y);
						else if (ini_key_equals(&ctx, &item, "z"))
							ini_value_float(&ctx, &item, &z);

						else if (ini_key_equals(&ctx, &item, "r"))
							ini_value_float(&ctx, &item, &r);
						else if (ini_key_equals(&ctx, &item, "g"))
							ini_value_float(&ctx, &item, &g);
						else if (ini_key_equals(&ctx, &item, "b"))
							ini_value_float(&ctx, &item, &b);
					}

					auto light = &m_data->Lights[m_data->NumLights];
					light->Type = LightType::Point;
					light->Point.Position[0] = x;
					light->Point.Position[1] = y;
					light->Point.Position[2] = z;

					light->Point.Color[0] = r;
					light->Point.Color[1] = g;
					light->Point.Color[2] = b;

					m_data->NumLights++;

					goto parse;
				}
				else if (ini_section_equals(&ctx, &item, "model"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "file"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Name, 128);
						}
						else if (ini_key_equals(&ctx, &item, "diffuse"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Diffuse, 128);
						}
						else if (ini_key_equals(&ctx, &item, "lightmap"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Lightmap, 128);
						}
					}

					result->NumModels++;

					goto parse;
				}
			}
		}

		FileSystem::Close(&f);

		return true;
	}

	void SceneManager::Process()
	{
		if (g_globals->DevMode)
		{
			if (m_data->SelectedModelIndex >= 0)
			{
				if (Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Left) &&
					Nxna::Input::InputState::GetKeyTransitionCount(g_inputState, Nxna::Input::Key::Left) == 1)
				{
					m_data->ModelTransforms[m_data->SelectedModelIndex].M41 -= 1.0f;
				}
				if (Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Right) &&
					Nxna::Input::InputState::GetKeyTransitionCount(g_inputState, Nxna::Input::Key::Right) == 1)
				{
					m_data->ModelTransforms[m_data->SelectedModelIndex].M41 += 1.0f;
				}
			}
		}
	}

	void SceneManager::Render(Nxna::Matrix* modelview)
	{
		Graphics::Model::BeginRender(m_device);

		for (uint32 i = 0; i < m_data->NumModels; i++)
		{
			Nxna::Matrix transform = m_data->ModelTransforms[i] * *modelview;
			Graphics::Model::Render(m_device, &transform, &m_data->Models[i]);

			if (g_globals->DevMode)
			{
				Nxna::Color color(255, 255, 255);
				if (m_data->SelectedModelIndex == i)
				{
					color.G = 0; color.B = 0;
				}

				Graphics::DrawUtils::DrawBoundingBox(m_data->Models[i].BoundingBox, &transform, color);
			}
		}

		if (g_globals->DevMode)
		{
			// draw the lights
			for (uint32 i = 0; i < m_data->NumLights; i++)
			{
				switch (m_data->Lights[i].Type)
				{
				case LightType::Point:
				{
					float color[4] = { m_data->Lights[i].Point.Color[0], m_data->Lights[i].Point.Color[1], m_data->Lights[i].Point.Color[2], 1.0f };
					Graphics::DrawUtils::DrawSphere(m_data->Lights[i].Point.Position, 1.0f, modelview, color);
					break;
				}		
				}
			}
		}
	}
}
