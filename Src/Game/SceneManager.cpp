#include "SceneManager.h"
#include "CharacterManager.h"
#include "NavMesh.h"
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
		uint32 SceneID;

		uint32 NumModels;
		StringRef ModelNameHash[SceneDesc::MaxModels];
		uint32 ModelNounHash[SceneDesc::MaxModels];
		Nxna::Matrix ModelTransforms[SceneDesc::MaxModels];
		Graphics::Model* Models[SceneDesc::MaxModels];
		float ModelAABB[SceneDesc::MaxModels][6];
		bool IsCharacterModel[SceneDesc::MaxModels];

		SceneLightDesc Lights[SceneDesc::MaxLights];
		uint32 NumLights;

		Graphics::Bitmap* WalkMap;
		Nxna::Graphics::Texture2D WalkMapTexture;
		int WalkMapXOffset;
		int WalkMapZOffset;

		int SelectedModelIndex;

		NavMesh Navigation;
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

	void cmdLoadScene(const char* arg)
	{
		if (SceneManager::CreateScene(arg))
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "Loaded scene %s", arg);
		else
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to load scene %s", arg);
	}

	void cmdLoadNav(const char* arg)
	{
		if (SceneManager::LoadNavMesh(arg) == false)
			WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unable to load nav mesh %s", arg);
		else
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "Loaded nav mesh %s", arg);
	}

	void SceneManager::Init()
	{
		ConsoleCommand cmd[] = {
			{ "load_scene", cmdLoadScene },
			{ "load_nav", cmdLoadNav }
		};
		Gui::Console::AddCommands(cmd, 2);
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

		m_data->SceneID = desc->SceneID;

		m_data->NumModels = 0;
		for (uint32 i = 0; i < desc->NumModels; i++)
		{
			if (desc->Models[i].Name == nullptr)
				m_data->ModelNameHash[i] = 0;
			else
				m_data->ModelNameHash[i] = HashStringManager::Set(HashStringManager::HashStringType::File, desc->Models[i].File);

			m_data->ModelNounHash[i] = desc->Models[i].NounHash;

			m_data->Models[i] = (Graphics::Model*)Content::ContentManager::Get(m_data->ModelNameHash[i], Content::ResourceType::Model);
			if (m_data->Models[i] == nullptr)
			{
				LOG_ERROR("Unable to add model with hash %u to scene", m_data->ModelNameHash[i]);
				return false;
			}

			m_data->ModelTransforms[i] = Nxna::Matrix::Identity;

			m_data->Models[i]->NumTextures = 0;
			for (uint32 j = 0; j < SceneModelDesc::MaxMeshes; j++)
			{
				if (desc->Models[i].Diffuse[j][0] != 0)
				{
					m_data->Models[i]->Textures[m_data->Models[i]->NumTextures] = HashStringManager::Set(HashStringManager::HashStringType::File, desc->Models[i].Diffuse[j]);
					m_data->Models[i]->Meshes[j].DiffuseTextureIndex = m_data->Models[i]->NumTextures;
					m_data->Models[i]->NumTextures++;
				}
				if (desc->Models[i].Lightmap[j][0] != 0)
				{
					m_data->Models[i]->Textures[m_data->Models[i]->NumTextures] = HashStringManager::Set(HashStringManager::HashStringType::File, desc->Models[i].Lightmap[j]);
					m_data->Models[i]->Meshes[j].LightmapTextureIndex = m_data->Models[i]->NumTextures;
					m_data->Models[i]->NumTextures++;
				}
			}

			m_data->NumModels++;
		}

		m_data->NumLights = desc->NumLights;
		for (uint32 i = 0; i < desc->NumLights; i++)
		{
			m_data->Lights[i] = desc->Lights[i];
		}

		// load the walk map
		{
			// TODO: rather than using a walk-map, use nav-meshes.
			// https://github.com/recastnavigation/recastnavigation

			NavMesh::Load(HashStringManager::Set(HashStringManager::HashStringType::File, desc->WalkMap), &m_data->Navigation);

			m_data->WalkMapXOffset = desc->WalkMapX;
			m_data->WalkMapZOffset = desc->WalkMapZ;
		}

		CharacterManager::Reset();
		for (uint32 i = 0; i < desc->NumCharacters; i++)
		{
			auto model = (Graphics::Model*)Content::ContentManager::Get(HashStringManager::Set(HashStringManager::HashStringType::File, desc->Characters[i].ModelFile), Content::ResourceType::Model);
			if (model == nullptr)
			{
				LOG_ERROR("Unable to add character model %s to scene", desc->Characters[i].ModelFile);
				return false;
			}

			model->NumTextures = 0;

			for (uint32 j = 0; j < SceneModelDesc::MaxMeshes; j++)
			{
				if (desc->Characters[i].Diffuse[j][0] != 0)
				{
					model->Textures[model->NumTextures] = HashStringManager::Set(HashStringManager::HashStringType::File, desc->Characters[i].Diffuse[j]);
					model->Meshes[j].DiffuseTextureIndex = model->NumTextures;
					model->NumTextures++;
				}
			}

			m_data->ModelNounHash[m_data->NumModels] = desc->Characters[i].NounHash;
			m_data->Models[m_data->NumModels] = model;

			m_data->ModelTransforms[i] = Nxna::Matrix::Identity;

			model->NumTextures = 0;
			for (uint32 j = 0; j < SceneModelDesc::MaxMeshes; j++)
			{
				if (desc->Characters[i].Diffuse[j][0] != 0)
				{
					model->Textures[model->NumTextures] = HashStringManager::Set(HashStringManager::HashStringType::File, desc->Characters[i].Diffuse[j]);
					model->Meshes[j].DiffuseTextureIndex = model->NumTextures;
					model->NumTextures++;
				}
			}

			SceneModelInfo info;
			info.Model = model;
			info.Transform = &m_data->ModelTransforms[m_data->NumModels];
			info.AABB = m_data->ModelAABB[m_data->NumModels];
			CharacterManager::AddCharacter(info, desc->Characters[i].Position, desc->Characters[i].Rotation, desc->Characters[i].Scale, desc->EgoCharacterIndex == i);

			m_data->IsCharacterModel[m_data->NumModels] = true;
			m_data->NumModels++;
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

		FoundFile f;
		if (FileFinder::OpenAndMap(sceneFile, &f) == false)
		{
			LOG_ERROR("Unable to open scene file %s", sceneFile);
			return false;
		}

		bool success = true;

		const char* txt = (char*)f.Memory;

		ini_context ctx;
		ini_init(&ctx, txt, txt + f.FileSize);

		ini_item item;

	parse:
		if (ini_next(&ctx, &item) == ini_result_success)
		{
			if (item.type == ini_itemtype::section)
			{
				if (ini_section_equals(&ctx, &item, "character"))
				{
					result->Characters[result->NumCharacters].Scale = 1.0f;

					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "name"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].Name, 64);
						else if (ini_key_equals(&ctx, &item, "noun"))
							result->Characters[result->NumCharacters].NounHash = Utils::CalcHash((const uint8*)ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						else if (ini_key_equals(&ctx, &item, "file"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].ModelFile, 64);
						else if (ini_key_equals(&ctx, &item, "x"))
							ini_value_float(&ctx, &item, &result->Characters[result->NumCharacters].Position[0]);
						else if (ini_key_equals(&ctx, &item, "y"))
							ini_value_float(&ctx, &item, &result->Characters[result->NumCharacters].Position[1]);
						else if (ini_key_equals(&ctx, &item, "z"))
							ini_value_float(&ctx, &item, &result->Characters[result->NumCharacters].Position[2]);
						else if (ini_key_equals(&ctx, &item, "rotation"))
							ini_value_float(&ctx, &item, &result->Characters[result->NumCharacters].Rotation);
						else if (ini_key_equals(&ctx, &item, "diffuse_0"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].Diffuse[0], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_1"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].Diffuse[1], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_2"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].Diffuse[2], 64);
						else if (ini_key_equals(&ctx, &item, "diffuse_3"))
							ini_value_copy(&ctx, &item, result->Characters[result->NumCharacters].Diffuse[3], 64);
						else if (ini_key_equals(&ctx, &item, "scale"))
							ini_value_float(&ctx, &item, &result->Characters[result->NumCharacters].Scale);
					}

					result->NumCharacters++;

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
						if (ini_key_equals(&ctx, &item, "name"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].Name, 64);
						}
						else if (ini_key_equals(&ctx, &item, "file"))
						{
							ini_value_copy(&ctx, &item, result->Models[result->NumModels].File, 64);
						}
						else if (ini_key_equals(&ctx, &item, "noun"))
							result->Models[result->NumCharacters].NounHash = Utils::CalcHash((const uint8*)ctx.source + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						else if (ini_key_equals(&ctx, &item, "static"))
						{
							int v;
							if (ini_value_int(&ctx, &item, &v))
								result->Models[result->NumModels].IsStatic = (v != 0);
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
				else if (ini_section_equals(&ctx, &item, "walk"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "file"))
							ini_value_copy(&ctx, &item, result->WalkMap, 64);
						else if (ini_key_equals(&ctx, &item, "x"))
							ini_value_int(&ctx, &item, &result->WalkMapX);
						else if (ini_key_equals(&ctx, &item, "z"))
							ini_value_int(&ctx, &item, &result->WalkMapZ);
					}

					goto parse;
				}
				else if (ini_section_equals(&ctx, &item, "global"))
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						if (ini_key_equals(&ctx, &item, "id"))
						{
							result->SceneID = Utils::CalcHash((uint8*)txt + item.keyvalue.value_start, item.keyvalue.value_end - item.keyvalue.value_start);
						}
						else if (ini_key_equals(&ctx, &item, "ego"))
						{
							for (uint32 i = 0; i < result->NumCharacters; i++)
							{
								if (ini_value_equals(&ctx, &item, result->Characters[i].Name))
								{
									result->EgoCharacterIndex = i;
									goto parse;
								}
							}

							// oops, couldn't find the ego character!
							char buffer[64];
							ini_value_copy(&ctx, &item, buffer, 64);
							WriteLog(LogSeverityType::Error, LogChannelType::Content, "Unable to set ego to character %s because it was not found", buffer);
							goto end;
						}
					}

					goto parse;
				}
				else
				{
					while (ini_next_within_section(&ctx, &item) == ini_result_success)
					{
						// nothing
					}

					goto parse;
				}
			}
		}

	end:
		FileFinder::Close(&f);

		return success;
	}
	
	bool SceneManager::LoadNavMesh(const char* nav)
	{
		auto f = HashStringManager::Set(HashStringManager::HashStringType::File, nav);
		return NavMesh::Load(f, &m_data->Navigation);
	}

	uint32 SceneManager::GetSceneID()
	{
		return m_data->SceneID;
	}

	static bool intersect_ray_plane(const Nxna::Vector3& pnormal, const Nxna::Vector3& porigin, const Nxna::Vector3& rorigin, const Nxna::Vector3& rdirection, float& t) \
	{
		float denom = Nxna::Vector3::Dot(pnormal, rdirection);
		if (denom > 1e-6 || denom < -1e-6) {
			auto p = porigin - rorigin;
			t = Nxna::Vector3::Dot(p, pnormal) / denom;
			return t >= 0;
		}

		return false;
	}

	static bool intersect_ray_aabb(Nxna::Vector3 bounds[2], Nxna::Vector3 rayOrigin, Nxna::Vector3 rayDirection)
	{
		// shamelessly stolen from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

		float tmin, tmax, tymin, tymax, tzmin, tzmax;

		Nxna::Vector3 invDir(1.0f / rayDirection.X, 1.0f / rayDirection.Y, 1.f / rayDirection.Z);
		int sign[3] = { invDir.X < 0 ? 1 : 0, invDir.Y < 0 ? 1 : 0, invDir.Z < 0 ? 1 : 0 };

		tmin = (bounds[sign[0]].X - rayOrigin.X) * invDir.X;
		tmax = (bounds[1 - sign[0]].X - rayOrigin.X) * invDir.X;
		tymin = (bounds[sign[1]].Y - rayOrigin.Y) * invDir.Y;
		tymax = (bounds[1 - sign[1]].Y - rayOrigin.Y) * invDir.Y;

		if ((tmin > tymax) || (tymin > tmax))
			return false;
		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;

		tzmin = (bounds[sign[2]].Z - rayOrigin.Z) * invDir.Z;
		tzmax = (bounds[1 - sign[2]].Z - rayOrigin.Z) * invDir.Z;

		if ((tmin > tzmax) || (tzmin > tmax))
			return false;
		if (tzmin > tmin)
			tmin = tzmin;
		if (tzmax < tmax)
			tmax = tzmax;

		return true;
	}

	SceneIntersectionTestResult SceneManager::QueryRayIntersection(Nxna::Vector3 start, Nxna::Vector3 direction, SceneIntersectionTestTarget target)
	{
		// TODO: make this a triangle interesction test, but for now an AABB is ok.

		SceneIntersectionTestResult result = {};
		
		if (((uint32)target & (uint32)SceneIntersectionTestTarget::Ground) == (uint32)SceneIntersectionTestTarget::Ground)
		{
			float t;
			if (intersect_ray_plane(Nxna::Vector3(0, 1.0f, 0), Nxna::Vector3::Zero, start, direction, t))
			{
				result.ResultType = SceneIntersectionTestTarget::Ground;
				result.Distance = t;
			}
		}

		if (((uint32)target & (uint32)SceneIntersectionTestTarget::Character) == (uint32)SceneIntersectionTestTarget::Character)
		{
			for (uint32 i = 0; i < m_data->NumModels; i++)
			{
				if (m_data->IsCharacterModel[i])
				{
					Nxna::Vector3 aabb[2] = {
						Nxna::Vector3(m_data->ModelAABB[i][0], m_data->ModelAABB[i][1], m_data->ModelAABB[i][2]),
						Nxna::Vector3(m_data->ModelAABB[i][3], m_data->ModelAABB[i][4], m_data->ModelAABB[i][5]),
					};
					if (intersect_ray_aabb(aabb, start, direction))
					{
						result.ResultType = SceneIntersectionTestTarget::Character;
						result.NounHash = m_data->ModelNounHash[i];
					}
				}
			}
		}

		return result;
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

				Graphics::Model::BeginRender(m_device);
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

			/*if (m_data->WalkMap != nullptr)
			{
				float center[] = { m_data->WalkMapXOffset, 2.0f, m_data->WalkMapZOffset };
				Graphics::DrawUtils::DrawQuadY(center, m_data->WalkMap->Width, m_data->WalkMap->Height, &m_data->WalkMapTexture, modelview);
			}*/

			m_data->Navigation.Render(g_device, modelview);
		}
	}
}
