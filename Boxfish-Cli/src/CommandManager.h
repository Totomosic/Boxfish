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
		void GoDepth(int depth);
		void GoTime(int milliseconds);
		void GoPonder();
		void Moves();
		void Stop();
		void Quit();

	};

}
