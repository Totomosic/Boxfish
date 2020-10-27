#pragma once
#include "Position.h"

namespace Boxfish
{

	enum MoveFlag : uint8_t
	{
		MOVE_NORMAL				= 0,
		MOVE_NULL				= 1 << 0,
		MOVE_CAPTURE			= 1 << 1,
		MOVE_DOUBLE_PAWN_PUSH	= 1 << 2,
		MOVE_KINGSIDE_CASTLE	= 1 << 3,
		MOVE_QUEENSIDE_CASTLE	= 1 << 4,
		MOVE_EN_PASSANT			= 1 << 5,
		MOVE_PROMOTION			= 1 << 6,
	};

	inline MoveFlag operator|(MoveFlag left, MoveFlag right)
	{
		return (MoveFlag)((int8_t)left | (int8_t)right);
	}

	class BOX_API Move
	{
	private:
		uint32_t m_Move;
		int m_Value;

	public:
		// Don't initialize
		inline Move() {}

		constexpr Move(SquareIndex from, SquareIndex to, Piece piece, MoveFlag flags)
			: m_Move(((flags & 0x7f) << 21) | ((to & 0x3f) << 15) | ((from & 0x3f) << 9) | (piece & 0x7)), m_Value(0)
		{
		}

		constexpr Move(uint32_t move)
			: m_Move(move), m_Value(0)
		{
		}

		inline int GetValue() const { return m_Value; }
		inline void SetValue(int value) { m_Value = value; }

		inline Square GetFromSquare() const { return BitBoard::BitIndexToSquare(GetFromSquareIndex()); }
		inline Square GetToSquare() const { return BitBoard::BitIndexToSquare(GetToSquareIndex()); }
		inline SquareIndex GetFromSquareIndex() const { return (SquareIndex)((m_Move >> 9) & 0x3F); }
		inline SquareIndex GetToSquareIndex() const { return (SquareIndex)((m_Move >> 15) & 0x3F); }
		inline Piece GetMovingPiece() const { return (Piece)(m_Move & 0x7); }
		inline MoveFlag GetFlags() const { return (MoveFlag)((m_Move >> 21) & 0x7F); }
		inline uint64_t GetKey() const { return (uint64_t)m_Move * 89812459128486124ULL + 9182461895918651269ULL; }

		inline Piece GetCapturedPiece() const { return (Piece)((m_Move >> 6) & 0x7); }

		inline void SetCapturedPiece(Piece piece)
		{
			constexpr uint32_t mask = 0x7 << 6;
			m_Move = (m_Move & ~mask) | ((piece << 6) & mask);
		}

		inline Piece GetPromotionPiece() const { return (Piece)((m_Move >> 3) & 0x7); }

		inline void SetPromotionPiece(Piece piece)
		{
			constexpr uint32_t mask = 0x7 << 3;
			m_Move = (m_Move & ~mask) | ((piece << 3) & mask);
		}

		inline void SetFlags(MoveFlag flag)
		{
			constexpr uint32_t mask = 0x7F << 21;
			m_Move = (m_Move & ~mask) | (flag << 21);
		}

		inline bool IsCapture() const { return GetFlags() & (MOVE_CAPTURE | MOVE_EN_PASSANT); }
		inline bool IsPromotion() const { return GetFlags() & MOVE_PROMOTION; }
		inline bool IsCaptureOrPromotion() const { return GetFlags() & (MOVE_CAPTURE | MOVE_PROMOTION | MOVE_EN_PASSANT); }
		inline bool IsCastle() const { return GetFlags() & (MOVE_KINGSIDE_CASTLE | MOVE_QUEENSIDE_CASTLE); }
		inline bool IsAdvancedPawnPush(Team team) const { return GetMovingPiece() == PIECE_PAWN && RelativeRank(team, BitBoard::RankOfIndex(GetFromSquareIndex())) > RANK_4; }
		inline bool IsEnpassant() const { return GetFlags() & MOVE_EN_PASSANT; }

		inline friend bool operator==(const Move& left, const Move& right) { return left.m_Move == right.m_Move; }
		inline friend bool operator!=(const Move& left, const Move& right) { return left.m_Move != right.m_Move; }
	};

	inline bool operator<(const Move& left, const Move& right)
	{
		return left.GetValue() < right.GetValue();
	}

	constexpr Move MOVE_NONE = Move(a1, a1, PIECE_PAWN, MOVE_NULL);

}
