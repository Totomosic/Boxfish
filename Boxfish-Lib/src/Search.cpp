#include "Search.h"
#include "Random.h"
#include "Uci.h"

namespace Boxfish
{

	void UpdatePV(Move* pv, const Move& move, Move* childPV)
	{
		/*for (*pv++ = move; childPV && *childPV != MOVE_NONE; )
			*pv++ = *childPV++;
		*pv = MOVE_NONE;*/
		Move* currentPV = pv;
		Move* currentChild = childPV;
		*currentPV++ = move;
		while (currentChild && *currentChild != MOVE_NONE)
		{
			*currentPV++ = *currentChild++;
		}
		*currentPV = MOVE_NONE;
	}

	bool ValidatePV(const Position& position, Move* pv)
	{
		if (*pv == MOVE_NONE)
			return true;
		Move moves[MAX_MOVES];
		MoveList list(nullptr, moves);
		MoveGenerator generator(position);
		generator.GetPseudoLegalMoves(list);
		generator.FilterLegalMoves(list);

		for (int i = 0; i < list.MoveCount; i++)
		{
			if (list.Moves[i] == *pv)
			{
				Position movedPosition = position;
				ApplyMove(movedPosition, *pv);
				return ValidatePV(movedPosition, pv + 1);
			}
		}
		return false;
	}

	PositionHistory::PositionHistory()
		: m_Hashes()
	{
	}

	const std::vector<ZobristHash>& PositionHistory::GetPositions() const
	{
		return m_Hashes;
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
		// Irreversible move played, clear history
		if (position.HalfTurnsSinceCaptureOrPush <= 0)
			m_Hashes.clear();
		m_Hashes.push_back(position.Hash);
	}

	void PositionHistory::Clear()
	{
		m_Hashes.clear();
	}

	Search::Search(size_t transpositionTableSize, bool log)
		: m_TranspositionTable(transpositionTableSize), m_CurrentPosition(), m_PositionHistory(), m_Limits(), m_MovePool(MOVE_POOL_SIZE), m_Nodes(0), m_SearchRootStartTime(), m_StartTime(),
		m_WasStopped(false), m_ShouldStop(false), m_Log(log), m_CounterMoves(), m_HistoryTable(), m_ButterflyTable()
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

#define BOX_UNDO_MOVES 0

