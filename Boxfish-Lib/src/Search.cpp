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
		SearchRoot(m_CurrentPosition, depth);
	}

	void Search::Reset()
	{
		m_TranspositionTable.Clear();
		m_BestMove = Move();
		m_BestScore = -INF;
	}

	void Search::SearchRoot(const Position& position, int depth)
	{
		m_Nodes = 0;
		int alpha = -INF;
		int beta = INF;
		MoveGenerator generator(position);
		std::vector<Move> legalMoves = generator.GetLegalMoves();
		MoveSelector selector(legalMoves);
		Centipawns currentScore = -INF;
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Position p = position;
			ApplyMove(p, move);
			Centipawns score = -Negamax(p, depth - 1, -beta, -alpha);
			if (score > currentScore)
			{
				m_BestMove = move;
				currentScore = score;
				if (score == INF)
					break;
			}
		}
		m_BestScore = currentScore;
		TranspositionTableEntry entry;
		entry.Hash = m_CurrentPosition.Hash;
		entry.BestMove = m_BestMove;
		entry.Flag = EXACT;
		entry.Depth = depth;
		entry.Age = m_CurrentPosition.GetTotalHalfMoves();
		entry.Score = m_BestScore;
		m_TranspositionTable.AddEntry(entry);
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
		if (position.HalfTurnsSinceCaptureOrPush >= 50)
			return 0;

		int originalAlpha = alpha;

		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash);
		if (entry && entry->Depth >= depth)
		{
			if (SanityCheckMove(position, entry->BestMove))
			{
				switch (entry->Flag)
				{
				case EXACT:
					return entry->Score;
				case UPPER_BOUND:
					beta = std::min(beta, entry->Score);
					break;
				case LOWER_BOUND:
					alpha = std::max(alpha, entry->Score);
					break;
				}
				if (alpha >= beta)
					return entry->Score;
			}
		}

		Move bestMove;
		MoveGenerator movegen(position);
		std::vector<Move> moves = movegen.GetLegalMoves();
		if (moves.empty())
		{
			return Evaluate(position, position.TeamToPlay);
		}
		Move defaultMove = moves[0];
		MoveSelector selector(moves);
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			Centipawns v = -Negamax(movedPosition, depth - 1, -beta, -alpha);

			// Beta cuttoff
			if (v >= beta)
			{
				TranspositionTableEntry entry;
				entry.Score = v;
				entry.Depth = depth;
				entry.Flag = LOWER_BOUND;
				entry.Hash = position.Hash;
				entry.BestMove = move;
				entry.Age = position.GetTotalHalfMoves();
				m_TranspositionTable.AddEntry(entry);
				return beta;
			}
			if (v > alpha)
			{
				alpha = v;
				bestMove = move;
			}
		}
		if (bestMove.GetFlags() & MOVE_NULL)
		{
			bestMove = defaultMove;
		}
		EntryFlag flag;
		if (alpha >= originalAlpha)
			flag = UPPER_BOUND;
		else
			flag = EXACT;
		TranspositionTableEntry newEntry;
		newEntry.Age = position.GetTotalHalfMoves();
		newEntry.Depth = depth;
		newEntry.BestMove = bestMove;
		newEntry.Flag = flag;
		newEntry.Score = alpha;
		newEntry.Hash = position.Hash;
		m_TranspositionTable.AddEntry(newEntry);
		return alpha;
	}

}
