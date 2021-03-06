#include "Search.h"
#include "Random.h"
#include "Format.h"
#include <thread>

namespace Boxfish
{

	// DepthReductions[NodeType][Improving][Depth][MoveIndex];
	static int DepthReductions[2][2][32][64];
	static int FutilityMoveCounts[2][16];

	void InitSearch()
	{
		for (int improving = 0; improving <= 1; improving++)
		{
			for (int depth = 1; depth < 32; depth++)
			{
				for (int moveIndex = 1; moveIndex < 64; moveIndex++)
				{
					double reduction = std::log(depth) * std::log(moveIndex) / 2.0;
					DepthReductions[Search::NodeType::NonPV][improving][depth][moveIndex] = (int)std::round(reduction);
					DepthReductions[Search::NodeType::PV][improving][depth][moveIndex] = std::max(DepthReductions[Search::NodeType::NonPV][improving][depth][moveIndex] - 1, 0);

					if (!improving && reduction > 1.0)
						DepthReductions[Search::NodeType::NonPV][improving][depth][moveIndex]++;
				}
			}
		}
		for (int d = 0; d < 16; d++)
		{
			FutilityMoveCounts[0][d] = int(2.4 + 0.74 * std::pow(d, 1.78)) + 10;
			FutilityMoveCounts[1][d] = int(5.0 + 1.00 * std::pow(d, 2.00)) + 10;
		}
	}

	template<Search::NodeType NT>
	int GetReduction(bool improving, int depth, int moveIndex)
	{
		BOX_ASSERT(depth > 0 && moveIndex > 0, "Invalid depth or MoveIndex");
		return DepthReductions[NT][improving][std::min(depth, 31)][std::min(moveIndex, 63)];
	}

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
		: m_TranspositionTable(transpositionTableSize), m_Settings(), m_Limits(), m_PositionHistory(), m_OpeningBook(nullptr), m_MovePool(MOVE_POOL_SIZE), m_Nodes(0), m_SearchRootStartTime(), m_StartTime(),
		m_WasStopped(false), m_ShouldStop(false), m_Log(log), m_OrderingTables()
	{
		m_OrderingTables.Clear();
	}

