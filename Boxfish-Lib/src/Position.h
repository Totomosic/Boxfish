#pragma once
#include "Bitboard.h"

namespace Boxfish
{

	struct BOX_API Move
	{
	public:
		Square From;
		Square To;
	};

	struct BOX_API Position
	{
	public:
		struct BOX_API TeamPosition
		{
		public:
			BitBoard Pieces[PIECE_MAX];
			bool CastleKingSide = true;
			bool CastleQueenSide = true;
		};
	public:
		TeamPosition Teams[TEAM_MAX];
		Team TeamToPlay = TEAM_WHITE;
		int HalfTurnsSinceCaptureOrPush = 0;
		int TotalTurns = 0;
		Square EnpassantSquare = INVALID_SQUARE;
	};

}
