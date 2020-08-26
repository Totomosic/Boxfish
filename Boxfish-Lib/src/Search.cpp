#include "Search.h"
#include "Random.h"
#include "Uci.h"

namespace Boxfish
{

	void UpdatePV(Move* pv, const Move& move, Move* childPV)
	{
		for (*pv++ = move; childPV && *childPV != MOVE_NONE; )
			*pv++ = *childPV++;
		*pv = MOVE_NONE;
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

	Search::Search(size_t transpositionTableSize, bool log)
		: m_TranspositionTable(transpositionTableSize), m_Settings(), m_Limits(), m_PositionHistory(), m_MovePool(MOVE_POOL_SIZE), m_Nodes(0), m_SearchRootStartTime(), m_StartTime(),
		m_WasStopped(false), m_ShouldStop(false), m_Log(log), m_OrderingTables()
	{
		m_OrderingTables.Clear();
	}

	void Search::SetSettings(const BoxfishSettings& settings)
	{
		m_Settings = settings;
	}

	void Search::SetLimits(const SearchLimits& limits)
	{
		m_Limits = limits;
	}

	void Search::PushPosition(const Position& position)
	{
		m_PositionHistory.push_back(position.Hash);
	}

	void Search::Reset()
	{
		m_PositionHistory.clear();
	}

#define BOX_UNDO_MOVES 0

	size_t Search::Perft(const Position& position, int depth)
	{
		size_t total = 0;
		MoveGenerator movegen(position);
		MoveList moves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(moves);
		movegen.FilterLegalMoves(moves);

		auto startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < moves.MoveCount; i++)
		{
#if BOX_UNDO_MOVES
			UndoInfo undo;
			ApplyMove(position, moves.Moves[i], &undo);
			uint64_t perft = PerftPosition(position, depth - 1);
			UndoMove(position, moves.Moves[i], undo);
#else
			Position movedPosition = position;
			ApplyMove(movedPosition, moves.Moves[i]);
			size_t perft = PerftPosition(movedPosition, depth - 1);
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

	Move Search::SearchBestMove(const Position& position, const SearchLimits& limits, const std::function<void(SearchResult)>& callback)
	{
		SetLimits(limits);
		m_TranspositionTable.Clear();
		m_OrderingTables.Clear();
		m_WasStopped = false;
		m_ShouldStop = false;
		m_StartTime = std::chrono::high_resolution_clock::now();

		Position searchPosition = position;

		RootMove rootMove = SearchRoot(searchPosition, (limits.Depth <= 0) ? MAX_PLY : limits.Depth, callback);
		if (rootMove.PV.empty())
			return MOVE_NONE;
		return rootMove.PV[0];
	}

	void Search::Ponder(const Position& position, const std::function<void(SearchResult)>& callback)
	{
		SearchLimits limits;
		limits.Infinite = true;
		SearchBestMove(position, limits, callback);
	}

	void Search::Stop()
	{
		m_ShouldStop = true;
	}

	size_t Search::PerftPosition(Position& position, int depth)
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
			nodes += PerftPosition(position, depth - 1);
			UndoMove(position, legalMoves.Moves[i], undo);
#else
			Position movedPosition = position;
			ApplyMove(movedPosition, legalMoves.Moves[i]);
			nodes += PerftPosition(movedPosition, depth - 1);
#endif
		}
		return nodes;
	}

