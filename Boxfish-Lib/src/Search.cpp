#include "Search.h"
#include "Random.h"

namespace Boxfish
{

	void UpdatePV(Move* pv, const Move& move, Move* childPV)
	{
		for (*pv++ = move; childPV && *childPV != Move::Null(); )
			*pv++ = *childPV++;
		*pv = Move::Null();
	}

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

	Search::Search(size_t transpositionTableSize, bool log)
		: m_TranspositionTable(transpositionTableSize), m_CurrentPosition(), m_PositionHistory(), m_Limits(), m_MovePool(MOVE_POOL_SIZE), m_Nodes(0), m_SearchRootStartTime(), m_StartTime(),
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

	void Search::SetCurrentPosition(const Position& position)
	{
		m_CurrentPosition = position;
	}

	void Search::SetCurrentPosition(Position&& position)
	{
		m_CurrentPosition = std::move(position);
	}

	Move Search::Go(int depth, const std::function<void(SearchResult)>& callback)
	{
		m_TranspositionTable.Clear();
		m_WasStopped = false;
		m_ShouldStop = false;
		m_StartTime = std::chrono::high_resolution_clock::now();

		SearchStats searchStats;

		SearchRoot(m_CurrentPosition, depth, searchStats, callback);
		if (m_RootMoves.size() > 0)
		{
			return m_RootMoves[0].PV[0];
		}
		return Move::Null();
	}

	void Search::Ponder(const std::function<void(SearchResult)>& callback)
	{
		m_Limits.Infinite = true;
		Go(MAX_PLY, callback);
	}

	void Search::Reset()
	{
		m_TranspositionTable.Clear();
		m_PositionHistory.Clear();
	}

	void Search::Stop()
	{
		m_ShouldStop = true;
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

	void Search::SearchRoot(const Position& position, int depth, SearchStats& stats, const std::function<void(SearchResult)>& callback)
	{
		SearchStack stack[MAX_PLY + 1];
		Move pv[MAX_PLY + 1];
		m_Nodes = 0;
		Centipawns alpha = -SCORE_MATE;
		Centipawns beta = SCORE_MATE;
		Centipawns delta = -SCORE_MATE;
		Centipawns bestScore;

		stack->PV = pv;
		stack->PV[0] = Move::Null();
		stack->Ply = 0;
		stack->InCheck = false;
		stack->StaticEvaluation = SCORE_NONE;

		int rootDepth = 0;

		while ((++rootDepth) < MAX_PLY)
		{
			m_RootMoves.clear();
			m_RootMoves.reserve(MAX_MOVES);
			m_Nodes = 0;
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();

			// Aspiration windows
			if (rootDepth >= 4 && false)
			{
				Centipawns prevMaxScore = bestScore;
				delta = 20;
				alpha = std::max(prevMaxScore - delta, -SCORE_MATE);
				beta = std::min(prevMaxScore + delta, SCORE_MATE);
			}

			while (true)
			{
				bestScore = SearchPosition<NodeType::PV>(position, stack, rootDepth, alpha, beta, stats);
				if (bestScore <= alpha)
				{
					beta = (alpha + beta) / 2;
					alpha = std::max(bestScore - delta, -SCORE_MATE);
				}
				else if (bestScore >= beta)
				{
					beta = std::min(bestScore + delta, SCORE_MATE);
				}
				else
				{
					break;
				}
				delta += delta / 4 + 5;
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				break;
			}

			std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());
			RootMove& rootMove = m_RootMoves[0];
			std::vector<Move>& rootPV = rootMove.PV;

			if (m_Log)
			{
				auto elapsed = std::chrono::high_resolution_clock::now() - m_SearchRootStartTime;

				std::cout << "info depth " << rootDepth << " score ";
				if (rootMove.Score != SCORE_MATE && rootMove.Score != -SCORE_MATE)
				{
					std::cout << "cp " << rootMove.Score;
				}
				else
				{
					std::cout << "mate " << ((int)rootPV.size() / 2) * ((rootMove.Score == SCORE_MATE) ? 1 : -1);
				}
				std::cout << " nodes " << m_Nodes;
				std::cout << " nps " << (size_t)(m_Nodes / (elapsed.count() / 1e9f));
				std::cout << " time " << (size_t)(elapsed.count() / 1e6f);
				std::cout << " pv";
				for (const Move& move : rootPV)
				{
					std::cout << " " << FormatMove(move);
				}
				std::cout << std::endl;
			}

			if (callback)
			{
				SearchResult result;
				result.BestMove = pv[0];
				result.PV = rootPV;
				result.Score = rootMove.Score;
				callback(result);
			}

			if (rootDepth >= depth)
			{
				break;
			}
		}
	}

	template<Search::NodeType NT>
	Centipawns Search::SearchPosition(const Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, SearchStats& stats)
	{
		constexpr bool IsPvNode = NT == NodeType::PV;
		const bool isRoot = IsPvNode && stack->Ply == 0;

		Move pv[MAX_PLY + 1];

		stack->MoveCount = 0;
		stack->StaticEvaluation = SCORE_NONE;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 2)->KillerMoves[0] = (stack + 2)->KillerMoves[1] = Move::Null();

		if (CheckLimits() || stack->Ply >= MAX_PLY)
		{
			m_WasStopped = true;
			return SCORE_DRAW;
		}

		m_Nodes++;

		// Check for draw
		if (position.HalfTurnsSinceCaptureOrPush >= 50)
		{
			Centipawns eval = EvaluateDraw(position);
			stack->StaticEvaluation = eval;
			return eval;
		}
		if (m_PositionHistory.Contains(position, stack->Ply))
		{
			Centipawns eval = EvaluateDraw(position);
			stack->StaticEvaluation = eval;
			return eval;
		}

