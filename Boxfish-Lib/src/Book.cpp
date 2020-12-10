#include "Book.h"
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>

namespace Boxfish
{

	size_t GetFileSize(const std::string& filename)
	{
		std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
		return in.tellg();
	}

	BookEntry BookEntryCollection::PickRandom() const
	{
		std::vector<BookEntry> sortedEntries = Entries;
		std::sort(sortedEntries.begin(), sortedEntries.end(), [](const BookEntry& a, const BookEntry& b)
		{
			return a.Count > b.Count;
		});

		std::mt19937 engine;
		engine.seed(std::chrono::system_clock::now().time_since_epoch().count());
		//std::uniform_int_distribution<int> dist(0, sortedEntries.size() - 1);
		std::uniform_int_distribution<int> dist(0, TotalCount - 1);
		int count = dist(engine);
		
		for (int i = 0; i < sortedEntries.size(); i++)
		{
			if (sortedEntries[i].Count <= count)
				return sortedEntries[i];
			count -= sortedEntries[i].Count;
		}
		BOX_ASSERT(false, "Logic error");
		return sortedEntries[0];
		//return sortedEntries[count];
	}

	OpeningBook::OpeningBook()
		: m_EntryCount(0), m_Entries(), m_Cardinality(0)
	{
	}

	size_t OpeningBook::GetEntryCount() const
	{
		return m_EntryCount;
	}

	int OpeningBook::GetCardinality() const
	{
		return m_Cardinality;
	}

	void OpeningBook::SetCardinality(int cardinality)
	{
		m_Cardinality = cardinality;
	}

	size_t OpeningBook::CalculateFileSize() const
	{
		return sizeof(m_EntryCount) + sizeof(m_Cardinality) + m_EntryCount * GetSerializedEntrySize();
	}

	bool OpeningBook::Serialize(void* buffer, size_t size) const
	{
		if (size < CalculateFileSize())
			return false;
		char* workingBuffer = (char*)buffer;
		memcpy(workingBuffer, &m_EntryCount, sizeof(m_EntryCount));
		workingBuffer += sizeof(m_EntryCount);
		memcpy(workingBuffer, &m_Cardinality, sizeof(m_Cardinality));
		workingBuffer += sizeof(m_Cardinality);

		for (const auto& pair : m_Entries)
		{
			for (const auto& entry : pair.second.Entries)
			{
				memcpy(workingBuffer, &entry.Hash.Hash, sizeof(entry.Hash.Hash));
				workingBuffer += sizeof(entry.Hash.Hash);
				memcpy(workingBuffer, &entry.From, sizeof(entry.From));
				workingBuffer += sizeof(entry.From);
				memcpy(workingBuffer, &entry.To, sizeof(entry.To));
				workingBuffer += sizeof(entry.To);
				memcpy(workingBuffer, &entry.Count, sizeof(entry.Count));
				workingBuffer += sizeof(entry.Count);
			}
		}
		return true;
	}

	void OpeningBook::WriteToFile(const std::string& filename) const
	{
		size_t bufferSize = CalculateFileSize() * sizeof(char);
		char* buffer = new char[bufferSize / sizeof(char)];
		if (Serialize(buffer, bufferSize))
		{
			std::ofstream file(filename, std::ios::binary);
			file.write(buffer, bufferSize);
			file.close();
		}
		delete[] buffer;
	}

	bool OpeningBook::AppendFromFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::in);
		if (file.good())
		{
			size_t filesize = GetFileSize(filename);
			if (filesize > sizeof(m_EntryCount) + sizeof(m_Cardinality))
			{
				char* buffer = new char[filesize];
				file.read(buffer, filesize);

				size_t entryCount;
				int cardinality;
				const char* workingBuffer = buffer;
				memcpy(&entryCount, workingBuffer, sizeof(entryCount));
				workingBuffer += sizeof(entryCount);
				memcpy(&cardinality, workingBuffer, sizeof(cardinality));
				workingBuffer += sizeof(cardinality);

				if (entryCount * GetSerializedEntrySize() + sizeof(cardinality) + sizeof(entryCount) == filesize)
				{
					for (size_t i = 0; i < entryCount; i++)
					{
						BookEntry entry;
						memcpy(&entry.Hash.Hash, workingBuffer, sizeof(entry.Hash.Hash));
						workingBuffer += sizeof(entry.Hash.Hash);
						memcpy(&entry.From, workingBuffer, sizeof(entry.From));
						workingBuffer += sizeof(entry.From);
						memcpy(&entry.To, workingBuffer, sizeof(entry.To));
						workingBuffer += sizeof(entry.To);
						memcpy(&entry.Count, workingBuffer, sizeof(entry.Count));
						workingBuffer += sizeof(entry.Count);
						AppendEntry(entry);
					}
					if (GetCardinality() < cardinality)
						SetCardinality(cardinality);
				}
				else
				{
					std::cout << "File size mismatch: Expected " << (entryCount * GetSerializedEntrySize()) << " got " << filesize << std::endl;
				}

				delete[] buffer;
			}
			return true;
		}
		return false;
	}

	void OpeningBook::AppendEntry(const BookEntry& entry)
	{
		if (GetCardinality() < 1)
			SetCardinality(1);
		BookEntryCollection& collection = m_Entries[entry.Hash];
		collection.TotalCount += entry.Count;
		for (auto& ent : collection.Entries)
		{
			if (ent.From == entry.From && ent.To == entry.To)
			{
				ent.Count += entry.Count;
				return;
			}
		}
		collection.Entries.push_back(entry);
		m_EntryCount++;
	}

	void OpeningBook::Clear()
	{
		m_EntryCount = 0;
		m_Entries.clear();
		m_Cardinality = 0;
	}

	std::optional<BookEntryCollection> OpeningBook::Probe(const ZobristHash& hash) const
	{
		auto it = m_Entries.find(hash);
		if (it != m_Entries.end())
		{
			return it->second;
		}
		return {};
	}

}
