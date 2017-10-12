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

	struct SceneCharacterDesc
	{
		char Name[64];
		char ModelFile[64];
		uint32 NounHash;
		float Position[3];
		float Rotation;
		float Scale;

		static const uint32 MaxMeshes = 4;
		char Diffuse[MaxMeshes][64];
	};

	struct SceneModelDesc
	{
		char Name[64];
		char File[64];
		uint32 NounHash;

		static const uint32 MaxMeshes = 4;
		char Diffuse[MaxMeshes][64];
		char Lightmap[MaxMeshes][64];

		float Position[3];
		float EulerOrientation[3];

		bool IsStatic;
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

	struct SceneNounDesc
	{
		uint32 NameHash;
		uint32 TargetIndex;
		uint32 ModelMeshIndex;
		bool IsCharacter;
	};

	struct SceneDesc
	{
		static const uint32 MaxCharacters = 10;
		static const uint32 MaxModels = 10;
		static const uint32 MaxLights = 5;
		static const uint32 MaxNouns = 100;

		uint32 SceneID;

		SceneCharacterDesc Characters[MaxCharacters];
		uint32 NumCharacters;

		SceneModelDesc Models[MaxModels];
		uint32 NumModels;

		SceneLightDesc Lights[MaxLights];
		uint32 NumLights;

		char WalkMap[64];
		int WalkMapX;
		int WalkMapZ;

		uint32 EgoCharacterIndex;
	};

	enum class SceneIntersectionTestTarget
	{
		None = 0,

		Ground = 1,
		Model = 2,
		ModelMesh = 4,
		Character = 8,

		All = Ground | ModelMesh | Character
	};

	struct SceneIntersectionTestResult
	{
		SceneIntersectionTestTarget ResultType;
		float Distance;
		uint32 NounHash;

		union
		{
			struct
			{
				uint32 ModelNameHash;
				Graphics::Model* Model;
				uint32 ModelMeshIndex;
			} Model;
			struct
			{
				uint32 CharacterNameHash;
			} Character;
		};
	};

	struct SceneModelInfo
	{
		Graphics::Model* Model;
		Nxna::Matrix* Transform;
		float* AABB;
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

		static uint32 GetSceneID();

		static SceneIntersectionTestResult QueryRayIntersection(Nxna::Vector3 start, Nxna::Vector3 direction, SceneIntersectionTestTarget target);

		static void Process(Nxna::Matrix* modelview, float elapsed);
		static void Render(Nxna::Matrix* modelview);
	};
}

#endif // GAME_SCENEMANAGER_H
