#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstring>

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef uint64 StringRef;

// PersitantString means the string will live long enough for the function receiving it to
// do what it needs to do. No copy is made.
typedef const char* PersistantString;

union LocaleCode
{
	char Code[2];
	uint16 NumericCode;

	LocaleCode(const char* code)
	{
		memcpy(Code, code, 2);
	}
};

#ifdef _WIN32
#define P_OUT_OPTIONAL _Out_opt_
#else
#define P_OUT_OPTIONAL
#endif

struct Globals
{
	bool DevMode;
};

namespace Nxna
{
namespace Input
{
	class InputState;
}
}

extern Globals* g_globals;
extern Nxna::Input::InputState* g_inputState;

#endif // COMMON_H