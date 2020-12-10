#pragma once
#include "Boxfish.h"
#include <unordered_map>
#include <thread>

namespace Boxfish
{

	class CommandManager
	{
	private:
		std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> m_CommandMap;

		OpeningBook m_OpeningBook;

		Position m_CurrentPosition;
		Search m_Search;
		BoxfishSettings m_Settings;
		std::atomic<bool> m_Searching;
		std::thread m_SearchThread;

	public:
		CommandManager();
		~CommandManager();

		void ExecuteCommand(const std::string& uciCommand);

	private:
		void Help();
		void IsReady();
		void NewGame();
		void PrintBoard();
		void SetOption(std::string name, const std::string& value);
		void SetPositionFen(const std::string& fen);
		void ApplyMoves(const std::vector<std::string>& moves);
		void Eval();
		void Perft(int depth);
		void GoDepth(int depth, const std::unordered_set<Move>& includedMoves);
		void GoTime(int milliseconds, const std::unordered_set<Move>& includedMoves);
		void GoPonder(const std::unordered_set<Move>& includedMoves);
		void Stop();
		void Quit();

		// Debug helpers
		void Moves();
		void ProbeTT();

		// Utils
		std::unordered_set<Move> GetMoveList(const std::vector<std::string>& args, int offset) const;

	};

}
