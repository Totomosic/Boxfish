#pragma once
#include "ZobristHash.h"
#include "Move.h"
#include "Evaluation.h"
#include <memory>

namespace Boxfish
{

	enum EntryFlag
	{
		EXACT,
		UPPER_BOUND,
		LOWER_BOUND
	};

	struct BOX_API TranspositionTableEntry
	{
	public:
		ZobristHash Hash;
		Move BestMove;
		int Depth;
		Centipawns Score;
		int Age;
		EntryFlag Flag;

	public:
		inline ZobristHash GetHash() const { return Hash; }
		inline Move GetMove() const { return BestMove; }
		inline int GetDepth() const { return Depth; }
		inline Centipawns GetScore() const { return Score; }
		inline EntryFlag GetFlag() const { return Flag; }
		inline int GetAge() const { return Age; }

		inline void Update(ZobristHash hash, Move move, int depth, Centipawns score, EntryFlag flag, int age)
		{
			Hash = hash;
			BestMove = move;
			Depth = depth;
			Score = score;
			Age = age;
			Flag = flag;
		}
	}
		
	;

	class BOX_API TranspositionTable
	{
	public:
		static constexpr uint64_t TABLE_SIZE = 50 * 1024 * 1024;

	private:
		std::unique_ptr<TranspositionTableEntry[]> m_Entries;
		size_t m_EntryCount;
		size_t m_Mask;

	public:
		TranspositionTable(size_t sizeBytes = TABLE_SIZE);

		TranspositionTableEntry* GetEntry(const ZobristHash& hash, bool& found) const;
		void Clear();

	private:
		size_t GetIndexFromHash(const ZobristHash& hash) const;
	};

}
