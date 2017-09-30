#ifndef GAME_CHARACTERMANAGER_H
#define GAME_CHARACTERMANAGER_H

#include "../Common.h"
#include "../MyNxna2.h"
#include "SceneManager.h"

namespace Game
{
	struct CharacterManagerData;

	class CharacterManager
	{
		static CharacterManagerData* m_data;
		static Nxna::Graphics::GraphicsDevice* m_device;

	public:
		static void SetGlobalData(CharacterManagerData** data, Nxna::Graphics::GraphicsDevice* device);
		static void Shutdown();

		static void Reset();
		static void AddCharacter(SceneModelInfo model, float position[3], float rotation, float scale, bool isEgo);

		static void Process(Nxna::Matrix* modelview, float elapsed);

		static uint32 GetEgo() { return 0; }
		static void Idle(uint32 character);
		static void Idle(uint32 character, float faceDirection);
	private:
		static void updateTransform();
	};
}

#endif // GAME_CHARACTERMANAGER_H
