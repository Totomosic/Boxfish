#pragma once
#include "Types.h"

#ifdef SWIG
#define BOX_API
#endif

namespace Boxfish
{

	struct BOX_API BoxfishSettings
	{
	public:
		int MultiPV = 1;
		int SkillLevel = 20;
		size_t HashTableBytes = 50 * 1024 * 1024;
		int Threads = 1; // Unused at the moment
		int Contempt = 0;
	};

}