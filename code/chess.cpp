#pragma once

// NOTE(hugo) : This file contains all the logic and rules 
// for a basic game of chess.

#define BOARD_COORD(P) (P).x + 8 * (P).y

#define PLACE_PIECE_AT(I, J, TypeP, ColorP)\
	Assert(!Chessboard[I + 8 * J]);\
	Chessboard[I + 8 * J] = PushStruct(Arena, chess_piece);\
	Chessboard[I + 8 * J]->Type = PieceType_##TypeP;\
	Chessboard[I + 8 * J]->Color = PieceColor_##ColorP;\

internal void
InitialiseChessboard(board_tile* Chessboard, memory_arena* Arena)
{
#if 0
	// NOTE(hugo) : White setup
	PLACE_PIECE_AT(0, 0, Rook, White);
	PLACE_PIECE_AT(1, 0, Knight, White);
	PLACE_PIECE_AT(2, 0, Bishop, White);
	PLACE_PIECE_AT(3, 0, Queen, White);
	PLACE_PIECE_AT(4, 0, King, White);
	PLACE_PIECE_AT(5, 0, Bishop, White);
	PLACE_PIECE_AT(6, 0, Knight, White);
	PLACE_PIECE_AT(7, 0, Rook, White);

	for(u32 RowIndex = 0; RowIndex < 8; ++RowIndex)
	{
		PLACE_PIECE_AT(RowIndex, 1, Pawn, White);
	}

	// NOTE(hugo) : Black setup
	PLACE_PIECE_AT(0, 7, Rook, Black);
	PLACE_PIECE_AT(1, 7, Knight, Black);
	PLACE_PIECE_AT(2, 7, Bishop, Black);
	PLACE_PIECE_AT(3, 7, Queen, Black);
	PLACE_PIECE_AT(4, 7, King, Black);
	PLACE_PIECE_AT(5, 7, Bishop, Black);
	PLACE_PIECE_AT(6, 7, Knight, Black);
	PLACE_PIECE_AT(7, 7, Rook, Black);

	for(u32 RowIndex = 0; RowIndex < 8; ++RowIndex)
	{
		PLACE_PIECE_AT(RowIndex, 6, Pawn, Black);
	}
#else
	PLACE_PIECE_AT(6, 5, Queen, White);
	PLACE_PIECE_AT(4, 6, King, White);
	PLACE_PIECE_AT(7, 0, King, Black);
	PLACE_PIECE_AT(0, 6, Pawn, White);
#endif
}

internal bool
ContainsPiece(board_tile* Chessboard, v2i TileP)
{
	chess_piece* Piece = Chessboard[TileP.x + 8 * TileP.y];
	bool Result = (Piece != 0);
	return(Result);
}

internal bool
ContainsPieceOfColor(board_tile* Chessboard, v2i TileP, piece_color Color)
{
	chess_piece* Piece = Chessboard[BOARD_COORD(TileP)];
	bool Result = (Piece != 0) && (Piece->Color == Color);
	return(Result);
}

internal bool
IsInsideBoard(v2i TileP)
{
	bool Result = (TileP.x >= 0) && (TileP.x < 8)
		&& (TileP.y >= 0) && (TileP.y < 8);
	return(Result);
}

piece_color OtherColor(piece_color Color)
{
	Assert(Color != PieceColor_Count);
	piece_color Result = (piece_color)(1 - Color);
	return(Result);
}

internal bool
IsPlayerUnderCheck(piece_color PieceColor, player_select Player)
{
	bool Result = false;
	if(PieceColor == PieceColor_White)
	{
		Result = ((PlayerSelect_White & Player) != 0);
	}
	else if(PieceColor == PieceColor_Black)
	{
		Result = ((PlayerSelect_Black & Player) != 0);
	}
	else
	{
		InvalidCodePath;
	}

	return(Result);
}

internal void
AddTile(tile_list** Sentinel, v2i TileP, move_type MoveType, memory_arena* Arena)
{
	tile_list* TileToAdd = PushStruct(Arena, tile_list);
	TileToAdd->P = TileP;
	TileToAdd->MoveType = MoveType;
	if(Sentinel)
	{
		TileToAdd->Next = (*Sentinel);
	}
	*Sentinel = TileToAdd;
}

#define ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE(P)\
	if(IsInsideBoard(P) && (!ContainsPiece(Chessboard, P))) \
	{\
			AddTile(&Sentinel, P, MoveType_Regular, Arena);\
	}

#define ADD_REGULAR_MOVE_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, Color)\
	if(IsInsideBoard(P) && (ContainsPieceOfColor(Chessboard, (P), (Color)))) \
	{\
			AddTile(&Sentinel, P, MoveType_Regular, Arena);\
	}

#define ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Color)\
	if(IsInsideBoard(P) && (!ContainsPieceOfColor(Chessboard, (P), (Color)))) \
	{\
			AddTile(&Sentinel, P, MoveType_Regular, Arena);\
	}

#define ADD_REGULAR_MOVE_IN_DIR(PieceP, Dir)\
	{\
		u32 K = 1;\
		while(IsInsideBoard(PieceP + K * Dir) && !ContainsPiece(Chessboard, PieceP + K * Dir))\
		{\
			AddTile(&Sentinel, PieceP + K * Dir, MoveType_Regular, Arena);\
			++K;\
		}\
		if(IsInsideBoard(PieceP + K * Dir))\
		{\
			v2i BlockingPieceP = PieceP + K * Dir;\
			chess_piece* BlockingPiece = Chessboard[BlockingPieceP.x + 8 * BlockingPieceP.y];\
			Assert(BlockingPiece);\
			if(BlockingPiece->Color != Piece->Color)\
			{\
				AddTile(&Sentinel, BlockingPieceP, MoveType_Regular, Arena);\
			}\
		}\
	}

enum castling_type
{
	CastlingType_KingSide,
	CastlingType_QueenSide,
};

