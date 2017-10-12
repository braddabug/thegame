#include "ScriptManager.h"
#include "../Utils.h"
#include "Verbs.h"
#include "../Logging.h"
#include "CharacterManager.h"

namespace Game
{
	bool TestNounVerb(uint32 nounHash, uint32 verbHash)
	{
		// TODO: this is an interesting way to do it.... but the problem is it's hard to
		// debug. How do we know which noun and verb this is?
		uint64 nv = ((uint64)nounHash << 32) | verbHash;

		switch (nv)
		{
		case ((uint64)Utils::CalcHash("ego") << 32) | (uint32)VerbHash::Look:
			return true;
		}

		return false;
	}

	uint32 GetActiveVerbs(uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs)
	{
		uint32 verbCount = 0;
		switch (nounHash)
		{
		case Utils::CalcHash("ego"):
			if (TestNounVerb(nounHash, (uint32)VerbHash::Look))
				verbs[verbCount++].VerbHash = (uint32)VerbHash::Look;
		}

		return verbCount;
	}

	void DoVerb(uint32 nounHash, uint32 verbHash)
	{
		if (TestNounVerb(nounHash, verbHash))
		{
			switch (nounHash)
			{
			case Utils::CalcHash("ego"):
				switch (verbHash)
				{
				case (uint32)VerbHash::Look:
				{
					CharacterManager::Idle(0, 1.5708f);
					LOG("That's me!");
					return;
				}
				}
			}
		}
	}



	struct ScriptManagerData
	{
		static const uint32 MaxScripts = 5;
		NvcScript Scripts[MaxScripts];
		uint32 NumScripts;
	};

	ScriptManagerData* ScriptManager::m_data;

	void ScriptManager::SetGlobalData(ScriptManagerData** data)
	{
		if (*data == nullptr)
		{
			*data = (ScriptManagerData*)g_memory->AllocTrack(sizeof(ScriptManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(ScriptManagerData));
			m_data = *data;
		}
		else
		{
			m_data = *data;
		}
	}

	void ScriptManager::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}


	bool Test(uint32 nounHash, uint32 verbHash, uint32 sceneID);
	uint32 GetVerbs(uint32 sceneID, uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs);
	void DoAction(uint32 nounHash, uint32 verbHash, uint32 actionHash);

	bool ScriptManager::TestNounVerb(uint32 sceneID, uint32 nounHash, uint32 verbHash)
	{
		return Test(nounHash, verbHash, sceneID);
	}

	uint32 ScriptManager::GetActiveVerbs(uint32 sceneID, uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs)
	{
		return GetVerbs(sceneID, nounHash, verbs, maxVerbs);
	}

	void ScriptManager::DoVerb(uint32 nounHash, uint32 verbHash, uint32 actionHash)
	{
		DoAction(nounHash, verbHash, actionHash);
	}

	// add game scripts here
	void Speak(uint32 nounHash, uint32 verbHash)
	{
		LOG("Speak action activated");
	}


	// make sure to include script.cpp last
	#include "Script.cpp"
}	