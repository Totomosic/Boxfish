#pragma once
#include <random>
#include "Logging.h"

namespace Boxfish
{

	class BOX_API Random
	{
	private:
		static std::mt19937 s_Engine;

	public:
		Random() = delete;

		static void Init();
		static void Seed(uint32_t seed);
		
		template<typename T>
		static T GetNext(T low, T high)
		{
			std::uniform_int_distribution<T> dist(low, high);
			return dist(s_Engine);
		}
	};

}
