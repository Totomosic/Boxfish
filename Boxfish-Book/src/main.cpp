#include "Book.h"
#include "Boxfish.h"
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

using namespace Boxfish;

std::unordered_map<std::string, std::function<bool(const std::vector<std::string>&)>> s_CommandHandlers;
Boxfish::OpeningBook s_Book;

bool HandleHelp(const std::vector<std::string>& args)
{
	std::cout << "Commands:" << std::endl;
	std::cout << "\tload filename" << std::endl;
	std::cout << "\twrite filename" << std::endl;
	std::cout << "\tadd mv [mv, [mv, ...]]" << std::endl;
	std::cout << "\taddpgn mv [mv, [mv, ...]]" << std::endl;
	std::cout << "\tloadpgn filename" << std::endl;
	return true;
}

bool HandleLoad(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		s_Book.AppendFromFile(args[0]);
		return true;
	}
	return false;
}

bool HandleWrite(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		s_Book.WriteToFile(args[0]);
		return true;
	}
	return false;
}

bool IsLegalMove(Move move, const Position& position)
{
	Move moveBuffer[MAX_MOVES];
	MoveList moves(moveBuffer);
	MoveGenerator generator(position);
	generator.GetPseudoLegalMoves(moves);
	generator.FilterLegalMoves(moves);
	for (int i = 0; i < moves.MoveCount; i++)
	{
		if (moves.Moves[i] == move)
			return true;
	}
	return false;
}

bool HandleAdd(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		Position position = CreateStartingPosition();
		std::vector<BookEntry> entries;
		for (const std::string& move : args)
		{
			Move mv = UCI::CreateMoveFromString(position, move);
			if (IsLegalMove(mv, position))
			{
				BookEntry entry;
				entry.Hash = position.Hash;
				entry.From = mv.GetFromSquareIndex();
				entry.To = mv.GetToSquareIndex();
				entry.Count = 1;
				entries.push_back(entry);
				Boxfish::ApplyMove(position, mv);
			}
			else
			{
				std::cout << "Invalid Move: " << move << std::endl;
				return false;
			}
		}
		for (const auto& entry : entries)
			s_Book.AppendEntry(entry);
		if (s_Book.GetCardinality() < entries.size())
			s_Book.SetCardinality(entries.size());
		return true;
	}
	return false;
}

bool HandleAddPGN(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		Position position = CreateStartingPosition();
		std::vector<BookEntry> entries;
		for (const std::string& move : args)
		{
			Move mv = PGN::CreateMoveFromString(position, move);
			if (IsLegalMove(mv, position))
			{
				BookEntry entry;
				entry.Hash = position.Hash;
				entry.From = mv.GetFromSquareIndex();
				entry.To = mv.GetToSquareIndex();
				entry.Count = 1;
				entries.push_back(entry);
				Boxfish::ApplyMove(position, mv);
			}
			else
			{
				std::cout << "Invalid Move: " << move << std::endl;
				return false;
			}
		}
		for (const auto& entry : entries)
			s_Book.AppendEntry(entry);
		if (s_Book.GetCardinality() < entries.size())
			s_Book.SetCardinality(entries.size());
		return true;
	}
	return false;
}

bool HandleProbe(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		Position position;
		if (args[0] == "startpos")
			position = CreateStartingPosition();
		else
		{
			std::string fen = args[0];
			for (int i = 1; i < args.size(); i++)
				fen += " " + args[i];
			position = CreatePositionFromFEN(fen);
		}
		auto collection = s_Book.Probe(position.Hash);
		if (collection)
		{
			std::cout << "Book entry for FEN: " << GetFENFromPosition(position) << std::endl;
			std::cout << "Total Count: " << collection->TotalCount << std::endl;
			std::cout << position << std::endl;
			for (const auto& entry : collection->Entries)
			{
				Move move = CreateMove(position, BitBoard::BitIndexToSquare(entry.From), BitBoard::BitIndexToSquare(entry.To));
				std::cout << UCI::FormatMove(move) << " " << PGN::FormatMove(move, position) << " Count: " << entry.Count << std::endl;
			}
		}
		else
		{
			std::cout << "No book entry found for FEN: " << GetFENFromPosition(position) << std::endl;
			std::cout << "Hash: " << std::hex << position.Hash.Hash << std::dec << std::endl;
		}
		return true;
	}
	return false;
}

bool HandleLoadPGN(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		constexpr int MOVE_COUNT = 10;
		std::vector<PGNMatch> matches = PGN::ReadFromFile(args[0]);
		for (const auto& match : matches)
		{
			if (match.Moves.size() >= MOVE_COUNT)
			{
				Position position = match.InitialPosition;
				for (int i = 0; i < MOVE_COUNT; i++)
				{
					BookEntry entry;
					entry.Hash = position.Hash;
					entry.Count = 1;
					entry.From = match.Moves[i].GetFromSquareIndex();
					entry.To = match.Moves[i].GetToSquareIndex();
					s_Book.AppendEntry(entry);
					ApplyMove(position, match.Moves[i]);
				}
				if (s_Book.GetCardinality() < MOVE_COUNT)
					s_Book.SetCardinality(MOVE_COUNT);
			}
		}
		return matches.size() > 0;
	}
	return false;
}

int main()
{
	Init();
	char buffer[8192];

	s_CommandHandlers["help"] = HandleHelp;
	s_CommandHandlers["load"] = HandleLoad;
	s_CommandHandlers["write"] = HandleWrite;
	s_CommandHandlers["add"] = HandleAdd;
	s_CommandHandlers["addpgn"] = HandleAddPGN;
	s_CommandHandlers["probe"] = HandleProbe;
	s_CommandHandlers["loadpgn"] = HandleLoadPGN;

	while (true)
	{
		std::cin.getline(buffer, sizeof(buffer));

		std::vector<std::string> parts = Split(buffer, " ");
		if (parts.size() > 0)
		{
			const std::string& command = parts[0];
			auto it = s_CommandHandlers.find(command);
			if (it != s_CommandHandlers.end())
			{
				if (it->second({ parts.begin() + 1, parts.end() }))
					std::cout << "Success" << std::endl;
				else
					std::cout << "Failed" << std::endl;
			}
			else
			{
				std::cout << "Unknown command: " << buffer << std::endl;
			}
		}
	}
}
