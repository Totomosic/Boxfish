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

	void InitSearch();

	struct BOX_API SearchLimits
	{
	public:
		bool Infinite = false;
		int Depth = -1;
		int64_t Nodes = -1;
		int Milliseconds = -1;
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
		ValueType StaticEvaluation = -SCORE_MATE;
		ValueType Contempt = 0;
		Move ExcludedMove = MOVE_NONE;
	};

	struct BOX_API SearchResult
	{
	public:
		std::vector<Move> PV;
		ValueType Score;
		Move BestMove;
		int PVIndex;
	};

	class BOX_API Search
	{
	public:
		enum NodeType
		{
			PV,
			NonPV
		};

		class RootMove
		{
		public:
			ValueType Score;
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

		struct BOX_API RootInfo
		{
		public:
			std::vector<RootMove>& Moves;
			int PVIndex;
			int PVLast;
		};

	private:
		TranspositionTable m_TranspositionTable;
		BoxfishSettings m_Settings;
		SearchLimits m_Limits;
		std::vector<ZobristHash> m_PositionHistory;

		MovePool m_MovePool;

		size_t m_Nodes;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_SearchRootStartTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;

		bool m_WasStopped;
		std::atomic<bool> m_ShouldStop;
		bool m_Log;

		OrderingTables m_OrderingTables;

	public:
		Search(size_t transpositionTableSize = TranspositionTable::TABLE_SIZE, bool log = true);

		const BoxfishSettings& GetSettings() const;

		void SetSettings(const BoxfishSettings& settings);
		void SetLimits(const SearchLimits& limits);
		void PushPosition(const Position& position);
		void Reset();

		size_t Perft(const Position& position, int depth);
		Move SearchBestMove(const Position& position, const SearchLimits& limits);
		Move SearchBestMove(const Position& position, const SearchLimits& limits, const std::function<void(SearchResult)>& callback);
		void Ponder(const Position& position, const std::function<void(SearchResult)>& callback = {});

		void Stop();

	private:
		size_t PerftPosition(Position& position, int depth);

		RootMove SearchRoot(Position& position, int depth, const std::function<void(SearchResult)>& callback);
		template<NodeType type>
		ValueType SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode, const RootInfo& rootInfo);
		template<NodeType type>
		ValueType QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta);

		bool CheckLimits() const;

		bool IsDraw(const Position& position, SearchStack* stack) const;
		ValueType EvaluateDraw(const Position& postion, ValueType contempt) const;
		ValueType MateIn(int ply) const;
		ValueType MatedIn(int ply) const;
		ValueType GetValueForTT(ValueType value, int currentPly) const;
		ValueType GetValueFromTT(ValueType value, int currentPly) const;
		int GetPliesFromMateScore(ValueType score) const;
		bool IsMateScore(ValueType score) const;
		ValueType StaticEvalPosition(const Position& position, ValueType alpha, ValueType beta, int ply) const;

		void UpdateQuietStats(const Position& position, SearchStack* stack, int depth, Move move);
		bool ReplaceTT(int depth, int age, EntryFlag flag, const TranspositionTableEntry* entry);

		std::vector<RootMove> GenerateRootMoves(const Position& position, SearchStack* stack);
		int ChooseBestMove(const std::vector<RootMove>& moves, int skillLevel, int maxPVs) const;
	};

}
