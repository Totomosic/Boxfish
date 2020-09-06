#include "Attacks.h"

namespace Boxfish
{

    bool s_Initialized = false;

    BitBoard s_NonSlidingAttacks[TEAM_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];
    BitBoard s_SlidingAttacks[TEAM_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];

    BitBoard s_RookMasks[FILE_MAX * RANK_MAX] = { 0 };
    BitBoard s_BishopMasks[FILE_MAX * RANK_MAX] = { 0 };

    BitBoard s_RookTable[FILE_MAX * RANK_MAX][4096] = { { 0 } };
    BitBoard s_BishopTable[FILE_MAX * RANK_MAX][1024] = { { 0 } };

    BitBoard GetBlockersFromIndex(int index, BitBoard mask)
    {
        BitBoard blockers = 0ULL;
        int bits = mask.GetCount();
        for (int i = 0; i < bits; i++)
        {
            int bitPosition = PopLeastSignificantBit(mask);
            if (index & (1 << i))
            {
                blockers |= BitBoard{ 1ULL << bitPosition };
            }
        }
        return blockers;
    }

    void InitPawnAttacks()
    {
        for (int i = 0; i < FILE_MAX * RANK_MAX; i++)
        {
            BitBoard start = 1ULL << i;
            BitBoard whiteAttack = ((start << 9) & ~FILE_A_MASK) | ((start << 7) & ~FILE_H_MASK);
            BitBoard blackAttack = ((start >> 9) & ~FILE_H_MASK) | ((start >> 7) & ~FILE_A_MASK);
            s_NonSlidingAttacks[TEAM_WHITE][PIECE_PAWN][i] = whiteAttack;
            s_NonSlidingAttacks[TEAM_BLACK][PIECE_PAWN][i] = blackAttack;
        }
    }

    void InitKnightAttacks()
    {
        for (int i = 0; i < FILE_MAX * RANK_MAX; i++)
        {
            BitBoard start = 1ULL << i;
            BitBoard attack = (((start << 15) | (start >> 17)) & ~FILE_H_MASK) |    // Left 1
                (((start >> 15) | (start << 17)) & ~FILE_A_MASK) |                  // Right 1
                (((start << 6) | (start >> 10)) & ~(FILE_G_MASK | FILE_H_MASK)) |   // Left 2
                (((start >> 6) | (start << 10)) & ~(FILE_A_MASK | FILE_B_MASK));    // Right 2
            s_NonSlidingAttacks[TEAM_WHITE][PIECE_KNIGHT][i] = attack;
            s_NonSlidingAttacks[TEAM_BLACK][PIECE_KNIGHT][i] = attack;
        }
    }

    void InitKingAttacks()
    {
        for (int i = 0; i < FILE_MAX * RANK_MAX; i++)
        {
            BitBoard start = 1ULL << i;
            BitBoard attack = (((start << 7) | (start >> 9) | (start >> 1)) & (~FILE_H_MASK)) |
                (((start << 9) | (start >> 7) | (start << 1)) & (~FILE_A_MASK)) |
                ((start >> 8) | (start << 8));
            s_NonSlidingAttacks[TEAM_WHITE][PIECE_KING][i] = attack;
            s_NonSlidingAttacks[TEAM_BLACK][PIECE_KING][i] = attack;
        }
    }

