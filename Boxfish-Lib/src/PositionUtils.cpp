#include "PositionUtils.h"
#include "Attacks.h"
#include "Format.h"

namespace Boxfish
{

	void InitialisePosition(Position& position)
	{
		position.InvalidateTeam(TEAM_WHITE);
		position.InvalidateTeam(TEAM_BLACK);
		position.InvalidateAll();

		position.InfoCache.NonPawnMaterial[TEAM_WHITE] = 0;
		position.InfoCache.NonPawnMaterial[TEAM_BLACK] = 0;

		for (SquareIndex square = a1; square < SQUARE_MAX; square++)
		{
			position.InfoCache.PieceOnSquare[square] = PIECE_INVALID;
		}

		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			position.InfoCache.PiecesByType[piece] = position.GetTeamPieces(TEAM_WHITE, piece) | position.GetTeamPieces(TEAM_BLACK, piece);
			if (piece != PIECE_PAWN && piece != PIECE_KING)
			{
				position.InfoCache.NonPawnMaterial[TEAM_WHITE] += position.GetTeamPieces(TEAM_WHITE, piece).GetCount() * GetPieceValue(piece, MIDGAME);
				position.InfoCache.NonPawnMaterial[TEAM_BLACK] += position.GetTeamPieces(TEAM_BLACK, piece).GetCount() * GetPieceValue(piece, MIDGAME);
			}
			BitBoard pieces = position.GetPieces(piece);
			while (pieces)
			{
				SquareIndex square = PopLeastSignificantBit(pieces);
				position.InfoCache.PieceOnSquare[square] = piece;
			}
		}

