#include "Search.h"
#include "Random.h"

namespace Boxfish
{

	PositionHistory::PositionHistory()
		: m_Hashes()
	{
		m_Hashes.reserve(8);
	}

	bool PositionHistory::Contains(const Position& position, int searchIndex) const
	{
		for (int i = 0; i < m_Hashes.size(); i++)
		{
			if (m_Hashes[i] == position.Hash)
			{
				return true;
			}
		}
		return false;
	}

	void PositionHistory::Push(const Position& position)
	{
		m_Hashes.push_back(position.Hash);
		if (m_Hashes.size() > 6)
			m_Hashes.erase(m_Hashes.begin());
	}

	void PositionHistory::Clear()
	{
		m_Hashes.clear();
	}

	Search::Search(bool log)
		: m_TranspositionTable(), m_CurrentPosition(), m_PositionHistory(), m_Limits(), m_BestMove(Move::Null()), m_BestScore(), m_SearchDepth(0), m_Nodes(0), m_SearchRootStartTime(), m_StartTime(), 
		m_WasStopped(false), m_ShouldStop(false), m_Log(log), m_OrderingInfo()
	{
	}

	PositionHistory& Search::GetHistory()
	{
		return m_PositionHistory;
	}

	void Search::SetLimits(const SearchLimits& limits)
	{
		m_Limits = limits;
	}

	const Move& Search::GetBestMove() const
	{
		return m_BestMove;
	}

	Centipawns Search::GetBestScore() const
	{
		return m_BestScore;
	}

	Line Search::GetPV() const
	{
		return GetPV(m_SearchDepth);
	}

	void Search::SetCurrentPosition(const Position& position)
	{
		m_CurrentPosition = position;
	}

	void Search::SetCurrentPosition(Position&& position)
	{
		m_CurrentPosition = std::move(position);
	}

