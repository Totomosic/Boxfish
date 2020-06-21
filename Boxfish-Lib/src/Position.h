#pragma once
#include "Bitboard.h"

namespace Boxfish
{

	struct BOX_API Position
	{
	public:
		struct BOX_API ColorPosition
		{
		public:
			BitBoard Pieces[PIECE_MAX];
			bool CastleKingSide = true;
			bool CastleQueenSide = true;
		};
	public:
		ColorPosition Teams[TEAM_MAX];
		Team TeamToPlay = TEAM_WHITE;
		int HalfTurnsSinceCaptureOrPush = 0;
		int TotalTurns = 0;
		Square EnpassantSquare = INVALID_SQUARE;
	};

}
