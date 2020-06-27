#include "Position.h"
#include <cstdarg>

namespace Boxfish
{

	const BitBoard& Position::GetTeamPieces(Team team) const
	{
		return InfoCache.TeamPieces[team];
	}

	const BitBoard& Position::GetAllPieces() const
	{
		return InfoCache.AllPieces;
	}

	BitBoard Position::GetNotOccupied() const
	{
		return ~GetAllPieces();
	}

	BitBoard Position::GetTeamPieces(Team team, Piece piece) const
	{
		return Teams[team].Pieces[piece];
	}

	BitBoard Position::GetTeamPieces(Team team, Piece piece, Piece piece2) const
	{
		return GetTeamPieces(team, piece) | GetTeamPieces(team, piece2);
	}

	BitBoard Position::GetTeamPieces(Team team, Piece piece, Piece piece2, Piece piece3) const
	{
		return GetTeamPieces(team, piece, piece2) | GetTeamPieces(team, piece3);
	}

	BitBoard Position::GetPieces(Piece piece) const
	{
		return Teams[TEAM_WHITE].Pieces[piece] | Teams[TEAM_BLACK].Pieces[piece];
	}

	BitBoard Position::GetPieces(Piece piece, Piece piece2) const
	{
		return GetPieces(piece) | GetPieces(piece2);
	}

	BitBoard Position::GetPieces(Piece piece, Piece piece2, Piece piece3) const
	{
		return GetPieces(piece, piece2) | GetPieces(piece3);
	}

	int Position::GetTotalHalfMoves() const
	{
		return 2 * TotalTurns + ((TeamToPlay == TEAM_BLACK) ? 1 : 0);
	}

	void Position::InvalidateTeam(Team team)
	{
		InfoCache.TeamPieces[team] = 0;
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
			InfoCache.TeamPieces[team] |= Teams[team].Pieces[piece];
	}

	void Position::InvalidateAll()
	{
		InfoCache.AllPieces = GetTeamPieces(TEAM_WHITE) | GetTeamPieces(TEAM_BLACK);
	}

}