	Move Search::Go(int depth)
	{
		m_SearchDepth = depth;
		m_WasStopped = false;
		m_ShouldStop = false;
		m_StartTime = std::chrono::high_resolution_clock::now();

		Line currentPV;

		for (int i = 1; i <= depth; i++)
		{
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();
			SearchRoot(m_CurrentPosition, i, currentPV);
			std::chrono::time_point<std::chrono::high_resolution_clock> endTime = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - m_SearchRootStartTime);
			if (m_Log && !m_WasStopped)
			{
				Line pv = GetPV(i);
				std::cout << "info depth " << i << " score ";
				if (m_BestScore != INF && m_BestScore != -INF)
				{
					std::cout << "cp " << m_BestScore;
				}
				else
				{
					std::cout << "mate " << pv.CurrentIndex;
				}
				std::cout << " nodes " << m_Nodes;
				std::cout << " nps " << (size_t)(m_Nodes / (elapsed.count() / 1e9f));
				std::cout << " time " << (size_t)(elapsed.count() / 1e6f);
				std::cout << " pv " << FormatLine(pv) << std::endl;

				currentPV = pv;
			}
		}
		return m_BestMove;
	}

	void Search::Reset()
	{
		m_TranspositionTable.Clear();
		m_BestMove = Move::Null();
		m_BestScore = -INF;
		m_PositionHistory.Clear();
	}

	Line Search::GetPV(int depth) const
	{
		Position position = m_CurrentPosition;
		Line pv;

		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash);
		for (int i = 0; i < depth && (entry && entry->Hash == position.Hash); i++)
		{
			Move move = entry->BestMove;
			pv.Moves[pv.CurrentIndex++] = move;
			ApplyMove(position, move);
			entry = m_TranspositionTable.GetEntry(position.Hash);
		}
		return pv;
	}

	void Search::SearchRoot(const Position& position, int depth, const Line& pv)
	{
		m_Nodes = 0;
		int alpha = -INF;
		int beta = INF;
		bool fullWindow = true;
		if (depth >= 3)
		{
			fullWindow = false;
			alpha = m_BestScore - 50;
			beta = m_BestScore + 50;
		}

		MoveGenerator generator(position);
		MoveList legalMoves = generator.GetPseudoLegalMoves();
		generator.FilterLegalMoves(legalMoves);

		MoveOrderingInfo ordering;
		ordering.CurrentPosition = &m_CurrentPosition;
		ordering.MoveEvaluator = std::bind(&Search::GetMoveScoreBonus, this, std::placeholders::_1, std::placeholders::_2);
		if (pv.CurrentIndex > 0)
		{
			ordering.PVMove = &pv.Moves[0];
		}

		if (legalMoves.Empty())
		{
			m_BestMove = Move::Null();
		}

		MoveSelector selector(m_OrderingInfo, &legalMoves);
		Centipawns currentScore = -INF;
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Line line;
			line.Moves[line.CurrentIndex++] = move;
			Position p = position;
			ApplyMove(p, move);
			Centipawns rootMoveBonus = GetMoveScoreBonus(position, move);

			Centipawns score;
			if (fullWindow)
			{
				score = rootMoveBonus - Negamax(p, depth - 1, rootMoveBonus - beta, rootMoveBonus - alpha, 1, pv);
			}
			else
			{
				score = rootMoveBonus - Negamax(p, depth - 1, rootMoveBonus - beta, rootMoveBonus - alpha, 1, pv);
				if (score > alpha)
				{
					alpha = -INF;
					beta = INF;
					score = rootMoveBonus - Negamax(p, depth - 1, rootMoveBonus - beta, rootMoveBonus - alpha, 1, pv);
					fullWindow = true;
				}
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				return;
			}

			if (score > currentScore || currentScore == -INF)
			{
				m_BestMove = move;
				currentScore = score;
				if (score == INF)
					break;
				if (score > alpha)
				{
					alpha = score;
				}
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

	Centipawns Search::Negamax(const Position& position, int depth, int alpha, int beta, int searchIndex, const Line& pv)
	{
		if (CheckLimits())
		{
			m_WasStopped = true;
			return 0;
		}

		m_Nodes++;

		// Check for draw
		if (position.HalfTurnsSinceCaptureOrPush >= 50)
		{
			return 0;
		}
		if (m_PositionHistory.Contains(position, searchIndex))
		{
			return 0;
		}

		if (depth <= 0)
		{
			return QuiescenceSearch(position, alpha, beta, searchIndex);
		}
		// Find hash move
		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash);
		if (entry && entry->Depth >= depth)
		{
			if (entry->Hash == position.Hash)
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
				{
					return entry->Score;
				}
			}
		}

		Move bestMove = Move::Null();
		MoveGenerator movegen(position);
		MoveList moves = movegen.GetPseudoLegalMoves();
		movegen.FilterLegalMoves(moves);
		if (moves.Empty())
		{
			return Evaluate(position, position.TeamToPlay);
		}
		Move defaultMove = moves.Moves[0];

		MoveOrderingInfo ordering;
		ordering.CurrentPosition = &position;
		if (pv.CurrentIndex > searchIndex)
		{
			ordering.PVMove = &pv.Moves[searchIndex];
		}

		MoveSelector selector(ordering, &moves);
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Position movedPosition = position;
			ApplyMove(movedPosition, move);

			Centipawns value = -Negamax(movedPosition, depth - 1, -beta, -alpha, searchIndex + 1, pv);

			if (CheckLimits())
			{
				m_WasStopped = true;
				return 0;
			}

			// Beta cuttoff - Fail high
			if (value >= beta)
			{
				TranspositionTableEntry failHighEntry;
				failHighEntry.Score = value;
				failHighEntry.Depth = depth;
				failHighEntry.Flag = LOWER_BOUND;
				failHighEntry.Hash = position.Hash;
				failHighEntry.BestMove = move;
				failHighEntry.Age = position.GetTotalHalfMoves();
				m_TranspositionTable.AddEntry(failHighEntry);
				return beta;
			}
			if (value > alpha)
			{
				alpha = value;
				bestMove = move;
			}
		}
		if (bestMove.GetFlags() & MOVE_NULL)
		{
			bestMove = defaultMove;
		}
		TranspositionTableEntry newEntry;
		newEntry.Age = position.GetTotalHalfMoves();
		newEntry.Depth = depth;
		newEntry.BestMove = bestMove;
		newEntry.Flag = EXACT;
		newEntry.Score = alpha;
		newEntry.Hash = position.Hash;
		m_TranspositionTable.AddEntry(newEntry);
		return alpha;
	}

	Centipawns Search::QuiescenceSearch(const Position& position, int alpha, int beta, int searchIndex)
	{
		if (CheckLimits())
		{
			m_WasStopped = true;
			return 0;
		}

		m_Nodes++;

		if (position.HalfTurnsSinceCaptureOrPush >= 50)
			return 0;
		if (m_PositionHistory.Contains(position, searchIndex))
			return 0;

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
			Centipawns score = -QuiescenceSearch(movedPosition, -beta, -alpha, searchIndex + 1);

			if (CheckLimits())
			{
				m_WasStopped = true;
				break;
			}

			if (score >= beta)
				return score;
			if (score > alpha)
				alpha = score;
		}
		return alpha;
	}

	bool Search::CheckLimits() const
	{
		if (m_Limits.Infinite)
			return m_ShouldStop;
		if (m_Limits.Milliseconds > 0)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime);
			if (elapsed.count() >= m_Limits.Milliseconds)
				return true;
		}
		if (m_Limits.Nodes > 0)
		{
			return m_Nodes >= m_Limits.Nodes;
		}
		return m_ShouldStop || m_WasStopped;
	}

	Centipawns Search::GetMoveScoreBonus(const Position& position, const Move& move) const
	{
		if (move.GetMovingPiece() == PIECE_PAWN)
		{
			if (move.GetFromSquareIndex() == e2 && move.GetToSquareIndex() == e4)
				return 10;
			if (move.GetFromSquareIndex() == d7 && move.GetToSquareIndex() == d5)
				return 10;
		}
		return 0;
	}

}
