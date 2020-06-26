#include "Search.h"

namespace Boxfish
{

	Search::Search(bool log)
		: m_TranspositionTable(), m_CurrentPosition(), m_BestMove(), m_BestScore(), m_Log(log)
	{
	}

	const Move& Search::GetBestMove() const
	{
		return m_BestMove;
	}

	Centipawns Search::GetBestScore() const
	{
		return m_BestScore;
	}

	void Search::SetCurrentPosition(const Position& position)
	{
		m_CurrentPosition = position;
	}

	void Search::SetCurrentPosition(Position&& position)
	{
		m_CurrentPosition = std::move(position);
	}

	void Search::Go(int depth)
	{
		StartSearch(m_CurrentPosition, depth);
	}

	void Search::Reset()
	{
		m_TranspositionTable.Clear();
		m_BestMove = Move();
		m_BestScore = -INF;
	}

	void Search::StartSearch(const Position& position, int depth)
	{
		m_Nodes = 0;
		int alpha = -INF;
		int beta = INF;
		MoveGenerator generator(position);
		const std::vector<Move>& legalMoves = generator.GetLegalMoves();
		Centipawns currentScore = -INF;
		for (const Move& move : legalMoves)
		{
			Position p = position;
			ApplyMove(p, move);
			Centipawns score = -Negamax(p, depth - 1, -beta, -alpha);
			if (score > currentScore)
			{
				m_BestMove = move;
				currentScore = score;
			}
		}
		m_BestScore = currentScore;
		std::cout << "Nodes: " << m_Nodes << std::endl;
	}

	Centipawns Search::Negamax(const Position& position, int depth, int alpha, int beta)
	{
		m_Nodes++;
		if (depth <= 0)
		{
			// Quiescence search instead
			return Evaluate(position, position.TeamToPlay);
		}
		Centipawns value = -INF;
		MoveGenerator movegen(position);
		const std::vector<Move>& moves = movegen.GetLegalMoves();
		for (const Move& move : moves)
		{
			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			Centipawns v = -Negamax(movedPosition, depth - 1, -beta, -alpha);
			value = std::max(value, v);
			alpha = std::max(value, alpha);
			if (alpha >= beta)
			{
				break;
			}
		}
		return value;
	}

}
