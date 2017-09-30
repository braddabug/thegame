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

			ClearNvcs();
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

	void ScriptManager::ClearNvcs()
	{
		// add the default global script, erase the rest
		m_data->Scripts[0].TestNounVerb = Game::TestNounVerb;
		m_data->Scripts[0].GetActiveVerbs = Game::GetActiveVerbs;
		m_data->Scripts[0].DoVerb = Game::DoVerb;
		m_data->NumScripts = 1;
	}

	bool ScriptManager::TestNounVerb(uint32 nounHash, uint32 verbHash)
	{
		// work backwards through the list of scripts. This assumes higher priority scripts are at the end.
		for (uint32 i = m_data->NumScripts; i-- > 0; )
		{
			if (m_data->Scripts[i].TestNounVerb(nounHash, verbHash))
				return true;
		}

		return false;
	}

	uint32 ScriptManager::GetActiveVerbs(uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs)
	{
		uint32 verbCount = 0;
		for (uint32 i = m_data->NumScripts; i-- > 0; )
		{
			VerbInfo tmpVerbs[10];
			auto tmpVerbCount = m_data->Scripts[i].GetActiveVerbs(nounHash, tmpVerbs, 10);

			// don't add duplicate verbs
			for (uint32 j = 0; j < tmpVerbCount; j++)
			{
				bool found = false;
				for (uint32 k = 0; k < verbCount; k++)
				{
					if (verbs[j].VerbHash == tmpVerbs[k].VerbHash)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					verbs[verbCount++] = tmpVerbs[j];
				}

				if (verbCount >= maxVerbs)
					return verbCount;
			}
		}

		return verbCount;
	}

	void ScriptManager::DoVerb(uint32 nounHash, uint32 verbHash)
	{
		for (uint32 i = m_data->NumScripts; i-- > 0; )
		{
			if (m_data->Scripts[i].TestNounVerb(nounHash, verbHash))
			{
				m_data->Scripts[i].DoVerb(nounHash, verbHash);
				break;
			}
		}
	}
}	