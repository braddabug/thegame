#include "CharacterManager.h"
#include "../MemoryManager.h"
#include "../Graphics/Model.h"
#include "ScriptManager.h"
#include <cstring>

namespace Game
{
	struct CharacterBrain
	{
		enum class State
		{
			Idle,
			WalkTo,
			Halt
		};

		State CurrentState;
		
		union
		{
			struct
			{
				float X, Z;
			} WalkTo;
			struct
			{
				float GoalRotation;
			} Idle;
		};
	};

	struct CharacterManagerData
	{
		static const uint32 MaxCharacters = 10;

		uint32 NumCharacters;
		SceneModelInfo Models[MaxCharacters];
		Nxna::Vector3 Positions[MaxCharacters];
		Nxna::Vector3 Velocities[MaxCharacters];
		float Rotations[MaxCharacters];
		float Scales[MaxCharacters];
		CharacterBrain Brains[MaxCharacters];

		int EgoIndex;
	};

	CharacterManagerData* CharacterManager::m_data;
	Nxna::Graphics::GraphicsDevice* CharacterManager::m_device;

	void CharacterManager::SetGlobalData(CharacterManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = (CharacterManagerData*)g_memory->AllocTrack(sizeof(CharacterManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(CharacterManagerData));
			(*data)->EgoIndex = -1;
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
		m_data->EgoIndex = -1;
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

		if (isEgo)
			m_data->EgoIndex = m_data->NumCharacters;

		m_data->NumCharacters++;
	}

	void CharacterManager::Process(Nxna::Matrix* modelview, float elapsed)
	{
		bool transformDirty[CharacterManagerData::MaxCharacters] = { false };

		if (m_data->EgoIndex >= 0)
		{
			Gui::CursorType cursor = Gui::CursorType::Pointer;

			Nxna::Vector3 mouse0((float)g_inputState->MouseX, (float)g_inputState->MouseY, 0);
			Nxna::Vector3 mouse1((float)g_inputState->MouseX, (float)g_inputState->MouseY, 1.0f);

			auto viewport = m_device->GetViewport();
			auto mp0 = viewport.Unproject(mouse0, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);
			auto mp1 = viewport.Unproject(mouse1, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);

			auto toPoint = Nxna::Vector3::Normalize(mp1 - mp0);

			Game::VerbInfo verbs[10];
			uint32 numVerbs = 0;
			auto r = SceneManager::QueryRayIntersection(mp0, toPoint, SceneIntersectionTestTarget::All);
			if (r.ResultType != SceneIntersectionTestTarget::None)
			{
				numVerbs = ScriptManager::GetActiveVerbs(SceneManager::GetSceneID(), r.NounHash, verbs, 10);

				if (numVerbs > 0)
				{
					cursor = Gui::CursorType::PointerHighlight1;
				}
			}


			if (NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[1]) ||
				NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[3]))
			{
				if (NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[1]))
				{
					float t;
					if (intersect_ray_plane(Nxna::Vector3(0, 1.0f, 0), Nxna::Vector3::Zero, mp0, toPoint, t))
					{
						auto dest = mp0 + toPoint * t;

						m_data->Brains[m_data->EgoIndex].CurrentState = CharacterBrain::State::WalkTo;
						m_data->Brains[m_data->EgoIndex].WalkTo.X = dest.X;
						m_data->Brains[m_data->EgoIndex].WalkTo.Z = dest.Z;

						transformDirty[m_data->EgoIndex] = true;
					}
				}
				else
				{
					if (numVerbs == 1)
					{
						// automatically do the 1 verb
						ScriptManager::DoVerb(r.NounHash, verbs[0].VerbHash, verbs[0].ActionHash);
					}
				}
			}

