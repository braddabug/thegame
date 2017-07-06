#include "Utils.h"
#include <cstdlib>
#include <cstring>

namespace Utils
{
	int CompareI(const char* strA, uint32 strALength, const char* strB)
	{
		int strBLength = strlen(strB);
		int lenDiff = strALength - strBLength;

		if (lenDiff != 0)
			return lenDiff;

#ifdef _WIN32
		return _strnicmp(strA, strB, strALength);
#else
		return strncasecmp(strA, strB, strALength);
#endif
	}

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