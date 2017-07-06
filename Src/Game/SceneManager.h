#ifndef GAME_SCENEMANAGER_H
#define GAME_SCENEMANAGER_H

#include "../MyNxna2.h"
#include "../Common.h"

namespace Graphics
{
	struct Model;
}

namespace Game
{
	struct SceneManagerData;

	struct SceneModelDesc
	{
		Graphics::Model* Model;
		const char* Name;
		float Position[3];
		float EulerOrientation[3];
	};

	enum class LightType
	{
		Point,
		Spot,
		Directional,
		Sky
	};

	struct SceneLightDesc
	{
		LightType Type;
		union
		{
			struct
			{
				float Position[3];
				float Color[3];
			} Point;
			struct
			{
				float Position[3];
				float Direction[3];
				float Color[3];
				float Radius;
			} Spot;
			struct
			{
				float Direction[3];
				float Color[3];
			} Directional;
			struct
			{
				// TODO: sky lights could be much more detailed, but for now start with this
				float Color[3];
			} Sky;
		};
	};

	struct SceneDesc
	{
		SceneModelDesc* Models;
		uint32 NumModels;

		SceneLightDesc* Lights;
		uint32 NumLights;
	};

	class SceneManager
	{
		static SceneManagerData* m_data;
		static Nxna::Graphics::GraphicsDevice* m_device;

	public:

		static void SetGlobalData(SceneManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static void CreateScene(SceneDesc* desc);
		static void CreateScene(const char* sceneFile);

		static void Process();
		static void Render(Nxna::Matrix* modelview);
	};
}

#endif // GAME_SCENEMANAGER_H
