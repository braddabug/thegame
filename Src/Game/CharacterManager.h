#ifndef GAME_CHARACTERMANAGER_H
#define GAME_CHARACTERMANAGER_H

#include "../Common.h"
#include "../MyNxna2.h"

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
		static void AddCharacter(Graphics::Model* model, float position[3], float rotation, float scale, bool isEgo);

		static void Process(Nxna::Matrix* modelview, float elapsed);
		static void Render(Nxna::Matrix* modelview);

	private:
		static void updateTransform();
	};
}

#endif // GAME_CHARACTERMANAGER_H
