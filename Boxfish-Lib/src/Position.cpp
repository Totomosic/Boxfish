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
