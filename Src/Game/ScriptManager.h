#ifndef GAME_SCRIPTMANAGER_H
#define GAME_SCRIPTMANAGER_H

#include "../Common.h"

namespace Game
{
	struct VerbInfo
	{
		uint32 VerbHash;
	};

	typedef bool(*TestNounVerbFunc)(uint32 nounHash, uint32 verbHash);
	typedef uint32(*GetActiveVerbsFunc)(uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs);
	typedef void(*DoVerbFunc)(uint32 nounHash, uint32 verbHash);

	struct NvcScript
	{
		TestNounVerbFunc TestNounVerb;
		GetActiveVerbsFunc GetActiveVerbs;
		DoVerbFunc DoVerb;
	};

	struct ScriptManagerData;

	class ScriptManager
	{
		static ScriptManagerData* m_data;

	public:
		static void SetGlobalData(ScriptManagerData** data);
		static void Shutdown();

		static void ClearNvcs();
		static void AddNvc(NvcScript* script);

		static bool TestNounVerb(uint32 nounHash, uint32 verbHash);
		static uint32 GetActiveVerbs(uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs);
		static void DoVerb(uint32 nounHash, uint32 verbHash);
	};
}

#endif // GAME_SCRIPTMANAGER_H
