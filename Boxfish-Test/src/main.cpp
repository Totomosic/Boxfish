#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "Boxfish.h"
#include "WinAtChess.h"

namespace Test
{

	using namespace Boxfish;

	TEST_CASE("SquareFromAlgebraic", "[SquareFromAlgebraic]")
	{
		REQUIRE(SquareFromAlgebraic("a1") == Square{ FILE_A, RANK_1 });
		REQUIRE(SquareFromAlgebraic("b1") == Square{ FILE_B, RANK_1 });
		REQUIRE(SquareFromAlgebraic("c1") == Square{ FILE_C, RANK_1 });
		REQUIRE(SquareFromAlgebraic("d1") == Square{ FILE_D, RANK_1 });
		REQUIRE(SquareFromAlgebraic("e1") == Square{ FILE_E, RANK_1 });
		REQUIRE(SquareFromAlgebraic("f1") == Square{ FILE_F, RANK_1 });
		REQUIRE(SquareFromAlgebraic("g1") == Square{ FILE_G, RANK_1 });
		REQUIRE(SquareFromAlgebraic("h1") == Square{ FILE_H, RANK_1 });

		REQUIRE(SquareFromAlgebraic("a2") == Square{ FILE_A, RANK_2 });
		REQUIRE(SquareFromAlgebraic("b2") == Square{ FILE_B, RANK_2 });
		REQUIRE(SquareFromAlgebraic("c2") == Square{ FILE_C, RANK_2 });
		REQUIRE(SquareFromAlgebraic("d2") == Square{ FILE_D, RANK_2 });
		REQUIRE(SquareFromAlgebraic("e2") == Square{ FILE_E, RANK_2 });
		REQUIRE(SquareFromAlgebraic("f2") == Square{ FILE_F, RANK_2 });
		REQUIRE(SquareFromAlgebraic("g2") == Square{ FILE_G, RANK_2 });
		REQUIRE(SquareFromAlgebraic("h2") == Square{ FILE_H, RANK_2 });

		REQUIRE(SquareFromAlgebraic("a3") == Square{ FILE_A, RANK_3 });
		REQUIRE(SquareFromAlgebraic("b3") == Square{ FILE_B, RANK_3 });
		REQUIRE(SquareFromAlgebraic("c3") == Square{ FILE_C, RANK_3 });
		REQUIRE(SquareFromAlgebraic("d3") == Square{ FILE_D, RANK_3 });
		REQUIRE(SquareFromAlgebraic("e3") == Square{ FILE_E, RANK_3 });
		REQUIRE(SquareFromAlgebraic("f3") == Square{ FILE_F, RANK_3 });
		REQUIRE(SquareFromAlgebraic("g3") == Square{ FILE_G, RANK_3 });
		REQUIRE(SquareFromAlgebraic("h3") == Square{ FILE_H, RANK_3 });

		REQUIRE(SquareFromAlgebraic("a4") == Square{ FILE_A, RANK_4 });
		REQUIRE(SquareFromAlgebraic("b4") == Square{ FILE_B, RANK_4 });
		REQUIRE(SquareFromAlgebraic("c4") == Square{ FILE_C, RANK_4 });
		REQUIRE(SquareFromAlgebraic("d4") == Square{ FILE_D, RANK_4 });
		REQUIRE(SquareFromAlgebraic("e4") == Square{ FILE_E, RANK_4 });
		REQUIRE(SquareFromAlgebraic("f4") == Square{ FILE_F, RANK_4 });
		REQUIRE(SquareFromAlgebraic("g4") == Square{ FILE_G, RANK_4 });
		REQUIRE(SquareFromAlgebraic("h4") == Square{ FILE_H, RANK_4 });

		REQUIRE(SquareFromAlgebraic("a5") == Square{ FILE_A, RANK_5 });
		REQUIRE(SquareFromAlgebraic("b5") == Square{ FILE_B, RANK_5 });
		REQUIRE(SquareFromAlgebraic("c5") == Square{ FILE_C, RANK_5 });
		REQUIRE(SquareFromAlgebraic("d5") == Square{ FILE_D, RANK_5 });
		REQUIRE(SquareFromAlgebraic("e5") == Square{ FILE_E, RANK_5 });
		REQUIRE(SquareFromAlgebraic("f5") == Square{ FILE_F, RANK_5 });
		REQUIRE(SquareFromAlgebraic("g5") == Square{ FILE_G, RANK_5 });
		REQUIRE(SquareFromAlgebraic("h5") == Square{ FILE_H, RANK_5 });

		REQUIRE(SquareFromAlgebraic("a6") == Square{ FILE_A, RANK_6 });
		REQUIRE(SquareFromAlgebraic("b6") == Square{ FILE_B, RANK_6 });
		REQUIRE(SquareFromAlgebraic("c6") == Square{ FILE_C, RANK_6 });
		REQUIRE(SquareFromAlgebraic("d6") == Square{ FILE_D, RANK_6 });
		REQUIRE(SquareFromAlgebraic("e6") == Square{ FILE_E, RANK_6 });
		REQUIRE(SquareFromAlgebraic("f6") == Square{ FILE_F, RANK_6 });
		REQUIRE(SquareFromAlgebraic("g6") == Square{ FILE_G, RANK_6 });
		REQUIRE(SquareFromAlgebraic("h6") == Square{ FILE_H, RANK_6 });

		REQUIRE(SquareFromAlgebraic("a7") == Square{ FILE_A, RANK_7 });
		REQUIRE(SquareFromAlgebraic("b7") == Square{ FILE_B, RANK_7 });
		REQUIRE(SquareFromAlgebraic("c7") == Square{ FILE_C, RANK_7 });
		REQUIRE(SquareFromAlgebraic("d7") == Square{ FILE_D, RANK_7 });
		REQUIRE(SquareFromAlgebraic("e7") == Square{ FILE_E, RANK_7 });
		REQUIRE(SquareFromAlgebraic("f7") == Square{ FILE_F, RANK_7 });
		REQUIRE(SquareFromAlgebraic("g7") == Square{ FILE_G, RANK_7 });
		REQUIRE(SquareFromAlgebraic("h7") == Square{ FILE_H, RANK_7 });

		REQUIRE(SquareFromAlgebraic("a8") == Square{ FILE_A, RANK_8 });
		REQUIRE(SquareFromAlgebraic("b8") == Square{ FILE_B, RANK_8 });
		REQUIRE(SquareFromAlgebraic("c8") == Square{ FILE_C, RANK_8 });
		REQUIRE(SquareFromAlgebraic("d8") == Square{ FILE_D, RANK_8 });
		REQUIRE(SquareFromAlgebraic("e8") == Square{ FILE_E, RANK_8 });
		REQUIRE(SquareFromAlgebraic("f8") == Square{ FILE_F, RANK_8 });
		REQUIRE(SquareFromAlgebraic("g8") == Square{ FILE_G, RANK_8 });
		REQUIRE(SquareFromAlgebraic("h8") == Square{ FILE_H, RANK_8 });
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

		size_t checks = 0;

		REQUIRE(search.Perft(position, 1) == 20);
		REQUIRE(search.Perft(position, 2) == 400);
		REQUIRE(search.Perft(position, 3) == 8902);
		REQUIRE(search.Perft(position, 4) == 197281);
		REQUIRE(search.Perft(position, 5) == 4865609);
		REQUIRE(search.Perft(position, 6) == 119060324);

		position = CreatePositionFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

		REQUIRE(search.Perft(position, 1) == 48);
		REQUIRE(search.Perft(position, 2) == 2039);
		REQUIRE(search.Perft(position, 3) == 97862);
		REQUIRE(search.Perft(position, 4) == 4085603);
		REQUIRE(search.Perft(position, 5) == 193690690);

		position = CreatePositionFromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");

		REQUIRE(search.Perft(position, 1) == 14);
		REQUIRE(search.Perft(position, 2) == 191);
		REQUIRE(search.Perft(position, 3) == 2812);
		REQUIRE(search.Perft(position, 4) == 43238);
		REQUIRE(search.Perft(position, 5) == 674624);
		REQUIRE(search.Perft(position, 6) == 11030083);

		position = CreatePositionFromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

		REQUIRE(search.Perft(position, 1) == 6);
		REQUIRE(search.Perft(position, 2) == 264);
		REQUIRE(search.Perft(position, 3) == 9467);
		REQUIRE(search.Perft(position, 4) == 422333);
		REQUIRE(search.Perft(position, 5) == 15833292);

		position = CreatePositionFromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");

		REQUIRE(search.Perft(position, 1) == 44);
		REQUIRE(search.Perft(position, 2) == 1486);
		REQUIRE(search.Perft(position, 3) == 62379);
		REQUIRE(search.Perft(position, 4) == 2103487);
		REQUIRE(search.Perft(position, 5) == 89941194);

		position = CreatePositionFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");

		REQUIRE(search.Perft(position, 1) == 46);
		REQUIRE(search.Perft(position, 2) == 2079);
		REQUIRE(search.Perft(position, 3) == 89890);
		REQUIRE(search.Perft(position, 4) == 3894594);
		REQUIRE(search.Perft(position, 5) == 164075551);

		// From Stockfish
		position = CreatePositionFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
		REQUIRE(search.Perft(position, 5) == 193690690);

		position = CreatePositionFromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
		REQUIRE(search.Perft(position, 6) == 11030083);

		position = CreatePositionFromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
		REQUIRE(search.Perft(position, 5) == 15833292);

		position = CreatePositionFromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
		REQUIRE(search.Perft(position, 5) == 89941194);

		position = CreatePositionFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
		REQUIRE(search.Perft(position, 5) == 164075551);
	}

