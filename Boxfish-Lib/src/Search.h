#pragma once
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "PositionUtils.h"
#include "TranspositionTable.h"
#include "MoveSelector.h"

#include <chrono>
#include <atomic>

namespace Boxfish
{

	struct BOX_API SearchLimits
	{
	public:
		bool Infinite = false;
		int64_t Nodes = -1;
		int Milliseconds = -1;
	};

	class BOX_API PositionHistory
	{
	private:
		std::vector<ZobristHash> m_Hashes;

	public:
		PositionHistory();

		bool Contains(const Position& position, int searchIndex) const;
		void Push(const Position& position);
		void Clear();
	};

	struct BOX_API SearchStats
	{
	public:
		Line PV;
		size_t TableHits = 0;
		size_t TableMisses = 0;
	};

	struct BOX_API SearchStack
	{
		Move* PV = nullptr;
		int Ply = 0;
		Move CurrentMove;
		Move KillerMoves[2];
		Centipawns StaticEvaluation = -SCORE_MATE;
		int MoveCount = 0;
		bool InCheck = false;
	};

	struct BOX_API SearchResult
	{
	public:
		std::vector<Move> PV;
		Centipawns Score;
		Move BestMove;
	};

	class RootMove
	{
	public:
		Centipawns Score;
		std::vector<Move> PV;

	public:
		inline bool operator==(const Move& move) const
		{
			return move == PV[0];
		}

		inline friend bool operator<(const RootMove& left, const RootMove& right)
		{
			return right.Score < left.Score;
		}
	};

	class BOX_API Search
	{
	private:
		enum class NodeType
		{
			PV,
			NonPV
		};

	private:
		TranspositionTable m_TranspositionTable;
		Position m_CurrentPosition;
		PositionHistory m_PositionHistory;
		SearchLimits m_Limits;

		MovePool m_MovePool;
		std::vector<RootMove> m_RootMoves;

		size_t m_Nodes;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_SearchRootStartTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;

		bool m_WasStopped;
		std::atomic<bool> m_ShouldStop;
		bool m_Log;

		MoveOrderingInfo m_OrderingInfo;

	public:
		Search(size_t transpositionTableSize = TranspositionTable::TABLE_SIZE, bool log = true);

		PositionHistory& GetHistory();
		void SetLimits(const SearchLimits& limits);

		void SetCurrentPosition(const Position& position);
		void SetCurrentPosition(Position&& position);
		Move Go(int depth, const std::function<void(SearchResult)>& callback = {});
		void Ponder(const std::function<void(SearchResult)>& callback = {});
		void Reset();

		void Stop();

	private:
		Line GetPV(int depth) const;

		void SearchRoot(const Position& position, int depth, SearchStats& stats, const std::function<void(SearchResult)>& callback);
		template<NodeType type>
		Centipawns SearchPosition(const Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, SearchStats& stats);
		template<NodeType type>
		Centipawns QuiescenceSearch(const Position& position, SearchStack* stack, Centipawns alpha, Centipawns beta);

		bool CheckLimits() const;

		Centipawns GetMoveScoreBonus(const Position& position, const Move& move) const;
		Centipawns EvaluateDraw(const Position& postion) const;
	};

}
