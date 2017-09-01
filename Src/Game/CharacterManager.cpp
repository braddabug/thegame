#include "CharacterManager.h"
#include "../MemoryManager.h"
#include "../Graphics/Model.h"
#include <cstring>

namespace Game
{
	struct CharacterManagerData
	{
		static const uint32 MaxCharacters = 10;

		uint32 NumCharacters;
		Graphics::Model* Models[MaxCharacters];
		float Positions[MaxCharacters][3];
		float Rotations[MaxCharacters];
		float Scales[MaxCharacters];
		Nxna::Matrix Transforms[MaxCharacters];
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
		m_data->Positions[m_data->NumCharacters][0] = position[0];
		m_data->Positions[m_data->NumCharacters][1] = position[1];
		m_data->Positions[m_data->NumCharacters][2] = position[2];
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
				m_data->Positions[0][0] = dest.X;
				m_data->Positions[0][1] = dest.Y;
				m_data->Positions[0][2] = dest.Z;

				transformDirty = true;
			}
		}

		// project the mouse, figure out where they clicked

		// find a path from the current location to the spot the player clicked

		// if a path exits, start walking, otherwise walk as close as possible


		if (Nxna::Input::InputState::IsKeyDown(g_inputState, Nxna::Input::Key::Up))
		{
			transformDirty = true;
			m_data->Positions[0][1] += elapsed * 1.0f;
		}

		if (transformDirty)
		{
			auto translation = Nxna::Matrix::CreateTranslation(m_data->Positions[0][0], m_data->Positions[0][1], m_data->Positions[0][2]);
			auto scalem = Nxna::Matrix::CreateScale(m_data->Scales[0]);
			m_data->Transforms[0] = scalem * translation;
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
