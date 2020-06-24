#include "MoveGenerator.h"
#include "Attacks.h"

namespace Boxfish
{

	MoveGenerator::MoveGenerator()
		: m_Position(), m_IsValid(false), m_PseudoLegalMoves(), m_LegalMoves()
	{
		Reset();
	}

	MoveGenerator::MoveGenerator(const Position& position)
		: m_Position(position), m_IsValid(false), m_PseudoLegalMoves(), m_LegalMoves()
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
		if (!m_IsValid)
		{
			GeneratePseudoLegalMoves();
		}
		return m_PseudoLegalMoves;
	}

	const std::vector<Move>& MoveGenerator::GetLegalMoves()
	{
		if (!m_IsValid)
		{
			GenerateLegalMoves();
		}
		return m_LegalMoves;
	}

	void MoveGenerator::Reset()
	{
		m_IsValid = false;
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
		m_IsValid = true;
	}

	void MoveGenerator::GenerateLegalMoves()
	{
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
			GeneratePawnPromotions(team, position);
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

	void MoveGenerator::GeneratePawnPromotions(Team team, const Position& position)
	{
	}

	void MoveGenerator::GeneratePawnSinglePushes(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << GetForwardShift(team)) : (pawns >> -GetForwardShift(team))) & position.GetNotOccupied();
		while (movedPawns)
		{
			int index = PopLeastSignificantBit(movedPawns);
			m_PseudoLegalMoves.push_back(Move({ BitBoard::BitIndexToSquare(index - GetForwardShift(team)), BitBoard::BitIndexToSquare(index), PIECE_PAWN }));
		}
		// TODO: Promotions
	}

	void MoveGenerator::GeneratePawnDoublePushes(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard originalPawns = pawns & ((team == TEAM_WHITE) ? RANK_2_MASK : RANK_7_MASK);
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (originalPawns << 2 * GetForwardShift(team)) : (originalPawns >>  2 * -GetForwardShift(team))) & position.GetNotOccupied();
		while (movedPawns)
		{
			int index = PopLeastSignificantBit(movedPawns);
			m_PseudoLegalMoves.push_back(Move({ BitBoard::BitIndexToSquare(index - 2 * GetForwardShift(team)), BitBoard::BitIndexToSquare(index), PIECE_PAWN, MOVE_DOUBLE_PAWN_PUSH }));
		}
	}

	void MoveGenerator::GeneratePawnLeftAttacks(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 9) : (pawns >> 7)) & position.GetTeamPieces(OtherTeam(team));
		while (movedPawns)
		{
			int index = PopLeastSignificantBit(movedPawns);
			Move move({ BitBoard::BitIndexToSquare(index - ((team == TEAM_WHITE) ? 9 : -7)), BitBoard::BitIndexToSquare(index), PIECE_PAWN, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), BitBoard::BitIndexToSquare(index)));
			m_PseudoLegalMoves.push_back(move);
		}
		// TODO: Promotions
	}

	void MoveGenerator::GeneratePawnRightAttacks(Team team, const Position& position)
	{
		BitBoard pawns = position.Teams[team].Pieces[PIECE_PAWN];
		BitBoard movedPawns = ((team == TEAM_WHITE) ? (pawns << 7) : (pawns >> 9)) & position.GetTeamPieces(OtherTeam(team));
		while (movedPawns)
		{
			int index = PopLeastSignificantBit(movedPawns);
			Move move({ BitBoard::BitIndexToSquare(index - ((team == TEAM_WHITE) ? 7 : -9)), BitBoard::BitIndexToSquare(index), PIECE_PAWN, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, OtherTeam(team), BitBoard::BitIndexToSquare(index)));
			m_PseudoLegalMoves.push_back(move);
		}
		// TODO: Promotions
	}

	void MoveGenerator::GenerateKnightMoves(Team team, const Position& position)
	{
		BitBoard knights = position.Teams[team].Pieces[PIECE_KNIGHT];
		while (knights)
		{
			int index = PopLeastSignificantBit(knights);
			BitBoard moves = GetNonSlidingAttacks(PIECE_KNIGHT, BitBoard::BitIndexToSquare(index), team) & ~position.GetTeamPieces(team);
			AddMoves(position, team, BitBoard::BitIndexToSquare(index), PIECE_KNIGHT, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateBishopMoves(Team team, const Position& position)
	{
		BitBoard bishops = position.Teams[team].Pieces[PIECE_BISHOP];
		while (bishops)
		{
			int index = PopLeastSignificantBit(bishops);
			BitBoard moves = GetSlidingAttacks(PIECE_BISHOP, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, BitBoard::BitIndexToSquare(index), PIECE_BISHOP, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateRookMoves(Team team, const Position& position)
	{
		BitBoard rooks = position.Teams[team].Pieces[PIECE_ROOK];
		while (rooks)
		{
			int index = PopLeastSignificantBit(rooks);
			BitBoard moves = GetSlidingAttacks(PIECE_ROOK, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, BitBoard::BitIndexToSquare(index), PIECE_ROOK, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateQueenMoves(Team team, const Position& position)
	{
		BitBoard queens = position.Teams[team].Pieces[PIECE_QUEEN];
		while (queens)
		{
			int index = PopLeastSignificantBit(queens);
			BitBoard moves = GetSlidingAttacks(PIECE_QUEEN, BitBoard::BitIndexToSquare(index), position.GetAllPieces()) & ~position.GetTeamPieces(team);
			AddMoves(position, team, BitBoard::BitIndexToSquare(index), PIECE_QUEEN, moves, position.GetTeamPieces(OtherTeam(team)));
		}
	}

	void MoveGenerator::GenerateKingMoves(Team team, const Position& position)
	{
		BitBoard kings = position.Teams[team].Pieces[PIECE_KING];
		while (kings)
		{
			int index = PopLeastSignificantBit(kings);
			BitBoard moves = GetNonSlidingAttacks(PIECE_KING, BitBoard::BitIndexToSquare(index), team) & ~position.GetTeamPieces(team);
			AddMoves(position, team, BitBoard::BitIndexToSquare(index), PIECE_KING, moves, position.GetTeamPieces(OtherTeam(team)));
		}
		// TODO: Castling
	}

	void MoveGenerator::AddMoves(const Position& position, Team team, const Square& fromSquare, Piece pieceType, const BitBoard& moves, const BitBoard& attackablePieces)
	{
		BitBoard nonAttacks = moves & ~attackablePieces;
		while (nonAttacks)
		{
			int toIndex = PopLeastSignificantBit(nonAttacks);
			m_PseudoLegalMoves.push_back(Move({ fromSquare, BitBoard::BitIndexToSquare(toIndex), pieceType }));
		}
		BitBoard attacks = moves & attackablePieces;
		Team otherTeam = OtherTeam(team);
		while (attacks)
		{
			int toIndex = PopLeastSignificantBit(attacks);
			Square toSquare = BitBoard::BitIndexToSquare(toIndex);
			Move move({ fromSquare, toSquare, pieceType, MOVE_CAPTURE });
			move.SetCapturedPiece(GetPieceAtSquare(position, otherTeam, toSquare));
			m_PseudoLegalMoves.push_back(std::move(move));
		}
	}

}