internal tile_list*
GetAttackingTileList(chess_game_context* ChessContext, chess_piece* Piece,
		v2i PieceP, memory_arena* Arena)
{
	tile_list* Sentinel = 0;
	board_tile* Chessboard = ChessContext->Chessboard;

	Assert(Piece);
	switch(Piece->Type)
	{
		case PieceType_Pawn:
		{
			if(Piece->Color == PieceColor_White)
			{
				v2i P = V2i(PieceP.x, PieceP.y + 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE(P);

				P = V2i(PieceP.x - 1, PieceP.y + 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_Black);

				P = V2i(PieceP.x + 1, PieceP.y + 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_Black);

				if((PieceP.y == 1) && !ContainsPiece(Chessboard, V2i(PieceP.x, PieceP.y + 1)))
				{
					P = V2i(PieceP.x, PieceP.y + 2);
					if(IsInsideBoard(P) && (!ContainsPiece(Chessboard, P))) 
					{
						AddTile(&Sentinel, P, MoveType_DoubleStepPawn, Arena);
					}
				}

				if((PieceP.y == 4) && (ChessContext->LastDoubleStepCol != NO_PREVIOUS_DOUBLE_STEP))
				{
					s32 PawnXDiffs = PieceP.x - ChessContext->LastDoubleStepCol;
					if(PawnXDiffs == +1 || PawnXDiffs == -1)
					{
						P = V2i(ChessContext->LastDoubleStepCol, 5);
						AddTile(&Sentinel, P, MoveType_EnPassant, Arena);
					}
				}
			}
			else if(Piece->Color == PieceColor_Black)
			{
				v2i P = V2i(PieceP.x, PieceP.y - 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE(P);

				P = V2i(PieceP.x - 1, PieceP.y - 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_White);

				P = V2i(PieceP.x + 1, PieceP.y - 1);
				ADD_REGULAR_MOVE_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_White);

				if((PieceP.y == 6) && !ContainsPiece(Chessboard, V2i(PieceP.x, PieceP.y - 1)))
				{
					P = V2i(PieceP.x, PieceP.y - 2);
					if(IsInsideBoard(P) && (!ContainsPiece(Chessboard, P))) 
					{
						AddTile(&Sentinel, P, MoveType_DoubleStepPawn, Arena);
					}
				}

				if((PieceP.y == 3) && (ChessContext->LastDoubleStepCol != NO_PREVIOUS_DOUBLE_STEP))
				{
					s32 PawnXDiffs = PieceP.x - ChessContext->LastDoubleStepCol;
					if(PawnXDiffs == +1 || PawnXDiffs == -1)
					{
						P = V2i(ChessContext->LastDoubleStepCol, 2);
						AddTile(&Sentinel, P, MoveType_EnPassant, Arena);
					}
				}
			}
			else
			{
				InvalidCodePath;
			}
		} break;

		case PieceType_Knight:
		{
			v2i P = V2i(PieceP.x - 1, PieceP.y + 2);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 2);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 2, PieceP.y + 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 2, PieceP.y - 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y - 2);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y - 2);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 2, PieceP.y - 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 2, PieceP.y + 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

		} break;

		case PieceType_Bishop:
		{
			v2i LeftTopDir = V2i(-1, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftTopDir);

			v2i RightTopDir = V2i(+1, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightTopDir);

			v2i RightDownDir = V2i(+1, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightDownDir);

			v2i LeftDownDir = V2i(-1, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftDownDir);
		} break;

		case PieceType_Rook:
		{
			v2i LeftDir = V2i(-1, 0);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftDir);

			v2i TopDir = V2i(0, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, TopDir);

			v2i RightDir = V2i(+1, 0);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightDir);

			v2i DownDir = V2i(0, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, DownDir);
		} break;

		case PieceType_Queen:
		{
			v2i LeftTopDir = V2i(-1, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftTopDir);

			v2i RightTopDir = V2i(+1, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightTopDir);

			v2i RightDownDir = V2i(+1, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightDownDir);

			v2i LeftDownDir = V2i(-1, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftDownDir);

			v2i LeftDir = V2i(-1, 0);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, LeftDir);

			v2i TopDir = V2i(0, +1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, TopDir);

			v2i RightDir = V2i(+1, 0);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, RightDir);

			v2i DownDir = V2i(0, -1);
			ADD_REGULAR_MOVE_IN_DIR(PieceP, DownDir);
		} break;

		case PieceType_King:
		{
			v2i P = V2i(PieceP.x - 1, PieceP.y - 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y + 0);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y + 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 0, PieceP.y - 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 0, PieceP.y + 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y - 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 0);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 1);
			ADD_REGULAR_MOVE_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);
		} break;

		InvalidDefaultCase;
	}

	return(Sentinel);
}

