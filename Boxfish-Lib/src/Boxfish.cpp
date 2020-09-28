#include "Boxfish.h"

namespace Boxfish
{

	void Init()
	{
#if (!defined(EMSCRIPTEN) || !defined(BOX_DIST))
		Logger::Init();
#endif
		Random::Init();
		InitZobristHash();
		InitRays();
		InitAttacks();
		InitEvaluation();
		InitSearch();
	}

}
