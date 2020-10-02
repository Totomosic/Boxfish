#ifdef EMSCRIPTEN
#include <string>
#include <vector>
#include <iostream>
#include "Boxfish.h"
using namespace Boxfish;

#include <emscripten/bind.h>
using namespace emscripten;

Move SearchBestMoveTime(Search& search, const Position& position, int milliseconds)
{
	SearchLimits limits;
	limits.Milliseconds = milliseconds;

	return search.SearchBestMove(position, limits);
}

Move SearchBestMoveDepth(Search& search, const Position& position, int depth)
{
	SearchLimits limits;
	limits.Depth = depth;

	return search.SearchBestMove(position, limits);
}

std::vector<Move> GenerateLegalMoves(const Position& position)
{
	Move buffer[MAX_MOVES];
	MoveList moves(buffer);

	MoveGenerator generator(position);
	generator.GetPseudoLegalMoves(moves);
	generator.FilterLegalMoves(moves);

	std::vector<Move> result;
	result.reserve(moves.MoveCount);
	for (int i = 0; i < moves.MoveCount; i++)
		result.push_back(moves.Moves[i]);
	return result;
}

Team PositionGetTeamToPlay(const Position& position)
{
	return position.TeamToPlay;
}

void PositionSetTeamToPlay(Position& position, Team ttp)
{
	position.TeamToPlay = ttp;
}

const Square& PositionGetEnpassant(const Position& position)
{
	return position.EnpassantSquare;
}

void PositionSetEnpassant(Position& position, const Square& enp)
{
	position.EnpassantSquare = enp;
}

SquareIndex PositionEnpassantCaptureSquare(const Position& position, Team movingTeam)
{
	return (SquareIndex)(BitBoard::SquareToBitIndex(position.EnpassantSquare) - GetForwardShift(movingTeam));
}

void ApplyUndoableMove(Position& position, Move move, UndoInfo& undo)
{
	ApplyMove(position, move, &undo);
}

bool MoveEquivalent(Move a, Move b)
{
	return a == b;
}

void SearchSetLevel(Search& search, int level)
{
	BoxfishSettings settings;
	settings.SkillLevel = level;
	search.SetSettings(settings);
}

