#include "CharacterManager.h"
#include "../MemoryManager.h"
#include "../Graphics/Model.h"
#include <cstring>

namespace Game
{
	struct CharacterBrain
	{
		enum class State
		{
			Idle,
			WalkTo
		};

		State CurrentState;
		
		union
		{
			struct
			{
				float X, Z;
			} WalkTo;
		};
	};

	struct CharacterManagerData
	{
		static const uint32 MaxCharacters = 10;

		uint32 NumCharacters;
		SceneModelInfo Models[MaxCharacters];
		Nxna::Vector3 Positions[MaxCharacters];
		float Rotations[MaxCharacters];
		float Scales[MaxCharacters];
		CharacterBrain Brains[MaxCharacters];
	};

	CharacterManagerData* CharacterManager::m_data;
	Nxna::Graphics::GraphicsDevice* CharacterManager::m_device;

	void CharacterManager::SetGlobalData(CharacterManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = (CharacterManagerData*)g_memory->AllocTrack(sizeof(CharacterManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(CharacterManagerData));
		}

		m_data = *data;
		m_device = device;
	}

	void CharacterManager::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	void CharacterManager::Reset()
	{
		m_data->NumCharacters = 0;
	}

	void CharacterManager::AddCharacter(SceneModelInfo model, float position[3], float rotation, float scale, bool isEgo)
	{
		if (m_data->NumCharacters >= CharacterManagerData::MaxCharacters)
			return;

		m_data->Models[m_data->NumCharacters] = model;
		m_data->Positions[m_data->NumCharacters].X = position[0];
		m_data->Positions[m_data->NumCharacters].Y = position[1];
		m_data->Positions[m_data->NumCharacters].Z = position[2];
		m_data->Rotations[m_data->NumCharacters] = rotation;
		m_data->Scales[m_data->NumCharacters] = scale;

		auto translation = Nxna::Matrix::CreateTranslation(position[0], position[1], position[2]);
		auto scalem = Nxna::Matrix::CreateScale(scale);
		*model.Transform = scalem * translation;
		Graphics::Model::UpdateAABB(model.Model->BoundingBox, model.Transform, model.AABB);

		m_data->NumCharacters++;
	}

	void CharacterManager::Process(Nxna::Matrix* modelview, float elapsed)
	{
		bool transformDirty = false;

		if (NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[1]) ||
			NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[3]))
		{
			Nxna::Vector3 mouse0((float)g_inputState->MouseX, (float)g_inputState->MouseY, 0);
			Nxna::Vector3 mouse1((float)g_inputState->MouseX, (float)g_inputState->MouseY, 1.0f);

			auto viewport = m_device->GetViewport();
			auto mp0 = viewport.Unproject(mouse0, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);
			auto mp1 = viewport.Unproject(mouse1, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);

			auto toPoint = Nxna::Vector3::Normalize(mp1 - mp0);

			if (NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[1]))
			{
				float t;
				if (intersect_ray_plane(Nxna::Vector3(0, 1.0f, 0), Nxna::Vector3::Zero, mp0, toPoint, t))
				{
					auto dest = mp0 + toPoint * t;

					m_data->Brains[0].CurrentState = CharacterBrain::State::WalkTo;
					m_data->Brains[0].WalkTo.X = dest.X;
					m_data->Brains[0].WalkTo.Z = dest.Z;

					transformDirty = true;
				}
			}
			else
			{
				auto r = SceneManager::QueryRayIntersection(mp0, toPoint, SceneIntersectionTestTarget::Character);
				if (r.ResultType == SceneIntersectionTestTarget::Character)
				{
					LOG("Clicked character");
				}
			}
		}

		for (uint32 i = 0; i < m_data->NumCharacters; i++)
		{
			if (m_data->Brains[i].CurrentState == CharacterBrain::State::WalkTo)
			{
				auto to = Nxna::Vector3(m_data->Brains[i].WalkTo.X - m_data->Positions[i].X, 0, m_data->Brains[i].WalkTo.Z - m_data->Positions[i].Z);
		
				float velocity = 20.0f;
				const float rvelocity = 15.0f;
				const float slowingRadius = 3.0f;

				float distance = to.Length();

				if (distance < slowingRadius)
				{
					float scale = (distance) / (slowingRadius);
					float damping = 1.0f - scale;

					velocity *= 1.0f - (damping * damping * damping);
				}
				
				float toTravel = velocity * elapsed;

				if (toTravel > distance || velocity < 0.01f)
				{
					m_data->Positions[i].X = m_data->Brains[i].WalkTo.X;
					m_data->Positions[i].Z = m_data->Brains[i].WalkTo.Z;
					m_data->Brains[i].CurrentState = CharacterBrain::State::Idle;
				}
				else
				{
					toTravel /= distance;

					m_data->Positions[i].X += to.X * toTravel;
					m_data->Positions[i].Z += to.Z * toTravel;


					float goalRotation = 1.57079632679f + atan2(-to.Z, to.X);
					float angleDiff = -Utils::AngleDiff(m_data->Rotations[0], goalRotation);

					m_data->Rotations[i] += angleDiff * rvelocity * elapsed;
				}

				transformDirty = true;
			}
		}


		if (Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Up))
		{
			transformDirty = true;
			m_data->Positions[0].Y += elapsed * 1.0f;
		}

		if (transformDirty)
		{
			auto translation = Nxna::Matrix::CreateTranslation(m_data->Positions[0].X, m_data->Positions[0].Y, m_data->Positions[0].Z);
			auto scalem = Nxna::Matrix::CreateScale(m_data->Scales[0]);
			auto rotationm = Nxna::Matrix::CreateRotationY(m_data->Rotations[0]);
			*m_data->Models[0].Transform = scalem * rotationm * translation;

			Graphics::Model::UpdateAABB(m_data->Models[0].Model->BoundingBox, m_data->Models[0].Transform, m_data->Models[0].AABB);
		}
	}
}