		position.InfoCache.KingSquare[TEAM_WHITE] = BackwardBitScan(position.GetTeamPieces(TEAM_WHITE, PIECE_KING));
		position.InfoCache.KingSquare[TEAM_BLACK] = BackwardBitScan(position.GetTeamPieces(TEAM_BLACK, PIECE_KING));
		CalculateKingBlockers(position, TEAM_WHITE);
		CalculateKingBlockers(position, TEAM_BLACK);
		CalculateCheckers(position, TEAM_WHITE);
		CalculateCheckers(position, TEAM_BLACK);
		position.Hash.SetFromPosition(position);
	}

	static char PieceToFEN(Piece piece, bool isWhite)
	{
		switch (piece)
		{
		case PIECE_PAWN:
			return (isWhite) ? 'P' : 'p';
		case PIECE_KNIGHT:
			return (isWhite) ? 'N' : 'n';
		case PIECE_BISHOP:
			return (isWhite) ? 'B' : 'b';
		case PIECE_ROOK:
			return (isWhite) ? 'R' : 'r';
		case PIECE_QUEEN:
			return (isWhite) ? 'Q' : 'q';
		case PIECE_KING:
			return (isWhite) ? 'K' : 'k';
		}
		BOX_ASSERT(false, "Invalid piece");
		return ' ';
	}

	Position CreateStartingPosition()
	{
		Position result;
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_A, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_B, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_C, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_D, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_E, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_F, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_G, RANK_2 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ FILE_H, RANK_2 });

		result.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ FILE_A, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ FILE_B, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ FILE_C, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_QUEEN].SetAt({ FILE_D, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KING].SetAt({ FILE_E, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ FILE_F, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ FILE_G, RANK_1 });
		result.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ FILE_H, RANK_1 });

		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_A, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_B, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_C, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_D, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_E, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_F, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_G, RANK_7 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ FILE_H, RANK_7 });

		result.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ FILE_A, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ FILE_B, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ FILE_C, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_QUEEN].SetAt({ FILE_D, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KING].SetAt({ FILE_E, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ FILE_F, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ FILE_G, RANK_8 });
		result.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ FILE_H, RANK_8 });

		InitialisePosition(result);
		return result;
	}

	Position CreatePositionFromFEN(const std::string& fen)
	{
		Position position;
		File currentFile = FILE_A;
		Rank currentRank = RANK_8;
		int index = 0;
		for (char c : fen)
		{
			index++;
			if (c == ' ')
				break;
			if (std::isdigit(c))
			{
				int count = c - '0';
				currentFile = (File)((int)currentFile + count);
			}
			if (c == '/')
			{
				currentRank--;
				currentFile = FILE_A;
			}
			switch (c)
			{
			case 'P':
				position.Teams[TEAM_WHITE].Pieces[PIECE_PAWN].SetAt({ currentFile++, currentRank });
				break;
			case 'N':
				position.Teams[TEAM_WHITE].Pieces[PIECE_KNIGHT].SetAt({ currentFile++, currentRank });
				break;
			case 'B':
				position.Teams[TEAM_WHITE].Pieces[PIECE_BISHOP].SetAt({ currentFile++, currentRank });
				break;
			case 'R':
				position.Teams[TEAM_WHITE].Pieces[PIECE_ROOK].SetAt({ currentFile++, currentRank });
				break;
			case 'Q':
				position.Teams[TEAM_WHITE].Pieces[PIECE_QUEEN].SetAt({ currentFile++, currentRank });
				break;
			case 'K':
				position.Teams[TEAM_WHITE].Pieces[PIECE_KING].SetAt({ currentFile++, currentRank });
				break;
			case 'p':
				position.Teams[TEAM_BLACK].Pieces[PIECE_PAWN].SetAt({ currentFile++, currentRank });
				break;
			case 'n':
				position.Teams[TEAM_BLACK].Pieces[PIECE_KNIGHT].SetAt({ currentFile++, currentRank });
				break;
			case 'b':
				position.Teams[TEAM_BLACK].Pieces[PIECE_BISHOP].SetAt({ currentFile++, currentRank });
				break;
			case 'r':
				position.Teams[TEAM_BLACK].Pieces[PIECE_ROOK].SetAt({ currentFile++, currentRank });
				break;
			case 'q':
				position.Teams[TEAM_BLACK].Pieces[PIECE_QUEEN].SetAt({ currentFile++, currentRank });
				break;
			case 'k':
				position.Teams[TEAM_BLACK].Pieces[PIECE_KING].SetAt({ currentFile++, currentRank });
				break;
			}
		}
		position.TeamToPlay = (fen[index] == 'w') ? TEAM_WHITE : TEAM_BLACK;
		index += 2;

		position.Teams[TEAM_WHITE].CastleKingSide = false;
		position.Teams[TEAM_WHITE].CastleQueenSide = false;
		position.Teams[TEAM_BLACK].CastleKingSide = false;
		position.Teams[TEAM_BLACK].CastleQueenSide = false;

		while (fen[index] != ' ')
		{
			switch (fen[index])
			{
			case 'K':
				position.Teams[TEAM_WHITE].CastleKingSide = true;
				break;
			case 'Q':
				position.Teams[TEAM_WHITE].CastleQueenSide = true;
				break;
			case 'k':
				position.Teams[TEAM_BLACK].CastleKingSide = true;
				break;
			case 'q':
				position.Teams[TEAM_BLACK].CastleQueenSide = true;
				break;
			}
			index++;
		}
		index++;

		if (fen[index] != '-')
		{
			position.EnpassantSquare = SquareFromAlgebraic(fen.substr(index, 2));
			index += 3;
		}
		else
		{
			index += 2;
		}

		size_t space = fen.find_first_of(' ', index);
		if (space != std::string::npos)
		{
			position.HalfTurnsSinceCaptureOrPush = std::stoi(fen.substr(index, space - index));
			index = space + 1;
			space = fen.find_first_of(' ', index);
			position.TotalTurns = std::stoi(fen.substr(index)) - 1;
		}
		else
		{
			position.HalfTurnsSinceCaptureOrPush = 0;
			position.TotalTurns = 0;
		}
		
		InitialisePosition(position);
		return position;
	}

	std::string GetFENFromPosition(const Position& position)
	{
		std::string result = "";
		BitBoard overall;

		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			int emptyCount = 0;
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFEN = 0;
				for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
				{
					if (position.Teams[TEAM_WHITE].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFEN = PieceToFEN(piece, true);
						break;
					}
					if (position.Teams[TEAM_BLACK].Pieces[piece].GetAt({ file, rank }))
					{
						pieceFEN = PieceToFEN(piece, false);
						break;
					}
				}
				if (pieceFEN != 0)
				{
					if (emptyCount > 0)
					{
						result += (char)(emptyCount + '0');
						emptyCount = 0;
					}
					result += pieceFEN;
				}
				else
				{
					emptyCount++;
				}
			}
			if (emptyCount > 0)
			{
				result += (char)(emptyCount + '0');
				emptyCount = 0;
			}
			if (rank != RANK_1)
				result += '/';
		}

		result += (position.TeamToPlay == TEAM_WHITE) ? " w" : " b";
		result += ' ';
		if (position.Teams[TEAM_WHITE].CastleKingSide || position.Teams[TEAM_WHITE].CastleQueenSide || position.Teams[TEAM_BLACK].CastleKingSide || position.Teams[TEAM_BLACK].CastleQueenSide)
		{
			if (position.Teams[TEAM_WHITE].CastleKingSide)
				result += 'K';
			if (position.Teams[TEAM_WHITE].CastleQueenSide)
				result += 'Q';
			if (position.Teams[TEAM_BLACK].CastleKingSide)
				result += 'k';
			if (position.Teams[TEAM_BLACK].CastleQueenSide)
				result += 'q';
		}
		else
		{
			result += '-';
		}

		if (position.EnpassantSquare == INVALID_SQUARE)
			result += " -";
		else
			result += " " + SquareToAlgebraic(position.EnpassantSquare);

		result += " " + std::to_string(position.HalfTurnsSinceCaptureOrPush);
		result += " " + std::to_string(position.TotalTurns + 1);

		return result;
	}

	Position MirrorPosition(const Position& position)
	{
		Position result;
		if (position.EnpassantSquare != INVALID_SQUARE)
			result.EnpassantSquare = { position.EnpassantSquare.File, (Rank)(RANK_MAX - position.EnpassantSquare.Rank - 1) };
		else
			result.EnpassantSquare = INVALID_SQUARE;
		result.TeamToPlay = OtherTeam(position.TeamToPlay);
		result.HalfTurnsSinceCaptureOrPush = position.HalfTurnsSinceCaptureOrPush;
		result.TotalTurns = position.TotalTurns;
		
		result.Teams[TEAM_WHITE] = position.Teams[TEAM_BLACK];
		result.Teams[TEAM_BLACK] = position.Teams[TEAM_WHITE];
		for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
		{
			result.Teams[TEAM_WHITE].Pieces[piece] = FlipVertically(result.Teams[TEAM_WHITE].Pieces[piece]);
			result.Teams[TEAM_BLACK].Pieces[piece] = FlipVertically(result.Teams[TEAM_BLACK].Pieces[piece]);
		}

		InitialisePosition(result);
		return result;
	}

	BitBoard GetAttackers(const Position& position, Team team, const Square& square, const BitBoard& blockers)
	{
		return GetAttackers(position, team, BitBoard::SquareToBitIndex(square), blockers);
	}

	BitBoard GetAttackers(const Position& position, Team team, SquareIndex square, const BitBoard& blockers)
	{
		return (GetNonSlidingAttacks<PIECE_PAWN>(square, OtherTeam(team)) & position.GetTeamPieces(team, PIECE_PAWN)) |
			(GetNonSlidingAttacks<PIECE_KNIGHT>(square) & position.GetTeamPieces(team, PIECE_KNIGHT)) |
			(GetNonSlidingAttacks<PIECE_KING>(square) & position.GetTeamPieces(team, PIECE_KING)) |
			(GetSlidingAttacks<PIECE_BISHOP>(square, blockers) & position.GetTeamPieces(team, PIECE_BISHOP, PIECE_QUEEN)) |
			(GetSlidingAttacks<PIECE_ROOK>(square, blockers) & position.GetTeamPieces(team, PIECE_ROOK, PIECE_QUEEN));
	}

	BitBoard GetSliderBlockers(const Position& position, const BitBoard& sliders, SquareIndex square, BitBoard* pinners)
	{
		BitBoard blockers = ZERO_BB;
		if (pinners)
			*pinners = ZERO_BB;
		BitBoard snipers = ((GetSlidingAttacks<PIECE_ROOK>(square, ZERO_BB) & position.GetPieces(PIECE_QUEEN, PIECE_ROOK))
			| (GetSlidingAttacks<PIECE_BISHOP>(square, ZERO_BB) & position.GetPieces(PIECE_QUEEN, PIECE_BISHOP))) & sliders;
		BitBoard occupancy = position.GetAllPieces() ^ snipers;
		Team teamAtSquare = position.GetTeamAt(square);
		while (snipers)
		{
			SquareIndex sniperSquare = PopLeastSignificantBit(snipers);
			BitBoard between = GetBitBoardBetween(square, sniperSquare) & occupancy;
			if (between && !MoreThanOne(between))
			{
				blockers |= between;
				if (pinners && (between & position.GetTeamPieces(teamAtSquare)))
					(*pinners) |= sniperSquare;
			}
		}
		return blockers;
	}

	void CalculateKingBlockers(Position& position, Team team)
	{
		position.InfoCache.BlockersForKing[team] = GetSliderBlockers(position, position.GetTeamPieces(OtherTeam(team)), position.GetKingSquare(team), &position.InfoCache.Pinners[OtherTeam(team)]);
	}

	void CalculateCheckers(Position& position, Team team)
	{
		position.InfoCache.CheckedBy[team] = GetAttackers(position, OtherTeam(team), position.InfoCache.KingSquare[team], position.GetAllPieces());
		position.InfoCache.InCheck[team] = (bool)position.InfoCache.CheckedBy[team];
	}

	bool IsSquareUnderAttack(const Position& position, Team byTeam, const Square& square)
	{
		return IsSquareUnderAttack(position, byTeam, BitBoard::SquareToBitIndex(square));
	}

	bool IsSquareUnderAttack(const Position& position, Team byTeam, SquareIndex square)
	{
		if (GetNonSlidingAttacks<PIECE_PAWN>(square, OtherTeam(byTeam)) & position.GetTeamPieces(byTeam, PIECE_PAWN))
			return true;
		if (GetNonSlidingAttacks<PIECE_KNIGHT>(square) & position.GetTeamPieces(byTeam, PIECE_KNIGHT))
			return true;
		if (GetNonSlidingAttacks<PIECE_KING>(square) & position.GetTeamPieces(byTeam, PIECE_KING))
			return true;
		if (GetSlidingAttacks<PIECE_BISHOP>(square, position.GetAllPieces()) & position.GetTeamPieces(byTeam, PIECE_BISHOP, PIECE_QUEEN))
			return true;
		if (GetSlidingAttacks<PIECE_ROOK>(square, position.GetAllPieces()) & position.GetTeamPieces(byTeam, PIECE_ROOK, PIECE_QUEEN))
			return true;
		return false;
	}

	template<Piece PIECE>
	Piece MinAttacker(const Position& position, SquareIndex to, const BitBoard& sideToMoveAttackers, BitBoard& occupied, BitBoard& attackers)
	{
		BitBoard b = sideToMoveAttackers & position.GetPieces(PIECE);
		if (!b)
		{
			return MinAttacker<(Piece)(PIECE + 1)>(position, to, sideToMoveAttackers, occupied, attackers);
		}
		occupied ^= ForwardBitScan(b);
		if (PIECE == PIECE_PAWN || PIECE == PIECE_BISHOP || PIECE == PIECE_QUEEN)
		{
			attackers |= GetSlidingAttacks<PIECE_BISHOP>(to, occupied) & position.GetPieces(PIECE_BISHOP, PIECE_QUEEN);
		}
		if (PIECE == PIECE_ROOK || PIECE == PIECE_QUEEN)
		{
			attackers |= GetSlidingAttacks<PIECE_ROOK>(to, occupied) & position.GetPieces(PIECE_ROOK, PIECE_QUEEN);
		}
		attackers &= occupied;
		return PIECE;
	}

	bool SeeGE(const Position& position, const Move& move, ValueType threshold)
	{
		if (move.IsCapture())
		{
			BitBoard stmAttackers;
			SquareIndex from = move.GetFromSquareIndex();
			SquareIndex to = move.GetToSquareIndex();
			Piece nextVictim = move.GetMovingPiece();
			Team team = position.TeamToPlay;
			Team sideToMove = OtherTeam(team);
			ValueType balance;

			Piece captured = move.GetCapturedPiece();
			balance = GetPieceValue(captured) - threshold;
			if (balance < 0)
				return false;

			// Assume they capture piece for free
			balance -= GetPieceValue(nextVictim);
			if (balance >= 0)
				return true;

			BitBoard occupied = position.GetAllPieces() ^ from ^ to;
			BitBoard attackers = GetAttackers(position, team, to, occupied) & occupied;

			while (true)
			{
				stmAttackers = attackers & position.GetTeamPieces(sideToMove);
				if (!(position.InfoCache.Pinners[OtherTeam(sideToMove)] & ~occupied))
					stmAttackers &= ~position.InfoCache.BlockersForKing[sideToMove];

				if (!stmAttackers)
					break;

				nextVictim = MinAttacker<PIECE_PAWN>(position, to, stmAttackers, occupied, attackers);
				sideToMove = OtherTeam(sideToMove);
				BOX_ASSERT(balance < 0, "Invalid balance");

				balance = -balance - 1 - GetPieceValue(nextVictim);
				if (balance >= 0)
				{
					if (nextVictim == PIECE_KING && (attackers & position.GetTeamPieces(sideToMove)))
						sideToMove = OtherTeam(sideToMove);
					break;
				}
				BOX_ASSERT(nextVictim != PIECE_KING, "Invalid victim");
			}
			return sideToMove != team;
		}
		return threshold <= 0;
	}

	void MovePiece(Position& position, Team team, Piece piece, SquareIndex from, SquareIndex to)
	{
		BitBoard mask = from | to;
		position.Teams[team].Pieces[piece] ^= mask;
		position.InfoCache.TeamPieces[team] ^= mask;
		position.InfoCache.AllPieces ^= mask;
		position.InfoCache.PiecesByType[piece] ^= mask;
		position.InfoCache.PieceOnSquare[from] = PIECE_INVALID;
		position.InfoCache.PieceOnSquare[to] = piece;

		position.Hash.RemovePieceAt(team, piece, from);
		position.Hash.AddPieceAt(team, piece, to);
	}

	void RemovePiece(Position& position, Team team, Piece piece, SquareIndex square)
	{
		position.Teams[team].Pieces[piece] ^= square;
		position.InfoCache.TeamPieces[team] ^= square;
		position.InfoCache.AllPieces ^= square;
		position.InfoCache.PiecesByType[piece] ^= square;
		position.InfoCache.PieceOnSquare[square] = PIECE_INVALID;
		if (piece != PIECE_PAWN && piece != PIECE_KING)
		{
			position.InfoCache.NonPawnMaterial[team] -= GetPieceValue(piece, MIDGAME);
		}
		position.Hash.RemovePieceAt(team, piece, square);
	}

	void AddPiece(Position& position, Team team, Piece piece, SquareIndex square)
	{
		position.Teams[team].Pieces[piece] |= square;
		position.InfoCache.TeamPieces[team] |= square;
		position.InfoCache.AllPieces |= square;
		position.InfoCache.PiecesByType[piece] |= square;
		position.InfoCache.PieceOnSquare[square] = piece;
		if (piece != PIECE_PAWN && piece != PIECE_KING)
		{
			position.InfoCache.NonPawnMaterial[team] += GetPieceValue(piece, MIDGAME);
		}
		position.Hash.AddPieceAt(team, piece, square);
	}

	void UpdateCastleInfoFromMove(Position& position, Team team, const Move& move)
	{
		if (move.GetMovingPiece() == PIECE_ROOK)
		{
			if (team == TEAM_WHITE && move.GetFromSquareIndex() == a1 && position.Teams[team].CastleQueenSide)
			{
				position.Teams[team].CastleQueenSide = false;
				position.Hash.RemoveCastleQueenside(team);
			}
			else if (team == TEAM_WHITE && move.GetFromSquareIndex() == h1 && position.Teams[team].CastleKingSide)
			{
				position.Teams[team].CastleKingSide = false;
				position.Hash.RemoveCastleKingside(team);
			}
			if (team == TEAM_BLACK && move.GetFromSquareIndex() == a8 && position.Teams[team].CastleQueenSide)
			{
				position.Teams[team].CastleQueenSide = false;
				position.Hash.RemoveCastleQueenside(team);
			}
			else if (team == TEAM_BLACK && move.GetFromSquareIndex() == h8 && position.Teams[team].CastleKingSide)
			{
				position.Teams[team].CastleKingSide = false;
				position.Hash.RemoveCastleKingside(team);
			}
		}
		else if (move.GetMovingPiece() == PIECE_KING)
		{
			if (position.Teams[team].CastleKingSide)
			{
				position.Teams[team].CastleKingSide = false;
				position.Hash.RemoveCastleKingside(team);
			}
			if (position.Teams[team].CastleQueenSide)
			{
				position.Teams[team].CastleQueenSide = false;
				position.Hash.RemoveCastleQueenside(team);
			}
		}
		if ((move.GetFlags() & MOVE_CAPTURE) && move.GetCapturedPiece() == PIECE_ROOK)
		{
			Team otherTeam = OtherTeam(team);
			Square capturedSquare = move.GetToSquare();
			if (capturedSquare.File == FILE_A && position.Teams[otherTeam].CastleQueenSide)
			{
				if ((otherTeam == TEAM_WHITE && capturedSquare.Rank == RANK_1) || (otherTeam == TEAM_BLACK && capturedSquare.Rank == RANK_8))
				{
					position.Teams[otherTeam].CastleQueenSide = false;
					position.Hash.RemoveCastleQueenside(otherTeam);
				}
			}
			if (capturedSquare.File == FILE_H && position.Teams[otherTeam].CastleKingSide)
			{
				if ((otherTeam == TEAM_WHITE && capturedSquare.Rank == RANK_1) || (otherTeam == TEAM_BLACK && capturedSquare.Rank == RANK_8))
				{
					position.Teams[otherTeam].CastleKingSide = false;
					position.Hash.RemoveCastleKingside(otherTeam);
				}
			}
		}
	}

	void ApplyMove(Position& position, Move move)
	{
		BOX_ASSERT(!(move.IsCapture() && move.GetCapturedPiece() == PIECE_KING), "Cannot capture king");
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			position.Hash.RemoveEnPassant(position.EnpassantSquare.File);
			position.EnpassantSquare = INVALID_SQUARE;
		}

		MoveFlag flags = move.GetFlags();
		const Team currentTeam = position.TeamToPlay;
		const Team otherTeam = OtherTeam(currentTeam);
		if (move != MOVE_NONE)
		{
			if (!flags)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
			}
			else if ((flags & MOVE_CAPTURE) && (flags & MOVE_PROMOTION))
			{
				Piece capturedPiece = move.GetCapturedPiece();
				RemovePiece(position, otherTeam, capturedPiece, move.GetToSquareIndex());
				RemovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex());
				Piece promotionPiece = move.GetPromotionPiece();
				AddPiece(position, currentTeam, promotionPiece, move.GetToSquareIndex());
			}
			else if (flags & MOVE_CAPTURE)
			{
				Piece capturedPiece = move.GetCapturedPiece();
				RemovePiece(position, otherTeam, capturedPiece, move.GetToSquareIndex());
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
			}
			else if (flags & MOVE_KINGSIDE_CASTLE)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
				if (currentTeam == TEAM_WHITE)
					MovePiece(position, currentTeam, PIECE_ROOK, h1, f1);
				else
					MovePiece(position, currentTeam, PIECE_ROOK, h8, f8);
			}
			else if (flags & MOVE_QUEENSIDE_CASTLE)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
				if (currentTeam == TEAM_WHITE)
					MovePiece(position, currentTeam, PIECE_ROOK, a1, d1);
				else
					MovePiece(position, currentTeam, PIECE_ROOK, a8, d8);
			}
			else if (flags & MOVE_PROMOTION)
			{
				RemovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex());
				AddPiece(position, currentTeam, move.GetPromotionPiece(), move.GetToSquareIndex());
			}
			else if (flags & MOVE_DOUBLE_PAWN_PUSH)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
				SquareIndex enPassantSquare = (SquareIndex)(move.GetToSquareIndex() - GetForwardShift(currentTeam));
				position.EnpassantSquare = BitBoard::BitIndexToSquare(enPassantSquare);
				position.Hash.AddEnPassant(position.EnpassantSquare.File);
			}
			else if (flags & MOVE_EN_PASSANT)
			{
				RemovePiece(position, otherTeam, PIECE_PAWN, (SquareIndex)(move.GetToSquareIndex() - GetForwardShift(currentTeam)));
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex(), move.GetToSquareIndex());
			}

			if (position.Teams[currentTeam].CastleKingSide || position.Teams[currentTeam].CastleQueenSide || ((flags & MOVE_CAPTURE) && move.GetCapturedPiece() == PIECE_ROOK))
				UpdateCastleInfoFromMove(position, currentTeam, move);

			if (move.GetMovingPiece() == PIECE_KING)
				position.InfoCache.KingSquare[currentTeam] = move.GetToSquareIndex();

			CalculateKingBlockers(position, otherTeam);
			CalculateCheckers(position, otherTeam);
		}

		if (move.GetMovingPiece() == PIECE_PAWN || (flags & MOVE_CAPTURE))
			position.HalfTurnsSinceCaptureOrPush = 0;
		else
			position.HalfTurnsSinceCaptureOrPush++;

		if (currentTeam == TEAM_BLACK)
			position.TotalTurns++;
		position.TeamToPlay = otherTeam;
		position.Hash.FlipTeamToPlay();
	}

	void ApplyMove(Position& position, Move move, UndoInfo* outUndoInfo)
	{
		if (outUndoInfo)
		{
			outUndoInfo->CheckedBy[TEAM_WHITE] = position.InfoCache.CheckedBy[TEAM_WHITE];
			outUndoInfo->CheckedBy[TEAM_BLACK] = position.InfoCache.CheckedBy[TEAM_BLACK];
			outUndoInfo->InCheck[TEAM_WHITE] = position.InfoCache.InCheck[TEAM_WHITE];
			outUndoInfo->InCheck[TEAM_BLACK] = position.InfoCache.InCheck[TEAM_BLACK];
			outUndoInfo->EnpassantSquare = position.EnpassantSquare;
			outUndoInfo->HalfTurnsSinceCaptureOrPush = position.HalfTurnsSinceCaptureOrPush;
			outUndoInfo->CastleKingSide[TEAM_WHITE] = position.Teams[TEAM_WHITE].CastleKingSide;
			outUndoInfo->CastleKingSide[TEAM_BLACK] = position.Teams[TEAM_BLACK].CastleKingSide;
			outUndoInfo->CastleQueenSide[TEAM_WHITE] = position.Teams[TEAM_WHITE].CastleQueenSide;
			outUndoInfo->CastleQueenSide[TEAM_BLACK] = position.Teams[TEAM_BLACK].CastleQueenSide;
		}
		ApplyMove(position, move);
	}

	void UndoMove(Position& position, Move move, const UndoInfo& undo)
	{
		if (position.TeamToPlay == TEAM_WHITE)
			position.TotalTurns--;
		position.TeamToPlay = OtherTeam(position.TeamToPlay);
		position.Hash.FlipTeamToPlay();

		MoveFlag flags = move.GetFlags();
		position.HalfTurnsSinceCaptureOrPush = undo.HalfTurnsSinceCaptureOrPush;

		Team currentTeam = position.TeamToPlay;
		if (move != MOVE_NONE)
		{
			if (!flags)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
			}
			else if ((flags & MOVE_CAPTURE) && (flags & MOVE_PROMOTION))
			{
				Piece promotionPiece = move.GetPromotionPiece();
				RemovePiece(position, currentTeam, promotionPiece, move.GetToSquareIndex());
				Piece capturedPiece = move.GetCapturedPiece();
				AddPiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex());
				AddPiece(position, OtherTeam(currentTeam), capturedPiece, move.GetToSquareIndex());
			}
			else if (flags & MOVE_CAPTURE)
			{
				Piece capturedPiece = move.GetCapturedPiece();
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
				AddPiece(position, OtherTeam(currentTeam), capturedPiece, move.GetToSquareIndex());
			}
			else if (flags & MOVE_KINGSIDE_CASTLE)
			{
				if (currentTeam == TEAM_WHITE)
					MovePiece(position, currentTeam, PIECE_ROOK, f1, h1);
				else
					MovePiece(position, currentTeam, PIECE_ROOK, f8, h8);
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
			}
			else if (flags & MOVE_QUEENSIDE_CASTLE)
			{
				if (currentTeam == TEAM_WHITE)
					MovePiece(position, currentTeam, PIECE_ROOK, d1, a1);
				else
					MovePiece(position, currentTeam, PIECE_ROOK, d8, a8);
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
			}
			else if (flags & MOVE_PROMOTION)
			{
				RemovePiece(position, currentTeam, move.GetPromotionPiece(), move.GetToSquareIndex());
				AddPiece(position, currentTeam, move.GetMovingPiece(), move.GetFromSquareIndex());
			}
			else if (flags & MOVE_DOUBLE_PAWN_PUSH)
			{
				position.Hash.RemoveEnPassant(position.EnpassantSquare.File);
				position.EnpassantSquare = INVALID_SQUARE;
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
			}
			else if (flags & MOVE_EN_PASSANT)
			{
				MovePiece(position, currentTeam, move.GetMovingPiece(), move.GetToSquareIndex(), move.GetFromSquareIndex());
				AddPiece(position, OtherTeam(currentTeam), PIECE_PAWN, (SquareIndex)(move.GetToSquareIndex() - GetForwardShift(currentTeam)));
			}

			if (move.GetMovingPiece() == PIECE_KING)
				position.InfoCache.KingSquare[currentTeam] = move.GetFromSquareIndex();

			if (position.Teams[TEAM_WHITE].CastleKingSide != undo.CastleKingSide[TEAM_WHITE])
			{
				position.Teams[TEAM_WHITE].CastleKingSide = undo.CastleKingSide[TEAM_WHITE];
				position.Hash.AddCastleKingside(TEAM_WHITE);
			}
			if (position.Teams[TEAM_BLACK].CastleKingSide != undo.CastleKingSide[TEAM_BLACK])
			{
				position.Teams[TEAM_BLACK].CastleKingSide = undo.CastleKingSide[TEAM_BLACK];
				position.Hash.AddCastleKingside(TEAM_BLACK);
			}
			if (position.Teams[TEAM_WHITE].CastleQueenSide != undo.CastleQueenSide[TEAM_WHITE])
			{
				position.Teams[TEAM_WHITE].CastleQueenSide = undo.CastleQueenSide[TEAM_WHITE];
				position.Hash.AddCastleQueenside(TEAM_WHITE);
			}
			if (position.Teams[TEAM_BLACK].CastleQueenSide != undo.CastleQueenSide[TEAM_BLACK])
			{
				position.Teams[TEAM_BLACK].CastleQueenSide = undo.CastleQueenSide[TEAM_BLACK];
				position.Hash.AddCastleQueenside(TEAM_BLACK);
			}
		}
		if (undo.EnpassantSquare != INVALID_SQUARE)
		{
			position.EnpassantSquare = undo.EnpassantSquare;
			position.Hash.AddEnPassant(undo.EnpassantSquare.File);
		}
		position.InfoCache.CheckedBy[TEAM_WHITE] = undo.CheckedBy[TEAM_WHITE];
		position.InfoCache.CheckedBy[TEAM_BLACK] = undo.CheckedBy[TEAM_BLACK];
		position.InfoCache.InCheck[TEAM_WHITE] = undo.InCheck[TEAM_WHITE];
		position.InfoCache.InCheck[TEAM_BLACK] = undo.InCheck[TEAM_BLACK];

		CalculateKingBlockers(position, TEAM_WHITE);
		CalculateKingBlockers(position, TEAM_BLACK);
	}

	void ApplyNullMove(Position& position, UndoInfo* outUndoInfo)
	{
		BOX_ASSERT(!GetAttackers(position, OtherTeam(position.TeamToPlay), position.GetKingSquare(position.TeamToPlay), position.GetAllPieces()), "In Check");
		outUndoInfo->EnpassantSquare = position.EnpassantSquare;
		if (position.EnpassantSquare != INVALID_SQUARE)
		{
			position.Hash.RemoveEnPassant(position.EnpassantSquare.File);
			position.EnpassantSquare = INVALID_SQUARE;
		}

		position.TeamToPlay = OtherTeam(position.TeamToPlay);
		position.Hash.FlipTeamToPlay();
		position.HalfTurnsSinceCaptureOrPush++;

		CalculateKingBlockers(position, position.TeamToPlay);
		CalculateCheckers(position, position.TeamToPlay);
	}

	void UndoNullMove(Position& position, const UndoInfo& undo)
	{
		if (undo.EnpassantSquare != INVALID_SQUARE)
		{
			position.EnpassantSquare = undo.EnpassantSquare;
			position.Hash.AddEnPassant(position.EnpassantSquare.File);
		}
		position.TeamToPlay = OtherTeam(position.TeamToPlay);
		position.Hash.FlipTeamToPlay();
		position.HalfTurnsSinceCaptureOrPush--;
	}

	bool SanityCheckMove(const Position& position, Move move)
	{
		if (!position.IsPieceOnSquare(position.TeamToPlay, move.GetMovingPiece(), move.GetFromSquareIndex()))
		{
			// There is no piece at the position to move
			return false;
		}
		if (position.GetPieceOnSquare(move.GetFromSquareIndex()) != move.GetMovingPiece())
		{
			// The wrong piece type is at the square
			return false;
		}
		if (move.IsCapture() && !position.IsPieceOnSquare(OtherTeam(position.TeamToPlay), move.GetCapturedPiece(), move.GetToSquareIndex()))
		{
			// No piece to capture
			return false;
		}
		return true;
	}

	Move CreateMove(const Position& position, const Square& from, const Square& to, Piece promotionPiece)
	{
		Move move(BitBoard::SquareToBitIndex(from), BitBoard::SquareToBitIndex(to), position.GetPieceOnSquare(BitBoard::SquareToBitIndex(from)), MOVE_NORMAL);
		SquareIndex toIndex = BitBoard::SquareToBitIndex(to);

		bool occupied = position.GetTeamPieces(OtherTeam(position.TeamToPlay)) & toIndex;
		if (occupied)
		{
			move.SetFlags(MOVE_CAPTURE);
			move.SetCapturedPiece(position.GetPieceOnSquare(BitBoard::SquareToBitIndex(to)));
			BOX_ASSERT(move.GetCapturedPiece() != PIECE_KING, "Cannot capture king");
		}

		if (move.GetMovingPiece() == PIECE_PAWN)
		{
			BitBoard promotionMask = (position.TeamToPlay == TEAM_WHITE) ? RANK_8_MASK : RANK_1_MASK;
			if (promotionMask & toIndex)
			{
				move.SetFlags(move.GetFlags() | MOVE_PROMOTION);
				move.SetPromotionPiece(promotionPiece);
			}
		}

		if (abs(to.Rank - from.Rank) == 2 && move.GetMovingPiece() == PIECE_PAWN)
			move.SetFlags(MOVE_DOUBLE_PAWN_PUSH);

		if (move.GetMovingPiece() == PIECE_KING && from.File == FILE_E && to.File == FILE_C)
			move.SetFlags(MOVE_QUEENSIDE_CASTLE);

		if (move.GetMovingPiece() == PIECE_KING && from.File == FILE_E && to.File == FILE_G)
			move.SetFlags(MOVE_KINGSIDE_CASTLE);
		
		if (to == position.EnpassantSquare && move.GetMovingPiece() == PIECE_PAWN)
			move.SetFlags(MOVE_EN_PASSANT);
		return move;
	}

	bool IsLegalMoveSlow(const Position& position, Move move)
	{
		Move buffer[MAX_MOVES];
		MoveList list(buffer);
		MoveGenerator generator(position);
		generator.GetPseudoLegalMoves(list);
		generator.FilterLegalMoves(list);
		for (int i = 0; i < list.MoveCount; i++)
		{
			if (list.Moves[i] == move)
				return true;
		}
		return false;
	}

	std::vector<Move> GetLegalMovesDebug(const Position& position)
	{
		std::vector<Move> result;
		Move buffer[MAX_MOVES];
		MoveList list(buffer);
		MoveGenerator generator(position);
		generator.GetPseudoLegalMoves(list);
		generator.FilterLegalMoves(list);
		for (int i = 0; i < list.MoveCount; i++)
			result.push_back(list.Moves[i]);
		return result;
	}

	std::ostream& operator<<(std::ostream& stream, const Position& position)
	{
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << "   +";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				stream << "---+";
			}
			stream << "\n " << char('1' + (rank - RANK_1)) << " |";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFen = ' ';
				SquareIndex square = BitBoard::SquareToBitIndex({ file, rank });
				Piece piece = position.GetPieceOnSquare(square);
				if (piece != PIECE_INVALID)
				{
					Team team = position.GetTeamAt(square);
					pieceFen = PieceToFEN(piece, team == TEAM_WHITE);
				}
				stream << ' ' << pieceFen << " |";
			}
			stream << '\n';
		}
		stream << "   +";
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			stream << "---+";
		}
		stream << "\n    ";
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			stream << ' ' << char('A' + (file - FILE_A)) << "  ";
		}
		return stream;
	}

}
