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

	class BOX_API Search
	{
	private:
		enum NodeType
		{
			PV,
			NonPV
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

		void SetSettings(const BoxfishSettings& settings);
		void SetLimits(const SearchLimits& limits);
		void PushPosition(const Position& position);
		void Reset();

		size_t Perft(const Position& position, int depth);
		Move SearchBestMove(const Position& position, const SearchLimits& limits, const std::function<void(SearchResult)>& callback = {});
		void Ponder(const Position& position, const std::function<void(SearchResult)>& callback = {});

		void Stop();

	private:
		size_t PerftPosition(Position& position, int depth);

		RootMove SearchRoot(Position& position, int depth, const std::function<void(SearchResult)>& callback);
		template<NodeType type>
		Centipawns SearchPosition(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta, const RootInfo& rootInfo);
		template<NodeType type>
		Centipawns QuiescenceSearch(Position& position, SearchStack* stack, int depth, Centipawns alpha, Centipawns beta);

		bool CheckLimits() const;

		bool IsDraw(const Position& position, SearchStack* stack) const;
		Centipawns EvaluateDraw(const Position& postion, Centipawns contempt) const;
		Centipawns MateIn(int ply) const;
		Centipawns MatedIn(int ply) const;
		bool IsMateScore(Centipawns score) const;
		Centipawns StaticEvalPosition(const Position& position, Centipawns alpha, Centipawns beta, int ply) const;

		std::vector<RootMove> GenerateRootMoves(const Position& position, SearchStack* stack);
		int ChooseBestMove(const std::vector<RootMove>& moves, int skillLevel, int maxPVs) const;
	};

}