	TEST_CASE("Transposition", "[Transposition]")
	{
		TranspositionTable tt;

		Position position = CreateStartingPosition();

		TranspositionTableEntry tte;
		tte.Update(position.Hash, MOVE_NONE, 10, 487, UPPER_BOUND, 160, false);

		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == MOVE_NONE);
		REQUIRE(tte.GetDepth() == 10);
		REQUIRE(tte.GetScore() == 487);
		REQUIRE(tte.GetFlag() == UPPER_BOUND);
		REQUIRE(tte.GetAge() == 160);
		REQUIRE(tte.IsPv() == false);

		bool found;
		TranspositionTableEntry* entry = tt.GetEntry(position.Hash, found);
		REQUIRE(!found);
		*entry = tte;
		TranspositionTableEntry* newEntry = tt.GetEntry(position.Hash, found);
		REQUIRE(newEntry == entry);
		REQUIRE(found);
		REQUIRE(newEntry->GetHash() == position.Hash);
		REQUIRE(newEntry->GetMove() == MOVE_NONE);
		REQUIRE(newEntry->GetDepth() == 10);
		REQUIRE(newEntry->GetScore() == 487);
		REQUIRE(newEntry->GetFlag() == UPPER_BOUND);
		REQUIRE(newEntry->GetAge() == 160);
		REQUIRE(newEntry->IsPv() == false);

