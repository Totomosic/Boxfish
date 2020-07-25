#include "TranspositionTable.h"

namespace Boxfish
{

	TranspositionTable::TranspositionTable(size_t sizeBytes)
		: m_Entries{ nullptr }, m_EntryCount()
	{
		size_t nEntries = sizeBytes / sizeof(TranspositionTableEntry);

		if (nEntries != 0)
		{
			uint8_t nBits = 1;
			size_t ret = 1;
			while (nEntries >>= 1)
			{
				nBits++;
				ret <<= 1;
			}
			m_EntryCount = ret - 1;
			m_Entries = std::make_unique<TranspositionTableEntry[]>(m_EntryCount);
			m_Mask = m_EntryCount;
		}

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
		for (size_t i = 0; i < m_EntryCount; i++)
		{
			m_Entries[i].Age = -1;
		}
	}

	size_t TranspositionTable::GetIndexFromHash(const ZobristHash& hash) const
	{
		return hash.Hash & m_Mask;
	}

}