internal bool
IsChessboardCleanForCastling(chess_game_context* ChessContext, piece_color Color, castling_type CastlingType, memory_arena* Arena)
{
	bool IsClean = true;

	u32 LowX = 0;
	u32 HighX = 0;
	if(CastlingType == CastlingType_KingSide)
	{
		LowX = 5;
		HighX = 7;
	}
	else if(CastlingType == CastlingType_QueenSide)
	{
		LowX = 1;
		HighX = 4;
	}
	else
	{
		InvalidCodePath;
	}


	u32 SquareY = (Color == PieceColor_White) ? 0 : 7;
	for(u32 SquareX = LowX; IsClean && (SquareX < HighX); ++SquareX)
	{
		chess_piece* Piece = ChessContext->Chessboard[SquareX + 8 * SquareY];
		if(Piece)
		{
			IsClean = false;
		}
	}
#ifdef CASTLING_PRINT_DEBUG
	if(!IsClean)
	{
		printf("Cannot do a castle because there are piece between the rook and the king\n");
	}
#endif

	for(u32 SquareY = 0; IsClean && (SquareY < 8); ++SquareY)
	{
		for(u32 SquareX = 0; IsClean && (SquareX < 8); ++SquareX)
		{
			chess_piece* Piece = ChessContext->Chessboard[SquareX + 8 * SquareY];
			if(Piece && (Piece->Color == OtherColor(Color)))
			{
				tile_list* PieceAttackingSquareSentinel =
					GetAttackingTileList(ChessContext, Piece, V2i(SquareX, SquareY), Arena);
				for(tile_list* AttackedSquare = PieceAttackingSquareSentinel;
						IsClean && AttackedSquare;
						AttackedSquare = AttackedSquare->Next)
				{
					v2i AttackedSquareP = AttackedSquare->P;

					u32 LowBoundX = 0;
					u32 HighBoundX = 0;

					if(CastlingType == CastlingType_KingSide)
					{
						LowBoundX = 5;
						HighBoundX = 6;
					}
					else if(CastlingType == CastlingType_QueenSide)
					{
						LowBoundX = 2;
						HighBoundX = 3;
					}
					else
					{
						InvalidCodePath;
					}

					if((u32(AttackedSquareP.x) >= LowBoundX) && (u32(AttackedSquareP.x) <= HighBoundX))
					{
						if((Color == PieceColor_White) && (AttackedSquareP.y == 0))
						{
							IsClean = false;
						}
						else if((Color == PieceColor_Black) && (AttackedSquareP.y == 7))
						{
							IsClean = false;
						}
					}
				}
			}
		}
	}

#ifdef CASTLING_PRINT_DEBUG
	if(!IsClean)
	{
		printf("Cannot castle because there are attacking piece on the way !\n");
	}
#endif

	return(IsClean);
}

internal tile_list*
GetPossibleMoveList(chess_game_context* ChessContext, chess_piece* Piece,
		v2i PieceP, memory_arena* Arena)
{
	// NOTE(hugo) : Getting the list of possible move for the piece is the same
	// as getting the list of its attacking square. The main difference is with the king
	// because it can do a castling but that does not mean it is attacking the square.
	// Therefore the disctiction.
	tile_list* Sentinel = GetAttackingTileList(ChessContext, Piece, PieceP, Arena);
	if(Piece->Type == PieceType_King)
	{
		/* NOTE(hugo) : From wikipedia, the rules for castling are the following :
			* The king and the chosen rook are on the player's first rank.
			* Neither the king nor the chosen rook has previously moved.
			* There are no pieces between the king and the chosen rook.
			* The king is not currently in check.
			* The king does not pass through a square that is attacked by an enemy piece.
			* The king does not end up in check. (True of any legal move.)
		*/
		if((!IsPlayerUnderCheck(Piece->Color, ChessContext->PlayerCheck)) &&
				(!ChessContext->CastlingPieceTracker[Piece->Color].KingHasMoved))
		{
			if(ChessContext->CastlingPieceTracker[Piece->Color].QueenRook.IsFirstRank &&
					(!ChessContext->CastlingPieceTracker[Piece->Color].QueenRook.HasMoved))
			{
				// NOTE(hugo) : Queen side castling
				if(IsChessboardCleanForCastling(ChessContext, Piece->Color, CastlingType_QueenSide, Arena))
				{
					v2i TileP = V2i(2, Piece->Color == PieceColor_White ? 0 : 7);
					AddTile(&Sentinel, TileP, MoveType_CastlingQueenSide, Arena);
				}
			}
#ifdef CASTLING_PRINT_DEBUG
			else
			{
				printf("Cannot Queen castle because the queen rook has moved or is not first rank\n");
			}
#endif

			if(ChessContext->CastlingPieceTracker[Piece->Color].KingRook.IsFirstRank &&
					(!ChessContext->CastlingPieceTracker[Piece->Color].KingRook.HasMoved))
			{
				// NOTE(hugo) : King side castling
				if(IsChessboardCleanForCastling(ChessContext, Piece->Color, CastlingType_KingSide, Arena))
				{
					v2i TileP = V2i(6, Piece->Color == PieceColor_White ? 0 : 7);
					AddTile(&Sentinel, TileP, MoveType_CastlingKingSide, Arena);
				}
			}
#ifdef CASTLING_PRINT_DEBUG
			else
			{
				printf("Cannot King castle because the king rook has moved or is not first rank\n");
			}
#endif
		}
#ifdef CASTLING_PRINT_DEBUG
		else
		{
			if(IsPlayerUnderCheck(Piece->Color, ChessContext->PlayerCheck))
			{
				printf("Cannot castle because player under check\n");
			}
			else
			{
				Assert((ChessContext->CastlingPieceTracker[Piece->Color].KingHasMoved));
				printf("Cannot castle because king has moved !\n");
			}
		}
#endif
	}

	return(Sentinel);
}

union kings_positions
{
	struct
	{
		v2i WhiteP;
		v2i BlackP;
	};
	v2i KingsP[2];
};

kings_positions FindKingsPositions(board_tile* Chessboard)
{
	bool OneKingFound = false;
	kings_positions Result = {};
	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
		{
			chess_piece* Piece = Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				if(Piece->Type == PieceType_King)
				{
					Result.KingsP[Piece->Color] = V2i(SquareX, SquareY);
					if(OneKingFound)
					{
						// NOTE(hugo): Both found, return.
						break;
					}
					else
					{
						OneKingFound = true;
					}
				}
			}
		}
	}

	return(Result);
}

internal player_select
SearchForKingCheck(chess_game_context* ChessContext, memory_arena* Arena)
{
	player_select Result = PlayerSelect_None;

	// TODO(hugo) : Maybe cache the result if time-critical ?
	kings_positions KingsPositions = FindKingsPositions(ChessContext->Chessboard);
	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
		{
			chess_piece* Piece = ChessContext->Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				temporary_memory TileListTempMemory = BeginTemporaryMemory(Arena);

				tile_list* AttackingTileList = GetAttackingTileList(ChessContext, Piece, V2i(SquareX, SquareY), Arena);
				for(tile_list* Tile = AttackingTileList;
					           Tile;
					           Tile = Tile->Next)
				{
					if((Piece->Color == PieceColor_Black) && (Tile->P.x == KingsPositions.WhiteP.x) && (Tile->P.y == KingsPositions.WhiteP.y))
					{
						Result = player_select(Result | PlayerSelect_White);
						break;
					}
					else if((Piece->Color == PieceColor_White) && (Tile->P.x == KingsPositions.BlackP.x) && (Tile->P.y == KingsPositions.BlackP.y))
					{
						Result = player_select(Result | PlayerSelect_Black);
						break;
					}
				}
				EndTemporaryMemory(TileListTempMemory);
			}
		}
	}

	return(Result);
}

