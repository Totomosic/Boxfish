#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "Boxfish.h"

namespace Test
{

	using namespace Boxfish;

	TEST_CASE("SquareFromString", "[SquareFromString]")
	{
		REQUIRE(SquareFromString("a1") == Square{ FILE_A, RANK_1 });
		REQUIRE(SquareFromString("b1") == Square{ FILE_B, RANK_1 });
		REQUIRE(SquareFromString("c1") == Square{ FILE_C, RANK_1 });
		REQUIRE(SquareFromString("d1") == Square{ FILE_D, RANK_1 });
		REQUIRE(SquareFromString("e1") == Square{ FILE_E, RANK_1 });
		REQUIRE(SquareFromString("f1") == Square{ FILE_F, RANK_1 });
		REQUIRE(SquareFromString("g1") == Square{ FILE_G, RANK_1 });
		REQUIRE(SquareFromString("h1") == Square{ FILE_H, RANK_1 });

		REQUIRE(SquareFromString("a2") == Square{ FILE_A, RANK_2 });
		REQUIRE(SquareFromString("b2") == Square{ FILE_B, RANK_2 });
		REQUIRE(SquareFromString("c2") == Square{ FILE_C, RANK_2 });
		REQUIRE(SquareFromString("d2") == Square{ FILE_D, RANK_2 });
		REQUIRE(SquareFromString("e2") == Square{ FILE_E, RANK_2 });
		REQUIRE(SquareFromString("f2") == Square{ FILE_F, RANK_2 });
		REQUIRE(SquareFromString("g2") == Square{ FILE_G, RANK_2 });
		REQUIRE(SquareFromString("h2") == Square{ FILE_H, RANK_2 });

		REQUIRE(SquareFromString("a3") == Square{ FILE_A, RANK_3 });
		REQUIRE(SquareFromString("b3") == Square{ FILE_B, RANK_3 });
		REQUIRE(SquareFromString("c3") == Square{ FILE_C, RANK_3 });
		REQUIRE(SquareFromString("d3") == Square{ FILE_D, RANK_3 });
		REQUIRE(SquareFromString("e3") == Square{ FILE_E, RANK_3 });
		REQUIRE(SquareFromString("f3") == Square{ FILE_F, RANK_3 });
		REQUIRE(SquareFromString("g3") == Square{ FILE_G, RANK_3 });
		REQUIRE(SquareFromString("h3") == Square{ FILE_H, RANK_3 });

		REQUIRE(SquareFromString("a4") == Square{ FILE_A, RANK_4 });
		REQUIRE(SquareFromString("b4") == Square{ FILE_B, RANK_4 });
		REQUIRE(SquareFromString("c4") == Square{ FILE_C, RANK_4 });
		REQUIRE(SquareFromString("d4") == Square{ FILE_D, RANK_4 });
		REQUIRE(SquareFromString("e4") == Square{ FILE_E, RANK_4 });
		REQUIRE(SquareFromString("f4") == Square{ FILE_F, RANK_4 });
		REQUIRE(SquareFromString("g4") == Square{ FILE_G, RANK_4 });
		REQUIRE(SquareFromString("h4") == Square{ FILE_H, RANK_4 });

		REQUIRE(SquareFromString("a5") == Square{ FILE_A, RANK_5 });
		REQUIRE(SquareFromString("b5") == Square{ FILE_B, RANK_5 });
		REQUIRE(SquareFromString("c5") == Square{ FILE_C, RANK_5 });
		REQUIRE(SquareFromString("d5") == Square{ FILE_D, RANK_5 });
		REQUIRE(SquareFromString("e5") == Square{ FILE_E, RANK_5 });
		REQUIRE(SquareFromString("f5") == Square{ FILE_F, RANK_5 });
		REQUIRE(SquareFromString("g5") == Square{ FILE_G, RANK_5 });
		REQUIRE(SquareFromString("h5") == Square{ FILE_H, RANK_5 });

		REQUIRE(SquareFromString("a6") == Square{ FILE_A, RANK_6 });
		REQUIRE(SquareFromString("b6") == Square{ FILE_B, RANK_6 });
		REQUIRE(SquareFromString("c6") == Square{ FILE_C, RANK_6 });
		REQUIRE(SquareFromString("d6") == Square{ FILE_D, RANK_6 });
		REQUIRE(SquareFromString("e6") == Square{ FILE_E, RANK_6 });
		REQUIRE(SquareFromString("f6") == Square{ FILE_F, RANK_6 });
		REQUIRE(SquareFromString("g6") == Square{ FILE_G, RANK_6 });
		REQUIRE(SquareFromString("h6") == Square{ FILE_H, RANK_6 });

		REQUIRE(SquareFromString("a7") == Square{ FILE_A, RANK_7 });
		REQUIRE(SquareFromString("b7") == Square{ FILE_B, RANK_7 });
		REQUIRE(SquareFromString("c7") == Square{ FILE_C, RANK_7 });
		REQUIRE(SquareFromString("d7") == Square{ FILE_D, RANK_7 });
		REQUIRE(SquareFromString("e7") == Square{ FILE_E, RANK_7 });
		REQUIRE(SquareFromString("f7") == Square{ FILE_F, RANK_7 });
		REQUIRE(SquareFromString("g7") == Square{ FILE_G, RANK_7 });
		REQUIRE(SquareFromString("h7") == Square{ FILE_H, RANK_7 });

		REQUIRE(SquareFromString("a8") == Square{ FILE_A, RANK_8 });
		REQUIRE(SquareFromString("b8") == Square{ FILE_B, RANK_8 });
		REQUIRE(SquareFromString("c8") == Square{ FILE_C, RANK_8 });
		REQUIRE(SquareFromString("d8") == Square{ FILE_D, RANK_8 });
		REQUIRE(SquareFromString("e8") == Square{ FILE_E, RANK_8 });
		REQUIRE(SquareFromString("f8") == Square{ FILE_F, RANK_8 });
		REQUIRE(SquareFromString("g8") == Square{ FILE_G, RANK_8 });
		REQUIRE(SquareFromString("h8") == Square{ FILE_H, RANK_8 });
	}

