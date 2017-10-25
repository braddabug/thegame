#ifndef GAME_SCRIPTMANAGER_H
#define GAME_SCRIPTMANAGER_H

#include "../Common.h"

#define CR_START(context) switch(context->State) { case 0: {
#define CR_STEP(context) } case __LINE__: { context->State = __LINE__;
#define CR_YIELD_WHILE(context, condition) case __LINE__: context->State = __LINE__; if (condition) return CoroutineResult::Suspended;
#define CR_BEGIN_WHILE(context, condition) case __LINE__: { context->State = __LINE__; { do { 
#define CR_END_WHILE(context) } while(0); } } case __LINE__: context->State = __LINE__;
#define CR_YIELD return CoroutineResult::Suspended;
#define CR_END } } return CoroutineResult::Success;

namespace Game
{
	struct VerbInfo
	{
		uint32 VerbHash;
		uint32 ActionHash;
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

	struct CRContext
	{
		static const unsigned int ContextDataSize = 128;
		unsigned char Data[ContextDataSize];
		unsigned int State;
	};

	enum class CoroutineResult
	{
		Success,
		Suspended,
		Error
	};

	typedef CoroutineResult(*Coroutine)(CRContext*);

	struct CoroutineParams
	{
		static const uint32 MaxParams = 24;

		uint8 Types[MaxParams];

		union
		{
			int IValue;
			uint32 UValue;
			float FValue;
		} Items[MaxParams];

		void Store(uint32 index, int value)
		{
			if (index < MaxParams)
			{
				Items[index].IValue = value;
				Types[index] = 0;
			}
		}

		void Store(uint32 index, uint32 value)
		{
			if (index < MaxParams)
			{
				Items[index].UValue = value;
				Types[index] = 1;
			}
		}

		void Store(uint32 index, float value)
		{
			if (index < MaxParams)
			{
				Items[index].FValue = value;
				Types[index] = 2;
			}
		}

		int GetI(uint32 index)
		{
			if (index < MaxParams && Types[index] == 0)
			{
				return Items[index].IValue;
			}

			return 0;
		}

		uint32 GetU(uint32 index)
		{
			if (index < MaxParams && Types[index] == 1)
			{
				return Items[index].UValue;
			}

			return 0;
		}

		float GetF(uint32 index)
		{
			if (index < MaxParams && Types[index] == 2)
			{
				return Items[index].FValue;
			}

			return 0;
		}
	};

	class ScriptManager
	{
		static ScriptManagerData* m_data;

	public:
		static void SetGlobalData(ScriptManagerData** data);
		static void Shutdown();

		static bool TestNounVerb(uint32 sceneID, uint32 nounHash, uint32 verbHash);
		static uint32 GetActiveVerbs(uint32 sceneID, uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs);
		static void DoVerb(uint32 nounHash, uint32 verbHash, uint32 actionHash);

		static uint32 BeginCoroutine(Coroutine routine, void* data, uint32 dataLength);
		static void StopCoroutine(uint32 contextHandle);

		static void RunAllScripts();
	};
}

#endif // GAME_SCRIPTMANAGER_H
