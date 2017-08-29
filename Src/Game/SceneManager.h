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

	struct SceneEgoDesc
	{
		char Name[64];
		float Position[3];
		float Rotation;
		float Scale;

		static const uint32 MaxMeshes = 4;
		char Diffuse[MaxMeshes][64];
	};

	struct SceneModelDesc
	{
		char Name[64];

		static const uint32 MaxMeshes = 4;
		char Diffuse[MaxMeshes][64];
		char Lightmap[MaxMeshes][64];

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
		static const uint32 MaxModels = 10;
		static const uint32 MaxLights = 5;

		SceneEgoDesc Ego;

		SceneModelDesc Models[MaxModels];
		uint32 NumModels;

		SceneLightDesc Lights[MaxLights];
		uint32 NumLights;
	};

	class SceneManager
	{
		static SceneManagerData* m_data;
		static Nxna::Graphics::GraphicsDevice* m_device;

	public:

		static void SetGlobalData(SceneManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static bool CreateScene(SceneDesc* desc);
		static bool CreateScene(const char* sceneFile);
		static bool LoadSceneDesc(const char* sceneFile, SceneDesc* result);

		static void Process(float elapsed);
		static void Render(Nxna::Matrix* modelview);
	};
}

#endif // GAME_SCENEMANAGER_H