			Gui::GuiManager::SetCursor(cursor);
		}

		for (uint32 i = 0; i < m_data->NumCharacters; i++)
		{
			const float rvelocity = 15.0f;
			
			if (m_data->Brains[i].CurrentState == CharacterBrain::State::WalkTo)
			{
				auto to = Nxna::Vector3(m_data->Brains[i].WalkTo.X - m_data->Positions[i].X, 0, m_data->Brains[i].WalkTo.Z - m_data->Positions[i].Z);
		
				const float maxVelocity = 20.0f;
				const float slowingRadius = 3.0f;
				float velocity = maxVelocity;

				float distance = to.Length();

				if (distance < slowingRadius)
				{
					float scale = (distance) / (slowingRadius);
					float damping = 1.0f - scale;

					velocity *= (1.0f - (damping * damping * damping));
				}
				
				float toTravel = velocity * elapsed;

				if (toTravel > distance || velocity < 0.01f)
				{
					m_data->Positions[i].X = m_data->Brains[i].WalkTo.X;
					m_data->Positions[i].Z = m_data->Brains[i].WalkTo.Z;
					m_data->Velocities[i] = Nxna::Vector3::Zero;
					m_data->Brains[i].CurrentState = CharacterBrain::State::Idle;
					m_data->Brains[i].Idle.GoalRotation = m_data->Rotations[i];
				}
				else
				{
					velocity /= distance;

					m_data->Velocities[i] = to * velocity;
					m_data->Positions[i].X += m_data->Velocities[i].X * elapsed;
					m_data->Positions[i].Z += m_data->Velocities[i].Z * elapsed;


					float goalRotation = 1.57079632679f + atan2(-to.Z, to.X);
					float angleDiff = -Utils::AngleDiff(m_data->Rotations[i], goalRotation);

					m_data->Rotations[i] += angleDiff * rvelocity * elapsed;
				}

				transformDirty[i] = true;
			}
			else if (m_data->Brains[i].CurrentState == CharacterBrain::State::Halt)
			{
				m_data->Velocities[i] += -m_data->Velocities[i] * 20 * elapsed;
				m_data->Positions[i].X += m_data->Velocities[i].X * elapsed;
				m_data->Positions[i].Z += m_data->Velocities[i].Z * elapsed;
				transformDirty[i] = true;

				if (m_data->Velocities[i].LengthSquared() < 0.1f)
				{
					m_data->Velocities[i] = Nxna::Vector3::Zero;
					m_data->Brains[i].CurrentState = CharacterBrain::State::Idle;
				}
			}
			else if (m_data->Brains[i].CurrentState == CharacterBrain::State::Idle)
			{
				float angleDiff = -Utils::AngleDiff(m_data->Rotations[i], m_data->Brains[i].Idle.GoalRotation);
				float rotate = angleDiff * elapsed;
				if (rotate > 0.01f || rotate <= 0.01f)
				{
					m_data->Rotations[i] += angleDiff * rvelocity * elapsed;
					transformDirty[i] = true;
				}
			}

			if (transformDirty[i])
			{
				auto translation = Nxna::Matrix::CreateTranslation(m_data->Positions[i].X, m_data->Positions[i].Y, m_data->Positions[i].Z);
				auto scalem = Nxna::Matrix::CreateScale(m_data->Scales[i]);
				auto rotationm = Nxna::Matrix::CreateRotationY(m_data->Rotations[i]);
				*m_data->Models[i].Transform = scalem * rotationm * translation;

				Graphics::Model::UpdateAABB(m_data->Models[i].Model->BoundingBox, m_data->Models[i].Transform, m_data->Models[i].AABB);
			}
		}
	}

	void CharacterManager::Idle(uint32 character)
	{
		if (character < CharacterManagerData::MaxCharacters)
		{
			Idle(character, m_data->Rotations[character]);
		}
	}

	void CharacterManager::Idle(uint32 character, float faceDirection)
	{
		if (character < CharacterManagerData::MaxCharacters)
		{
			if (m_data->Brains[character].CurrentState == CharacterBrain::State::WalkTo)
				m_data->Brains[character].CurrentState = CharacterBrain::State::Halt;

			m_data->Brains[character].Idle.GoalRotation = faceDirection;
		}
	}
}
