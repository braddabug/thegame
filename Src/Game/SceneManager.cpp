#include "SceneManager.h"
#include "../Graphics/Model.h"
#include "../MemoryManager.h"
#include "../Utils.h"

namespace Game
{
	struct SceneManagerData
	{
		uint32 NumModels;
		static const uint32 MaxModels = 10;
		uint32 ModelNameHash[MaxModels];
		Nxna::Matrix ModelTransforms[MaxModels];
		Graphics::Model Models[MaxModels];
	};

	SceneManagerData* SceneManager::m_data = nullptr;
	Nxna::Graphics::GraphicsDevice* SceneManager::m_device = nullptr;

	void SceneManager::SetGlobalData(SceneManagerData** data, Nxna::Graphics::GraphicsDevice* device)
	{
		if (*data == nullptr)
		{
			*data = (SceneManagerData*)g_memory->AllocTrack(sizeof(SceneManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(SceneManagerData));
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
		assert(desc->NumModels <= SceneManagerData::MaxModels);
		// TODO: do we own the models or not?

		m_data->NumModels = desc->NumModels;
		for (uint32 i = 0; i < desc->NumModels; i++)
		{
			if (desc->Models[i].Name == nullptr)
				m_data->ModelNameHash[i] = 0;
			else
				m_data->ModelNameHash[i] = Utils::CalcHash((const uint8*)desc->Models[i].Name);

			m_data->Models[i] = *desc->Models[i].Model;
		}
	}

	void SceneManager::Render(Nxna::Matrix* modelview)
	{
		Graphics::Model::Render(m_device, modelview, m_data->Models, m_data->NumModels);
	}
}