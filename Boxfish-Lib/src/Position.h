#pragma once
#include "Bitboard.h"
#include "ZobristHash.h"

namespace Boxfish
{

	using Centipawns = int;

	struct PositionInfo
	{
	public:
		BitBoard TeamPieces[TEAM_MAX];
		BitBoard PiecesByType[PIECE_MAX];
		BitBoard AllPieces;

		SquareIndex KingSquare[TEAM_MAX];
		BitBoard CheckedBy[TEAM_MAX];
		bool InCheck[TEAM_MAX];

		BitBoard BlockersForKing[TEAM_MAX];
		BitBoard Pinners[TEAM_MAX];

		Centipawns NonPawnMaterial[TEAM_MAX];
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
		inline BitBoard GetNotOccupied() const { return ~GetAllPieces(); }

		inline SquareIndex GetKingSquare(Team team) const { return InfoCache.KingSquare[team]; }

		inline const BitBoard& GetTeamPieces(Team team, Piece piece) const { return Teams[team].Pieces[piece]; }
		inline BitBoard GetTeamPieces(Team team, Piece piece, Piece piece2) const { return GetTeamPieces(team, piece) | GetTeamPieces(team, piece2); }
		inline BitBoard GetTeamPieces(Team team, Piece piece, Piece piece2, Piece piece3) const { return GetTeamPieces(team, piece, piece2) | GetTeamPieces(team, piece3); }
		inline const BitBoard& GetPieces(Piece piece) const { return InfoCache.PiecesByType[piece]; }
		inline BitBoard GetPieces(Piece piece, Piece piece2) const { return GetPieces(piece) | GetPieces(piece2); }
		inline BitBoard GetPieces(Piece piece, Piece piece2, Piece piece3) const { return GetPieces(piece, piece2) | GetPieces(piece3); }
		inline Centipawns GetNonPawnMaterial(Team team) const { return InfoCache.NonPawnMaterial[team]; }
		inline Centipawns GetNonPawnMaterial() const { return GetNonPawnMaterial(TEAM_WHITE) + GetNonPawnMaterial(TEAM_BLACK); }

		inline int GetTotalHalfMoves() const { return 2 * TotalTurns + ((TeamToPlay == TEAM_BLACK) ? 1 : 0); }

		void InvalidateTeam(Team team);
		void InvalidateAll();
	};

}
