#pragma once
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "PositionUtils.h"
#include "TranspositionTable.h"
#include "MoveSelector.h"

#include <chrono>

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

	class BOX_API Search
	{
	private:
		struct BOX_API SearchData
		{
		public:
			int Depth;
			int Ply;
			int Alpha;
			int Beta;
			bool IsPV;
		};

	private:
		TranspositionTable m_TranspositionTable;
		Position m_CurrentPosition;
		PositionHistory m_PositionHistory;
		SearchLimits m_Limits;

		MovePool m_MovePool;

		Move m_BestMove;
		Centipawns m_BestScore;
		int m_SearchDepth;

		size_t m_Nodes;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_SearchRootStartTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;

		bool m_WasStopped;
		bool m_ShouldStop;
		bool m_Log;

		MoveOrderingInfo m_OrderingInfo;

	public:
		Search(bool log = true);

		PositionHistory& GetHistory();
		void SetLimits(const SearchLimits& limits);

		const Move& GetBestMove() const;
		Centipawns GetBestScore() const;
		Line GetPV() const;

		void SetCurrentPosition(const Position& position);
		void SetCurrentPosition(Position&& position);
		Move Go(int depth);
		void Reset();

	private:
		Line GetPV(int depth) const;

		void SearchRoot(const Position& position, int depth, SearchStats& stats);
		Centipawns Negamax(const Position& position, SearchData data, SearchStats& stats);
		Centipawns QuiescenceSearch(const Position& position, int alpha, int beta, int searchIndex);

		bool CheckLimits() const;

		Centipawns GetMoveScoreBonus(const Position& position, const Move& move) const;
	};

}
