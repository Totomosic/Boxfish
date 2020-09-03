#include "Boxfish.h"

namespace Boxfish
{

	void Init()
	{
		Logger::Init();
		Random::Init();
		InitZobristHash();
		InitRays();
		InitAttacks();
		InitEvaluation();
		InitSearch();
	}

}