		if (depth <= 0)
		{
			return QuiescenceSearch<NT>(position, stack, alpha, beta);
			//return Evaluate(position, position.TeamToPlay);
		}

		int originalAlpha = alpha;

		// Find hash move
		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash);
		if (entry && entry->Depth >= depth && !IsPvNode)
		{
			if (entry->Hash == position.Hash)
			{
				stats.TableHits++;
				switch (entry->Flag)
				{
				case EXACT:
				{
					return entry->Score;
				}
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
			stats.TableMisses++;
		}

		Move bestMove = Move::Null();
		MoveGenerator movegen(position);
		MoveList legalMoves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(legalMoves);
		movegen.FilterLegalMoves(legalMoves);
		if (legalMoves.Empty())
		{
			return Evaluate(position, position.TeamToPlay);
		}
		Move defaultMove = legalMoves.Moves[0];

		MoveOrderingInfo ordering;
		ordering.CurrentPosition = &position;
		MoveSelector selector(ordering, &legalMoves);

		int moveIndex = 0;
		int movesSinceBetaCutoff = 0;
		int depthExtension = 0;

		Centipawns bestValue = -SCORE_MATE;

		while (!selector.Empty())
		{
			moveIndex++;
			Move move;
			if (moveIndex == 1 && IsPvNode && stack->PV[0] != Move::Null())
				move = stack->PV[0];
			else
			{
				move = selector.GetNextMove();
				if (IsPvNode && move == stack->PV[0])
					continue;
			}

			(stack + 1)->CurrentMove = move;

			Position movedPosition = position;
			ApplyMove(movedPosition, move);

			if ((move.GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION)) && depthExtension < 0)
			{
				depthExtension = 0;
			}

			int childDepth = depth + depthExtension - 1;

			stack->MoveCount++;

			Centipawns value;
			if (!IsPvNode || moveIndex != 1)
			{
				value = -SearchPosition<NodeType::NonPV>(movedPosition, stack + 1, childDepth, -beta, -alpha, stats);
			}
			if (IsPvNode && (moveIndex == 1 || (value > alpha && (isRoot || value < beta))))
			{
				childDepth = depth - 1;
				(stack + 1)->PV = pv;
				(stack + 1)->PV[0] = Move::Null();
				value = -SearchPosition<NodeType::PV>(movedPosition, stack + 1, childDepth, -beta, -alpha, stats);
			}
			
			if (isRoot)
			{
				RootMove rm;
				if (moveIndex == 1 || value > alpha && (stack)->PV)
				{
					UpdatePV(stack->PV, move, (stack + 1)->PV);
					rm.Score = value;
					for (Move* m = stack->PV; *m != Move::Null(); m++)
					{
						rm.PV.push_back(*m);
					}
				}
				else
				{
					rm.Score = -SCORE_MATE;
				}
				m_RootMoves.push_back(rm);
			}

			if (value > bestValue)
			{
				bestMove = move;
				bestValue = value;
				if (value > alpha)
				{
					alpha = value;
				}
				if (IsPvNode)
				{
					UpdatePV(stack->PV, move, (stack + 1)->PV);
				}
			}

			if (value >= SCORE_MATE)
				break;

			// Beta cuttoff - Fail high
			if (value >= beta)
			{
				movesSinceBetaCutoff = 0;
				depthExtension = 0;
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

			if (CheckLimits())
			{
				m_WasStopped = true;
				return SCORE_DRAW;
			}
			
			movesSinceBetaCutoff++;
			if (depth > 3)
			{
				if (movesSinceBetaCutoff > 10)
				{
					depthExtension = -depth / 3;
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
		newEntry.Depth = depth;
		newEntry.BestMove = bestMove;
		newEntry.Flag = (alpha > originalAlpha) ? EXACT : UPPER_BOUND;
		newEntry.Score = bestValue;
		newEntry.Hash = position.Hash;
		m_TranspositionTable.AddEntry(newEntry);
		return bestValue;
	}

	template<Search::NodeType NT>
	Centipawns Search::QuiescenceSearch(const Position& position, SearchStack* stack, Centipawns alpha, Centipawns beta)
	{
		constexpr bool IsPvNode = NT == NodeType::PV;

		if (CheckLimits())
		{
			m_WasStopped = true;
			return SCORE_DRAW;
		}

		(stack + 1)->Ply = stack->Ply + 1;

		m_Nodes++;

		if (position.HalfTurnsSinceCaptureOrPush >= 50)
			return EvaluateDraw(position);
		if (m_PositionHistory.Contains(position, stack->Ply))
			return EvaluateDraw(position);

		Centipawns evaluation = Evaluate(position, position.TeamToPlay);
		if (evaluation >= beta)
			return beta;
		if (alpha < evaluation)
			alpha = evaluation;

		MoveGenerator generator(position);
		MoveList legalMoves = m_MovePool.GetList();
		generator.GetPseudoLegalMoves(legalMoves);
		QuiescenceMoveSelector selector(position, legalMoves);
		if (selector.Empty())
			return evaluation;
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();
			if (generator.IsLegal(move))
			{
				Position movedPosition = position;
				ApplyMove(movedPosition, move);
				Centipawns score = -QuiescenceSearch<NT>(movedPosition, stack + 1, -beta, -alpha);

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

	Centipawns Search::EvaluateDraw(const Position& position) const
	{
		return SCORE_DRAW + 2 * ((int)(position.Hash.Hash + (size_t)position.GetTotalHalfMoves() * 1248125) & 1) - 1;
	}

}