    void InitRookMasks()
    {
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++) {
            s_RookMasks[square] = (GetRay(RAY_NORTH, square) & ~RANK_8_MASK) | (GetRay(RAY_SOUTH, square) & ~RANK_1_MASK) | (GetRay(RAY_EAST, square) & ~FILE_H_MASK) | (GetRay(RAY_WEST, square) & ~FILE_A_MASK);
        }
    }

    void InitBishopMasks()
    {
        BitBoard notEdges = ~(FILE_A_MASK | FILE_H_MASK | RANK_1_MASK | RANK_8_MASK);
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++) {
            s_BishopMasks[square] = ((GetRay(RAY_NORTH_EAST, square)) | (GetRay(RAY_SOUTH_EAST, square)) | (GetRay(RAY_SOUTH_WEST, square)) | (GetRay(RAY_NORTH_WEST, square))) & notEdges;
        }
    }

    BitBoard GetRookAttacksSlow(SquareIndex square, BitBoard blockers)
    {
        BitBoard attacks = 0;
        BitBoard northRay = GetRay(RAY_NORTH, square);
        attacks |= northRay;
        if (northRay & blockers)
        {
            attacks &= ~(GetRay(RAY_NORTH, ForwardBitScan(northRay & blockers)));
        }

        BitBoard southRay = GetRay(RAY_SOUTH, square);
        attacks |= southRay;
        if (southRay & blockers)
        {
            attacks &= ~(GetRay(RAY_SOUTH, BackwardBitScan(southRay & blockers)));
        }

        BitBoard eastRay = GetRay(RAY_EAST, square);
        attacks |= eastRay;
        if (eastRay & blockers)
        {
            attacks &= ~(GetRay(RAY_EAST, ForwardBitScan(eastRay & blockers)));
        }

        BitBoard westRay = GetRay(RAY_WEST, square);
        attacks |= westRay;
        if (westRay & blockers)
        {
            attacks &= ~(GetRay(RAY_WEST, BackwardBitScan(westRay & blockers)));
        }
        return attacks;
    }

    BitBoard GetBishopAttacksSlow(SquareIndex square, BitBoard blockers)
    {
        BitBoard attacks = 0;
        BitBoard northEastRay = GetRay(RAY_NORTH_EAST, square);
        attacks |= northEastRay;
        if (northEastRay & blockers)
        {
            attacks &= ~(GetRay(RAY_NORTH_EAST, ForwardBitScan(northEastRay & blockers)));
        }

        BitBoard northWestRay = GetRay(RAY_NORTH_WEST, square);
        attacks |= northWestRay;
        if (northWestRay & blockers)
        {
            attacks &= ~(GetRay(RAY_NORTH_WEST, ForwardBitScan(northWestRay & blockers)));
        }

        BitBoard southEastRay = GetRay(RAY_SOUTH_EAST, square);
        attacks |= southEastRay;
        if (southEastRay & blockers)
        {
            attacks &= ~(GetRay(RAY_SOUTH_EAST, BackwardBitScan(southEastRay & blockers)));
        }

        BitBoard southWestRay = GetRay(RAY_SOUTH_WEST, square);
        attacks |= southWestRay;
        if (southWestRay & blockers)
        {
            attacks &= ~(GetRay(RAY_SOUTH_WEST, BackwardBitScan(southWestRay & blockers)));
        }
        return attacks;
    }

    void InitRookMagicTable()
    {
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
        {
            int maxBlockers = 1 << s_RookIndexBits[square];
            for (int blockerIndex = 0; blockerIndex < maxBlockers; blockerIndex++)
            {
                BitBoard blockers = GetBlockersFromIndex(blockerIndex, s_RookMasks[square]);
                s_RookTable[square][(blockers.Board * s_RookMagics[square].Board) >> (int)(64 - s_RookIndexBits[square])] = GetRookAttacksSlow(square, blockers);
            }
        }
    }

    void InitBishopMagicTable()
    {
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
        {
            int maxBlockers = 1 << s_BishopIndexBits[square];
            for (int blockerIndex = 0; blockerIndex < maxBlockers; blockerIndex++)
            {
                BitBoard blockers = GetBlockersFromIndex(blockerIndex, s_BishopMasks[square]);
                s_BishopTable[square][(blockers.Board * s_BishopMagics[square].Board) >> (int)(64 - s_BishopIndexBits[square])] = GetBishopAttacksSlow(square, blockers);
            }
        }
    }

    BitBoard s_Lines[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

    void InitLines()
    {
        for (SquareIndex s1 = a1; s1 < FILE_MAX * RANK_MAX; s1++)
        {
            for (SquareIndex s2 = a1; s2 < FILE_MAX * RANK_MAX; s2++)
            {
                if (GetSlidingAttacks<PIECE_ROOK>(s1, 0ULL) & s2)
                    s_Lines[s1][s2] = (GetSlidingAttacks<PIECE_ROOK>(s1, 0ULL) & GetSlidingAttacks<PIECE_ROOK>(s2, 0ULL)) | s1 | s2;
                if (GetSlidingAttacks<PIECE_BISHOP>(s1, 0ULL) & s2)
                    s_Lines[s1][s2] = (GetSlidingAttacks<PIECE_BISHOP>(s1, 0ULL) & GetSlidingAttacks<PIECE_BISHOP>(s2, 0ULL)) | s1 | s2;
            }
        }
    }

    void InitAttacks()
    {
        if (!s_Initialized)
        {
            InitPawnAttacks();
            InitKnightAttacks();
            InitKingAttacks();
            InitRookMasks();
            InitBishopMasks();
            InitRookMagicTable();
            InitBishopMagicTable();
            InitLines();
            s_Initialized = true;
        }
    }

}
