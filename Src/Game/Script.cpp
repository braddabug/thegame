#include "../Common.h"

bool Test(uint32 nounHash, uint32 verbHash, uint32 sceneID)
{
	switch(sceneID) {
		case Utils::CalcHash("scene"):
		switch(nounHash) {
			case Utils::CalcHash("ego"):
			switch(verbHash) {
				case Utils::CalcHash("look"):
					return true;
			}
			break;
		}
		break;
	}
	return false;
}

uint32 GetVerbs(uint32 sceneID, uint32 nounHash, VerbInfo* verbs, uint32 maxVerbs)
{
	uint32 verbCount = 0;

	switch (sceneID) {
		case Utils::CalcHash("scene"):
		switch (nounHash) {
			case Utils::CalcHash("ego"):
				verbs[verbCount].ActionHash = Utils::CalcHash("Speak");
				verbs[verbCount++].VerbHash = Utils::CalcHash("look");
			break;
		}
		break;
	}
	return verbCount;
}

void DoAction(uint32 nounHash, uint32 verbHash, uint32 actionHash)
{
	Game::CoroutineParams params;
	params.Store(0, nounHash);
	params.Store(1, verbHash);

    switch(actionHash) {
    case Utils::CalcHash("Speak"): 
	{
		Game::ScriptManager::BeginCoroutine(Speak, &params, sizeof(Game::CoroutineParams)); break;
	}
    }
}