internal bool
IsPlayerCheckmate(chess_game_context* ChessContext, piece_color PlayerColor, memory_arena* Arena)
{
	board_tile* Chessboard = ChessContext->Chessboard;
	bool SavingMoveFound = false;
	for(u32 SquareY = 0; (!SavingMoveFound) && (SquareY < 8); ++SquareY)
	{
		for(u32 SquareX = 0; (!SavingMoveFound) && (SquareX < 8); ++SquareX)
		{
			chess_piece* Piece = Chessboard[SquareX + 8 * SquareY];
			if(Piece && (Piece->Color == PlayerColor))
			{
				temporary_memory CheckMateTempMemory = BeginTemporaryMemory(Arena);

				v2i PieceP = V2i(SquareX, SquareY);
				tile_list* PossibleMoves = GetAttackingTileList(ChessContext, Piece, PieceP, Arena);
				for(tile_list* Move = PossibleMoves;
						Move;
						Move = Move->Next)
				{
					v2i PieceDest = Move->P;
					chess_piece* OldPieceAtDestSave = Chessboard[BOARD_COORD(PieceDest)];
					Chessboard[BOARD_COORD(PieceP)] = 0;
					Chessboard[BOARD_COORD(PieceDest)] = Piece;
					player_select NewCheckPlayer = SearchForKingCheck(ChessContext, Arena);
					if(!IsPlayerUnderCheck(PlayerColor, NewCheckPlayer))
					{
						SavingMoveFound = true;
					}

					// NOTE(hugo) : Putting it back like it was before
					Chessboard[BOARD_COORD(PieceP)] = Piece;
					Chessboard[BOARD_COORD(PieceDest)] = OldPieceAtDestSave;
				}

				EndTemporaryMemory(CheckMateTempMemory);
			}
		}
	}

	return(!SavingMoveFound);
}
 
internal void
DeleteInvalidMoveDueToCheck(chess_game_context* ChessContext, chess_piece* Piece, v2i PieceP, tile_list** MoveSentinel, piece_color CheckPlayer, memory_arena* Arena)
{
	Assert(MoveSentinel);
	Assert(Piece);
	board_tile* Chessboard = ChessContext->Chessboard;
	piece_color PlayerMovingColor = Piece->Color;
	tile_list* PreviousMove = 0;
	for(tile_list* CurrentMove = *MoveSentinel; CurrentMove; CurrentMove = CurrentMove->Next)
	{
		bool DeleteMove = false;

		v2i PieceDest = CurrentMove->P;
		chess_piece* OldPieceAtDestSave = Chessboard[BOARD_COORD(PieceDest)];

		// NOTE(hugo) : Apply the move
		switch(CurrentMove->MoveType)
		{
			case MoveType_Regular:
			case MoveType_DoubleStepPawn:
				{
					Chessboard[BOARD_COORD(PieceP)] = 0;
					Chessboard[BOARD_COORD(PieceDest)] = Piece;
				} break;

			case MoveType_CastlingQueenSide:
				{
					s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
					chess_piece* QueenRook = Chessboard[BOARD_COORD(V2i(0, LineIndex))];
					Assert(QueenRook->Type == PieceType_Rook);
					Chessboard[BOARD_COORD(PieceP)] = 0;
					Assert(Chessboard[BOARD_COORD(PieceDest)] == 0);
					Chessboard[BOARD_COORD(PieceDest)] = Piece;
					Chessboard[BOARD_COORD(V2i(0, LineIndex))] = 0;
					Chessboard[BOARD_COORD(V2i(3, LineIndex))] = QueenRook;

					// NOTE(hugo) : We do not do this is because it is just a test and does not affect the chess context
					//ChessContext->CastlingPieceTracker[PlayerMovingColor].KingHasMoved = true;
					//ChessContext->CastlingPieceTracker[PlayerMovingColor].QueenRook.HasMoved = true;
				} break;

			case MoveType_CastlingKingSide:
				{
					s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
					chess_piece* KingRook = Chessboard[BOARD_COORD(V2i(7, LineIndex))];
					Assert(KingRook->Type == PieceType_Rook);
					Chessboard[BOARD_COORD(PieceP)] = 0;
					Assert(Chessboard[BOARD_COORD(PieceDest)] == 0);
					Chessboard[BOARD_COORD(PieceDest)] = Piece;
					Chessboard[BOARD_COORD(V2i(7, LineIndex))] = 0;
					Chessboard[BOARD_COORD(V2i(5, LineIndex))] = KingRook;
				} break;

			case MoveType_EnPassant:
				{
					if(PlayerMovingColor == PieceColor_White)
					{
						Assert(u32(PieceDest.x) == ChessContext->LastDoubleStepCol);
						Assert(PieceDest.y == 5);
						Assert(!Chessboard[ChessContext->LastDoubleStepCol + 8 * 5]);

						OldPieceAtDestSave = Chessboard[ChessContext->LastDoubleStepCol + 8 * 4];

						Assert(OldPieceAtDestSave);
						Assert(OldPieceAtDestSave->Type == PieceType_Pawn);
						Assert(OldPieceAtDestSave->Color == PieceColor_Black);

						Chessboard[ChessContext->LastDoubleStepCol + 8 * 4] = 0;
						Chessboard[ChessContext->LastDoubleStepCol + 8 * 5] = Piece;
						Chessboard[BOARD_COORD(PieceP)] = 0;
					}
					else if(PlayerMovingColor == PieceColor_Black)
					{
						Assert(u32(PieceDest.x) == ChessContext->LastDoubleStepCol);
						Assert(PieceDest.y == 2);
						Assert(!Chessboard[ChessContext->LastDoubleStepCol + 8 * 2]);

						OldPieceAtDestSave = Chessboard[ChessContext->LastDoubleStepCol + 8 * 3];

						Assert(OldPieceAtDestSave);
						Assert(OldPieceAtDestSave->Type == PieceType_Pawn);
						Assert(OldPieceAtDestSave->Color == PieceColor_White);

						Chessboard[ChessContext->LastDoubleStepCol + 8 * 3] = 0;
						Chessboard[ChessContext->LastDoubleStepCol + 8 * 2] = Piece;
						Chessboard[BOARD_COORD(PieceP)] = 0;
					}
					else
					{
						InvalidCodePath;
					}
				} break;

			InvalidDefaultCase;
		}

		player_select NewCheckPlayer = SearchForKingCheck(ChessContext, Arena);
		if(IsPlayerUnderCheck(CheckPlayer, NewCheckPlayer))
		{
			if(PreviousMove)
			{
				PreviousMove->Next = CurrentMove->Next;
			}
			else
			{
				*MoveSentinel = CurrentMove->Next;
			}
			DeleteMove = true;
		}

		// NOTE(hugo) : Putting it back like it was before
		switch(CurrentMove->MoveType)
		{
			case MoveType_Regular:
			case MoveType_DoubleStepPawn:
				{
					Chessboard[BOARD_COORD(PieceP)] = Piece;
					Chessboard[BOARD_COORD(PieceDest)] = OldPieceAtDestSave;
				} break;

			case MoveType_CastlingQueenSide:
				{
					s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
					chess_piece* QueenRook = Chessboard[BOARD_COORD(V2i(3, LineIndex))];
					Assert(QueenRook->Type == PieceType_Rook);
					Chessboard[BOARD_COORD(PieceP)] = Piece;
					Chessboard[BOARD_COORD(PieceDest)] = 0;
					Chessboard[BOARD_COORD(V2i(3, LineIndex))] = 0;
					Chessboard[BOARD_COORD(V2i(0, LineIndex))] = QueenRook;
				} break;

			case MoveType_CastlingKingSide:
				{
					s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
					chess_piece* KingRook = Chessboard[BOARD_COORD(V2i(5, LineIndex))];
					Assert(KingRook->Type == PieceType_Rook);
					Chessboard[BOARD_COORD(PieceP)] = Piece;
					Chessboard[BOARD_COORD(PieceDest)] = 0;
					Chessboard[BOARD_COORD(V2i(5, LineIndex))] = 0;
					Chessboard[BOARD_COORD(V2i(7, LineIndex))] = KingRook;
				} break;
			case MoveType_EnPassant:
				{
					if(PlayerMovingColor == PieceColor_White)
					{
						Chessboard[BOARD_COORD(PieceP)] = Piece;
						Chessboard[BOARD_COORD(PieceDest)] = 0;
						Chessboard[ChessContext->LastDoubleStepCol + 8 * 4] = OldPieceAtDestSave;
					}
					else if(PlayerMovingColor == PieceColor_Black)
					{
						Chessboard[BOARD_COORD(PieceP)] = Piece;
						Chessboard[BOARD_COORD(PieceDest)] = 0;
						Chessboard[ChessContext->LastDoubleStepCol + 8 * 3] = OldPieceAtDestSave;
					}
					else
					{
						InvalidCodePath;
					}
				} break;

			InvalidDefaultCase;

		}

		if(!DeleteMove)
		{
			PreviousMove = CurrentMove;
		}
	}
}

