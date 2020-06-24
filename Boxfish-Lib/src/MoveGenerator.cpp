#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	MoveGenerator::MoveGenerator()
		: m_Position(), m_PseudoLegalValid(false), m_LegalValid(false), m_PseudoLegalMoves(), m_LegalMoves()
	{
		Reset();
	}

	MoveGenerator::MoveGenerator(const Position& position)
		: m_Position(position), m_PseudoLegalValid(false), m_LegalValid(false), m_PseudoLegalMoves(), m_LegalMoves()
	{
		Reset();
	}

	const Position& MoveGenerator::GetPosition() const
	{
		return m_Position;
	}

	void MoveGenerator::SetPosition(const Position& position)
	{
		m_Position = position;
		Reset();
	}

	void MoveGenerator::SetPosition(Position&& position)
	{
		m_Position = std::move(position);
		Reset();
	}

	const std::vector<Move>& MoveGenerator::GetPseudoLegalMoves()
	{
		if (!m_PseudoLegalValid)
		{
			GeneratePseudoLegalMoves();
			m_PseudoLegalValid = true;
		}
		return m_PseudoLegalMoves;
	}

	const std::vector<Move>& MoveGenerator::GetLegalMoves()
	{
		if (!m_LegalValid)
		{			
			GenerateLegalMoves(GetPseudoLegalMoves());
			m_LegalValid = true;
		}
		return m_LegalMoves;
	}

	void MoveGenerator::Reset()
	{
		m_PseudoLegalValid = false;
		m_LegalValid = false;
		m_LegalMoves.clear();
		m_LegalMoves.reserve(MAX_MOVES);
		m_PseudoLegalMoves.clear();
		m_PseudoLegalMoves.reserve(MAX_MOVES);
	}

	void MoveGenerator::GeneratePseudoLegalMoves()
	{
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			GenerateMoves(m_Position.TeamToPlay, piece, m_Position);
		}
	}

	void MoveGenerator::GenerateLegalMoves(const std::vector<Move>& pseudoLegalMoves)
	{
		for (const Move& move : pseudoLegalMoves)
		{
			Position position = m_Position;
			ApplyMove(position, move);
			if (!IsInCheck(position, OtherTeam(position.TeamToPlay)))
			{
				m_LegalMoves.push_back(move);
			}
		}
	}

	void MoveGenerator::GenerateMoves(Team team, Piece pieceType, const Position& position)
	{
		switch (pieceType)
		{
		case PIECE_PAWN:
		{
			GeneratePawnSinglePushes(team, position);
			GeneratePawnDoublePushes(team, position);
			GeneratePawnLeftAttacks(team, position);
			GeneratePawnRightAttacks(team, position);
			break;
		}
		case PIECE_KNIGHT:
			GenerateKnightMoves(team, position);
			break;
		case PIECE_BISHOP:
			GenerateBishopMoves(team, position);
			break;
		case PIECE_ROOK:
			GenerateRookMoves(team, position);
			break;
		case PIECE_QUEEN:
			GenerateQueenMoves(team, position);
			break;
		case PIECE_KING:
			GenerateKingMoves(team, position);
			break;
		}
	}

	void MoveGenerator::GeneratePawnPromotions(SquareIndex fromSquare, SquareIndex toSquare, MoveFlag flags, Piece capturedPiece)
	{
		Move promotion({ fromSquare, toSquare, PIECE_PAWN, flags | MOVE_PROMOTION });
		if (flags & MOVE_CAPTURE)
		{
			promotion.SetCapturedPiece(capturedPiece);
		}
		Move queenPromotion = promotion;
		queenPromotion.SetPromotionPiece(PIECE_QUEEN);
		m_PseudoLegalMoves.push_back(queenPromotion);

		Move knightPromotion = promotion;
		knightPromotion.SetPromotionPiece(PIECE_KNIGHT);
		m_PseudoLegalMoves.push_back(knightPromotion);

		Move rookPromotion = promotion;
		rookPromotion.SetPromotionPiece(PIECE_ROOK);
		m_PseudoLegalMoves.push_back(rookPromotion);

		Move bishopPromotion = promotion;
		bishopPromotion.SetPromotionPiece(PIECE_BISHOP);
		m_PseudoLegalMoves.push_back(bishopPromotion);
	}

	void MoveGenerator::GeneratePawnSinglePushes(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << GetForwardShift(team)) : (pawns >> -GetForwardShift(team))) & position.GetNotOccupied();
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			m_PseudoLegalMoves.push_back(Move({ (SquareIndex)(index - GetForwardShift(team)), index, PIECE_PAWN }));
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions((SquareIndex)(index - GetForwardShift(team)), index, (MoveFlag)0, PIECE_MAX);
		}
	}

	void MoveGenerator::GeneratePawnDoublePushes(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard originalPawns = pawns & ((team == TEAM_WHITE) ? RANK_2_MASK : RANK_7_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (originalPawns << 2 * GetForwardShift(team)) : (originalPawns >>  2 * -GetForwardShift(team))) & position.GetNotOccupied();
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			m_PseudoLegalMoves.push_back(Move({ (SquareIndex)(index - 2 * GetForwardShift(team)), index, PIECE_PAWN, MOVE_DOUBLE_PAWN_PUSH }));
		}
	}

	void MoveGenerator::GeneratePawnLeftAttacks(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard fileMask = (team == TEAM_WHITE) ? (~FILE_H_MASK) : (~FILE_A_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 7) : (pawns >> 7));
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			BitBoard enPassant = movedPawns & (BitBoard{ BitBoard::SquareToBitIndex(position.EnpassantSquare) }) & fileMask;
			if (enPassant)
			{
				SquareIndex index = PopLeastSignificantBit(enPassant);
				m_PseudoLegalMoves.push_back(Move({ (SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, PIECE_PAWN, MOVE_EN_PASSANT }));
			}
		}
		movedPawns &= fileMask & position.GetTeamPieces(OtherTeam(team));
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			Move move({ (SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, PIECE_PAWN, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), index));
			m_PseudoLegalMoves.push_back(move);
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions((SquareIndex)(index - ((team == TEAM_WHITE) ? 7 : -7)), index, MOVE_CAPTURE, GetPieceAtSquare(position, OtherTeam(team), index));
		}
	}

	void MoveGenerator::GeneratePawnRightAttacks(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard fileMask = (team == TEAM_WHITE) ? (~FILE_A_MASK) : (~FILE_H_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 9) : (pawns >> 9));
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			BitBoard enPassant = movedPawns & (BitBoard{ BitBoard::SquareToBitIndex(position.EnpassantSquare) }) & fileMask;
			if (enPassant)
			{
				SquareIndex index = PopLeastSignificantBit(enPassant);
				m_PseudoLegalMoves.push_back(Move({ (SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, PIECE_PAWN, MOVE_EN_PASSANT }));
			}
		}
		movedPawns &= fileMask & position.GetTeamPieces(OtherTeam(team));
		BitBoard promotionMask = ((team == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK);
		BitBoard promotions = movedPawns & promotionMask;
		movedPawns &= ~promotionMask;
		while (movedPawns)
		{
			SquareIndex index = PopLeastSignificantBit(movedPawns);
			Move move({ (SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, PIECE_PAWN, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), index));
			m_PseudoLegalMoves.push_back(move);
		}
		while (promotions)
		{
			SquareIndex index = PopLeastSignificantBit(promotions);
			GeneratePawnPromotions((SquareIndex)(index - ((team == TEAM_WHITE) ? 9 : -9)), index, MOVE_CAPTURE, GetPieceAtSquare(position, OtherTeam(team), index));
		}
	}

	void MoveGenerator::GenerateKnightMoves(Team team, const Position& position)
	{
		BitBoard knights = position.Teams[team].Pieces[PIECE_KNIGHT];
		while (knights)
		{
			SquareIndex index = PopLeastSignificantBit(knights);
			BitBoard moves = GetNonSlidingAttacks(PIECE_KNIGHT, BitBoard::BitIndexToSquare(index), team) & ~position.GetTeamPieces(team);
			AddMoves(position, team, index, PIECE_KNIGHT, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateBishopMoves(Team team, const Position& position)
	{
		BitBoard bishops = position.Teams[team].Pieces[PIECE_BISHOP];
		while (bishops)
		{
			SquareIndex index = PopLeastSignificantBit(bishops);
			BitBoard moves = GetSlidingAttacks(PIECE_BISHOP, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, index, PIECE_BISHOP, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateRookMoves(Team team, const Position& position)
	{
		BitBoard rooks = position.Teams[team].Pieces[PIECE_ROOK];
		while (rooks)
		{
			SquareIndex index = PopLeastSignificantBit(rooks);
			BitBoard moves = GetSlidingAttacks(PIECE_ROOK, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, index, PIECE_ROOK, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateQueenMoves(Team team, const Position& position)
	{
		BitBoard queens = position.Teams[team].Pieces[PIECE_QUEEN];
		while (queens)
		{
			SquareIndex index = PopLeastSignificantBit(queens);
			BitBoard moves = GetSlidingAttacks(PIECE_QUEEN, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, index, PIECE_QUEEN, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateKingMoves(Team team, const Position& position)
	{
		BitBoard kings = position.Teams[team].Pieces[PIECE_KING];
		while (kings)
		{
			SquareIndex index = PopLeastSignificantBit(kings);
			BitBoard moves = GetNonSlidingAttacks(PIECE_KING, BitBoard::BitIndexToSquare(index), team) & ~position.GetTeamPieces(team);
			AddMoves(position, team, index, PIECE_KING, moves, position.GetTeamPieces(OtherTeam(team)));
		}
		BitBoard occupied = position.GetAllPieces();
		if (team == TEAM_WHITE)
		{
			if (position.Teams[TEAM_WHITE].CastleKingSide)
			{
				BitBoard passThrough = BitBoard(f1) | BitBoard(g1);
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), f1) | IsSquareUnderAttack(position, OtherTeam(team), g1) | IsInCheck(position, team);
				if (!squaresOccupied && !underAttack)
				{
					m_PseudoLegalMoves.push_back(Move({ e1, g1, PIECE_KING, MOVE_KINGSIDE_CASTLE }));
				}
			}
			if (position.Teams[TEAM_WHITE].CastleQueenSide)
			{
				BitBoard passThrough = BitBoard(b1) | BitBoard(c1) | BitBoard(d1);
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), b1) | IsSquareUnderAttack(position, OtherTeam(team), c1) | IsSquareUnderAttack(position, OtherTeam(team), d1) | IsInCheck(position, team);
				if (!squaresOccupied && !underAttack)
				{
					m_PseudoLegalMoves.push_back(Move({ e1, c1, PIECE_KING, MOVE_QUEENSIDE_CASTLE }));
				}
			}
		}
		if (team == TEAM_BLACK)
		{
			if (position.Teams[TEAM_BLACK].CastleKingSide)
			{
				BitBoard passThrough = BitBoard(f8) | BitBoard(g8);
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), f8) | IsSquareUnderAttack(position, OtherTeam(team), g8) | IsInCheck(position, team);
				if (!squaresOccupied && !underAttack)
				{
					m_PseudoLegalMoves.push_back(Move({ e8, g8, PIECE_KING, MOVE_KINGSIDE_CASTLE }));
				}
			}
			if (position.Teams[TEAM_BLACK].CastleQueenSide)
			{
				BitBoard passThrough = BitBoard(b8) | BitBoard(c8) | BitBoard(d8);
				bool squaresOccupied = passThrough & occupied;
				bool underAttack = IsSquareUnderAttack(position, OtherTeam(team), b8) | IsSquareUnderAttack(position, OtherTeam(team), c8) | IsSquareUnderAttack(position, OtherTeam(team), d8) | IsInCheck(position, team);
				if (!squaresOccupied && !underAttack)
				{
					m_PseudoLegalMoves.push_back(Move({ e8, c8, PIECE_KING, MOVE_QUEENSIDE_CASTLE }));
				}
			}
		}
	}

	void MoveGenerator::AddMoves(const Position& position, Team team, SquareIndex fromSquare, Piece pieceType, const BitBoard& moves, const BitBoard& attackablePieces)
	{
		BitBoard nonAttacks = moves & ~attackablePieces;
		while (nonAttacks)
		{
			SquareIndex toIndex = PopLeastSignificantBit(nonAttacks);
			m_PseudoLegalMoves.push_back(Move({ fromSquare, toIndex, pieceType }));
		}
		BitBoard attacks = moves & attackablePieces;
		Team otherTeam = OtherTeam(team);
		while (attacks)
		{
			SquareIndex toIndex = PopLeastSignificantBit(attacks);
			Move move({ fromSquare, toIndex, pieceType, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, otherTeam, toIndex));
			m_PseudoLegalMoves.push_back(std::move(move));
		}
	}

}
