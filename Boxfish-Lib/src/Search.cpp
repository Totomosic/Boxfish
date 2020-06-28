#include "Search.h"

namespace Boxfish
{

	Search::Search(bool log)
		: m_TranspositionTable(), m_CurrentPosition(), m_BestMove(Move::Null()), m_BestScore(), m_Log(log)
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

	const Line& Search::GetPV() const
	{
		return m_PV;
	}

	void Search::SetCurrentPosition(const Position& position)
	{
		m_CurrentPosition = position;
		m_PV = Line();
	}

	void Search::SetCurrentPosition(Position&& position)
	{
		m_CurrentPosition = std::move(position);
		m_PV = Line();
	}

	void Search::Go(int depth)
	{
		for (int i = 1; i <= depth; i++)
		{
			SearchRoot(m_CurrentPosition, i);
			if (m_Log)
			{
				std::cout << "info depth " << i << " score ";
				if (m_BestScore != INF && m_BestScore != -INF)
				{
					std::cout << "cp " << m_BestScore;
				}
				else
				{
					std::cout << "mate " << m_PV.CurrentIndex;
				}
				std::cout << " nodes " << m_Nodes;
				std::cout << " pv " << FormatLine(m_PV) << std::endl;
			}
		}
	}

	void Search::Reset()
	{
		m_TranspositionTable.Clear();
		m_BestMove = Move::Null();
		m_BestScore = -INF;
	}

	void Search::SearchRoot(const Position& position, int depth)
	{
		m_Nodes = 0;
		int alpha = -INF;
		int beta = INF;
		MoveGenerator generator(position);
		MoveList legalMoves = generator.GetPseudoLegalMoves();
		generator.FilterLegalMoves(legalMoves);

		MoveOrderingInfo ordering;
		if (m_PV.CurrentIndex > 0)
		{
			ordering.PVMove = &m_PV.Moves[0];
		}

		MoveSelector selector(&ordering, &legalMoves);
		Centipawns currentScore = -INF;
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Line line;
			line.Moves[line.CurrentIndex++] = move;
			Position p = position;
			ApplyMove(p, move);
			Centipawns score = -Negamax(p, depth - 1, -beta, -alpha, 1, line);
			if (score > currentScore)
			{
				m_BestMove = move;
				m_PV = line;
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
	}

	Centipawns Search::Negamax(const Position& position, int depth, int alpha, int beta, int searchIndex, Line& line)
	{
		m_Nodes++;
		if (depth <= 0)
		{
			// Quiescence search instead
			return QuiescenceSearch(position, alpha, beta);
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
					line.Moves[line.CurrentIndex++] = entry->BestMove;
					return entry->Score;
				case UPPER_BOUND:
					beta = std::min(beta, entry->Score);
					break;
				case LOWER_BOUND:
					alpha = std::max(alpha, entry->Score);
					break;
				}
				if (alpha >= beta)
				{
					line.Moves[line.CurrentIndex++] = entry->BestMove;
					return entry->Score;
				}
			}
		}

		Move bestMove;
		Line bestLine;
		MoveGenerator movegen(position);
		MoveList moves = movegen.GetPseudoLegalMoves();
		movegen.FilterLegalMoves(moves);
		if (moves.Empty())
		{
			return Evaluate(position, position.TeamToPlay);
		}
		Move defaultMove = moves.Moves[0];

		MoveOrderingInfo ordering;
		if (m_PV.CurrentIndex > searchIndex)
		{
			ordering.PVMove = &m_PV.Moves[searchIndex];
		}

		MoveSelector selector(&ordering, &moves);
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Line thisLine;
			thisLine.Moves[thisLine.CurrentIndex++] = move;
			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			Centipawns v = -Negamax(movedPosition, depth - 1, -beta, -alpha, searchIndex + 1, thisLine);

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
				bestLine = thisLine;
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
		for (size_t i = 0; i < bestLine.CurrentIndex; i++)
		{
			line.Moves[line.CurrentIndex++] = bestLine.Moves[i];
		}
		return alpha;
	}

	Centipawns Search::QuiescenceSearch(const Position& position, int alpha, int beta)
	{
		m_Nodes++;
		Centipawns evaluation = Evaluate(position, position.TeamToPlay);
		if (evaluation >= beta)
			return beta;
		if (alpha < evaluation)
			alpha = evaluation;

		MoveGenerator movegen(position);
		MoveList legalMoves = movegen.GetPseudoLegalMoves();
		movegen.FilterLegalMoves(legalMoves);
		if (legalMoves.Empty())
			return evaluation;
		QuiescenceMoveSelector selector(position, legalMoves);
		if (selector.Empty())
			return evaluation;
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			BOX_ASSERT(move.GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION), "Invalid QMove");
			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			Centipawns score = -QuiescenceSearch(movedPosition, -beta, -alpha);
			if (score >= beta)
				return score;
			if (score > alpha)
				alpha = score;
		}
		return alpha;
	}

}
