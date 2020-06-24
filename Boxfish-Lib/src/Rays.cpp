#include "Rays.h"

namespace Boxfish
{

	static bool s_Initialized = false;

	static BitBoard s_Rays[8][FILE_MAX * RANK_MAX];

	void InitRays()
	{
		if (!s_Initialized)
		{
			for (int index = 0; index < FILE_MAX * RANK_MAX; index++)
			{
				s_Rays[RAY_NORTH][index] = 0x0101010101010100ULL << index;
				s_Rays[RAY_SOUTH][index] = 0x0080808080808080ULL >> (63 - index);
				s_Rays[RAY_EAST][index] = 2 * ((1ULL << (index | 7)) - (1ULL << index));
				s_Rays[RAY_WEST][index] = (1ULL << index) - (1ULL << (index & 56));
				s_Rays[RAY_NORTH_WEST][index] = ShiftWest(0x102040810204000ULL, 7 - BitBoard::FileOfIndex(index)) << (BitBoard::RankOfIndex(index) * 8);
				s_Rays[RAY_NORTH_EAST][index] = ShiftEast(0x8040201008040200ULL, BitBoard::FileOfIndex(index)) << (BitBoard::RankOfIndex(index) * 8);
				s_Rays[RAY_SOUTH_WEST][index] = ShiftWest(0x40201008040201ULL, 7 - BitBoard::FileOfIndex(index)) >> ((7 - BitBoard::RankOfIndex(index)) * 8);
				s_Rays[RAY_SOUTH_EAST][index] = ShiftEast(0x2040810204080ULL, BitBoard::FileOfIndex(index)) >> ((7 - BitBoard::RankOfIndex(index)) * 8);
			}
			s_Initialized = true;
		}
	}

	BitBoard GetRay(RayDirection direction, SquareIndex square)
	{
		return s_Rays[direction][square];
	}

}
