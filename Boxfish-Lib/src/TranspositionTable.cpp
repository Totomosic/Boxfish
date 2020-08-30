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

	TranspositionTableEntry* TranspositionTable::GetEntry(const ZobristHash& hash, bool& found) const
	{
		TranspositionTableEntry* entry = &m_Entries[GetIndexFromHash(hash)];
		found = entry->GetAge() >= 0 && entry->GetHash() == hash;
		return entry;
	}

	void TranspositionTable::Clear()
	{
		for (size_t i = 0; i < m_EntryCount; i++)
		{
			m_Entries[i].Update(0ULL, MOVE_NONE, -1, SCORE_NONE, LOWER_BOUND, -1);
		}
	}

	size_t TranspositionTable::GetIndexFromHash(const ZobristHash& hash) const
	{
		return hash.Hash & m_Mask;
	}

}