internal bool
ConfigMatch(chessboard_config* ConfigA, chessboard_config* ConfigB)
{
	bool Match = true;
	for(u32 TileIndex = 0;
			Match && (TileIndex < ArrayCount(ConfigA->Tiles));
			++TileIndex)
	{
		u8 TileA = ConfigA->Tiles[TileIndex];
		u8 TileB = ConfigB->Tiles[TileIndex];
		Match = (TileA == TileB);
	}

	return(Match);
}

// TODO(hugo) : Test this function
internal bool
IsDraw(chess_game_context* ChessContext, memory_arena* Arena)
{
	/* NOTE(hugo) : From Wikipedia :
	 The game ends in a draw if any of these conditions occur:
		* The game is automatically a draw if the player to move is not in check but has no legal move. This situation is called a stalemate. An example of such a position is shown in the adjacent diagram.
		* The game is immediately drawn when there is no possibility of checkmate for either side with any series of legal moves. This draw is often due to insufficient material, including the endgames
			- king against king;
			- king against king and bishop;
			- king against king and knight;
			- king and bishop against king and bishop, with both bishops on squares of the same color;
		* Both players agree to a draw after one of the players makes such an offer.
		* The player having the move may claim a draw by declaring that one of the following conditions exists, or by declaring an intention to make a move which will bring about one of these conditions:
		* Fifty-move rule: There has been no capture or pawn move in the last fifty moves by each player.
		* Threefold repetition: The same board position has occurred three times with the same player to move and all pieces having the same rights to move, including the right to castle or capture en passant.
	*/

	bool DrawCaseFound = false;

	// NOTE(hugo) : Checking for no possibility of checkmate
	// NOTE(hugo) : The following code is very verbose, but at least
	// it's understandable.
	bool WhiteDiagWhiteBishopPresent = false;
	bool BlackDiagWhiteBishopPresent = false;
	bool WhiteDiagBlackBishopPresent = false;
	bool BlackDiagBlackBishopPresent = false;
	u32 WhiteKnightPresent = 0;
	u32 BlackKnightPresent = 0;
	bool OneWhiteBishopPresent = false;
	bool OneBlackBishopPresent = false;
	bool PieceOtherThanKingBishopKnightPresent = false;
	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
		{
			chess_piece* Piece = ChessContext->Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				switch(Piece->Type)
				{
					case PieceType_Pawn:
					case PieceType_Rook:
					case PieceType_Queen:
						{
							PieceOtherThanKingBishopKnightPresent = true;
						} break;
					case PieceType_Knight:
						{
							if(Piece->Color == PieceColor_White)
							{
								++WhiteKnightPresent;
							}
							else if(Piece->Color == PieceColor_Black)
							{
								++BlackKnightPresent;
							}
							else
							{
								InvalidCodePath;
							}
						} break;
					case PieceType_Bishop:
						{
							if(Piece->Color == PieceColor_White)
							{
								bool IsWhiteTile = (SquareX + SquareY) % 2 != 0;
								if(IsWhiteTile)
								{
									WhiteDiagWhiteBishopPresent = true;
								}
								else
								{
									BlackDiagWhiteBishopPresent = true;
								}
							}
							else if(Piece->Color == PieceColor_Black)
							{
								bool IsWhiteTile = (SquareX + SquareY) % 2 != 0;
								if(IsWhiteTile)
								{
									WhiteDiagBlackBishopPresent = true;
								}
								else
								{
									BlackDiagBlackBishopPresent = true;
								}
							}
							else
							{
								InvalidCodePath;
							}
						} break;
					case PieceType_King:
						{
						} break;
					InvalidDefaultCase;
				}
			}
		}
	}

	OneBlackBishopPresent = (WhiteDiagBlackBishopPresent && !BlackDiagBlackBishopPresent) ||
		(!WhiteDiagBlackBishopPresent && BlackDiagBlackBishopPresent);
	OneWhiteBishopPresent = (WhiteDiagWhiteBishopPresent && !BlackDiagWhiteBishopPresent) ||
		(!WhiteDiagWhiteBishopPresent && BlackDiagWhiteBishopPresent);
	bool WhiteBishopPresent = WhiteDiagWhiteBishopPresent || BlackDiagWhiteBishopPresent;
	bool BlackBishopPresent = WhiteDiagBlackBishopPresent || BlackDiagBlackBishopPresent;

	bool KnightPresent = (WhiteKnightPresent > 0) && (BlackKnightPresent > 0);
	bool BishopPresent = WhiteBishopPresent || BlackBishopPresent;

	bool OnlyOneBishop = (WhiteBishopPresent && !BlackBishopPresent) ||
		(BlackBishopPresent && !WhiteBishopPresent);

	bool OnlyOneKnight = (WhiteKnightPresent + BlackKnightPresent == 1);

	bool SameColorBishopPresent = (WhiteDiagWhiteBishopPresent && WhiteDiagBlackBishopPresent && (!BlackDiagWhiteBishopPresent) && (!BlackDiagBlackBishopPresent)) ||
		(BlackDiagWhiteBishopPresent && BlackDiagBlackBishopPresent && (!WhiteDiagWhiteBishopPresent) && (!WhiteDiagBlackBishopPresent));

	if((!PieceOtherThanKingBishopKnightPresent) && (!KnightPresent) && (!BishopPresent))
	{
		DrawCaseFound = true;
	}
	else if((!PieceOtherThanKingBishopKnightPresent) && (!KnightPresent) && (OnlyOneBishop))
	{
		DrawCaseFound = true;
	}
	else if((!PieceOtherThanKingBishopKnightPresent) && (!BishopPresent) && (OnlyOneKnight))
	{
		DrawCaseFound = true;
	}
	else if((!PieceOtherThanKingBishopKnightPresent) && (!KnightPresent) && SameColorBishopPresent)
	{
		DrawCaseFound = true;
	}

	// NOTE(hugo) : Checking for stalemate
	bool ValidMoveFoundForCurrentPlayer = false;
	piece_color CurrentPlayer = OtherColor(ChessContext->PlayerToPlay); // NOTE(hugo) : We check the next player to play
	for(u32 SquareY = 0; !ValidMoveFoundForCurrentPlayer && (SquareY < 8); ++SquareY)
	{
		for(u32 SquareX = 0; !ValidMoveFoundForCurrentPlayer && (SquareX < 8); ++SquareX)
		{
			chess_piece* Piece = ChessContext->Chessboard[SquareX + 8 * SquareY];
			if(Piece && Piece->Color == CurrentPlayer)
			{
				temporary_memory AttackingListTempMemory = BeginTemporaryMemory(Arena);

				tile_list* AttackingTileList = GetAttackingTileList(ChessContext, 
						Piece, V2i(SquareX, SquareY), Arena);
				if(AttackingTileList)
				{
					DeleteInvalidMoveDueToCheck(ChessContext, Piece, V2i(SquareX, SquareY),
							&AttackingTileList, CurrentPlayer, Arena);
				}
				if(AttackingTileList)
				{
					// NOTE(hugo): If there is still at least a move after this, 
					// then we have found a valid move.
					ValidMoveFoundForCurrentPlayer = true;
				}

				EndTemporaryMemory(AttackingListTempMemory);
			}
		}
	}

	if(!ValidMoveFoundForCurrentPlayer)
	{
		DrawCaseFound = true;
	}

	// NOTE(hugo) : Checking for threefold repetition
	// NOTE(hugo) : Since the current configuration was added 
	// before checking for the draw, the current config is just the
	// sentinel of the current context
	chessboard_config CurrentConfig = ChessContext->ChessboardConfigSentinel->Config;
	u32 ConfigMatchCounter = 0;
	for(chessboard_config_list* CurrentTestConfig = ChessContext->ChessboardConfigSentinel->Next;
			!DrawCaseFound && CurrentTestConfig;
			CurrentTestConfig = CurrentTestConfig->Next)
	{
		chessboard_config TestConfig = CurrentTestConfig->Config;
		if(ConfigMatch(&TestConfig, &CurrentConfig))
		{
			++ConfigMatchCounter;
			if(ConfigMatchCounter >= 2)
			{
				// NOTE(hugo) : We found two _other_ identical configurations
				// Therefore this is the third one. This is a draw.
				DrawCaseFound = true;
			}
		}
	}

	return(DrawCaseFound);
}