	Search::RootMove Search::SearchRoot(Position& position, int depth, const std::function<void(SearchResult)>& callback)
	{
		SearchStack stack[MAX_PLY + 1];
		Move pv[MAX_PLY + 1];
		// 50 move rule => 100 plies + MAX_PLY potential search
		ZobristHash positionHistory[2 * MAX_PLY + 1];
		m_Nodes = 0;
		Centipawns alpha = -SCORE_MATE;
		Centipawns beta = SCORE_MATE;
		Centipawns delta = -SCORE_MATE;
		Centipawns bestScore = SCORE_NONE;

		ZobristHash* historyPtr = positionHistory;
		const std::vector<ZobristHash>& history = m_PositionHistory;
		for (size_t i = 0; i < history.size(); i++)
		{
			*historyPtr++ = history[i];
		}

		SearchStack* stackPtr = stack;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr;
		stackPtr->PlySinceCaptureOrPawnPush = history.size() > 1 ? history.size() - 2 : 0;
		stackPtr->Ply = -2;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->CanNull = true;
		stackPtr->Contempt = m_Settings.Contempt;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		stackPtr++;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr;
		stackPtr->PlySinceCaptureOrPawnPush = history.size() > 0 ? history.size() - 1 : 0;
		stackPtr->Ply = -1;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->CanNull = true;
		stackPtr->Contempt = -m_Settings.Contempt;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		stackPtr++;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr + 1;
		stackPtr->PlySinceCaptureOrPawnPush = history.size();
		stackPtr->Ply = 0;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->CanNull = true;
		stackPtr->Contempt = m_Settings.Contempt;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		std::vector<RootMove> rootMoveCache = GenerateRootMoves(position, stackPtr);
		std::vector<RootMove> result;

		int rootDepth = 0;
		int multiPV = m_Settings.MultiPV;

		if (m_Settings.SkillLevel < 20)
			multiPV = std::max(multiPV, 4);
		multiPV = std::min(multiPV, (int)rootMoveCache.size());

		while ((++rootDepth) < MAX_PLY)
		{
			std::vector<RootMove> rootMoves = rootMoveCache;
			m_Nodes = 0;
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();

			RootMove selectedMove;

			for (int pvIndex = 0; pvIndex < multiPV; pvIndex++)
			{
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
					int newBestScore = SearchPosition<PV>(position, stackPtr, rootDepth, alpha, beta, RootInfo{ rootMoves, pvIndex, (int)rootMoves.size() });
					if (pvIndex == 0)
						bestScore = newBestScore;
					if (newBestScore <= alpha)
					{
						beta = (alpha + beta) / 2;
						alpha = std::max(newBestScore - delta, -SCORE_MATE);
					}
					else if (newBestScore >= beta)
					{
						beta = std::min(newBestScore + delta, SCORE_MATE);
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
					std::stable_sort(rootMoves.begin() + pvIndex, rootMoves.end());
					/*for (int i = pvIndex; i < rootMoves.size(); i++)
						rootMoves[i].Score = SCORE_NONE;*/
				}

				if (CheckLimits())
				{
					m_WasStopped = true;
					break;
				}

				std::stable_sort(rootMoves.begin() + pvIndex, rootMoves.end());

				RootMove& rootMove = rootMoves[pvIndex];
				std::vector<Move>& rootPV = rootMove.PV;

				// MultiPV may be set by skill level -> don't log other PVs in that case
				if (m_Log && pvIndex < m_Settings.MultiPV)
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
					std::cout << " multipv " << (pvIndex + 1);
					std::cout << " pv";
					for (const Move& move : rootPV)
					{
						std::cout << " " << UCI::FormatMove(move);
					}
					std::cout << std::endl;
				}
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				if (result.empty())
				{
					RootMove mv;
					mv.Score = -SCORE_MATE;
					mv.PV = { MOVE_NONE };
					result.push_back(mv);
				}
				break;
			}

			result = rootMoves;
			if (result.empty())
			{
				RootMove mv;
				mv.Score = -SCORE_MATE;
				mv.PV = { MOVE_NONE };
				result.push_back(mv);
			}

			RootMove& rootMove = result[0];
			std::vector<Move>& rootPV = rootMove.PV;

			if (callback)
			{
				SearchResult result;
				result.BestMove = pv[0];
				result.PV = rootPV;
				result.Score = rootMove.Score;
				callback(result);
			}

			if (IsMateScore(rootMove.Score) && rootMove.Score > 0)
				break;

			if (rootDepth >= depth)
			{
				break;
			}
		}
		if (result.size() > 0)
		{
			int chosenIndex = ChooseBestMove(result, m_Settings.SkillLevel, 4);
			return result[chosenIndex];
		}
		return rootMoveCache[0];
	}

	inline void Prefetch(void* address)
	{
		_mm_prefetch((char*)address, _MM_HINT_T0);
	}

	template<Search::NodeType NT>
	Centipawns Search::SearchPosition(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, const Search::RootInfo& rootInfo)
	{
		BOX_ASSERT(alpha < beta && beta >= -SCORE_MATE && beta <= SCORE_MATE && alpha >= -SCORE_MATE && alpha <= SCORE_MATE, "Invalid bounds");
		constexpr bool IsPvNode = NT == PV;
		const bool isRoot = IsPvNode && stack->Ply == 0;
		const bool inCheck = IsInCheck(position);

		MoveList pvMoveList = m_MovePool.GetList();
		Move* pv = pvMoveList.Moves;

		stack->StaticEvaluation = SCORE_NONE;
		(stack + 1)->PositionHistory = stack->PositionHistory + 1;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 2)->KillerMoves[0] = (stack + 2)->KillerMoves[1] = MOVE_NONE;
		(stack + 1)->Contempt = -stack->Contempt;

		int originalAlpha = alpha;

		if (depth <= 0)
		{
			return QuiescenceSearch<NT>(position, stack, depth, alpha, beta);
		}

		// Check for draw
		if (!isRoot && IsDraw(position, stack))
		{
			Centipawns eval = EvaluateDraw(position, stack->Contempt);
			stack->StaticEvaluation = eval;
			return eval;
		}

		stack->PositionHistory[0] = position.Hash;

		bool ttFound;
		TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(position.Hash, &ttFound);

		if (ttFound && ttEntry->Depth >= depth)
		{
			switch (ttEntry->Flag)
			{
			case EXACT:
				if (!IsPvNode)
					return ttEntry->Score;
				break;
			case UPPER_BOUND:
				beta = std::min(beta, ttEntry->Score);
				break;
			case LOWER_BOUND:
				alpha = std::max(alpha, ttEntry->Score);
				break;
			}
			if (alpha >= beta)
			{
				return ttEntry->Score;
			}
		}

		Move ttMove =
			(isRoot) ? rootInfo.Moves[rootInfo.PVIndex].PV[0] :
			(IsPvNode && stack->PV[0] != MOVE_NONE) ? stack->PV[0] :
			(ttFound && SanityCheckMove(position, ttEntry->BestMove)) ? ttEntry->BestMove : MOVE_NONE;

		// Null move pruning
		if (!IsPvNode && !inCheck && depth >= 3 && stack->CanNull && !IsEndgame(position))
		{
			stack->StaticEvaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);
			if (stack->StaticEvaluation >= beta)
			{
				(stack + 1)->CanNull = false;
				stack->CurrentMove = MOVE_NONE;
				Position movedPosition = position;
				ApplyMove(movedPosition, MOVE_NONE);

				int r = 2;
				if (depth > 6)
					r = 3;

				Centipawns value = -SearchPosition<NonPV>(movedPosition, stack + 1, std::max(1, depth - r - 1), -beta, -beta + 1, rootInfo);
				if (value >= beta)
					return beta;
			}
		}

		Move bestMove = MOVE_NONE;
		MoveGenerator movegen(position);
		MoveList legalMoves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(legalMoves);
		movegen.FilterLegalMoves(legalMoves);
		if (legalMoves.Empty())
		{
			if (inCheck)
				return MatedIn(stack->Ply);
			return EvaluateDraw(position, stack->Contempt);
		}
		Move previousMove = (stack - 1)->CurrentMove;

		(stack + 1)->CanNull = true;

		MoveOrderingInfo ordering;
		ordering.CurrentPosition = &position;
		ordering.KillerMoves = stack->KillerMoves;
		ordering.PreviousMove = previousMove;
		ordering.Tables = &m_OrderingTables;
		MoveSelector selector(ordering, &legalMoves);

		constexpr int FIRST_MOVE_INDEX = 1;

		int moveIndex = 0;
		Centipawns bestValue = -SCORE_MATE;

 		while (!selector.Empty())
		{
			moveIndex++;
			Move move;
			if (moveIndex == FIRST_MOVE_INDEX && ttMove != MOVE_NONE)
				move = ttMove;
			else
			{
				move = selector.GetNextMove();
				if (move == ttMove)
					continue;
			}

			if (isRoot && std::count(rootInfo.Moves.begin() + rootInfo.PVIndex, rootInfo.Moves.begin() + rootInfo.PVLast, move) == 0)
				continue;

			const bool isCaptureOrPromotion = move.IsCaptureOrPromotion();
			stack->CurrentMove = move;
			stack->MoveCount++;

			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			m_Nodes++;

			const bool givesCheck = IsInCheck(movedPosition);

			// Irreversible move was played
			if (movedPosition.HalfTurnsSinceCaptureOrPush == 0)
				(stack + 1)->PlySinceCaptureOrPawnPush = 0;
			else
				(stack + 1)->PlySinceCaptureOrPawnPush = stack->PlySinceCaptureOrPawnPush + 1;

			bool fullDepthSearch = false;
			int depthReduction = 0;
			int depthExtension = 0;
			Centipawns value = -SCORE_MATE;

			if (move.IsCastle() || ((inCheck || givesCheck) && SeeGE(position, move)))
				depthExtension++;

			int extendedDepth = depth - 1 + depthExtension;

			// Late move reduction
			if (!IsPvNode && depth >= 4 && moveIndex > 5 + FIRST_MOVE_INDEX && !inCheck && !isCaptureOrPromotion && !givesCheck)
			{
				depthReduction = 1;
				if (moveIndex > 8 + FIRST_MOVE_INDEX)
					depthReduction++;
				int d = extendedDepth - std::max(depthReduction, 0);
				value = -SearchPosition<NodeType::NonPV>(movedPosition, stack + 1, d, -(alpha + 1), -alpha, rootInfo);
				fullDepthSearch = value > alpha && d != extendedDepth;
			}
			else
			{
				fullDepthSearch = !IsPvNode || moveIndex > FIRST_MOVE_INDEX;
			}

			int childDepth = extendedDepth - depthReduction;
			if (fullDepthSearch)
			{
				value = -SearchPosition<NonPV>(movedPosition, stack + 1, childDepth, -(alpha + 1), -alpha, rootInfo);
			}
			if (IsPvNode && (moveIndex == FIRST_MOVE_INDEX || (value > alpha && (isRoot || value < beta))))
			{
				pv[0] = MOVE_NONE;
				(stack + 1)->PV = pv;
				value = -SearchPosition<PV>(movedPosition, stack + 1, childDepth, -beta, -alpha, rootInfo);
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				return SCORE_NONE;
			}

			if (isRoot)
			{
				RootMove& rm = *std::find(rootInfo.Moves.begin(), rootInfo.Moves.end(), move);
				
				if (moveIndex == FIRST_MOVE_INDEX || value > alpha)
				{
					rm.Score = value;
					rm.PV.resize(1);
					for (Move* m = (stack + 1)->PV; *m != MOVE_NONE; m++)
					{
						rm.PV.push_back(*m);
					}
				}
				else
				{
					rm.Score = -SCORE_MATE;
				}
			}

			if (value > bestValue)
			{
				bestMove = move;
				bestValue = value;
				if (IsPvNode && !isRoot)
					UpdatePV(stack->PV, move, (stack + 1)->PV);
				if (value > alpha && IsPvNode)
				{
					alpha = value;
				}
				if (IsMateScore(value) && value > 0)
					break;
			}

			// Alpha cutoff - Fail low
			if (value <= alpha)
			{
				if (rootInfo.PVIndex == 0 && (!ttFound || depth >= ttEntry->Depth))
				{
					ttEntry->Age = position.GetTotalHalfMoves();
					ttEntry->BestMove = move;
					ttEntry->Depth = depth;
					ttEntry->Flag = UPPER_BOUND;
					ttEntry->Hash = position.Hash;
					ttEntry->Score = value;
				}
			}

			// Beta cutoff - Fail high
			if (value >= beta)
			{
				if (!isCaptureOrPromotion && rootInfo.PVIndex == 0)
				{
					m_OrderingTables.CounterMoves[previousMove.GetFromSquareIndex()][previousMove.GetToSquareIndex()] = move;
 					if (move != stack->KillerMoves[0] && move != stack->KillerMoves[1])
					{
						stack->KillerMoves[1] = stack->KillerMoves[0];
						stack->KillerMoves[0] = move;
					}
					m_OrderingTables.History[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
				}
				if (rootInfo.PVIndex == 0 && (!ttFound || depth >= ttEntry->Depth))
				{
					ttEntry->Age = position.GetTotalHalfMoves();
					ttEntry->BestMove = move;
					ttEntry->Depth = depth;
					ttEntry->Flag = LOWER_BOUND;
					ttEntry->Hash = position.Hash;
					ttEntry->Score = value;
				}
				return beta;
			}
			else if (!isCaptureOrPromotion && rootInfo.PVIndex == 0)
			{
				m_OrderingTables.Butterfly[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
			}
		}

		if (bestMove == MOVE_NONE)
		{
			return alpha;
		}

		if (rootInfo.PVIndex == 0 && (!ttFound || depth >= ttEntry->Depth))
		{
			ttEntry->Age = position.GetTotalHalfMoves();
			ttEntry->BestMove = bestMove;
			ttEntry->Depth = depth;
			ttEntry->Flag = (bestValue > originalAlpha) ? EXACT : UPPER_BOUND;
			ttEntry->Hash = position.Hash;
			ttEntry->Score = bestValue;
		}
		return bestValue;
	}

	bool IsBadCapture(const Position& position, const Move& move)
	{
		if (move.GetMovingPiece() == PIECE_PAWN)
			return false;
		if (GetPieceValue(move.GetCapturedPiece()) >= GetPieceValue(move.GetMovingPiece()) - 50)
			return false;
		return !SeeGE(position, move);
	}

	template<Search::NodeType NT>
	Centipawns Search::QuiescenceSearch(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta)
	{
		constexpr bool IsPvNode = NT == NodeType::PV;
		const bool inCheck = IsInCheck(position);

		Centipawns originalAlpha = alpha;

		bool ttFound;
		TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(position.Hash, &ttFound);
		if (ttFound)
		{
			if (ttEntry->Flag == EXACT)
				return ttEntry->Score;
			if (ttEntry->Flag == UPPER_BOUND && ttEntry->Score <= alpha)
				return ttEntry->Score;
			if (ttEntry->Flag == LOWER_BOUND && ttEntry->Score >= beta)
				return ttEntry->Score;
		}

		stack->PositionHistory[0] = position.Hash;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 1)->PositionHistory = stack->PositionHistory + 1;
		(stack + 1)->Contempt = -stack->Contempt;

		if (IsDraw(position, stack))
			return EvaluateDraw(position, stack->Contempt);

		Centipawns evaluation = 0;
		if (!inCheck)
		{
			evaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);

			if (evaluation >= beta && !inCheck)
			{
				if (!ttFound && depth >= ttEntry->Depth)
				{
					ttEntry->Age = position.GetTotalHalfMoves();
					ttEntry->Depth = depth;
					ttEntry->BestMove = MOVE_NONE;
					ttEntry->Flag = LOWER_BOUND;
					ttEntry->Hash = position.Hash;
					ttEntry->Score = evaluation;
				}
				return beta;
			}
			if (alpha < evaluation)
				alpha = evaluation;
		}

		MoveGenerator generator(position);
		MoveList legalMoves = m_MovePool.GetList();
		generator.GetPseudoLegalMoves(legalMoves);

		if (legalMoves.MoveCount <= 0)
		{
			if (inCheck)
				return MatedIn(stack->Ply);
			return EvaluateDraw(position, stack->Contempt);
		}

		Centipawns bestValue = -SCORE_MATE;

		QuiescenceMoveSelector selector(position, legalMoves);
		while (!selector.Empty())
		{
			Move move = selector.GetNextMove();

			// TODO: Also find moves that give check

			constexpr Centipawns delta = 200;
			if (!inCheck && (move.IsCapture() && (evaluation + GetPieceValue(move.GetCapturedPiece()) + delta < alpha) && !move.IsPromotion()))
				continue;

			if (generator.IsLegal(move))
			{
				if (!inCheck && move.IsCapture() && IsBadCapture(position, move))
					continue;

				Position movedPosition = position;
				ApplyMove(movedPosition, move);
				m_Nodes++;

				// Irreversible move was played
				if (movedPosition.HalfTurnsSinceCaptureOrPush == 0)
					(stack + 1)->PlySinceCaptureOrPawnPush = 0;
				else
					(stack + 1)->PlySinceCaptureOrPawnPush = stack->PlySinceCaptureOrPawnPush + 1;

				Centipawns score = -QuiescenceSearch<NT>(movedPosition, stack + 1, depth -1, -beta, -alpha);
				if (score > bestValue)
					bestValue = score;

				if (CheckLimits())
				{
					m_WasStopped = true;
					return SCORE_NONE;
				}

				if (score >= beta)
				{
					if (!ttFound && depth >= ttEntry->Depth)
					{
						ttEntry->Age = position.GetTotalHalfMoves();
						ttEntry->Depth = depth;
						ttEntry->BestMove = MOVE_NONE;
						ttEntry->Flag = LOWER_BOUND;
						ttEntry->Hash = position.Hash;
						ttEntry->Score = beta;
					}
					return beta;
				}
				if (score > alpha)
					alpha = score;
			}
		}

		if (inCheck && bestValue == -SCORE_MATE)
			return MatedIn(stack->Ply);

		if (!ttFound && depth >= ttEntry->Depth)
		{
			ttEntry->Age = position.GetTotalHalfMoves();
			ttEntry->BestMove = MOVE_NONE;
			ttEntry->Depth = depth;
			ttEntry->Flag = (alpha > originalAlpha) ? EXACT : LOWER_BOUND;
			ttEntry->Hash = position.Hash;
			ttEntry->Score = alpha;
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

	Centipawns Search::EvaluateDraw(const Position& position, Centipawns contempt) const
	{
		return SCORE_DRAW - contempt;
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

	std::vector<Search::RootMove> Search::GenerateRootMoves(const Position& position, SearchStack* stack)
	{
		MoveList list = m_MovePool.GetList();
		MoveGenerator generator(position);
		generator.GetPseudoLegalMoves(list);
		generator.FilterLegalMoves(list);
		MoveOrderingInfo info;
		info.CurrentPosition = &position;
		info.KillerMoves = stack->KillerMoves;
		info.PreviousMove = MOVE_NONE;
		info.Tables = &m_OrderingTables;

		MoveSelector selector(info, &list);
		std::vector<RootMove> result;
		while (!selector.Empty())
		{
			RootMove mv;
			mv.Score = SCORE_NONE;
			mv.PV = { selector.GetNextMove() };
			result.push_back(mv);
		}
		return result;
	}

	int Search::ChooseBestMove(const std::vector<RootMove>& moves, int skillLevel, int maxPVs) const
	{
		if (skillLevel >= 20 || moves.empty())
			return 0;

		Random::Seed(time(nullptr));
		int pvs = std::min((int)moves.size(), maxPVs);

		Centipawns bestScore = moves[0].Score;
		Centipawns delta = std::min(bestScore - moves[pvs - 1].Score, GetPieceValue(PIECE_PAWN));
		Centipawns weakness = 120 - 2 * skillLevel;
		Centipawns maxScore = -SCORE_MATE;

		int bestIndex = 0;

		for (int i = 0; i < pvs; i++)
		{
			int push = (weakness * int(bestScore - moves[i].Score) + delta * (Random::GetNext(0, weakness))) / 120;
			if (moves[i].Score + push >= maxScore)
			{
				maxScore = moves[i].Score + push;
				bestIndex = i;
			}
		}
		return bestIndex;
	}

}
