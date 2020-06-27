#pragma once
#include "Bitboard.h"
#include "Rays.h"

namespace Boxfish
{

	void InitAttacks();
	BitBoard GetNonSlidingAttacks(Piece piece, const Square& fromSquare, Team team);
	BitBoard GetSlidingAttacks(Piece piece, const Square& fromSquare, const BitBoard& blockers);
	BitBoard GetNonSlidingAttacks(Piece piece, SquareIndex fromSquare, Team team);
	BitBoard GetSlidingAttacks(Piece piece, SquareIndex fromSquare, const BitBoard& blockers);

	// Return a bitboard representing the squares between a and b not including a or b
	// If not on same rank/file/diagonal return 0
	BitBoard GetBitBoardBetween(SquareIndex a, SquareIndex b);
	BitBoard GetLineBetween(SquareIndex a, SquareIndex b);
	bool IsAligned(SquareIndex a, SquareIndex b, SquareIndex c);

}
