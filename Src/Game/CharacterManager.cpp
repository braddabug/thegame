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
		Graphics::Model* Models[MaxCharacters];
		Nxna::Vector3 Positions[MaxCharacters];
		float Rotations[MaxCharacters];
		float Scales[MaxCharacters];
		Nxna::Matrix Transforms[MaxCharacters];
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

	void CharacterManager::AddCharacter(Graphics::Model* model, float position[3], float rotation, float scale, bool isEgo)
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
		m_data->Transforms[m_data->NumCharacters] = scalem * translation;

		m_data->NumCharacters++;
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

	void CharacterManager::Process(Nxna::Matrix* modelview, float elapsed)
	{
		bool transformDirty = false;

		if (NXNA_BUTTON_CLICKED_UP(g_inputState->MouseButtonData[1]))
		{
			Nxna::Vector3 mouse0(g_inputState->MouseX, g_inputState->MouseY, 0);
			Nxna::Vector3 mouse1(g_inputState->MouseX, g_inputState->MouseY, 1.0f);

			auto viewport = m_device->GetViewport();
			auto mp0 = viewport.Unproject(mouse0, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);
			auto mp1 = viewport.Unproject(mouse1, *modelview, Nxna::Matrix::Identity, Nxna::Matrix::Identity);

			auto toPoint = Nxna::Vector3::Normalize(mp1 - mp0);

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
			m_data->Transforms[0] = scalem * rotationm * translation;
		}
	}

	void CharacterManager::Render(Nxna::Matrix* modelview)
	{
		Graphics::Model::BeginRender(m_device);

		for (uint32 i = 0; i < m_data->NumCharacters; i++)
		{
			Nxna::Matrix transform = m_data->Transforms[i] * *modelview;
			Graphics::Model::Render(m_device, &transform, m_data->Models[i]);
		}
	}
}
