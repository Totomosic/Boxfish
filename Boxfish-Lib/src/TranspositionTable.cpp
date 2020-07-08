#include "TranspositionTable.h"

namespace Boxfish
{

	TranspositionTable::TranspositionTable(size_t sizeBytes)
		: m_Entries{ std::make_unique<TranspositionTableEntry[]>(sizeBytes / sizeof(TranspositionTableEntry)) }
	{
		Clear();
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
		size_t index = GetIndexFromHash(entry.Hash);
		if (m_Entries[index].Age < 0)
		{
			m_Entries[index] = entry;
		}
		else
		{
			TranspositionTableEntry& existingEntry = m_Entries[index];
			// Decide whether to replace the entry
			if (existingEntry.Depth < entry.Depth)
			{
				existingEntry = entry;
			}
		}
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
