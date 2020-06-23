#pragma once
#include "Bitboard.h"
#include "Rays.h"

namespace Boxfish
{

	void InitAttacks();
	BitBoard GetNonSlidingAttacks(Piece piece, const Square& fromSquare, Team team);
	BitBoard GetSlidingAttacks(Piece piece, const Square& fromSquare, const BitBoard& blockers);

}
