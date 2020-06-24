#pragma once
#include "Bitboard.h"

namespace Boxfish
{

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
	private:
		mutable bool m_IsValid[TEAM_MAX] = { false, false };
		mutable BitBoard m_TeamPieces[TEAM_MAX];
		mutable BitBoard m_AllPieces;

	public:
		TeamPosition Teams[TEAM_MAX];
		Team TeamToPlay = TEAM_WHITE;
		int HalfTurnsSinceCaptureOrPush = 0;
		int TotalTurns = 0;
		Square EnpassantSquare = INVALID_SQUARE;

	public:
		const BitBoard& GetTeamPieces(Team team) const;
		const BitBoard& GetAllPieces() const;
		BitBoard GetNotOccupied() const;

		void InvalidateTeam(Team team);
		void InvalidateAll();
	};

}
