#pragma once
#include "Position.h"
#include "PositionUtils.h"

namespace Boxfish
{

	enum MoveFlag : int
	{
		MOVE_NULL = 1 << 0,
		MOVE_CAPTURE = 1 << 1,
		MOVE_DOUBLE_PAWN_PUSH = 1 << 2,
		MOVE_KINGSIDE_CASTLE = 1 << 3,
		MOVE_QUEENSIDE_CASTLE = 1 << 4,
		MOVE_EN_PASSANT = 1 << 5,
		MOVE_PROMOTION = 1 << 6
	};

	struct BOX_API MoveDefinition
	{
	public:
		Square FromSquare;
		Square ToSquare;
		Piece MovingPiece;
		MoveFlag Flags = (MoveFlag)0;
	};

	class BOX_API Move
	{
	private:
		uint32_t m_Move;

	public:
		Move();
		Move(const MoveDefinition& definition);

		MoveDefinition GetDefinition() const;
		Square GetFromSquare() const;
		Square GetToSquare() const;
		Piece GetMovingPiece() const;

		MoveFlag GetFlags() const;
		void SetFlags(MoveFlag flags);

		Piece GetCapturedPiece() const;
		void SetCapturedPiece(Piece piece);
		Piece GetPromotionPiece() const;
		void SetPromotionPiece(Piece piece);
	};

}
