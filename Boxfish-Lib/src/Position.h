#pragma once
#include "Bitboard.h"
#include "ZobristHash.h"

namespace Boxfish
{

	struct PositionInfo
	{
	public:
		BitBoard TeamPieces[TEAM_MAX];
		BitBoard AllPieces;

		BitBoard BlockersForKing[TEAM_MAX];
		BitBoard Pinners[TEAM_MAX];
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
		ZobristHash Hash;

		PositionInfo InfoCache;

	public:
		const BitBoard& GetTeamPieces(Team team) const;
		const BitBoard& GetAllPieces() const;
		BitBoard GetNotOccupied() const;

		BitBoard GetTeamPieces(Team team, Piece piece) const;
		BitBoard GetTeamPieces(Team team, Piece piece, Piece piece2) const;
		BitBoard GetTeamPieces(Team team, Piece piece, Piece piece2, Piece piece3) const;
		BitBoard GetPieces(Piece piece) const;
		BitBoard GetPieces(Piece piece, Piece piece2) const;
		BitBoard GetPieces(Piece piece, Piece piece2, Piece piece3) const;

		int GetTotalHalfMoves() const;

		void InvalidateTeam(Team team);
		void InvalidateAll();
	};

}