	TEST_CASE("ZobristHash", "[Hash]")
	{
		Init();
		ZobristHash hash;
		hash.SetFromPosition(CreateStartingPosition());

		uint64_t startingHash = hash.Hash;
		uint64_t currentHash = startingHash;

		hash.AddCastleKingside(TEAM_WHITE);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleKingside(TEAM_WHITE);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleKingside(TEAM_BLACK);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleKingside(TEAM_BLACK);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleQueenside(TEAM_WHITE);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleQueenside(TEAM_WHITE);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleQueenside(TEAM_BLACK);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleQueenside(TEAM_BLACK);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_A);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_A);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_B);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_B);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_C);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_C);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_D);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_D);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_E);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_E);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_F);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_F);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_G);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_G);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_H);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_H);
		REQUIRE(hash.Hash == currentHash);
	}

	TEST_CASE("Perft", "[Perft]")
	{
		Init();
		Position position = CreateStartingPosition();
		Search search(50 * 1024, false);
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 20);
		REQUIRE(search.Perft(2) == 400);
		REQUIRE(search.Perft(3) == 8902);
		REQUIRE(search.Perft(4) == 197281);
		REQUIRE(search.Perft(5) == 4865609);
		REQUIRE(search.Perft(6) == 119060324);

		position = CreatePositionFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 48);
		REQUIRE(search.Perft(2) == 2039);
		REQUIRE(search.Perft(3) == 97862);
		REQUIRE(search.Perft(4) == 4085603);
		REQUIRE(search.Perft(5) == 193690690);

		position = CreatePositionFromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 14);
		REQUIRE(search.Perft(2) == 191);
		REQUIRE(search.Perft(3) == 2812);
		REQUIRE(search.Perft(4) == 43238);
		REQUIRE(search.Perft(5) == 674624);
		REQUIRE(search.Perft(6) == 11030083);

		position = CreatePositionFromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 6);
		REQUIRE(search.Perft(2) == 264);
		REQUIRE(search.Perft(3) == 9467);
		REQUIRE(search.Perft(4) == 422333);
		REQUIRE(search.Perft(5) == 15833292);

		position = CreatePositionFromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 44);
		REQUIRE(search.Perft(2) == 1486);
		REQUIRE(search.Perft(3) == 62379);
		REQUIRE(search.Perft(4) == 2103487);
		REQUIRE(search.Perft(5) == 89941194);

		position = CreatePositionFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
		search.SetCurrentPosition(position);

		REQUIRE(search.Perft(1) == 46);
		REQUIRE(search.Perft(2) == 2079);
		REQUIRE(search.Perft(3) == 89890);
		REQUIRE(search.Perft(4) == 3894594);
		REQUIRE(search.Perft(5) == 164075551);
	}

}