internal void
InitialiseChessContext(chess_game_context* ChessContext, memory_arena* Arena)
{
	InitialiseChessboard(ChessContext->Chessboard, Arena);
	ChessContext->ChessboardConfigSentinel = 0;

	// NOTE(hugo) : We need to check that we are not in a custom config.
	chess_piece* WhiteKingRook = ChessContext->Chessboard[0 + 8 * 0];
	chess_piece* BlackKingRook = ChessContext->Chessboard[0 + 8 * 7];
	chess_piece* BlackQueenRook = ChessContext->Chessboard[7 + 8 * 7];

	ChessContext->CastlingPieceTracker[PieceColor_White].KingHasMoved = false;

	chess_piece* WhiteQueenRook = ChessContext->Chessboard[7 + 8 * 0];
	ChessContext->CastlingPieceTracker[PieceColor_White].QueenRook.IsFirstRank = (WhiteQueenRook && WhiteQueenRook->Type == PieceType_Rook && WhiteQueenRook->Color == PieceColor_White);
	ChessContext->CastlingPieceTracker[PieceColor_White].QueenRook.HasMoved = false;

	ChessContext->CastlingPieceTracker[PieceColor_White].KingRook.IsFirstRank = (WhiteKingRook && WhiteKingRook->Type == PieceType_Rook && WhiteKingRook->Color == PieceColor_White);
	ChessContext->CastlingPieceTracker[PieceColor_White].KingRook.HasMoved = false;

	ChessContext->CastlingPieceTracker[PieceColor_Black].KingHasMoved = false;
	ChessContext->CastlingPieceTracker[PieceColor_Black].QueenRook.IsFirstRank = (BlackQueenRook && BlackQueenRook->Type == PieceType_Rook && BlackQueenRook->Color == PieceColor_Black);
	ChessContext->CastlingPieceTracker[PieceColor_Black].QueenRook.HasMoved = false;
	ChessContext->CastlingPieceTracker[PieceColor_Black].KingRook.IsFirstRank = (BlackKingRook && BlackKingRook->Type == PieceType_Rook && BlackKingRook->Color == PieceColor_Black);
	ChessContext->CastlingPieceTracker[PieceColor_Black].KingRook.HasMoved = false;
}

