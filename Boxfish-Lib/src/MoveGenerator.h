#pragma once
#include "Move.h"
#include "PositionUtils.h"
#include "Position.h"

namespace Boxfish
{

	constexpr int MAX_MOVES = 218;

	class BOX_API MoveList
	{
	public:
		uint8_t MoveCount = 0;
		Move Moves[MAX_MOVES];

	public:
		inline bool Empty() const { return MoveCount <= 0; }
	};

	class BOX_API MoveGenerator
	{
	private:
		Position m_Position;

	public:
		MoveGenerator();
		MoveGenerator(const Position& position);

		const Position& GetPosition() const;
		void SetPosition(const Position& position);
		void SetPosition(Position&& position);

		MoveList GetPseudoLegalMoves();
		MoveList GetLegalMoves(const MoveList& pseudoLegalMoves);
		void FilterLegalMoves(MoveList& pseudoLegalMoves);

		bool HasAtLeastOneLegalMove();

	private:
		void GeneratePseudoLegalMoves(MoveList& moveList);
		void GenerateLegalMoves(MoveList& moveList, const MoveList& pseudoLegalMoves);
		bool IsMoveLegal(const Move& move, const BitBoard& checkers, bool multipleCheckers) const;

		void GenerateMoves(MoveList& moveList, Team team, Piece pieceType, const Position& position);

		void GeneratePawnPromotions(MoveList& moveList, SquareIndex fromSquare, SquareIndex toSquare, MoveFlag flags, Piece capturedPiece);
		void GeneratePawnSinglePushes(MoveList& moveList, Team team, const Position& position);
		void GeneratePawnDoublePushes(MoveList& moveList, Team team, const Position& position);
		void GeneratePawnLeftAttacks(MoveList& moveList, Team team, const Position& position);
		void GeneratePawnRightAttacks(MoveList& moveList, Team team, const Position& position);

		void GenerateKnightMoves(MoveList& moveList, Team team, const Position& position);
		void GenerateBishopMoves(MoveList& moveList, Team team, const Position& position);
		void GenerateRookMoves(MoveList& moveList, Team team, const Position& position);
		void GenerateQueenMoves(MoveList& moveList, Team team, const Position& position);
		void GenerateKingMoves(MoveList& moveList, Team team, const Position& position);

		void AddMoves(MoveList& moveList, const Position& position, Team team, SquareIndex fromSquare, Piece pieceType, const BitBoard& moves, const BitBoard& attackablePieces);
	};

}
