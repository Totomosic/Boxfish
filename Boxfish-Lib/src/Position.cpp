#include "Position.h"

namespace Boxfish
{

	const BitBoard& Position::GetTeamPieces(Team team) const
	{
		if (!m_IsValid[team])
		{
			m_TeamPieces[team] = 0;
			for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
				m_TeamPieces[team] |= Teams[team].Pieces[piece];
		}
		return m_TeamPieces[team];
	}

	const BitBoard& Position::GetAllPieces() const
	{
		if (!m_IsValid[TEAM_WHITE] || !m_IsValid[TEAM_BLACK])
		{
			m_AllPieces = GetTeamPieces(TEAM_WHITE) | GetTeamPieces(TEAM_BLACK);
		}
		return m_AllPieces;
	}

	BitBoard Position::GetNotOccupied() const
	{
		return ~GetAllPieces();
	}

	int Position::GetTotalHalfMoves() const
	{
		return 2 * TotalTurns + ((TeamToPlay == TEAM_BLACK) ? 1 : 0);
	}

	void Position::InvalidateTeam(Team team)
	{
		m_IsValid[team] = false;
	}

	void Position::InvalidateAll()
	{
		m_IsValid[TEAM_WHITE] = false;
		m_IsValid[TEAM_BLACK] = false;
	}

}
