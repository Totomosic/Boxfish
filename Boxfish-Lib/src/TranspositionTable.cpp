#include "TranspositionTable.h"
#include <bitset>

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
			m_Entries = std::make_unique<TranspositionTableEntry[]>(m_EntryCount + 1);
			m_Mask = m_EntryCount;
		}

		Clear();
	}

	int TranspositionTable::GetFullProportion() const
	{
		int count = 0;
		int samples = 1023 * 64;
		for (uint64_t i = 0; i < samples; i++)
		{
			if (m_Entries[(i * 67) & m_Mask].GetAge() >= 0)
				count++;
		}
		return (int)(count * 1000 / samples);
	}

	void TranspositionTable::Clear()
	{
		for (size_t i = 0; i < m_EntryCount; i++)
		{
			m_Entries[i].Update(0ULL, MOVE_NONE, -1, SCORE_NONE, LOWER_BOUND, -1, false);
		}
	}

}
