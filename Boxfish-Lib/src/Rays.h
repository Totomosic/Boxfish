#pragma once
#include "Bitboard.h"

namespace Boxfish
{

	enum RayDirection
	{
		RAY_NORTH,
		RAY_NORTH_EAST,
		RAY_EAST,
		RAY_SOUTH_EAST,
		RAY_SOUTH,
		RAY_SOUTH_WEST,
		RAY_WEST,
		RAY_NORTH_WEST
	};

	void InitRays();
	BitBoard GetRay(RayDirection direction, SquareIndex square);

}
