#include "TranspositionTable.h"

namespace Boxfish
{

	TranspositionTable::TranspositionTable()
		: m_Entries{}
	{
	}

	const TranspositionTableEntry* TranspositionTable::GetEntry(const ZobristHash& hash) const
	{
		const TranspositionTableEntry* entry = &m_Entries[GetIndexFromHash(hash)];
		if (entry->Age >= 0)
			return entry;
		return nullptr;
	}

	void TranspositionTable::AddEntry(const TranspositionTableEntry& entry)
	{
	}

	void TranspositionTable::Clear()
	{
		for (size_t i = 0; i < TABLE_SIZE; i++)
		{
			m_Entries[i].Age = -1;
		}
	}

	size_t TranspositionTable::GetIndexFromHash(const ZobristHash& hash) const
	{
		return hash.Hash % TABLE_SIZE;
	}

}