		tte.Update(position.Hash, MOVE_NONE, 0, -23, EXACT, 2, true);
		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == MOVE_NONE);
		REQUIRE(tte.GetDepth() == 0);
		REQUIRE(tte.GetScore() == -23);
		REQUIRE(tte.GetFlag() == EXACT);
		REQUIRE(tte.GetAge() == 2);
		REQUIRE(tte.IsPv() == true);

		Move move = PGN::CreateMoveFromString(position, "e4");

		tte.Update(position.Hash, move, -5, SCORE_MATE - 10, EXACT, 22, true);
		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == move);
		REQUIRE(tte.GetDepth() == -5);
		REQUIRE(tte.GetScore() == SCORE_MATE - 10);
		REQUIRE(tte.GetFlag() == EXACT);
		REQUIRE(tte.GetAge() == 22);
		REQUIRE(tte.IsPv() == true);
	}

	TEST_CASE("Checks", "[Check]")
	{
		Init();
		Position position = CreatePositionFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck(TEAM_WHITE) == false);
		REQUIRE(position.InCheck(TEAM_BLACK) == false);

		position = CreatePositionFromFEN("rnbqkbnr/ppppQppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck(TEAM_WHITE) == false);
		REQUIRE(position.InCheck(TEAM_BLACK) == true);

		position = CreatePositionFromFEN("rnbqkbnr/ppppQppp/8/8/8/8/PPPPrPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck(TEAM_WHITE) == true);
		REQUIRE(position.InCheck(TEAM_BLACK) == true);

		position = CreatePositionFromFEN("rnbqkbnr/pppppppp/8/1B6/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck(TEAM_WHITE) == false);
		REQUIRE(position.InCheck(TEAM_BLACK) == false);

		Move move = UCI::CreateMoveFromString(position, "b5d7");
		ApplyMove(position, move);

		REQUIRE(position.InCheck(TEAM_WHITE) == false);
		REQUIRE(position.InCheck(TEAM_BLACK) == true);
	}

	TEST_CASE("Mirror", "[Evaluation]")
	{
		Init();

		for (const std::string& fen : {
				"r3qb2/2n2R1p/1p6/p2kBQ2/8/8/PPP2PRP/6K1 w - - 7 51",
				"r4b2/2R4p/1p6/p2kqQ2/8/8/PPP2PRP/6K1 w - - 0 52",
				"r1b2rk1/ppp2qpp/4p3/2P1Pn2/2Q5/3BP2R/PP4P1/R3K1N1 b Q - 6 21",
				"r4rk1/pppb1qpp/4p3/2P1Pn2/2Q1B3/4P2R/PP4P1/R3K1N1 b Q - 8 22",
				"rnb1kbnr/p1ppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQK2R w KQkq - 1 6",
				"rn2kbnr/pbppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQ1K1R w kq - 3 7",
				"rnb1kbnr/p1ppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQ1K1R b kq - 2 6",
				"1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -",
				"3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -",
				"2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -",
				"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -",
				"r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -",
				"2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -",
				"1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -",
				"4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -",
				"2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - -",
				"3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -",
				"2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - -",
				"r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -",
				"r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -",
				"rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - -",
				"2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -",
				"r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -",
				"r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -",
				"r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -",
				"3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -",
				"r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - -",
				"3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -",
				"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -",
				"r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq -",
				"r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -",
				"2r1r1k1/ppp3p1/2N1p2p/2nqQ3/8/P7/1PPB1PPP/3RR1K1 b - - 0 23",
				"2r3k1/1pp1r1p1/p1q1R2p/1n6/3Q4/PPBR4/2P2PPP/6K1 w - - 1 31",
				"r5k1/2p1r1p1/p1p4p/8/R7/PP4BP/2P2PP1/6K1 b - - 6 36",
				"8/3kb3/1p1r4/p1p1Q3/2P4p/4P3/P4PP1/5K2 b - - 8 43",
				"4n3/1p1q1kp1/1Q3p2/8/p4B1r/P7/1PP2P2/4R1K1 w - - 0 35",
				"r1bqk2r/pppp1ppp/2n1p3/bB1nP3/3P4/2P2N2/PP3PPP/RNBQK2R w KQkq - 1 7",
				"2k5/ppp3pp/4Kp2/2p5/2rn1r2/8/P4PPP/2R5 w - - 2 33",
				"8/8/8/4Q2p/3PK3/2k3P1/8/8 w - - 5 68",
				"4r1k1/3nbppp/bqr2B2/p7/2p5/6P1/P2N1PBP/1R1QR1K1 b - - 0 1",
				"2r2knr/2P5/1R3pp1/p4p1p/3P1B2/5N2/PP1N1PPP/3QR1K1 w - - 0 20",
				"r3r3/1ppq1ppk/p6p/3pP2Q/2bPnB1N/6R1/P1P2PPP/6RK w - - 1 30",
				"8/3b1kb1/2p2p1p/p2p4/1p3RpN/4r3/PPP3PP/3R2K1 b - - 1 32",
			})
		{
			Position position = CreatePositionFromFEN(fen);
 
			ValueType eval = Evaluate(position, position.TeamToPlay);
			REQUIRE(eval == Evaluate(MirrorPosition(position), OtherTeam(position.TeamToPlay)));
		}
	}

}
