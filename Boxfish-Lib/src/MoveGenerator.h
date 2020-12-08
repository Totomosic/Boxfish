#pragma once
#include "Move.h"
#include "PositionUtils.h"
#include "Position.h"
#include <array>

#ifdef SWIG
#define BOX_API
#endif

namespace Boxfish
{

	constexpr int MAX_PLY = 100;
	constexpr int MAX_MOVES = 218;
	constexpr int MOVE_POOL_SIZE = MAX_MOVES * 2 * (MAX_PLY + 1);

	class MovePool;

	class BOX_API MoveList
	{
	private:
		MovePool* m_Pool;

	public:
		uint8_t MoveCount = 0;
		Move* Moves;
		BOX_DEBUG_ONLY(uint32_t Index);

	public:
		MoveList(Move* moves);
		MoveList(MovePool* pool, Move* moves);
		MoveList(const MoveList& other) = delete;
		MoveList& operator=(const MoveList& other) = delete;
		MoveList(MoveList&& other);
		MoveList& operator=(MoveList&& other);
		~MoveList();

		inline bool Empty() const { return MoveCount <= 0; }
	};

	class BOX_API MovePool
	{
	private:
		std::unique_ptr<Move[]> m_MovePool;
		size_t m_Capacity;
		size_t m_CurrentIndex;

	public:
		MovePool(size_t size);

		MoveList GetList();
		void FreeList(const MoveList* list);
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

		void GetPseudoLegalMoves(MoveList& moveList);
		void FilterLegalMoves(MoveList& pseudoLegalMoves);
		bool IsLegal(const Move& move) const;

		bool HasAtLeastOneLegalMove(MoveList& list);

		static BitBoard GetReachableKingSquares(const Position& position, Team team);

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