u8 SafeTruncateU32ToU8(u32 Value)
{
	Assert(Value <= 0xFF);
	u8 Result = (Value & 0xFF);

	return(Result);
}

internal chessboard_config
WriteConfig(board_tile* Chessboard)
{
	chessboard_config Result = {};
	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
		{
			u32 SquareIndex = SquareX + 8 * SquareY;
			Result.Tiles[SquareIndex] = u8(0x0000);
			chess_piece* Piece = Chessboard[SquareIndex];
			if(Piece)
			{
				Result.Tiles[SquareIndex] = SafeTruncateU32ToU8(1 + Piece->Type + PieceType_Count * Piece->Color);
			}
		}
	}
	return(Result);
}

internal void
ApplyMove(chess_game_context* ChessContext, move_params MoveParams, memory_arena* Arena)
{
	move_type MoveType = MoveParams.Type;
	v2i InitialP = MoveParams.InitialP;
	v2i DestP = MoveParams.DestP;
	switch(MoveType)
	{
		case MoveType_Regular:
		case MoveType_DoubleStepPawn:
			{
				chess_piece* SelectedPiece = ChessContext->Chessboard[BOARD_COORD(InitialP)];
				chess_piece* EatenPiece = ChessContext->Chessboard[BOARD_COORD(DestP)];
				Assert(SelectedPiece);
				ChessContext->Chessboard[BOARD_COORD(InitialP)] = 0;
				ChessContext->Chessboard[BOARD_COORD(DestP)] = SelectedPiece;
				if((SelectedPiece->Type == PieceType_Pawn) && 
						((ChessContext->PlayerToPlay == PieceColor_White && DestP.y == 7)||
						  (ChessContext->PlayerToPlay == PieceColor_Black && DestP.y == 0)))
				{
					// TODO(hugo) : Reverting to automatic promotion to queen for now.
					// Because of networking issues
					chess_piece* PromotedPawn = ChessContext->Chessboard[BOARD_COORD(DestP)];
					//GameState->UserMode = UserMode_PromotePawn;
					//GameState->PawnToPromote = PromotedPawn;
					PromotedPawn->Type = PieceType_Queen;
				}


				// TODO(hugo) : Make sure we chage the castling_state
				// of the player if a rook or the king is moved
				// maybe a possibility to do this efficiently would be :
				// in the chess_game_context track the exact location
				// of each piece (or a pointer to them) so that,
				// if a pawn is promoted to a tower, we don't have issues
				// TODO(hugo) : NOTE(hugo) : This is not fully exhaustive because it does not take into account the fact
				// that a rook could have been into.
				// What I mean is that we don't need to have an exact castling_piece_tracker since, if some parameters are
				// correct, then the castling will be disabled for a player. Therefore it is useless to have the other
				// parameters be exact since the firsts are already meaningful to the decision.
				castling_piece_tracker* CastlingPieceTrackerPlayer = ChessContext->CastlingPieceTracker + ChessContext->PlayerToPlay;
				if(SelectedPiece->Type == PieceType_King)
				{
					CastlingPieceTrackerPlayer->KingHasMoved = true;
				}
				if(SelectedPiece->Type == PieceType_Rook)
				{
					v2i SelectedPieceP = InitialP;
					if((SelectedPieceP.x == 0) &&
							((ChessContext->PlayerToPlay == PieceColor_White) && (InitialP.y == 0) ||
							 (ChessContext->PlayerToPlay == PieceColor_Black) && (InitialP.y == 7)))
					{
						CastlingPieceTrackerPlayer->QueenRook.HasMoved = true;
					}
					else if((SelectedPieceP.x == 7) &&
							((ChessContext->PlayerToPlay == PieceColor_White) && (InitialP.y == 0) ||
							 (ChessContext->PlayerToPlay == PieceColor_Black) && (InitialP.y == 7)))
					{
						CastlingPieceTrackerPlayer->KingRook.HasMoved = true;
					}
				}
				if(EatenPiece && EatenPiece->Type == PieceType_Rook)
				{
					v2i SelectedPieceP = DestP;
					if((SelectedPieceP.x == 0) &&
							((ChessContext->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
							 (ChessContext->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
					{
						CastlingPieceTrackerPlayer->QueenRook.IsFirstRank = true;
					}
					else if((SelectedPieceP.x == 7) &&
							((ChessContext->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
							 (ChessContext->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
					{
						CastlingPieceTrackerPlayer->KingRook.IsFirstRank = true;
					}
				}
			} break;
		case MoveType_CastlingKingSide:
			{
				piece_color PlayerMovingColor = ChessContext->PlayerToPlay;
				s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
				chess_piece* KingRook = ChessContext->Chessboard[BOARD_COORD(V2i(7, LineIndex))];
				Assert(KingRook->Type == PieceType_Rook);
				v2i PieceP = InitialP;
				v2i PieceDestP = DestP;
				chess_piece* King = ChessContext->Chessboard[BOARD_COORD(PieceP)];
				Assert(King->Type == PieceType_King);
				ChessContext->Chessboard[BOARD_COORD(PieceP)] = 0;
				Assert(ChessContext->Chessboard[BOARD_COORD(PieceDestP)] == 0);
				ChessContext->Chessboard[BOARD_COORD(PieceDestP)] = King;
				ChessContext->Chessboard[BOARD_COORD(V2i(7, LineIndex))] = 0;
				ChessContext->Chessboard[BOARD_COORD(V2i(5, LineIndex))] = KingRook;

				// NOTE(hugo) : Disabling future castling possibilities
				castling_piece_tracker* CastlingPieceTrackerPlayer = ChessContext->CastlingPieceTracker + ChessContext->PlayerToPlay;
				CastlingPieceTrackerPlayer->KingHasMoved = true;
			} break;
		case MoveType_CastlingQueenSide:
			{
				piece_color PlayerMovingColor = ChessContext->PlayerToPlay;
				s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
				chess_piece* QueenRook = ChessContext->Chessboard[BOARD_COORD(V2i(0, LineIndex))];
				Assert(QueenRook->Type == PieceType_Rook);
				v2i PieceP = InitialP;
				v2i PieceDestP = DestP;
				chess_piece* King = ChessContext->Chessboard[BOARD_COORD(PieceP)];
				Assert(King->Type == PieceType_King);
				ChessContext->Chessboard[BOARD_COORD(PieceP)] = 0;
				Assert(ChessContext->Chessboard[BOARD_COORD(PieceDestP)] == 0);
				ChessContext->Chessboard[BOARD_COORD(PieceDestP)] = King;
				ChessContext->Chessboard[BOARD_COORD(V2i(0, LineIndex))] = 0;
				ChessContext->Chessboard[BOARD_COORD(V2i(3, LineIndex))] = QueenRook;

				// NOTE(hugo) : Disabling future castling possibilities
				castling_piece_tracker* CastlingPieceTrackerPlayer = ChessContext->CastlingPieceTracker + ChessContext->PlayerToPlay;
				CastlingPieceTrackerPlayer->KingHasMoved = true;
			} break;
		case MoveType_EnPassant:
			{
				chess_piece* SelectedPiece = ChessContext->Chessboard[BOARD_COORD(InitialP)];
				Assert(SelectedPiece);
				piece_color PlayerMovingColor = ChessContext->PlayerToPlay;
				v2i PieceDestP = DestP;
				v2i PieceP = InitialP;
				if(PlayerMovingColor == PieceColor_White)
				{
					Assert(u32(PieceDestP.x) == ChessContext->LastDoubleStepCol);
					Assert(PieceDestP.y == 5);
					Assert(!ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 5]);

					ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 4] = 0;
					ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 5] = SelectedPiece;
					ChessContext->Chessboard[BOARD_COORD(PieceP)] = 0;
				}
				else if(PlayerMovingColor == PieceColor_Black)
				{
					Assert(u32(PieceDestP.x) == ChessContext->LastDoubleStepCol);
					Assert(PieceDestP.y == 2);
					Assert(!ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 2]);

					ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 3] = 0;
					ChessContext->Chessboard[ChessContext->LastDoubleStepCol + 8 * 2] = SelectedPiece;
					ChessContext->Chessboard[BOARD_COORD(PieceP)] = 0;
				}
				else
				{
					InvalidCodePath;
				}
			} break;
		InvalidDefaultCase;
	}
	ChessContext->LastDoubleStepCol = (MoveType == MoveType_DoubleStepPawn) ? DestP.x : NO_PREVIOUS_DOUBLE_STEP;

	// NOTE(hugo): Update the config list
	chessboard_config_list* NewChessboardConfigList = 
		PushStruct(Arena, chessboard_config_list);
	NewChessboardConfigList->Config = WriteConfig(ChessContext->Chessboard);
	NewChessboardConfigList->Next = ChessContext->ChessboardConfigSentinel;
	ChessContext->ChessboardConfigSentinel = NewChessboardConfigList;

	// NOTE(hugo) : Resolve situation for the other player
	ChessContext->PlayerCheck = SearchForKingCheck(ChessContext, Arena);
	bool IsCurrentPlayerCheckmate = false;
	if(IsPlayerUnderCheck(OtherColor(ChessContext->PlayerToPlay), ChessContext->PlayerCheck))
	{
		IsCurrentPlayerCheckmate = IsPlayerCheckmate(ChessContext, OtherColor(ChessContext->PlayerToPlay), Arena);
	}
	if(IsCurrentPlayerCheckmate)
	{
		printf("Checkmate !!\n");
		//DEBUGWriteConfigListToFile(ChessContext->ChessboardConfigSentinel);
	}
	else if(IsDraw(ChessContext, Arena))
	{
		printf("PAT !!\n");
		//DEBUGWriteConfigListToFile(ChessContext->ChessboardConfigSentinel);
	}

	ChessContext->PlayerToPlay = OtherColor(ChessContext->PlayerToPlay);
}
