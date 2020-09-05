#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{
	MoveList::MoveList(Move* moves) : MoveList(nullptr, moves)
	{
	}

	MoveList::MoveList(MovePool* pool, Move* moves)
		: m_Pool(pool), MoveCount(0), Moves(moves)
	{
	}

	MoveList::MoveList(MoveList&& other)
		: m_Pool(other.m_Pool), MoveCount(other.MoveCount), Moves(other.Moves)
	{
		BOX_DEBUG_ONLY(Index = other.Index);
		other.m_Pool = nullptr;
	}

	MoveList& MoveList::operator=(MoveList&& other)
	{
		std::swap(m_Pool, other.m_Pool);
		std::swap(MoveCount, other.MoveCount);
		std::swap(Moves, other.Moves);
		BOX_DEBUG_ONLY(std::swap(Index, other.Index));
		return *this;
	}

	MoveList::~MoveList()
	{
		if (m_Pool)
			m_Pool->FreeList(this);
	}

	MovePool::MovePool(size_t size)
		: m_MovePool(std::make_unique<Move[]>(size)), m_Capacity(size), m_CurrentIndex(0)
	{
	}

	MoveList MovePool::GetList()
	{
		BOX_ASSERT(m_CurrentIndex + MAX_MOVES <= m_Capacity, "No more move lists");
		MoveList list(this, m_MovePool.get() + m_CurrentIndex);
		BOX_DEBUG_ONLY(list.Index = m_CurrentIndex);
		m_CurrentIndex += MAX_MOVES;
		return list;
	}

	void MovePool::FreeList(const MoveList* list)
	{
		m_CurrentIndex -= MAX_MOVES;
		BOX_ASSERT(list->Index == m_CurrentIndex, "Invalid");
	}

	MoveGenerator::MoveGenerator()
		: m_Position()
	{
	}

	MoveGenerator::MoveGenerator(const Position& position)
		: m_Position(position)
	{
	}

	const Position& MoveGenerator::GetPosition() const
	{
		return m_Position;
	}

	void MoveGenerator::SetPosition(const Position& position)
	{
		m_Position = position;
	}

	void MoveGenerator::SetPosition(Position&& position)
	{
		m_Position = std::move(position);
	}

	void MoveGenerator::GetPseudoLegalMoves(MoveList& moveList)
	{
		GeneratePseudoLegalMoves(moveList);
	}

	void MoveGenerator::FilterLegalMoves(MoveList& pseudoLegalMoves)
	{
		if (pseudoLegalMoves.MoveCount <= 0)
			return;
		const BitBoard& checkers = m_Position.InfoCache.CheckedBy[m_Position.TeamToPlay];
		bool multipleCheckers = MoreThanOne(checkers);

		int index = 0;
		for (int i = 0; i < pseudoLegalMoves.MoveCount; i++)
		{
			if (IsMoveLegal(pseudoLegalMoves.Moves[i], checkers, multipleCheckers))
			{
				if (index != i)
				{
					std::swap(pseudoLegalMoves.Moves[i], pseudoLegalMoves.Moves[index]);
				}
				index++;
			}
		}
		pseudoLegalMoves.MoveCount = index;
	}

	bool MoveGenerator::IsLegal(const Move& move) const
	{
		const BitBoard& checkers = m_Position.InfoCache.CheckedBy[m_Position.TeamToPlay];
		bool multipleCheckers = MoreThanOne(checkers);
		return IsMoveLegal(move, checkers, multipleCheckers);
	}

	bool MoveGenerator::HasAtLeastOneLegalMove(MoveList& moveList)
	{
		moveList.MoveCount = 0;
		Team team = m_Position.TeamToPlay;
		GeneratePawnSinglePushes(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GeneratePawnDoublePushes(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GeneratePawnLeftAttacks(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GeneratePawnRightAttacks(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GenerateKingMoves(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GenerateQueenMoves(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GenerateKnightMoves(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GenerateBishopMoves(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		GenerateRookMoves(moveList, team, m_Position);
		FilterLegalMoves(moveList);
		if (moveList.MoveCount > 0)
			return true;
		return false;
	}

	BitBoard MoveGenerator::GetReachableKingSquares(const Position& position, Team team)
	{
		SquareIndex kingSquare = position.GetKingSquare(team);
		return GetNonSlidingAttacks(PIECE_KING, kingSquare, team) & ~position.GetTeamPieces(team);
	}

	void MoveGenerator::GeneratePseudoLegalMoves(MoveList& moves)
	{
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			GenerateMoves(moves, m_Position.TeamToPlay, piece, m_Position);
		}
	}

	void MoveGenerator::GenerateLegalMoves(MoveList& moveList, const MoveList& pseudoLegalMoves)
	{
		const BitBoard& checkers = m_Position.InfoCache.CheckedBy[m_Position.TeamToPlay];
		bool multipleCheckers = MoreThanOne(checkers);
		for (int i = 0; i < pseudoLegalMoves.MoveCount; i++)
		{
			const Move& move = pseudoLegalMoves.Moves[i];
			if (IsMoveLegal(move, checkers, multipleCheckers))
			{
				moveList.Moves[moveList.MoveCount++] = move;
			}
		}
	}

	bool MoveGenerator::IsMoveLegal(const Move& move, const BitBoard& checkers, bool multipleCheckers) const
	{
		SquareIndex kingSquare = m_Position.GetKingSquare(m_Position.TeamToPlay);
		if (move.GetFlags() & MOVE_EN_PASSANT)
		{
			SquareIndex captureSquare = (SquareIndex)(move.GetToSquareIndex() - GetForwardShift(m_Position.TeamToPlay));
			BitBoard occupied = (m_Position.GetAllPieces() ^ move.GetFromSquareIndex() ^ captureSquare) | move.GetToSquareIndex();
			return !(GetSlidingAttacks(PIECE_ROOK, kingSquare, occupied) & m_Position.GetTeamPieces(OtherTeam(m_Position.TeamToPlay), PIECE_QUEEN, PIECE_ROOK))
				&& !(GetSlidingAttacks(PIECE_BISHOP, kingSquare, occupied) & m_Position.GetTeamPieces(OtherTeam(m_Position.TeamToPlay), PIECE_QUEEN, PIECE_BISHOP));
		}
		if (checkers)
		{
			if (move.GetMovingPiece() != PIECE_KING)
			{
				if (multipleCheckers)
					return false;
				SquareIndex checkingSquare = BackwardBitScan(checkers);
				// Move must block the checker or capture the blocker
				if (!((GetBitBoardBetween(checkingSquare, kingSquare) | checkers) & move.GetToSquareIndex()))
				{
					return false;
				}
			}
			else if (GetAttackers(m_Position, OtherTeam(m_Position.TeamToPlay), move.GetToSquareIndex(), m_Position.GetAllPieces() ^ move.GetFromSquareIndex()))
			{
				return false;
			}
		}
		if (move.GetMovingPiece() == PIECE_KING)
		{
			return !IsSquareUnderAttack(m_Position, OtherTeam(m_Position.TeamToPlay), move.GetToSquareIndex());
		}
		return !(m_Position.InfoCache.BlockersForKing[m_Position.TeamToPlay] & move.GetFromSquareIndex())
					|| IsAligned(move.GetFromSquareIndex(), move.GetToSquareIndex(), kingSquare);
	}

	void MoveGenerator::GenerateMoves(MoveList& moveList, Team team, Piece pieceType, const Position& position)
	{
		switch (pieceType)
		{
		case PIECE_PAWN:
		{
			GeneratePawnSinglePushes(moveList, team, position);
			GeneratePawnDoublePushes(moveList, team, position);
			GeneratePawnLeftAttacks(moveList, team, position);
			GeneratePawnRightAttacks(moveList, team, position);
			break;
		}
		case PIECE_KNIGHT:
			GenerateKnightMoves(moveList, team, position);
			break;
		case PIECE_BISHOP:
			GenerateBishopMoves(moveList, team, position);
			break;
		case PIECE_ROOK:
			GenerateRookMoves(moveList, team, position);
			break;
		case PIECE_QUEEN:
			GenerateQueenMoves(moveList, team, position);
			break;
		case PIECE_KING:
			GenerateKingMoves(moveList, team, position);
			break;
		}
	}

	void MoveGenerator::GeneratePawnPromotions(MoveList& moveList, SquareIndex fromSquare, SquareIndex toSquare, MoveFlag flags, Piece capturedPiece)
	{
		Move promotion(fromSquare, toSquare, PIECE_PAWN, flags | MOVE_PROMOTION);
		if (flags & MOVE_CAPTURE)
		{
			promotion.SetCapturedPiece(capturedPiece);
			BOX_ASSERT(promotion.GetCapturedPiece() != PIECE_KING, "Cannot capture king");
		}
		Move queenPromotion = promotion;
		queenPromotion.SetPromotionPiece(PIECE_QUEEN);
		moveList.Moves[moveList.MoveCount++] = queenPromotion;

		Move knightPromotion = promotion;
		knightPromotion.SetPromotionPiece(PIECE_KNIGHT);
		moveList.Moves[moveList.MoveCount++] = knightPromotion;

		Move rookPromotion = promotion;
		rookPromotion.SetPromotionPiece(PIECE_ROOK);
		moveList.Moves[moveList.MoveCount++] = rookPromotion;

		Move bishopPromotion = promotion;
		bishopPromotion.SetPromotionPiece(PIECE_BISHOP);
		moveList.Moves[moveList.MoveCount++] = bishopPromotion;
	}

	void MoveGenerator::GeneratePawnSinglePushes(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << GetForwardShift(team)) : (pawns >> -GetForwardShift(team))) & position.GetNotOccupied();
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			moveList.Moves[moveList.MoveCount++] = Move((SquareIndex)(index - GetForwardShift(team)), index, PIECE_PAWN, MOVE_NORMAL);
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions(moveList, (SquareIndex)(index - GetForwardShift(team)), index, (MoveFlag)0, PIECE_MAX);
		}
	}

	void MoveGenerator::GeneratePawnDoublePushes(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		BitBoard originalPawns = pawns & ((team == TEAM_WHITE) ? RANK_2_MASK : RANK_7_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (originalPawns << GetForwardShift(team)) : (originalPawns >> -GetForwardShift(team))) & position.GetNotOccupied();
		movedPawns = ((team == TEAM_WHITE) ? (movedPawns << GetForwardShift(team)) : (movedPawns >> -GetForwardShift(team))) & position.GetNotOccupied();
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			moveList.Moves[moveList.MoveCount++] = Move((SquareIndex)(index - 2 * GetForwardShift(team)), index, PIECE_PAWN, MOVE_DOUBLE_PAWN_PUSH);
		}
	}

	void MoveGenerator::GeneratePawnLeftAttacks(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		BitBoard fileMask = (team == TEAM_WHITE) ? (~FILE_H_MASK) : (~FILE_A_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 7) : (pawns >> 7)) & fileMask;
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			BitBoard enPassant = movedPawns & BitBoard::SquareToBitIndex(position.EnpassantSquare);
			if (enPassant)
			{
				SquareIndex index = PopLeastSignificantBit(enPassant);
				moveList.Moves[moveList.MoveCount++] = Move((SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, PIECE_PAWN, MOVE_EN_PASSANT);
			}
		}
		movedPawns &= position.GetTeamPieces(OtherTeam(team));
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			Move move({ (SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, PIECE_PAWN, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), index));
			BOX_ASSERT(move.GetCapturedPiece() != PIECE_KING, "Cannot capture king");
			moveList.Moves[moveList.MoveCount++] = move;
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions(moveList, (SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, MOVE_CAPTURE, GetPieceAtSquare(position, OtherTeam(team), index));
		}
	}

	void MoveGenerator::GeneratePawnRightAttacks(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard pawns = position.GetTeamPieces(team, PIECE_PAWN);
		BitBoard fileMask = (team == TEAM_WHITE) ? (~FILE_A_MASK) : (~FILE_H_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 9) : (pawns >> 9)) & fileMask;
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			BitBoard enPassant = movedPawns & BitBoard::SquareToBitIndex(position.EnpassantSquare);
			if (enPassant)
			{
				SquareIndex index = PopLeastSignificantBit(enPassant);
				moveList.Moves[moveList.MoveCount++] = Move((SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, PIECE_PAWN, MOVE_EN_PASSANT);
			}
		}
		movedPawns &= position.GetTeamPieces(OtherTeam(team));
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			Move move((SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, PIECE_PAWN, MOVE_CAPTURE);
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), index));
			BOX_ASSERT(move.GetCapturedPiece() != PIECE_KING, "Cannot capture king");
			moveList.Moves[moveList.MoveCount++] = move;
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions(moveList, (SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, MOVE_CAPTURE, GetPieceAtSquare(position, OtherTeam(team), index));
		}
	}

	void MoveGenerator::GenerateKnightMoves(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard knights = position.GetTeamPieces(team, PIECE_KNIGHT);
		while (knights)
		{
			SquareIndex index = PopLeastSignificantBit(knights);
			BitBoard moves = GetNonSlidingAttacks(PIECE_KNIGHT, index, team) & ~position.GetTeamPieces(team);
			AddMoves(moveList, position, team, index, PIECE_KNIGHT, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateBishopMoves(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard bishops = position.GetTeamPieces(team, PIECE_BISHOP);
		while (bishops)
		{
			SquareIndex index = PopLeastSignificantBit(bishops);
			BitBoard moves = GetSlidingAttacks(PIECE_BISHOP, index, position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(moveList, position, team, index, PIECE_BISHOP, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateRookMoves(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard rooks = position.GetTeamPieces(team, PIECE_ROOK);
		while (rooks)
		{
			SquareIndex index = PopLeastSignificantBit(rooks);
			BitBoard moves = GetSlidingAttacks(PIECE_ROOK, index, position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(moveList, position, team, index, PIECE_ROOK, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateQueenMoves(MoveList& moveList, Team team, const Position& position)
	{
		BitBoard queens = position.GetTeamPieces(team, PIECE_QUEEN);
		while (queens)
		{
			SquareIndex index = PopLeastSignificantBit(queens);
			BitBoard moves = GetSlidingAttacks(PIECE_QUEEN, index, position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(moveList, position, team, index, PIECE_QUEEN, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateKingMoves(MoveList& moveList, Team team, const Position& position)
	{
		SquareIndex square = position.GetKingSquare(team);
		BitBoard moves = GetNonSlidingAttacks(PIECE_KING, square, team) & ~position.GetTeamPieces(team);
		AddMoves(moveList, position, team, square, PIECE_KING, moves, position.GetTeamPieces(OtherTeam(team)));
		BitBoard occupied = position.GetAllPieces();
		const bool inCheck = IsInCheck(position, team);
		if (team == TEAM_WHITE && !inCheck)
		{
			if (position.Teams[TEAM_WHITE].CastleKingSide)
			{
				constexpr BitBoard passThrough = f1 | g1;
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), f1) || IsSquareUnderAttack(position, OtherTeam(team), g1);
				if (!squaresOccupied && !underAttack)
				{
					moveList.Moves[moveList.MoveCount++] = Move(e1, g1, PIECE_KING, MOVE_KINGSIDE_CASTLE);
				}
			}
			if (position.Teams[TEAM_WHITE].CastleQueenSide)
			{
				constexpr BitBoard passThrough = b1 | c1 | d1;
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), c1) || IsSquareUnderAttack(position, OtherTeam(team), d1);
				if (!squaresOccupied && !underAttack)
				{
					moveList.Moves[moveList.MoveCount++] = Move(e1, c1, PIECE_KING, MOVE_QUEENSIDE_CASTLE);
				}
			}
		}
		else if (team == TEAM_BLACK && !inCheck)
		{
			if (position.Teams[TEAM_BLACK].CastleKingSide)
			{
				constexpr BitBoard passThrough = f8 | g8;
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), f8) || IsSquareUnderAttack(position, OtherTeam(team), g8);
				if (!squaresOccupied && !underAttack)
				{
					moveList.Moves[moveList.MoveCount++] = Move(e8, g8, PIECE_KING, MOVE_KINGSIDE_CASTLE);
				}
			}
			if (position.Teams[TEAM_BLACK].CastleQueenSide)
			{
				constexpr BitBoard passThrough = b8 | c8 | d8;
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), c8) || IsSquareUnderAttack(position, OtherTeam(team), d8);
				if (!squaresOccupied && !underAttack)
				{
					moveList.Moves[moveList.MoveCount++] = Move(e8, c8, PIECE_KING, MOVE_QUEENSIDE_CASTLE);
				}
			}
		}
	}

	void MoveGenerator::AddMoves(MoveList& moveList, const Position& position, Team team, SquareIndex fromSquare, Piece pieceType, const BitBoard& moves, const BitBoard& attackablePieces)
	{
		BitBoard nonAttacks = moves & ~attackablePieces;
		while (nonAttacks)
		{
			SquareIndex toIndex = PopLeastSignificantBit(nonAttacks);
			moveList.Moves[moveList.MoveCount++] = Move(fromSquare, toIndex, pieceType, MOVE_NORMAL);
		}
		BitBoard attacks = moves & attackablePieces;
		Team otherTeam = OtherTeam(team);
		while (attacks)
		{
			SquareIndex toIndex = PopLeastSignificantBit(attacks);
			Move move(fromSquare, toIndex, pieceType, MOVE_CAPTURE);
			move.SetCapturedPiece(GetPieceAtSquare(position, otherTeam, toIndex));
			BOX_ASSERT(move.GetCapturedPiece() != PIECE_KING, "Cannot capture king");
			moveList.Moves[moveList.MoveCount++] = move;
		}
	}

}
