#include "MoveGenerator.h"

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

		}
		return m_PseudoLegalMoves;
	}

	const std::vector<Move>& MoveGenerator::GetLegalMoves()
	{
		if (!m_IsValid)
		{
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
	}

	void MoveGenerator::GeneratePawnDoublePushes(Team team, const Position& position)
	{
	}

	void MoveGenerator::GeneratePawnLeftAttacks(Team team, const Position& position)
	{
	}

	void MoveGenerator::GeneratePawnRightAttacks(Team team, const Position& position)
	{
	}

	void MoveGenerator::GenerateKnightMoves(Team team, const Position& position)
	{
	}

	void MoveGenerator::GenerateBishopMoves(Team team, const Position& position)
	{
	}

	void MoveGenerator::GenerateRookMoves(Team team, const Position& position)
	{
	}

	void MoveGenerator::GenerateQueenMoves(Team team, const Position& position)
	{
	}

	void MoveGenerator::GenerateKingMoves(Team team, const Position& position)
	{
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
