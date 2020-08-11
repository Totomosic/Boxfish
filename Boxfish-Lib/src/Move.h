#pragma once
#include "Position.h"

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

	inline MoveFlag operator|(MoveFlag left, MoveFlag right)
	{
		return (MoveFlag)((int)left | (int)right);
	}

	struct BOX_API MoveDefinition
	{
	public:
		SquareIndex FromSquare;
		SquareIndex ToSquare;
		Piece MovingPiece;
		MoveFlag Flags = (MoveFlag)0;
	};

	class BOX_API Move
	{
	private:
		uint32_t m_Move;
		int m_Value;

	public:
		Move();
		Move(const MoveDefinition& definition);
		constexpr Move(uint32_t move)
			: m_Move(move), m_Value(0)
		{
		}

		inline int GetValue() const { return m_Value; }
		inline void SetValue(int value) { m_Value = value; }

		MoveDefinition GetDefinition() const;
		Square GetFromSquare() const;
		Square GetToSquare() const;
		SquareIndex GetFromSquareIndex() const;
		SquareIndex GetToSquareIndex() const;
		Piece GetMovingPiece() const;

		MoveFlag GetFlags() const;
		void SetFlags(MoveFlag flags);

		Piece GetCapturedPiece() const;
		void SetCapturedPiece(Piece piece);
		Piece GetPromotionPiece() const;
		void SetPromotionPiece(Piece piece);

		inline bool IsCapture() const { return GetFlags() & MOVE_CAPTURE; }
		inline bool IsPromotion() const { return GetFlags() & MOVE_PROMOTION; }
		inline bool IsCaptureOrPromotion() const { return GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION); }
		inline bool IsCastle() const { return GetFlags() & (MOVE_KINGSIDE_CASTLE | MOVE_QUEENSIDE_CASTLE); }
		inline bool IsAdvancedPawnPush(Team team) const { return GetMovingPiece() == PIECE_PAWN && RelativeRank(team, GetFromSquare().Rank) > RANK_4; }

		inline friend bool operator==(const Move& left, const Move& right) { return left.m_Move == right.m_Move; }
		inline friend bool operator!=(const Move& left, const Move& right) { return left.m_Move != right.m_Move; }
	};

	constexpr Move MOVE_NONE = 0U | ((MOVE_NULL & 0x7f) << 21);

}
