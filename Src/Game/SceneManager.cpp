#include "SceneManager.h"
#include "../Graphics/Model.h"
#include "../MemoryManager.h"

namespace Game
{
	struct SceneManagerData
	{
		uint32 NumModels;
		SceneModelDesc* Models;
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
		// TODO: free the memory once Realloc() is working!
		// if (m_data->Models) g_memory->FreeTrack(m_data->Models, __FILE__, __LINE__);
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	void SceneManager::CreateScene(SceneDesc* desc)
	{
		// TODO: do we own the models or not?

		m_data->NumModels = desc->NumModels;
		m_data->Models = (SceneModelDesc*)g_memory->ReallocTrack(m_data->Models, sizeof(SceneModelDesc) * desc->NumModels, __FILE__, __LINE__);
		memcpy(m_data->Models, desc->Models, sizeof(SceneManagerData) * desc->NumModels);
	}

	void SceneManager::Render(Nxna::Matrix* modelview)
	{
		Graphics::Model::Render(m_device, modelview, m_data->Models[0].Model, m_data->NumModels, sizeof(SceneModelDesc));
	}
}