#ifndef WAITMANAGER_H
#define WAITMANAGER_H

#include <vector>
#include <cstddef>

struct WaitManagerData
{
	// when checksum is even (or 0) that means it's dead, otherwise alive (in use)
	std::vector<uint16> Checksums;
};

class WaitManager
{
	static WaitManagerData* m_data;

public:

	enum class WaitStatus
	{
		Waiting,
		Done
	};

	static const int INVALID_WAIT = -1;

	static int CreateWait();

	static WaitStatus GetWaitStatus(int wait);

	static void SetWaitDone(int wait);
};
#endif // WAITMANAGER_H
