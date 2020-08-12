#pragma once
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "PositionUtils.h"
#include "TranspositionTable.h"
#include "MoveSelector.h"
#include "Settings.h"

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

		const std::vector<ZobristHash>& GetPositions() const;

		bool Contains(const Position& position, int searchIndex) const;
		void Push(const Position& position);
		void Clear();
	};

	struct BOX_API SearchStats
	{
	public:
		size_t TableHits = 0;
		size_t TableMisses = 0;
	};

	struct BOX_API SearchStack
	{
		Move* PV = nullptr;
		ZobristHash* PositionHistory = nullptr;
		int Ply = 0;
		int PlySinceCaptureOrPawnPush = 0;
		int MoveCount = 0;
		Move CurrentMove = MOVE_NONE;
		Move KillerMoves[2] = { MOVE_NONE, MOVE_NONE };
		Centipawns StaticEvaluation = -SCORE_MATE;
		bool CanNull = true;
		Centipawns Contempt = 0;
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

		struct BOX_API RootInfo
		{
		public:
			std::vector<RootMove>& Moves;
			int PVIndex;
			int PVLast;
		};

	private:
		TranspositionTable m_TranspositionTable;
		Position m_CurrentPosition;
		PositionHistory m_PositionHistory;
		BoxfishSettings m_Settings;
		SearchLimits m_Limits;

		MovePool m_MovePool;

		size_t m_Nodes;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_SearchRootStartTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;

		bool m_WasStopped;
		std::atomic<bool> m_ShouldStop;
		bool m_Log;

		Move m_CounterMoves[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];
		Centipawns m_HistoryTable[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];
		Centipawns m_ButterflyTable[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

	public:
		Search(size_t transpositionTableSize = TranspositionTable::TABLE_SIZE, bool log = true);

		PositionHistory& GetHistory();
		void SetSettings(const BoxfishSettings& settings);
		void SetLimits(const SearchLimits& limits);

		void SetCurrentPosition(const Position& position);
		void SetCurrentPosition(Position&& position);
		size_t Perft(int depth);
		Move Go(int depth, const std::function<void(SearchResult)>& callback = {});
		void Ponder(const std::function<void(SearchResult)>& callback = {});
		void Reset();

		void Stop();

	private:
		size_t Perft(Position& position, int depth);

		RootMove SearchRoot(Position& position, int depth, SearchStats& stats, const std::function<void(SearchResult)>& callback);
		template<NodeType type>
		Centipawns SearchPosition(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, SearchStats& stats, const RootInfo& rootInfo);
		template<NodeType type>
		Centipawns QuiescenceSearch(Position& position, SearchStack* stack, Centipawns alpha, Centipawns beta);

		bool CheckLimits() const;

		bool IsDraw(const Position& position, SearchStack* stack) const;
		Centipawns EvaluateDraw(const Position& postion, Centipawns contempt) const;
		Centipawns MateIn(int ply) const;
		Centipawns MatedIn(int ply) const;
		bool IsMateScore(Centipawns score) const;
		Centipawns StaticEvalPosition(const Position& position, Centipawns alpha, Centipawns beta, int ply) const;

		void ClearCounterMoves();
		void ClearTable(Centipawns (&table)[TEAM_MAX][FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX]);

		std::vector<RootMove> GenerateRootMoves(const Position& position, SearchStack* stack);
		int ChooseBestMove(const std::vector<RootMove>& moves, int skillLevel, int maxPVs) const;
	};

}
