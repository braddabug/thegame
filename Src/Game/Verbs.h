#ifndef GAME_VERBS_H
#define GAME_VERBS_H

namespace Game
{
#ifdef DVERB
#undef DVERB
#endif
#define DVERB(v) v,

	enum class Verb
	{
#include "Verbs.inc"

		LAST
	};

#ifdef DVERB
#undef DVERB
#endif
#define DVERB(v) v = Utils::CalcHash( #v ),

	enum class VerbHash
	{
#include "Verbs.inc"
	};
}

#endif // GAME_VERBS_H
