#include "Random.h"

namespace Boxfish
{

	std::mt19937 Random::s_Engine;

	void Random::Init()
	{
		
	}

	void Random::Seed(uint32_t seed)
	{
		s_Engine.seed(seed);
	}

}
