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
		m_TranspositionTable.Clear();
		m_SearchDepth = depth;
		m_WasStopped = false;
		m_ShouldStop = false;
		m_StartTime = std::chrono::high_resolution_clock::now();

		SearchStats searchStats;

		for (int i = 1; i <= depth; i++)
		{
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();
			SearchRoot(m_CurrentPosition, i, searchStats);
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

				searchStats.PV = pv;
			}
		}
		std::cout << "Table Hits: " << searchStats.TableHits << std::endl;
		std::cout << "Table Misses: " << searchStats.TableMisses << std::endl;
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

	void Search::SearchRoot(const Position& position, int depth, SearchStats& stats)
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
		if (stats.PV.CurrentIndex > 0)
		{
			ordering.PVMove = stats.PV.Moves[0];
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

			SearchData childData;
			childData.Depth = depth - 1;
			childData.Ply = 1;
			childData.Alpha = rootMoveBonus - beta;
			childData.Beta = rootMoveBonus - alpha;
			childData.IsPV = move == stats.PV.Moves[0];

			Centipawns score;
			if (fullWindow)
			{
				score = rootMoveBonus - Negamax(p, childData, stats);
			}
			else
			{
				score = rootMoveBonus - Negamax(p, childData, stats);
				if (score > alpha)
				{
					childData.Alpha = -beta;
					childData.Beta = -alpha;
					score = rootMoveBonus - Negamax(p, childData, stats);
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

	Centipawns Search::Negamax(const Position& position, SearchData data, SearchStats& stats)
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
		if (m_PositionHistory.Contains(position, data.Ply))
		{
			return 0;
		}

		if (data.Depth <= 0)
		{
			return QuiescenceSearch(position, data.Alpha, data.Beta, data.Ply);
		}

		int originalAlpha = data.Alpha;

		// Find hash move
		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash);
		if (entry && entry->Depth >= data.Depth)
		{
			if (entry->Hash == position.Hash)
			{
				stats.TableHits++;
				switch (entry->Flag)
				{
				case EXACT:
					return entry->Score;
				case UPPER_BOUND:
					data.Beta = std::min(data.Beta, entry->Score);
					break;
				case LOWER_BOUND:
					data.Alpha = std::max(data.Alpha, entry->Score);
					break;
				}
				if (data.Alpha >= data.Beta)
				{
					return entry->Score;
				}
			}
			stats.TableMisses++;
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
		if (data.IsPV && stats.PV.CurrentIndex > data.Ply)
		{
			ordering.PVMove = stats.PV.Moves[data.Ply];
		}
		MoveSelector selector(ordering, &moves);

		int moveIndex = 0;
		int movesSinceBetaCutoff = 0;
		int depthExtension = 0;

		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			Position movedPosition = position;
			ApplyMove(movedPosition, move);

			if (move.GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION) && depthExtension < 0)
			{
				depthExtension = 0;
			}

			SearchData childData;
			childData.Depth = data.Depth - 1 + depthExtension;
			childData.Ply = data.Ply + 1;
			childData.Alpha = -data.Beta;
			childData.Beta = -data.Alpha;
			childData.IsPV = data.IsPV && move == stats.PV.Moves[data.Ply];

			Centipawns value = -Negamax(movedPosition, childData, stats);

			if (CheckLimits())
			{
				m_WasStopped = true;
				return 0;
			}

			// Beta cuttoff - Fail high
			if (value >= data.Beta)
			{
				movesSinceBetaCutoff = 0;
				depthExtension = 0;
				TranspositionTableEntry failHighEntry;
				failHighEntry.Score = value;
				failHighEntry.Depth = data.Depth;
				failHighEntry.Flag = LOWER_BOUND;
				failHighEntry.Hash = position.Hash;
				failHighEntry.BestMove = move;
				failHighEntry.Age = position.GetTotalHalfMoves();
				m_TranspositionTable.AddEntry(failHighEntry);
				return data.Beta;
			}
			if (value > data.Alpha && depthExtension < 0)
			{
				// Re search
				childData.Depth = data.Depth - 1;
				value = -Negamax(movedPosition, childData, stats);
			}
			if (value > data.Alpha)
			{
				data.Alpha = value;
				bestMove = move;
			}
			movesSinceBetaCutoff++;
			if (data.Depth > 3)
			{
				if (movesSinceBetaCutoff > 10)
				{
					depthExtension = -data.Depth / 3;
				}
				else if (movesSinceBetaCutoff > 3)
				{
					depthExtension = -1;
				}
			}
		}
		if (bestMove.GetFlags() & MOVE_NULL)
		{
			bestMove = defaultMove;
		}
		TranspositionTableEntry newEntry;
		newEntry.Age = position.GetTotalHalfMoves();
		newEntry.Depth = data.Depth;
		newEntry.BestMove = bestMove;
		newEntry.Flag = (data.Alpha > originalAlpha) ? EXACT : UPPER_BOUND;
		newEntry.Score = data.Alpha;
		newEntry.Hash = position.Hash;
		m_TranspositionTable.AddEntry(newEntry);
		return data.Alpha;
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
		return 0;
	}

}
