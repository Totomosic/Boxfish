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
		EntryFlag Flag;
		int Age = -1; // Half move clock
	};

	class BOX_API TranspositionTable
	{
	public:
		static constexpr uint64_t TABLE_SIZE = 50 * 1024 * 1024;

	private:
		std::unique_ptr<TranspositionTableEntry[]> m_Entries;
		size_t m_EntryCount;

	public:
		TranspositionTable(size_t sizeBytes = TABLE_SIZE);

		const TranspositionTableEntry* GetEntry(const ZobristHash& hash) const;
		void AddEntry(const TranspositionTableEntry& entry);
		void Clear();

	private:
		size_t GetIndexFromHash(const ZobristHash& hash) const;
	};

}
