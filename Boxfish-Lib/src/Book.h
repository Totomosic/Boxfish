#pragma once
#include "ZobristHash.h"
#include "Position.h"
#include "Move.h"

#include <optional>
#include <unordered_map>

namespace Boxfish
{

	struct BOX_API BookEntry
	{
	public:
		ZobristHash Hash = 0ULL;
		SquareIndex From = a1;
		SquareIndex To = a1;
		int Count = 1;
	};

	struct BOX_API BookEntryCollection
	{
	public:
		std::vector<BookEntry> Entries = {};
		int TotalCount = 0;

	public:
		// Weighted based on entry Count / TotalCount
		BookEntry PickRandom() const;
	};

	class BOX_API OpeningBook
	{
	private:
		size_t m_EntryCount;
		std::unordered_map<ZobristHash, BookEntryCollection> m_Entries;
		// Max depth of any entry
		int m_Cardinality;

	public:
		OpeningBook();

		size_t GetEntryCount() const;
		int GetCardinality() const;
		void SetCardinality(int cardinality);

		size_t CalculateFileSize() const;
		bool Serialize(void* buffer, size_t size) const;
		void WriteToFile(const std::string& filename) const;

		void AppendFromFile(const std::string& filename);
		void AppendEntry(const BookEntry& entry);

		std::optional<BookEntryCollection> Probe(const ZobristHash& hash) const;

	private:
		static constexpr size_t GetSerializedEntrySize() { return (sizeof(BookEntry::Hash.Hash) + sizeof(BookEntry::From) + sizeof(BookEntry::To) + sizeof(BookEntry::Count)); }

	};

}