	size_t Search::Perft(int depth)
	{
		size_t total = 0;
		MoveGenerator movegen(m_CurrentPosition);
		MoveList moves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(moves);
		movegen.FilterLegalMoves(moves);

		auto startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < moves.MoveCount; i++)
		{
#if BOX_UNDO_MOVES
			UndoInfo undo;
			ApplyMove(position, moves.Moves[i], &undo);
			uint64_t perft = Perft(position, depth - 1);
			UndoMove(position, moves.Moves[i], undo);
#else
			Position movedPosition = m_CurrentPosition;
			ApplyMove(movedPosition, moves.Moves[i]);
			size_t perft = Perft(movedPosition, depth - 1);
#endif
			total += perft;
			if (m_Log)
				std::cout << UCI::FormatMove(moves.Moves[i]) << ": " << perft << std::endl;
		}
		auto endTime = std::chrono::high_resolution_clock::now();
		auto elapsed = endTime - startTime;
		if (m_Log)
		{
			std::cout << "====================================" << std::endl;
			std::cout << "Total Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms" << std::endl;
			std::cout << "Total Nodes: " << total << std::endl;
			std::cout << "Nodes per Second: " << (size_t)(total / (double)std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() * 1e9) << std::endl;
		}
		return total;
	}

	Move Search::Go(int depth, const std::function<void(SearchResult)>& callback)
	{
		ClearCounterMoves();
		ClearTable(m_HistoryTable);
		ClearTable(m_ButterflyTable);
		m_TranspositionTable.Clear();
		m_WasStopped = false;
		m_ShouldStop = false;
		m_StartTime = std::chrono::high_resolution_clock::now();

		SearchStats searchStats;

		SearchRoot(m_CurrentPosition, depth, searchStats, callback);
		if (m_RootMoves.size() > 0 && m_RootMoves[0].PV.size() > 0)
		{
			return m_RootMoves[0].PV[0];
		}
		return MOVE_NONE;
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

	size_t Search::Perft(Position& position, int depth)
	{
		if (depth <= 0)
			return 1;
		if (depth == 1)
		{
			MoveGenerator movegen(position);
			MoveList moves = m_MovePool.GetList();
			movegen.GetPseudoLegalMoves(moves);
			movegen.FilterLegalMoves(moves);
			return moves.MoveCount;
		}

#if BOX_UNDO_MOVES
		UndoInfo undo;
#endif
		MoveGenerator movegen(position);
		size_t nodes = 0;
		MoveList legalMoves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(legalMoves);
		movegen.FilterLegalMoves(legalMoves);
		for (int i = 0; i < legalMoves.MoveCount; i++)
		{
#if BOX_UNDO_MOVES
			ApplyMove(position, legalMoves.Moves[i], &undo);
			nodes += Perft(position, depth - 1);
			UndoMove(position, legalMoves.Moves[i], undo);
#else
			Position movedPosition = position;
			ApplyMove(movedPosition, legalMoves.Moves[i]);
			nodes += Perft(movedPosition, depth - 1);
#endif
		}
		return nodes;
	}

	void Search::SearchRoot(Position& position, int depth, SearchStats& stats, const std::function<void(SearchResult)>& callback)
	{
		SearchStack stack[MAX_PLY + 1];
		Move pv[MAX_PLY + 1];
		// 50 move rule => 100 plies + MAX_PLY potential search
		ZobristHash positionHistory[2 * MAX_PLY + 1];
		m_Nodes = 0;
		Centipawns alpha = -SCORE_MATE;
		Centipawns beta = SCORE_MATE;
		Centipawns delta = -SCORE_MATE;
		Centipawns bestScore;

		ZobristHash* historyPtr = positionHistory;
		const std::vector<ZobristHash>& history = GetHistory().GetPositions();
		for (size_t i = 0; i < history.size(); i++)
		{
			*historyPtr++ = history[i];
		}

		SearchStack* stackPtr = stack;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr;
		stackPtr->PlySinceCaptureOrPawnPush = history.size() > 0 ? history.size() - 1 : 0;
		stackPtr->Ply = -1;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->CanNull = true;

		stackPtr++;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr + 1;
		stackPtr->PlySinceCaptureOrPawnPush = history.size();
		stackPtr->Ply = 0;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->CanNull = true;

		int rootDepth = 0;

		RootMove previousBestMove;

		while ((++rootDepth) < MAX_PLY)
		{
			m_RootMoves.clear();
			m_RootMoves.reserve(MAX_MOVES);
			m_Nodes = 0;
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();

			// Aspiration windows
			if (rootDepth >= 4)
			{
				Centipawns prevMaxScore = bestScore;
				delta = 19;
				alpha = std::max(prevMaxScore - delta, -SCORE_MATE);
				beta = std::min(prevMaxScore + delta, SCORE_MATE);
			}

			while (true)
			{
				bestScore = SearchPosition<NodeType::PV>(position, stackPtr, rootDepth, alpha, beta, stats);
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
				if (CheckLimits())
				{
					m_WasStopped = true;
					break;
				}
				delta += delta / 4 + 5;
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				m_RootMoves.clear();
				m_RootMoves.push_back(previousBestMove);
				break;
			}

			std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());

			if (m_RootMoves.empty())
			{
				RootMove rm;
				rm.Score = StaticEvalPosition(position, alpha, beta, stack->Ply);
				rm.PV = { MOVE_NONE };
				m_RootMoves.push_back(rm);
			}

			RootMove& rootMove = m_RootMoves[0];
			std::vector<Move>& rootPV = rootMove.PV;
			previousBestMove = rootMove;

			if (m_Log)
			{
				auto elapsed = std::chrono::high_resolution_clock::now() - m_SearchRootStartTime;

				std::cout << "info depth " << rootDepth << " score ";
				if (!IsMateScore(rootMove.Score))
				{
					std::cout << "cp " << rootMove.Score;
				}
				else
				{
					if (rootMove.Score > 0)
						std::cout << "mate " << ((int)rootPV.size()) / 2 + 1;
					else
						std::cout << "mate " << -((int)rootPV.size()) / 2 - 1;
				}
				std::cout << " nodes " << m_Nodes;
				std::cout << " nps " << (size_t)(m_Nodes / (elapsed.count() / 1e9f));
				std::cout << " time " << (size_t)(elapsed.count() / 1e6f);
				std::cout << " pv";
				for (const Move& move : rootPV)
				{
					std::cout << " " << UCI::FormatMove(move);
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
	Centipawns Search::SearchPosition(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, SearchStats& stats)
	{
		BOX_ASSERT(alpha < beta && beta >= -SCORE_MATE && beta <= SCORE_MATE && alpha >= -SCORE_MATE && alpha <= SCORE_MATE, "Invalid bounds");
		constexpr bool IsPvNode = NT == NodeType::PV;
		const bool isRoot = IsPvNode && stack->Ply == 0;
		const bool inCheck = IsInCheck(position, position.TeamToPlay);

		Move pv[MAX_PLY + 1];

		stack->MoveCount = 0;
		stack->StaticEvaluation = SCORE_NONE;
		stack->PositionHistory[0] = position.Hash;
		(stack + 1)->PositionHistory = stack->PositionHistory + 1;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 2)->KillerMoves[0] = (stack + 2)->KillerMoves[1] = MOVE_NONE;

		m_Nodes++;

		// Check for draw
		if (!isRoot && IsDraw(position, stack))
		{
			Centipawns eval = EvaluateDraw(position);
			stack->StaticEvaluation = eval;
			return eval;
		}

		if (depth <= 0)
		{
			return QuiescenceSearch<NT>(position, stack, alpha, beta);
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

		Move bestMove = MOVE_NONE;
		MoveGenerator movegen(position);
		MoveList legalMoves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(legalMoves);
		movegen.FilterLegalMoves(legalMoves);
		if (legalMoves.Empty())
		{
			Centipawns eval = StaticEvalPosition(position, alpha, beta, stack->Ply);
			stack->StaticEvaluation = eval;
			return eval;
		}
		Move defaultMove = legalMoves.Moves[0];
		Move previousMove = (stack - 1)->CurrentMove;

		// Eval pruning
		if (depth < 3 && !IsPvNode && !inCheck && !IsEndgame(position))
		{
			Centipawns eval = StaticEvalPosition(position, alpha, beta, stack->Ply);
			stack->StaticEvaluation = eval;
			Centipawns margin = depth * 120;
			if (eval - margin >= beta)
			{
				return eval - margin;
			}
		}

		// Null move pruning
		if (!IsPvNode && !inCheck && depth > 1 && stack->CanNull && !IsEndgame(position))
		{
			stack->CurrentMove = MOVE_NONE;
			Position movedPosition = position;
			ApplyMove(movedPosition, MOVE_NONE);
			(stack + 1)->CanNull = false;

			int r = 2;
			if (depth > 6)
				r = 3;

			Centipawns score = -SearchPosition<NodeType::NonPV>(movedPosition, stack + 1, std::max(0, depth - r), -beta, -beta + 1, stats);
			stack->MoveCount++;
			if (score >= beta)
			{
				return beta;
			}
		}

		// Futility pruning
		bool futilityPrune = false;
		constexpr int futilityMargins[4] = { 0, 200, 300, 500 };
		if (depth <= 3 && !IsPvNode && !inCheck && !IsMateScore(alpha))
		{
			if (stack->StaticEvaluation == SCORE_NONE)
				stack->StaticEvaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);
			if (stack->StaticEvaluation + futilityMargins[depth] <= alpha)
				futilityPrune = true;
		}

		(stack + 1)->CanNull = true;

		MoveOrderingInfo ordering;
		ordering.CurrentPosition = &position;
		ordering.KillerMoves = stack->KillerMoves;
		ordering.CounterMove = m_CounterMoves[previousMove.GetFromSquareIndex()][previousMove.GetToSquareIndex()];
		ordering.HistoryTable = &m_HistoryTable;
		ordering.ButterflyTable = &m_ButterflyTable;
		MoveSelector selector(ordering, &legalMoves);

		int moveIndex = 0;
		Centipawns bestValue = SCORE_NONE;

 		while (!selector.Empty())
		{
			moveIndex++;
			Move move;
			if (moveIndex == 1 && IsPvNode && stack->PV[0] != MOVE_NONE && movegen.IsLegal(stack->PV[0]) && SanityCheckMove(position, stack->PV[0]))
				move = stack->PV[0];
			else
			{
				move = selector.GetNextMove();
				if (IsPvNode && move == stack->PV[0])
					continue;
			}

			stack->CurrentMove = move;

			Position movedPosition = position;
			ApplyMove(movedPosition, move);

			// Irreversible move was played
			if (movedPosition.HalfTurnsSinceCaptureOrPush == 0)
				(stack + 1)->PlySinceCaptureOrPawnPush = 0;
			else
				(stack + 1)->PlySinceCaptureOrPawnPush = stack->PlySinceCaptureOrPawnPush + 1;


			if (futilityPrune && moveIndex > 1 && !move.IsCaptureOrPromotion() && !IsInCheck(movedPosition, movedPosition.TeamToPlay))
				continue;

			int depthReduction = 0;
			int childDepth = depth - 1;
			if (inCheck)
				childDepth++;

			if (!IsPvNode && childDepth > 3 && moveIndex > 3 && !inCheck && !move.IsCaptureOrPromotion() && !IsInCheck(movedPosition, movedPosition.TeamToPlay))
			{
				depthReduction = 1;
				if (moveIndex > 6)
					depthReduction++;
				childDepth -= depthReduction;
			}

			stack->MoveCount++;

			Centipawns value = SCORE_MATE;
			if (!IsPvNode || moveIndex != 1)
			{
				value = -SearchPosition<NodeType::NonPV>(movedPosition, stack + 1, childDepth, -alpha - 1, -alpha, stats);
			}
			if (IsPvNode && (moveIndex == 1 || (value > alpha && (isRoot || value < beta))))
			{
				pv[0] = MOVE_NONE;
				(stack + 1)->PV = pv;
				value = -SearchPosition<NodeType::PV>(movedPosition, stack + 1, childDepth, -beta, -alpha, stats);
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				return SCORE_NONE;
			}

			if (isRoot)
			{
				RootMove rm;
				if (moveIndex == 1 || value > alpha)
				{
					UpdatePV(stack->PV, move, (stack + 1)->PV);
					rm.Score = value;
					for (Move* m = stack->PV; *m != MOVE_NONE; m++)
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
					if (IsPvNode)
					{
						UpdatePV(stack->PV, move, (stack + 1)->PV);
					}
				}
			}

			// Beta cuttoff - Fail high
			if (value >= beta)
			{
				if (!move.IsCaptureOrPromotion())
				{
					if (move != stack->KillerMoves[1] && move != stack->KillerMoves[0])
					{
						// Store killer move
						stack->KillerMoves[1] = stack->KillerMoves[0];
						stack->KillerMoves[0] = move;
					}
					m_HistoryTable[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
				}
				if (previousMove != MOVE_NONE && !move.IsCaptureOrPromotion())
				{
					m_CounterMoves[previousMove.GetFromSquareIndex()][previousMove.GetToSquareIndex()] = move;
				}
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
			else if (!move.IsCaptureOrPromotion())
			{
				m_ButterflyTable[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
			}
		}
		if (bestMove.GetFlags() & MOVE_NULL)
		{
			return bestValue;
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
	Centipawns Search::QuiescenceSearch(Position& position, SearchStack* stack, Centipawns alpha, Centipawns beta)
	{
		constexpr bool IsPvNode = NT == NodeType::PV;
		const bool inCheck = IsInCheck(position, position.TeamToPlay);

		stack->PositionHistory[0] = position.Hash;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 1)->PositionHistory = stack->PositionHistory + 1;

		m_Nodes++;

		if (IsDraw(position, stack))
			return EvaluateDraw(position);

		Centipawns evaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);
		if (evaluation >= beta && !inCheck)
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

			constexpr Centipawns delta = 200;
			if (!inCheck && ((move.GetFlags() & MOVE_CAPTURE) && (evaluation + GetPieceValue(move.GetCapturedPiece()) + delta < alpha) && !(move.GetFlags() & MOVE_PROMOTION)))
				continue;

			if (generator.IsLegal(move))
			{
				Position movedPosition = position;
				ApplyMove(movedPosition, move);

				// Irreversible move was played
				if (movedPosition.HalfTurnsSinceCaptureOrPush == 0)
					(stack + 1)->PlySinceCaptureOrPawnPush = 0;
				else
					(stack + 1)->PlySinceCaptureOrPawnPush = stack->PlySinceCaptureOrPawnPush + 1;

				Centipawns score = -QuiescenceSearch<NT>(movedPosition, stack + 1, -beta, -alpha);

				if (CheckLimits())
				{
					m_WasStopped = true;
					return SCORE_NONE;
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

	bool Search::IsDraw(const Position& position, SearchStack* stack) const
	{
		// 50 move rule => 100 half moves
		if (position.HalfTurnsSinceCaptureOrPush >= 100)
			return true;
		int ply = 2;
		ZobristHash hash = position.Hash;
		int count = 0;
		while (ply < stack->PlySinceCaptureOrPawnPush)
		{
			if (*(stack->PositionHistory - ply) == hash)
			{
				count++;
				if (count >= 2)
					return true;
			}
			ply += 2;
		}
		return false;
	}

	Centipawns Search::EvaluateDraw(const Position& position) const
	{
		return SCORE_DRAW + 2 * ((int)(position.Hash.Hash + (size_t)position.GetTotalHalfMoves() * 1248125) & 1) - 1;
	}

	Centipawns Search::MateIn(int ply) const
	{
		return SCORE_MATE - ply;
	}

	Centipawns Search::MatedIn(int ply) const
	{
		return -SCORE_MATE + ply;
	}

	bool Search::IsMateScore(Centipawns score) const
	{
		return score >= MateIn(MAX_PLY) || score <= MatedIn(MAX_PLY);
	}

	Centipawns Search::StaticEvalPosition(const Position& position, Centipawns alpha, Centipawns beta, int ply) const
	{
		Centipawns eval = Evaluate(position, position.TeamToPlay, alpha, beta);
		if (eval == SCORE_MATE)
			eval = MateIn(ply);
		else if (eval == -SCORE_MATE)
			eval = MatedIn(ply);
		return eval;
	}

	void Search::ClearCounterMoves()
	{
		for (SquareIndex from = a1; from < FILE_MAX * RANK_MAX; from++)
		{
			for (SquareIndex to = a1; to < FILE_MAX * RANK_MAX; to++)
			{
				m_CounterMoves[from][to] = MOVE_NONE;
			}
		}
	}

	void Search::ClearTable(Centipawns (&table)[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX])
	{
		for (SquareIndex from = a1; from < FILE_MAX * RANK_MAX; from++)
		{
			for (SquareIndex to = a1; to < FILE_MAX * RANK_MAX; to++)
			{
				table[TEAM_WHITE][from][to] = 0;
				table[TEAM_BLACK][from][to] = 0;
			}
		}
	}

}