EMSCRIPTEN_BINDINGS(my_module) {
	register_vector<Move>("MoveList");

	constant("INVALID_SQUARE", INVALID_SQUARE);
	constant("MOVE_NONE", MOVE_NONE);

	function("Init", &Init);
	function("OtherTeam", &OtherTeam);
	function("CreateStartingPosition", &CreateStartingPosition);
	function("CreatePositionFromFEN", &CreatePositionFromFEN);
	function("GetFENFromPosition", &GetFENFromPosition);
	function("CreateMove", &CreateMove);
	function("CreateMoveFromString", &CreateMoveFromString);
	function("ApplyMove", select_overload<void(Position&, Move)>(&ApplyMove));
	function("ApplyUndoableMove", &ApplyUndoableMove);
	function("UndoMove", &UndoMove);
	function("GenerateLegalMoves", &GenerateLegalMoves);
	function("IsSameMove", &MoveEquivalent);

	function("IsInCheck", select_overload<bool(const Position&)>(&IsInCheck));
	function("GetTeamAt", &GetTeamAt);

	enum_<Team>("Team")
		.value("White", TEAM_WHITE)
		.value("Black", TEAM_BLACK);
	enum_<Piece>("Piece")
		.value("Pawn", PIECE_PAWN)
		.value("Knight", PIECE_KNIGHT)
		.value("Bishop", PIECE_BISHOP)
		.value("Rook", PIECE_ROOK)
		.value("Queen", PIECE_QUEEN)
		.value("King", PIECE_KING)
		.value("Invalid", PIECE_INVALID)
		.value("All", PIECE_ALL);
	enum_<File>("File")
		.value("FILE_A", FILE_A)
		.value("FILE_B", FILE_B)
		.value("FILE_C", FILE_C)
		.value("FILE_D", FILE_D)
		.value("FILE_E", FILE_E)
		.value("FILE_F", FILE_F)
		.value("FILE_G", FILE_G)
		.value("FILE_H", FILE_H)
		.value("FILE_MAX", FILE_MAX)
		.value("FILE_INVALID", FILE_INVALID);
	enum_<Rank>("Rank")
		.value("RANK_1", RANK_1)
		.value("RANK_2", RANK_2)
		.value("RANK_3", RANK_3)
		.value("RANK_4", RANK_4)
		.value("RANK_5", RANK_5)
		.value("RANK_6", RANK_6)
		.value("RANK_7", RANK_7)
		.value("RANK_8", RANK_8)
		.value("RANK_MAX", RANK_MAX)
		.value("RANK_INVALID", RANK_INVALID);
	enum_<SquareIndex>("SquareIndex")
		.value("a1", a1)
		.value("b1", b1)
		.value("c1", c1)
		.value("d1", d1)
		.value("e1", e1)
		.value("f1", f1)
		.value("g1", g1)
		.value("h1", h1)

		.value("a2", a2)
		.value("b2", b2)
		.value("c2", c2)
		.value("d2", d2)
		.value("e2", e2)
		.value("f2", f2)
		.value("g2", g2)
		.value("h2", h2)

		.value("a3", a3)
		.value("b3", b3)
		.value("c3", c3)
		.value("d3", d3)
		.value("e3", e3)
		.value("f3", f3)
		.value("g3", g3)
		.value("h3", h3)

		.value("a4", a4)
		.value("b4", b4)
		.value("c4", c4)
		.value("d4", d4)
		.value("e4", e4)
		.value("f4", f4)
		.value("g4", g4)
		.value("h4", h4)

		.value("a5", a5)
		.value("b5", b5)
		.value("c5", c5)
		.value("d5", d5)
		.value("e5", e5)
		.value("f5", f5)
		.value("g5", g5)
		.value("h5", h5)

		.value("a6", a6)
		.value("b6", b6)
		.value("c6", c6)
		.value("d6", d6)
		.value("e6", e6)
		.value("f6", f6)
		.value("g6", g6)
		.value("h6", h6)

		.value("a7", a7)
		.value("b7", b7)
		.value("c7", c7)
		.value("d7", d7)
		.value("e7", e7)
		.value("f7", f7)
		.value("g7", g7)
		.value("h7", h7)

		.value("a8", a8)
		.value("b8", b8)
		.value("c8", c8)
		.value("d8", d8)
		.value("e8", e8)
		.value("f8", f8)
		.value("g8", g8)
		.value("h8", h8);

	value_object<Square>("Square")
		.field("File", &Square::File)
		.field("Rank", &Square::Rank);
	function("IndexToSquare", &BitBoard::BitIndexToSquare);
	function("SquareToIndex", &BitBoard::SquareToBitIndex);

	class_<UndoInfo>("UndoInfo");
	class_<UCI>("UCI")
		.class_function("FormatMove", &UCI::FormatMove);
	class_<Move>("Move")
		.constructor<>()
		.function("GetFromSquareIndex", &Move::GetFromSquareIndex)
		.function("GetToSquareIndex", &Move::GetToSquareIndex)
		.function("GetFromSquare", &Move::GetFromSquare)
		.function("GetToSquare", &Move::GetToSquare)
		.function("IsCapture", &Move::IsCapture)
		.function("IsPromotion", &Move::IsPromotion)
		.function("IsCaptureOrPromotion", &Move::IsCaptureOrPromotion)
		.function("IsCastle", &Move::IsCastle)
		.function("IsEnpassant", &Move::IsEnpassant)
		.function("GetMovingPiece", &Move::GetMovingPiece)
		.function("GetCapturedPiece", &Move::GetCapturedPiece)
		.function("GetPromotionPiece", &Move::GetPromotionPiece);
	class_<Position>("Position")
		.property("TeamToPlay", &PositionGetTeamToPlay, &PositionSetTeamToPlay)
		.property("EnpassantSquare", &PositionGetEnpassant, &PositionSetEnpassant)
		.function("GetPieceOnSquare", &Position::GetPieceOnSquare)
		.function("GetKingSquare", &Position::GetKingSquare)
		.function("GetEnpassantCaptureSquare", &PositionEnpassantCaptureSquare);
	value_object<SearchLimits>("SearchLimits")
		.field("Infinite", &SearchLimits::Infinite)
		.field("Depth", &SearchLimits::Depth)
		.field("Nodes", &SearchLimits::Nodes)
		.field("Milliseconds", &SearchLimits::Milliseconds);
	class_<Search>("Search")
		.constructor<size_t, bool>()
		.function("SetLevel", &SearchSetLevel)
		.function("Perft", &Search::Perft)
		.function("SearchMoveTime", &SearchBestMoveTime)
		.function("SearchDepth", &SearchBestMoveDepth);
}

#endif
