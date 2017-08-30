#include "SceneManager.h"
#include "CharacterManager.h"
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
		Graphics::Model* Models[SceneDesc::MaxModels];

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

	bool SceneManager::CreateScene(SceneDesc* desc)
	{
		assert(desc->NumModels <= SceneDesc::MaxModels);
		assert(desc->NumLights <= SceneDesc::MaxLights);

		// create a list of files to load
		{
			uint32 files[Content::ContentManager::MaxLoadedFiles];
			Content::ResourceType types[Content::ContentManager::MaxLoadedFiles];
			uint32 numFiles = 0;

			auto addFile = [&files, &types, &numFiles](Content::ResourceType type, uint32 hash) -> bool
			{
				// has this already been added?
				for (uint32 i = 0; i < numFiles; i++)
				{
					if (files[i] == hash)
						return false;
				}

				files[numFiles] = hash;
				types[numFiles++] = type;

				return true;
			};

			if (desc->Ego.Name[0] != 0)
			{
				addFile(Content::ResourceType::Model, Utils::CalcHash(desc->Ego.Name));

				if (desc->Ego.Diffuse[0][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Ego.Diffuse[0]));
				if (desc->Ego.Diffuse[1][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Ego.Diffuse[1]));
				if (desc->Ego.Diffuse[2][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Ego.Diffuse[2]));
				if (desc->Ego.Diffuse[3][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Ego.Diffuse[3]));
			}

			for (uint32 i = 0; i < desc->NumModels; i++)
			{
				addFile(Content::ResourceType::Model, Utils::CalcHash(desc->Models[i].Name));

				if (desc->Models[i].Diffuse[0][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Diffuse[0]));
				if (desc->Models[i].Diffuse[1][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Diffuse[1]));
				if (desc->Models[i].Diffuse[2][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Diffuse[2]));
				if (desc->Models[i].Diffuse[3][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Diffuse[3]));
													  
				if (desc->Models[i].Lightmap[0][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Lightmap[0]));
				if (desc->Models[i].Lightmap[1][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Lightmap[1]));
				if (desc->Models[i].Lightmap[2][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Lightmap[2]));
				if (desc->Models[i].Lightmap[3][0] != 0) addFile(Content::ResourceType::Texture2D, Utils::CalcHash(desc->Models[i].Lightmap[3]));
			}

			Content::ContentManager::BeginLoad(files, types, numFiles);
		}

		Content::LoadResult result;
		while ((result = Content::ContentManager::PendingLoads(true, nullptr)) == Content::LoadResult::Pending)
		{ }
		//if (result == Content::LoadResult::Error)
		//	return false;


		m_data->NumModels = desc->NumModels;
		for (uint32 i = 0; i < desc->NumModels; i++)
		{
			if (desc->Models[i].Name == nullptr)
				m_data->ModelNameHash[i] = 0;
			else
				m_data->ModelNameHash[i] = Utils::CalcHash(desc->Models[i].Name);

			m_data->Models[i] = (Graphics::Model*)Content::ContentManager::Get(m_data->ModelNameHash[i], Content::ResourceType::Model);
			if (m_data->Models[i] == nullptr)
				return false;

			m_data->ModelTransforms[i] = Nxna::Matrix::Identity;

			m_data->Models[i]->NumTextures = 0;
			for (uint32 j = 0; j < SceneModelDesc::MaxMeshes; j++)
			{
				if (desc->Models[i].Diffuse[j][0] != 0)
				{
					m_data->Models[i]->Textures[m_data->Models[i]->NumTextures] = Utils::CalcHash(desc->Models[i].Diffuse[j]);
					m_data->Models[i]->Meshes[j].DiffuseTextureIndex = m_data->Models[i]->NumTextures;
					m_data->Models[i]->NumTextures++;
				}
				if (desc->Models[i].Lightmap[j][0] != 0)
				{
					m_data->Models[i]->Textures[m_data->Models[i]->NumTextures] = Utils::CalcHash(desc->Models[i].Lightmap[j]);
					m_data->Models[i]->Meshes[j].LightmapTextureIndex = m_data->Models[i]->NumTextures;
					m_data->Models[i]->NumTextures++;
				}
			}
		}

		m_data->NumLights = desc->NumLights;
		for (uint32 i = 0; i < desc->NumLights; i++)
		{
			m_data->Lights[i] = desc->Lights[i];
		}

		CharacterManager::Reset();
		if (desc->Ego.Name[0] != 0)
		{
			auto egoModel = (Graphics::Model*)Content::ContentManager::Get(Utils::CalcHash(desc->Ego.Name), Content::ResourceType::Model);
			if (egoModel == nullptr)
				return false;

			egoModel->NumTextures = 0;

			for (uint32 j = 0; j < SceneModelDesc::MaxMeshes; j++)
			{
				if (desc->Ego.Diffuse[j][0] != 0)
				{
					egoModel->Textures[egoModel->NumTextures] = Utils::CalcHash(desc->Ego.Diffuse[j]);
					egoModel->Meshes[j].DiffuseTextureIndex = egoModel->NumTextures;
					egoModel->NumTextures++;
				}
			}


			CharacterManager::AddCharacter(egoModel, desc->Ego.Position, desc->Ego.Rotation, desc->Ego.Scale, true);
		}

		return true;
	}

	bool SceneManager::CreateScene(const char* sceneFile)
	{
		SceneDesc desc = {};
		if (LoadSceneDesc(sceneFile, &desc) == false)
			return false;

		return CreateScene(&desc);
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
				if (ini_section_equals(&ctx, &item, "ego"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "file"))
							ini_value_copy(&ctx, &item, result->Ego.Name, 64);
						else if (ini_key_equals(&ctx, &item, "x"))
							ini_value_float(&ctx, &item, &result->Ego.Position[0]);
						else if (ini_key_equals(&ctx, &item, "y"))
							ini_value_float(&ctx, &item, &result->Ego.Position[1]);
						else if (ini_key_equals(&ctx, &item, "z"))
							ini_value_float(&ctx, &item, &result->Ego.Position[2]);
						else if (ini_key_equals(&ctx, &item, "rotation"))
							ini_value_float(&ctx, &item, &result->Ego.Rotation);
						else if (ini_key_equals(&ctx, &item, "diffuse_0"))
							ini_value_copy(&ctx, &item, result->Ego.Diffuse[0], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_1"))
							ini_value_copy(&ctx, &item, result->Ego.Diffuse[1], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_2"))
							ini_value_copy(&ctx, &item, result->Ego.Diffuse[2], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_3"))
							ini_value_copy(&ctx, &item, result->Ego.Diffuse[3], 64);
						else if (ini_key_equals(&ctx, &item, "scale"))
							ini_value_float(&ctx, &item, &result->Ego.Scale);
					}

					goto parse;
				}
				else if (ini_section_equals(&ctx, &item, "light"))
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
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Name, 64);
						}
						else if (ini_key_equals(&ctx, &item, "diffuse_0"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Diffuse[0], 64);
						}
						else if (ini_key_equals(&ctx, &item, "diffuse_1"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Diffuse[1], 64);
						}
						else if (ini_key_equals(&ctx, &item, "diffuse_2"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Diffuse[2], 64);
						}
						else if (ini_key_equals(&ctx, &item, "diffuse_3"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Diffuse[3], 64);
						}
						else if (ini_key_equals(&ctx, &item, "lightmap_0"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Lightmap[0], 64);
						}
						else if (ini_key_equals(&ctx, &item, "lightmap_1"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Lightmap[1], 64);
						}
						else if (ini_key_equals(&ctx, &item, "lightmap_2"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Lightmap[2], 64);
						}
						else if (ini_key_equals(&ctx, &item, "lightmap_3"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Lightmap[3], 64);
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

	void SceneManager::Process(Nxna::Matrix* modelview, float elapsed)
	{
		CharacterManager::Process(modelview, elapsed);

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
			Graphics::Model::Render(m_device, &transform, m_data->Models[i]);

			if (g_globals->DevMode)
			{
				Nxna::Color color(255, 255, 255);
				if (m_data->SelectedModelIndex == i)
				{
					color.G = 0; color.B = 0;
				}

				Graphics::DrawUtils::DrawBoundingBox(m_data->Models[i]->BoundingBox, &transform, color);
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

		CharacterManager::Render(modelview);
	}
}
