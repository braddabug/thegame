#ifndef WAITMANAGER_H
#define WAITMANAGER_H

#include <vector>
#include <cstddef>
#include "Common.h"

struct WaitManagerData;

typedef uint32 WaitHandle;

class WaitManager
{
	static WaitManagerData* m_data;

public:

	enum class WaitStatus
	{
		Waiting,
		Done
	};

	static const WaitHandle INVALID_WAIT = (WaitHandle)-1;

	static void SetGlobalData(WaitManagerData** data);
	static void Shutdown();
	static void Reset();

	static WaitHandle CreateWait();

	static WaitStatus GetWaitStatus(WaitHandle wait);

	static void SetWaitDone(WaitHandle wait);
};

#define WAIT_GET_INDEX(w) (w & 0xffff) 

#endif // WAITMANAGER_H
