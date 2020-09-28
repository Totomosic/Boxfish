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
		int8_t Depth;
		ValueType Score;
		int Age = -1;
		EntryFlag Flag;

	public:
		inline ZobristHash GetHash() const { return Hash; }
		inline Move GetMove() const { return BestMove; }
		inline int GetDepth() const { return (int)Depth; }
		inline ValueType GetScore() const { return Score; }
		inline EntryFlag GetFlag() const { return Flag; }
		inline int GetAge() const { return Age; }

		inline void Update(ZobristHash hash, Move move, int depth, ValueType score, EntryFlag flag, int age)
		{
			BOX_ASSERT(flag >= 0 && flag <= 2, "Invalid flag");
			Hash = hash;
			BestMove = move;
			Depth = (int8_t)depth;
			Score = score;
			Age = age;
			Flag = flag;
		}
	};

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

		int GetFullProportion() const;

		inline void Prefetch(const ZobristHash& hash) const
		{
#ifndef EMSCRIPTEN
			void* address = &m_Entries[GetIndexFromHash(hash)];
	#ifdef BOX_PLATFORM_WINDOWS
			_mm_prefetch((const char*)address, _MM_HINT_T0);
	#else
			__builtin_prefetch(address);
	#endif
#endif
		}

		inline TranspositionTableEntry* GetEntry(const ZobristHash& hash, bool& found) const
		{
			TranspositionTableEntry* entry = &m_Entries[GetIndexFromHash(hash)];
			found = entry->GetAge() >= 0 && entry->GetHash() == hash;
			return entry;
		}

		void Clear();

	private:
		inline size_t GetIndexFromHash(const ZobristHash& hash) const { return hash.Hash & m_Mask; }
	};

}