	const BoxfishSettings& Search::GetSettings() const
	{
		return m_Settings;
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

	void Search::SetOpeningBook(const OpeningBook* book)
	{
		m_OpeningBook = book;
	}

	const TranspositionTableEntry* Search::ProbeTranspostionTable(const Position& position) const
	{
		bool found;
		const TranspositionTableEntry* entry = m_TranspositionTable.GetEntry(position.Hash, found);
		if (found)
			return entry;
		return nullptr;
	}

#define BOX_UNDO_MOVES 0

	size_t Search::Perft(const Position& pos, int depth)
	{
		Position position = pos;
		size_t total = 0;
		MoveGenerator movegen(position);
		MoveList moves = m_MovePool.GetList();
		movegen.GetPseudoLegalMoves(moves);
		movegen.FilterLegalMoves(moves);

		auto startTime = std::chrono::high_resolution_clock::now();
#if BOX_UNDO_MOVES
		UndoInfo undo;
#endif
		for (int i = 0; i < moves.MoveCount; i++)
		{
#if BOX_UNDO_MOVES
			ApplyMove(position, moves.Moves[i], &undo);
			size_t perft = PerftPosition(position, depth - 1);
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

	Move Search::SearchBestMove(const Position& position, SearchLimits limits)
	{
		return SearchBestMove(position, limits, [](SearchResult) {});
	}

	Move Search::SearchBestMove(const Position& position, SearchLimits limits, const std::function<void(SearchResult)>& callback)
	{
		// Don't use book moves when pondering
		if (!limits.Infinite && m_OpeningBook != nullptr && position.GetTotalHalfMoves() <= m_OpeningBook->GetCardinality())
		{
			auto bookCollection = m_OpeningBook->Probe(position.Hash);
			if (bookCollection)
			{
				BookEntry entry = bookCollection->PickRandom();
				Move move = CreateMove(position, BitBoard::BitIndexToSquare(entry.From), BitBoard::BitIndexToSquare(entry.To));
				limits.Only.insert(move);
			}
		}

		SetLimits(limits);
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

	void Search::Ponder(const Position& position, SearchLimits limits, const std::function<void(SearchResult)>& callback)
	{
		limits.Infinite = true;
		SearchBestMove(position, limits, callback);
	}

	void Search::Ponder(const Position& position, const std::function<void(SearchResult)>& callback)
	{
		return Ponder(position, SearchLimits{}, callback);
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
		SearchStack stack[MAX_PLY + 5];
		Move pv[MAX_PLY + 1];
		// 50 move rule => 100 plies + MAX_PLY potential search
		std::unique_ptr<ZobristHash[]> positionHistory = std::make_unique<ZobristHash[]>(m_PositionHistory.size() + MAX_PLY + 1);
		m_Nodes = 0;
		ValueType alpha = -SCORE_MATE;
		ValueType beta = SCORE_MATE;
		ValueType delta = -SCORE_MATE;
		ValueType bestScore = SCORE_NONE;

		ZobristHash* historyPtr = positionHistory.get();
		const std::vector<ZobristHash>& history = m_PositionHistory;
		for (size_t i = 0; i < history.size(); i++)
		{
			*historyPtr++ = history[i];
		}

		SearchStack* stackPtr = stack;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr;
		stackPtr->Ply = -2;
		stackPtr->MoveCount = 0;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->Contempt = m_Settings.Contempt;
		stackPtr->TTIsPv = false;
		stackPtr->TTHit = false;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		stackPtr++;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr;
		stackPtr->Ply = -1;
		stackPtr->MoveCount = 0;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->Contempt = -m_Settings.Contempt;
		stackPtr->TTIsPv = false;
		stackPtr->TTHit = false;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		stackPtr++;

		stackPtr->PV = pv;
		stackPtr->PV[0] = MOVE_NONE;
		stackPtr->PositionHistory = historyPtr + 1;
		stackPtr->Ply = 0;
		stackPtr->MoveCount = 0;
		stackPtr->CurrentMove = MOVE_NONE;
		stackPtr->StaticEvaluation = SCORE_NONE;
		stackPtr->Contempt = m_Settings.Contempt;
		stackPtr->TTIsPv = false;
		stackPtr->TTHit = false;
		stackPtr->KillerMoves[1] = stackPtr->KillerMoves[0] = MOVE_NONE;

		std::vector<RootMove> rootMoveCache = GenerateRootMoves(position, stackPtr);
		std::vector<RootMove> result;

		int rootDepth = 0;
		int multiPV = m_Settings.MultiPV;

		if (m_Settings.SkillLevel < 20)
			multiPV = std::max(multiPV, 4);
		multiPV = std::min(multiPV, (int)rootMoveCache.size());

		std::vector<RootMove> rootMoves = rootMoveCache;

		while ((++rootDepth) < MAX_PLY)
		{
			m_Nodes = 0;
			m_SearchRootStartTime = std::chrono::high_resolution_clock::now();

			RootMove selectedMove;

			for (int pvIndex = 0; pvIndex < multiPV; pvIndex++)
			{
				int selDepth = 0;
				// Aspiration windows
				if (rootDepth >= 4)
				{
					ValueType prevMaxScore = bestScore;
					delta = 16;
					alpha = std::max(prevMaxScore - delta, -SCORE_MATE);
					beta = std::min(prevMaxScore + delta, SCORE_MATE);
				}

				int betaCutoffs = 0;
				while (true)
				{
					int adjustedDepth = std::max(1, rootDepth - betaCutoffs);
					ValueType newBestScore = SearchPosition<PV>(position, stackPtr, adjustedDepth, alpha, beta, selDepth, false, RootInfo{ rootMoves, pvIndex, (int)rootMoves.size() });

					std::stable_sort(rootMoves.begin() + pvIndex, rootMoves.end());

					if (pvIndex == 0)
						bestScore = newBestScore;
					if (newBestScore <= alpha && newBestScore != -SCORE_MATE)
					{
						beta = (alpha + beta) / 2;
						alpha = std::max(newBestScore - delta, -SCORE_MATE);
					}
					else if (newBestScore >= beta && newBestScore != SCORE_MATE)
					{
						beta = std::min(newBestScore + delta, SCORE_MATE);
						betaCutoffs++;
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
					break;
				}

				RootMove& rootMove = rootMoves[pvIndex];
				std::vector<Move>& rootPV = rootMove.PV;

				// MultiPV may be set by skill level -> don't log other PVs in that case
				if (m_Log && pvIndex < m_Settings.MultiPV)
				{
					auto elapsed = std::chrono::high_resolution_clock::now() - m_SearchRootStartTime;
					int hashFull = m_TranspositionTable.GetFullProportion();

					std::cout << "info depth " << rootDepth << " seldepth " << rootMove.SelDepth << " score ";
					if (!IsMateScore(rootMove.Score))
					{
						std::cout << "cp " << rootMove.Score;
					}
					else
					{
						if (rootMove.Score > 0)
							std::cout << "mate " << (GetPliesFromMateScore(rootMove.Score) / 2 + 1);
						else
							std::cout << "mate " << -(GetPliesFromMateScore(rootMove.Score) / 2 - 1);
					}
					std::cout << " nodes " << m_Nodes;
					std::cout << " nps " << (size_t)(m_Nodes / (elapsed.count() / 1e9f));
					std::cout << " time " << (size_t)(elapsed.count() / 1e6f);
					std::cout << " multipv " << (pvIndex + 1);
					if (hashFull >= 500)
						std::cout << " hashfull " << hashFull;
					if (m_Limits.Only.size() == 1)
						std::cout << " bookmove";
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
					result.BestMove = rootPV.size() > 0 ? rootPV[0] : MOVE_NONE;
					result.PV = rootPV;
					result.Score = rootMove.Score;
					result.PVIndex = pvIndex;
					result.Depth = rootDepth;
					result.SelDepth = selDepth;
					callback(result);
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

			if (IsMateScore(rootMove.Score) && !m_Limits.Infinite)
				break;

			if (rootDepth >= depth)
				break;
		}

		// Don't return if pondering (exceeded max ply)
		while (m_Limits.Infinite && !m_ShouldStop)
			std::this_thread::yield();

		if (result.size() > 0)
		{
			int chosenIndex = ChooseBestMove(result, m_Settings.SkillLevel, 4);
			return result[chosenIndex];
		}
		if (rootMoveCache.size() > 0)
			return rootMoveCache[0];
		RootMove nullMove;
		nullMove.PV = { MOVE_NONE };
		return nullMove;
	}

	template<Search::NodeType NT>
	ValueType Search::SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode, const Search::RootInfo& rootInfo)
	{
		BOX_ASSERT(alpha < beta && beta >= -SCORE_MATE && beta <= SCORE_MATE && alpha >= -SCORE_MATE && alpha <= SCORE_MATE, "Invalid bounds");
		constexpr int FirstMoveIndex = 1;
		constexpr bool IsPvNode = NT == PV;
		const bool IsRoot = IsPvNode && stack->Ply == 0;
		const bool inCheck = position.InCheck();

		BOX_ASSERT(inCheck == !!GetAttackers(position, OtherTeam(position.TeamToPlay), position.GetKingSquare(position.TeamToPlay), position.GetAllPieces()), "Mismatch");
		BOX_ASSERT(!(IsPvNode && cutNode), "Invalid");
		BOX_ASSERT(IsPvNode || (alpha == beta - 1), "Invalid alpha/beta");

		MoveList pvMoveList = m_MovePool.GetList();
		Move* pv = pvMoveList.Moves;

		stack->MoveCount = 0;
		stack->StaticEvaluation = SCORE_NONE;
		(stack + 1)->TTIsPv = false;
		(stack + 1)->PositionHistory = stack->PositionHistory + 1;
		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 2)->KillerMoves[0] = (stack + 2)->KillerMoves[1] = MOVE_NONE;
		(stack + 1)->Contempt = -stack->Contempt;
		(stack + 1)->PV = nullptr;

		ValueType originalAlpha = alpha;

		if (stack->Ply >= MAX_PLY)
			return EvaluateDraw(position, stack->Contempt);

		if (depth <= 0)
			return QuiescenceSearch<NT>(position, stack, 0, alpha, beta);

		stack->PositionHistory[0] = position.Hash;

		// Check for draw
		if (!IsRoot && IsDraw(position, stack))
		{
			ValueType eval = EvaluateDraw(position, stack->Contempt);
			stack->StaticEvaluation = eval;
			return eval;
		}

		// Mate distance pruning
		if (!IsRoot)
		{
			alpha = std::max(MatedIn(stack->Ply), alpha);
			beta = std::min(MateIn(stack->Ply), beta);
			if (alpha >= beta)
				return alpha;
		}

		if (IsPvNode && selDepth < stack->Ply + 1)
			selDepth = stack->Ply + 1;

		ZobristHash ttHash = position.Hash;
		if (stack->ExcludedMove != MOVE_NONE)
			ttHash ^= stack->ExcludedMove.GetKey();
		TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(ttHash, stack->TTHit);

		if (stack->ExcludedMove == MOVE_NONE)
			stack->TTIsPv = IsPvNode || (stack->TTHit && ttEntry->IsPv());
		bool formerPv = stack->TTIsPv && !IsPvNode;

		stack->TTHit = stack->TTHit && SanityCheckMove(position, ttEntry->GetMove());

		Move ttMove =
			IsRoot ? rootInfo.Moves[rootInfo.PVIndex].PV[0] :
			stack->TTHit ? ttEntry->GetMove() : MOVE_NONE;
		ValueType ttValue =
			stack->TTHit ? GetValueFromTT(ttEntry->GetScore(), stack->Ply) : SCORE_NONE;

		if (!IsPvNode && stack->TTHit && ttEntry->GetDepth() >= depth && (ttValue >= beta ? (ttEntry->GetFlag() & LOWER_BOUND) : (ttEntry->GetFlag() & UPPER_BOUND)))
		{
			if (ttMove != MOVE_NONE)
			{
				if (ttValue >= beta && !ttMove.IsCaptureOrPromotion())
					UpdateQuietStats(position, stack, depth, ttMove);
				else if (!ttMove.IsCaptureOrPromotion())
					m_OrderingTables.History[position.TeamToPlay][ttMove.GetFromSquareIndex()][ttMove.GetToSquareIndex()] += depth * depth;
			}
			return ttValue;
		}

		if (!inCheck)
		{
			if (stack->TTHit)
			{
				stack->StaticEvaluation = ttValue;
			}
			else
			{
				if ((stack - 1)->CurrentMove == MOVE_NONE && !IsRoot)
					stack->StaticEvaluation = -(stack - 1)->StaticEvaluation + 20;
				else
					stack->StaticEvaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);
			}
		}

		// Razoring
		if (!IsRoot && depth == 1 && !inCheck && stack->StaticEvaluation <= alpha - 250)
		{
			return QuiescenceSearch<NT>(position, stack, 0, alpha, beta);
		}

		const bool improving = !inCheck &&
			((stack - 2)->StaticEvaluation == SCORE_NONE ?
			 (stack->StaticEvaluation > (stack - 4)->StaticEvaluation || (stack - 4)->StaticEvaluation == SCORE_NONE) :
			 (stack->StaticEvaluation > (stack - 2)->StaticEvaluation));
			
		// Futility pruning
		if (!IsPvNode && !inCheck && depth < 8 && stack->StaticEvaluation - 110 * (depth - improving) >= beta && !IsMateScore(stack->StaticEvaluation))
			return stack->StaticEvaluation;

		// Null move pruning
		if (!IsPvNode && !inCheck && (stack - 1)->CurrentMove != MOVE_NONE && stack->ExcludedMove == MOVE_NONE && stack->StaticEvaluation >= (beta + 90 - 15 * depth - 14 * improving + 40 * stack->TTIsPv) && !IsEndgame(position))
		{
			int depthReduction = std::max((stack->StaticEvaluation - beta) * depth / 300, 3);

			UndoInfo undo;
			ApplyNullMove(position, &undo);

			stack->MoveCount = FirstMoveIndex;
			stack->CurrentMove = MOVE_NONE;

			ValueType value = -SearchPosition<NonPV>(position, stack + 1, depth - depthReduction, -beta, -beta + 1, selDepth, !cutNode, rootInfo);

			UndoNullMove(position, undo);

			if (value >= beta)
				return IsMateScore(value) ? beta : value;
		}

		if (IsPvNode && depth >= 6 && ttMove == MOVE_NONE && !inCheck)
		{
			depth -= 2;
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
		Move counterMove = m_OrderingTables.CounterMoves[previousMove.GetFromSquareIndex()][previousMove.GetToSquareIndex()];
		Move move = MOVE_NONE;

		MoveSelector selector(&legalMoves, &position, ttMove, counterMove, previousMove, &m_OrderingTables, stack->KillerMoves);

		const bool ttMoveIsCapture = ttMove != MOVE_NONE && ttMove.IsCapture();
		int moveIndex = 0;
		ValueType bestValue = -SCORE_MATE;

		bool moveCountPruning = false;
		bool singularExtension = false;

		while ((move = selector.GetNextMove(moveCountPruning)) != MOVE_NONE)
		{
			const bool isCaptureOrPromotion = move.IsCaptureOrPromotion();

			if (move == stack->ExcludedMove)
				continue;
			if (IsRoot && std::count(rootInfo.Moves.begin() + rootInfo.PVIndex, rootInfo.Moves.begin() + rootInfo.PVLast, move) == 0)
				continue;

			moveIndex++;
			BOX_ASSERT(moveIndex > FirstMoveIndex || move == ttMove || ttMove == MOVE_NONE || ttMove == stack->ExcludedMove, "Invalid move ordering");
			int depthExtension = 0;

			Position movedPosition = position;
			ApplyMove(movedPosition, move);
			m_Nodes++;

			stack->MoveCount = moveIndex;

			const bool givesCheck = movedPosition.InCheck();
			const bool givesGoodCheck = givesCheck && SeeGE(position, move);

			if (!IsRoot && position.GetNonPawnMaterial(position.TeamToPlay) > 0 && !IsMateScore(bestValue))
			{
				moveCountPruning = moveIndex >= (3 + depth * depth) / (2 - improving);

				if (!isCaptureOrPromotion && !givesCheck && (!move.IsAdvancedPawnPush(position.TeamToPlay) || position.GetNonPawnMaterial() > 3500))
				{
					int lmrDepth = std::max(depth - 1 - GetReduction<PV>(improving, depth, moveIndex), 0);

					if (!SeeGE(position, move, -(25 - std::min(lmrDepth, 18)) * lmrDepth * lmrDepth))
						continue;
				}
				else if (!SeeGE(position, move, -110 * depth))
					continue;
			}

			// Singular extension
			if (!IsRoot && depth >= 7 && move == ttMove && stack->ExcludedMove == MOVE_NONE && !IsMateScore(ttValue) && (ttEntry->GetFlag() & LOWER_BOUND) && ttEntry->GetDepth() >= depth - 3)
			{
				ValueType singularBeta = ttValue - ((formerPv + 4) * depth) / 2;
				int singularDepth = (depth - 1 + 3 * formerPv) / 2;

				stack->ExcludedMove = move;
				ValueType value = SearchPosition<NonPV>(position, stack, singularDepth, singularBeta - 1, singularBeta, selDepth, cutNode, rootInfo);
				stack->ExcludedMove = MOVE_NONE;

				if (value < singularBeta)
				{
					depthExtension = 1;
					singularExtension = !ttMoveIsCapture;
				}
				else if (singularBeta >= beta)
					return singularBeta;
				else if (ttValue >= beta)
				{
					stack->ExcludedMove = move;
					value = SearchPosition<NonPV>(position, stack, (depth + 3) / 2, beta - 1, beta, selDepth, cutNode, rootInfo);
					stack->ExcludedMove = MOVE_NONE;

					if (value >= beta)
						return value;
				}
			}
			else if (move.IsCastle() || givesGoodCheck)
				depthExtension = 1;

			int extendedDepth = depth - 1 + depthExtension;

			stack->CurrentMove = move;
			m_TranspositionTable.Prefetch(movedPosition.Hash);

			bool fullDepthSearch = false;
			int depthReduction = 0;
			ValueType value = -SCORE_MATE;

			// Late move reduction
			if (depth >= 3 && moveIndex > FirstMoveIndex + 2 * IsRoot && (!isCaptureOrPromotion || cutNode || moveCountPruning))
			{
				int reduction = GetReduction<NT>(improving, depth, moveIndex);
				if ((stack - 1)->MoveCount > 13)
					reduction--;

				if (stack->TTIsPv)
					reduction -= 2;

				if (singularExtension)
					reduction--;

				if (!isCaptureOrPromotion)
				{
					if (ttMoveIsCapture)
						reduction++;
					if (cutNode)
						reduction += 2;
				}
				else
				{
					if (depth < 8 && moveIndex > FirstMoveIndex + 1)
						reduction++;
				}

				int d = std::clamp(extendedDepth - reduction, 1, extendedDepth);

				value = -SearchPosition<NonPV>(movedPosition, stack + 1, d, -(alpha + 1), -alpha, selDepth, true, rootInfo);
				fullDepthSearch = value > alpha && d != extendedDepth;
			}
			else
			{
				fullDepthSearch = !IsPvNode || moveIndex > FirstMoveIndex;
			}

			if (fullDepthSearch)
			{
				value = -SearchPosition<NonPV>(movedPosition, stack + 1, extendedDepth, -(alpha + 1), -alpha, selDepth, !cutNode, rootInfo);
			}
			if (IsPvNode && (moveIndex == FirstMoveIndex || (value > alpha && (IsRoot || value < beta))))
			{
				pv[0] = MOVE_NONE;
				(stack + 1)->PV = pv;
				value = -SearchPosition<PV>(movedPosition, stack + 1, extendedDepth, -beta, -alpha, selDepth, false, rootInfo);
			}

			if (CheckLimits())
			{
				m_WasStopped = true;
				return SCORE_NONE;
			}

			if (IsRoot)
			{
				RootMove& rm = *std::find(rootInfo.Moves.begin(), rootInfo.Moves.end(), move);
				
				if (moveIndex == FirstMoveIndex || value > alpha)
				{
					rm.Score = value;
					rm.SelDepth = selDepth;
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
				bestValue = value;
				if (value > alpha)
				{
					bestMove = move;
					if (IsPvNode && !IsRoot)
						UpdatePV(stack->PV, move, (stack + 1)->PV);
					if (IsPvNode && value < beta)
						alpha = value;
				}
			}

			// Beta cutoff - Fail high
			if (value >= beta)
			{
				if (!isCaptureOrPromotion)
				{
					m_OrderingTables.CounterMoves[previousMove.GetFromSquareIndex()][previousMove.GetToSquareIndex()] = move;
					UpdateQuietStats(position, stack, depth, move);
				}
				if (stack->ExcludedMove == MOVE_NONE && rootInfo.PVIndex == 0)
				{
					ttEntry->Update(ttHash, move, depth, GetValueForTT(value, stack->Ply), LOWER_BOUND, position.GetTotalHalfMoves(), IsPvNode);
				}
				return value;
			}
			else if (!isCaptureOrPromotion)
			{
				m_OrderingTables.Butterfly[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
			}
		}

		if (bestValue <= alpha)
			stack->TTIsPv = stack->TTIsPv || ((stack->TTIsPv) && depth > 3);
		else if (depth > 3)
			stack->TTIsPv = stack->TTIsPv && (stack + 1)->TTIsPv;

		EntryFlag entryFlag = (bestValue > originalAlpha) ? EXACT : UPPER_BOUND;
		if (stack->ExcludedMove == MOVE_NONE && !(IsRoot && rootInfo.PVIndex == 0))
		{
			ttEntry->Update(ttHash, bestMove, depth, GetValueForTT(bestValue, stack->Ply), entryFlag, position.GetTotalHalfMoves(), IsPvNode);
		}
		return bestValue;
	}

	template<Search::NodeType NT>
	ValueType Search::QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta)
	{
		constexpr bool IsPvNode = NT == PV;
		const bool inCheck = position.InCheck();

		MoveList pvMoveList = m_MovePool.GetList();
		Move* pv = pvMoveList.Moves;

		ValueType originalAlpha = alpha;

		ZobristHash ttHash = position.Hash;
		if (stack->ExcludedMove != MOVE_NONE)
			ttHash ^= stack->ExcludedMove.GetKey();
		TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(ttHash, stack->TTHit);
		ValueType ttValue = stack->TTHit ? GetValueFromTT(ttEntry->GetScore(), stack->Ply) : SCORE_NONE;

		if (!IsPvNode && stack->TTHit && !IsMateScore(ttValue))
		{
			if (ttEntry->GetFlag() == EXACT)
				return ttValue;
			if (ttEntry->GetFlag() == UPPER_BOUND && ttValue <= alpha)
				return ttValue;
			if (ttEntry->GetFlag() == LOWER_BOUND && ttValue >= beta)
				return ttValue;
		}

		if (IsDraw(position, nullptr))
			return EvaluateDraw(position, stack->Contempt);

		ValueType evaluation = MatedIn(stack->Ply);
		if (!inCheck)
		{
			if (stack->TTHit && ttEntry->GetDepth() >= depth - 3 && (ttEntry->GetFlag() & LOWER_BOUND) && !IsMateScore(ttValue))
				evaluation = ttValue;
			else
				evaluation = StaticEvalPosition(position, alpha, beta, stack->Ply);

			if (evaluation >= beta)
			{
				if (stack->ExcludedMove != MOVE_NONE)
					ttEntry->Update(ttHash, MOVE_NONE, depth, GetValueForTT(evaluation, stack->Ply), LOWER_BOUND, position.GetTotalHalfMoves(), IsPvNode);
				return evaluation;
			}
			if (alpha < evaluation)
				alpha = evaluation;
		}

		if (stack->Ply >= MAX_PLY)
			return evaluation;

		(stack + 1)->Ply = stack->Ply + 1;
		(stack + 1)->Contempt = -stack->Contempt;

		MoveGenerator generator(position);
		MoveList legalMoves = m_MovePool.GetList();
		generator.GetPseudoLegalMoves(legalMoves);

		if (legalMoves.MoveCount <= 0)
		{
			if (inCheck)
				return MatedIn(stack->Ply);
			return EvaluateDraw(position, stack->Contempt);
		}

		if (!inCheck && evaluation >= beta + 250)
		{
			if (IsMateScore(evaluation))
				return evaluation;
			return evaluation - 250;
		}

		ValueType bestValue = MatedIn(stack->Ply);

		constexpr int FIRST_MOVE_INDEX = 1;
		int moveIndex = 0;
		QuiescenceMoveSelector selector(position, legalMoves, inCheck);
		Move move;

		while (!selector.Empty())
		{
			moveIndex++;
			move = selector.GetNextMove();

			if (generator.IsLegal(move))
			{
				Position movedPosition = position;
				ApplyMove(movedPosition, move);
				m_Nodes++;

				m_TranspositionTable.Prefetch(movedPosition.Hash);

				stack->CurrentMove = move;

				if (IsPvNode)
				{
					pv[0] = MOVE_NONE;
					(stack + 1)->PV = pv;
				}

				ValueType score = -QuiescenceSearch<NT>(movedPosition, stack + 1, depth - 1, -beta, -alpha);
				if (score > bestValue)
					bestValue = score;

				if (CheckLimits())
				{
					m_WasStopped = true;
					return SCORE_NONE;
				}

				if (score > alpha)
				{
					alpha = score;
					if (IsPvNode && (stack + 1)->PV)
					{
						UpdatePV(stack->PV, move, (stack + 1)->PV);
					}
				}

				if (score >= beta)
				{
					if (stack->ExcludedMove != MOVE_NONE)
						ttEntry->Update(ttHash, move, depth, GetValueForTT(score, stack->Ply), LOWER_BOUND, position.GetTotalHalfMoves(), IsPvNode);
					return score;
				}
			}
		}

		EntryFlag entryFlag = (alpha > originalAlpha) ? EXACT : UPPER_BOUND;
		if (stack->ExcludedMove == MOVE_NONE)
			ttEntry->Update(ttHash, MOVE_NONE, depth, GetValueForTT(alpha, stack->Ply), entryFlag, position.GetTotalHalfMoves(), IsPvNode);

		return alpha;
	}

	bool Search::CheckLimits() const
	{
		if (m_Limits.Infinite)
			return m_ShouldStop;
		// Only check every 1024 nodes
		if (m_Limits.Milliseconds > 0 && !(m_Nodes & 1023))
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
		if (stack && stack->Ply + m_PositionHistory.size() >= 8)
		{
			int ply = 4;
			ZobristHash hash = position.Hash;
			int count = 0;
			while (true)
			{
				if (*(stack->PositionHistory - ply) == hash)
				{
					count++;
					if (count >= 1) // 2
						return true;
				}
				else
					break;
				ply += 4;
			}
		}
		return false;
	}

	ValueType Search::EvaluateDraw(const Position& position, ValueType contempt) const
	{
		return SCORE_DRAW - contempt;
	}

	ValueType Search::MateIn(int ply) const
	{
		return SCORE_MATE - ply;
	}

	ValueType Search::MatedIn(int ply) const
	{
		return -SCORE_MATE + ply;
	}

	ValueType Search::GetValueForTT(ValueType value, int currentPly) const
	{
		if (IsMateScore(value))
		{
			if (value > 0)
				return value + currentPly;
			else
				return value - currentPly;
		}
		return value;
	}

	ValueType Search::GetValueFromTT(ValueType value, int currentPly) const
	{
		if (IsMateScore(value))
		{
			if (value > 0)
				return value - currentPly;
			else
				return value + currentPly;
		}
		return value;
	}

	int Search::GetPliesFromMateScore(ValueType score) const
	{
		if (score > 0)
			return SCORE_MATE - score;
		return score + SCORE_MATE;
	}

	bool Search::IsMateScore(ValueType score) const
	{
		return score != SCORE_NONE && (score >= MateIn(MAX_PLY) || score <= MatedIn(MAX_PLY));
	}

	ValueType Search::StaticEvalPosition(const Position& position, ValueType alpha, ValueType beta, int ply) const
	{
		BOX_ASSERT(!position.InCheck(), "Cannot evaluate position in check");
		return Evaluate(position, position.TeamToPlay, alpha, beta);
	}

	void Search::UpdateQuietStats(const Position& position, SearchStack* stack, int depth, Move move)
	{
		if (move != stack->KillerMoves[0] && move != stack->KillerMoves[1])
		{
			stack->KillerMoves[1] = stack->KillerMoves[0];
			stack->KillerMoves[0] = move;
		}
		m_OrderingTables.History[position.TeamToPlay][move.GetFromSquareIndex()][move.GetToSquareIndex()] += depth * depth;
	}

	std::vector<Search::RootMove> Search::GenerateRootMoves(const Position& position, SearchStack* stack)
	{
		MoveList list = m_MovePool.GetList();
		MoveGenerator generator(position);
		generator.GetPseudoLegalMoves(list);
		generator.FilterLegalMoves(list);
		std::vector<RootMove> result;
		for (int i = 0; i < list.MoveCount; i++)
		{
			if (m_Limits.Only.empty() || (m_Limits.Only.find(list.Moves[i]) != m_Limits.Only.end()))
			{
				RootMove mv;
				mv.Score = SCORE_NONE;
				mv.PV = { list.Moves[i] };
				result.push_back(mv);
			}
		}
		return result;
	}

	int Search::ChooseBestMove(const std::vector<RootMove>& moves, int skillLevel, int maxPVs) const
	{
		if (skillLevel >= 20 || moves.empty())
			return 0;

		Random::Seed(time(nullptr));
		int pvs = std::min((int)moves.size(), maxPVs);

		ValueType bestScore = moves[0].Score;
		ValueType delta = std::min(bestScore - moves[pvs - 1].Score, GetPieceValue(PIECE_PAWN));
		ValueType weakness = 120 - 2 * skillLevel;
		ValueType maxScore = -SCORE_MATE;

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
