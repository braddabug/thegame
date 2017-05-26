#include "Utils.h"

namespace Utils
{
	uint32 CalcHash(const uint8 *str)
	{
		// this is djb2: http://www.cse.yorku.ca/~oz/hash.html
		uint32 hash = 5381;
		int c;

		while (c = *str++)
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

		return hash;
	}
}