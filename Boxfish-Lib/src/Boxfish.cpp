#include "Boxfish.h"

namespace Boxfish
{

	Boxfish::Boxfish()
	{
		Logger::Init();
		Random::Init();
		InitZobristHash();
		InitRays();
		InitAttacks();
		InitEvaluation();
	}

}
