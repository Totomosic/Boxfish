#pragma once
#include "Types.h"

namespace Boxfish
{

	struct Position;

	void InitZobristHash();

	class BOX_API ZobristHash
	{
	public:
		uint64_t Hash;

	public:
		ZobristHash();
		ZobristHash(uint64_t hash);

		void SetFromPosition(const Position& position);
		void RemovePieceAt(Team team, Piece piece, SquareIndex square);
		void AddPieceAt(Team team, Piece piece, SquareIndex square);
		void FlipTeamToPlay();
		void RemoveEnPassant(File file);
		void AddEnPassant(File file);

		void AddCastleKingside(Team team);
		void AddCastleQueenside(Team team);
		void RemoveCastleKingside(Team team);
		void RemoveCastleQueenside(Team team);

		friend ZobristHash operator^(const ZobristHash& left, const ZobristHash& right);
		ZobristHash& operator^=(const ZobristHash& right);
		friend bool operator==(const ZobristHash& left, const ZobristHash& right);
		friend bool operator!=(const ZobristHash& left, const ZobristHash& right);
	};

}
