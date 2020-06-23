#pragma once
#include "Move.h"
#include "Position.h"

namespace Boxfish
{

	class BOX_API MoveGenerator
	{
	public:
		static constexpr size_t MAX_MOVES = 256;

	private:
		Position m_Position;
		bool m_IsValid;
		std::vector<Move> m_PseudoLegalMoves;
		std::vector<Move> m_LegalMoves;

	public:
		MoveGenerator();
		MoveGenerator(const Position& position);

		const Position& GetPosition() const;
		void SetPosition(const Position& position);
		void SetPosition(Position&& position);

		const std::vector<Move>& GetPseudoLegalMoves();
		const std::vector<Move>& GetLegalMoves();

	private:
		void Reset();

		void GeneratePseudoLegalMoves();
		void GenerateLegalMoves();

		void GenerateMoves(Team team, Piece pieceType, const Position& position);

		void GeneratePawnPromotions(Team team, const Position& position);
		void GeneratePawnSinglePushes(Team team, const Position& position);
		void GeneratePawnDoublePushes(Team team, const Position& position);
		void GeneratePawnLeftAttacks(Team team, const Position& position);
		void GeneratePawnRightAttacks(Team team, const Position& position);

		void GenerateKnightMoves(Team team, const Position& position);
		void GenerateBishopMoves(Team team, const Position& position);
		void GenerateRookMoves(Team team, const Position& position);
		void GenerateQueenMoves(Team team, const Position& position);
		void GenerateKingMoves(Team team, const Position& position);

		void AddMoves(const Position& position, Team team, const Square& fromSquare, Piece pieceType, const BitBoard& moves, const BitBoard& attackablePieces);
	};

}
