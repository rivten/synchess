#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <rivten.h>
#include <rivten_math.h>

#include "input.h"

global_variable u32 GlobalWindowWidth = 512;
global_variable u32 GlobalWindowHeight = 512;

internal SDL_Rect
SDLRect(rect2 Rect)
{
	SDL_Rect Result = {};
	v2 Size = RectSize(Rect);
	Result.x = Rect.Min.x;
	Result.y = Rect.Min.y + Size.y;
	Result.w = Size.x;
	Result.h = Size.y;

	return(Result);
}

internal void
SDLSetRenderColor(SDL_Renderer* Renderer, v4 Color)
{
	SDL_SetRenderDrawColor(Renderer, 255 * Color.r, 255 * Color.g, 255 * Color.b, 255 * Color.a);
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
struct bitmap
{
	u32 Width;
	u32 Height;
	void* Data;

	// NOTE(hugo) : Offset is in percent relative to the bitmap size
	v2 Offset;
	bool IsValid;
};

u32 SafeCastToU32(s32 Value)
{
	Assert(Value >= 0);
	u32 Result = (u32)Value;
	return(Result);
}

bitmap LoadBitmap(char* Filename, v2 Offset = V2(0.0f, 0.0f))
{
	// TODO(hugo) : Load off an arena
	bitmap Result = {};
	s32 Depth = 0;
	s32 Width = 0;
	s32 Height = 0;
	Result.Data = stbi_load(Filename, &Width, &Height, &Depth, 4);
	Assert(Result.Data);

	Result.Width = SafeCastToU32(Width);
	Result.Height = SafeCastToU32(Height);
	Assert(Result.Width > 0);
	Assert(Result.Height > 0);

	Result.Offset = Offset;

	Result.IsValid = true;

	return(Result);
}

void FreeBitmap(bitmap Bitmap)
{
	stbi_image_free(Bitmap.Data);
}


#include "render.h"
#include "render.cpp"

struct game_memory
{
	u64 StorageSize;
	void* Storage;
};

#include "synchess.h"

struct rgb8
{
	u8 r;
	u8 g;
	u8 b;
};

internal rgb8
RGB8(u8 r, u8 g, u8 b)
{
	rgb8 Result = {r, g, b};
	return(Result);
}

internal v4
RGB8ToV4(rgb8 Color)
{
	v4 Result = V4(Color.r, Color.g, Color.b, 1.0f);
	Result = Result / 255.0f;

	return(Result);
}

#define PLACE_PIECE_AT(I, J, TypeP, ColorP)\
	Assert(!Chessboard[I + 8 * J]);\
	Chessboard[I + 8 * J] = PushStruct(GameArena, chess_piece);\
	Chessboard[I + 8 * J]->Type = PieceType_##TypeP;\
	Chessboard[I + 8 * J]->Color = PieceColor_##ColorP;\

internal void
InitialiseChessboard(board_tile* Chessboard, memory_arena* GameArena)
{
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
}

internal void
DisplayChessboardToConsole(board_tile* Chessboard)
{
	for(s32 LineIndex = SQUARE_PER_SIDE - 1; LineIndex >= 0; --LineIndex)
	{
		for(u32 RowIndex = 0; RowIndex < SQUARE_PER_SIDE; ++RowIndex)
		{
			chess_piece* Piece = Chessboard[RowIndex + 8 * LineIndex];
			if(Piece)
			{
				switch(Piece->Type)
				{
					case PieceType_Pawn:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("p");
							}
							else
							{
								printf("P");
							}
						} break;
					case PieceType_Knight:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("n");
							}
							else
							{
								printf("N");
							}
						} break;
					case PieceType_Bishop:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("b");
							}
							else
							{
								printf("B");
							}
						} break;
					case PieceType_Rook:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("r");
							}
							else
							{
								printf("R");
							}
						} break;
					case PieceType_Queen:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("q");
							}
							else
							{
								printf("Q");
							}
						} break;
					case PieceType_King:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("k");
							}
							else
							{
								printf("K");
							}
						} break;
					InvalidDefaultCase;
				}
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
	}
}

internal bool
ContainsPiece(board_tile* Chessboard, v2i TileP)
{
	chess_piece* Piece = Chessboard[TileP.x + SQUARE_PER_SIDE * TileP.y];
	bool Result = (Piece != 0);
	return(Result);
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
	for(u32 SquareX = 0; SquareX < 8; ++SquareX)
	{
		for(u32 SquareY = 0; SquareY < 8; ++SquareY)
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

#define BOARD_COORD(P) (P).x + 8 * (P).y

internal bool
ContainsPieceOfColor(board_tile* Chessboard, v2i TileP, piece_color Color)
{
	chess_piece* Piece = Chessboard[BOARD_COORD(TileP)];
	bool Result = (Piece != 0) && (Piece->Color == Color);
	return(Result);
}

internal void
ClearTileHighlighted(game_state* GameState)
{
	for(u32 TileIndex = 0; TileIndex < ArrayCount(GameState->IsTileHighlighted); ++TileIndex)
	{
		GameState->IsTileHighlighted[TileIndex] = false;
	}
}

internal bool
IsInsideBoard(v2i TileP)
{
	bool Result = (TileP.x >= 0) && (TileP.x < SQUARE_PER_SIDE)
		&& (TileP.y >= 0) && (TileP.y < SQUARE_PER_SIDE);
	return(Result);
}

internal void
AddTile(tile_list** Sentinel, v2i TileP, memory_arena* Arena)
{
	tile_list* TileToAdd = PushStruct(Arena, tile_list);
	TileToAdd->P = TileP;
	if(Sentinel)
	{
		TileToAdd->Next = (*Sentinel);
	}
	*Sentinel = TileToAdd;
}

#define ADD_IF_IN_BOARD_AND_NO_PIECE(P)\
	if(IsInsideBoard(P) && (!ContainsPiece(Chessboard, P))) \
	{\
			AddTile(&Sentinel, P, Arena);\
	}

#define ADD_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, Color)\
	if(IsInsideBoard(P) && (ContainsPieceOfColor(Chessboard, (P), (Color)))) \
	{\
			AddTile(&Sentinel, P, Arena);\
	}

#define ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Color)\
	if(IsInsideBoard(P) && (!ContainsPieceOfColor(Chessboard, (P), (Color)))) \
	{\
			AddTile(&Sentinel, P, Arena);\
	}

#define ADD_IN_DIR(PieceP, Dir)\
	{\
		u32 K = 1;\
		while(IsInsideBoard(PieceP + K * Dir) && !ContainsPiece(Chessboard, PieceP + K * Dir))\
		{\
			AddTile(&Sentinel, PieceP + K * Dir, Arena);\
			++K;\
		}\
		if(IsInsideBoard(PieceP + K * Dir))\
		{\
			v2i BlockingPieceP = PieceP + K * Dir;\
			chess_piece* BlockingPiece = Chessboard[BlockingPieceP.x + 8 * BlockingPieceP.y];\
			Assert(BlockingPiece);\
			if(BlockingPiece->Color != Piece->Color)\
			{\
				AddTile(&Sentinel, BlockingPieceP, Arena);\
			}\
		}\
	}

internal tile_list*
GetAttackingTileList(board_tile* Chessboard, chess_piece* Piece,
		v2i PieceP, memory_arena* Arena)
{
	tile_list* Sentinel = 0;

	Assert(Piece);
	switch(Piece->Type)
	{
		case PieceType_Pawn:
		{
			if(Piece->Color == PieceColor_White)
			{
				v2i P = V2i(PieceP.x, PieceP.y + 1);
				ADD_IF_IN_BOARD_AND_NO_PIECE(P);

				P = V2i(PieceP.x - 1, PieceP.y + 1);
				ADD_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_Black);

				P = V2i(PieceP.x + 1, PieceP.y + 1);
				ADD_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_Black);

				if((PieceP.y == 1) && !ContainsPiece(Chessboard, V2i(PieceP.x, PieceP.y + 1)))
				{
					P = V2i(PieceP.x, PieceP.y + 2);
					ADD_IF_IN_BOARD_AND_NO_PIECE(P);
				}
			}
			else if(Piece->Color == PieceColor_Black)
			{
				v2i P = V2i(PieceP.x, PieceP.y - 1);
				ADD_IF_IN_BOARD_AND_NO_PIECE(P);

				P = V2i(PieceP.x - 1, PieceP.y - 1);
				ADD_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_White);

				P = V2i(PieceP.x + 1, PieceP.y - 1);
				ADD_IF_IN_BOARD_AND_PIECE_OF_COLOR(P, PieceColor_White);

				if((PieceP.y == 6) && !ContainsPiece(Chessboard, V2i(PieceP.x, PieceP.y - 1)))
				{
					P = V2i(PieceP.x, PieceP.y - 2);
					ADD_IF_IN_BOARD_AND_NO_PIECE(P);
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
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 2);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 2, PieceP.y + 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 2, PieceP.y - 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y - 2);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y - 2);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 2, PieceP.y - 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 2, PieceP.y + 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

		} break;

		case PieceType_Bishop:
		{
			v2i LeftTopDir = V2i(-1, +1);
			ADD_IN_DIR(PieceP, LeftTopDir);

			v2i RightTopDir = V2i(+1, +1);
			ADD_IN_DIR(PieceP, RightTopDir);

			v2i RightDownDir = V2i(+1, -1);
			ADD_IN_DIR(PieceP, RightDownDir);

			v2i LeftDownDir = V2i(-1, -1);
			ADD_IN_DIR(PieceP, LeftDownDir);
		} break;

		case PieceType_Rook:
		{
			v2i LeftDir = V2i(-1, 0);
			ADD_IN_DIR(PieceP, LeftDir);

			v2i TopDir = V2i(0, +1);
			ADD_IN_DIR(PieceP, TopDir);

			v2i RightDir = V2i(+1, 0);
			ADD_IN_DIR(PieceP, RightDir);

			v2i DownDir = V2i(0, -1);
			ADD_IN_DIR(PieceP, DownDir);
		} break;

		case PieceType_Queen:
		{
			v2i LeftTopDir = V2i(-1, +1);
			ADD_IN_DIR(PieceP, LeftTopDir);

			v2i RightTopDir = V2i(+1, +1);
			ADD_IN_DIR(PieceP, RightTopDir);

			v2i RightDownDir = V2i(+1, -1);
			ADD_IN_DIR(PieceP, RightDownDir);

			v2i LeftDownDir = V2i(-1, -1);
			ADD_IN_DIR(PieceP, LeftDownDir);

			v2i LeftDir = V2i(-1, 0);
			ADD_IN_DIR(PieceP, LeftDir);

			v2i TopDir = V2i(0, +1);
			ADD_IN_DIR(PieceP, TopDir);

			v2i RightDir = V2i(+1, 0);
			ADD_IN_DIR(PieceP, RightDir);

			v2i DownDir = V2i(0, -1);
			ADD_IN_DIR(PieceP, DownDir);
		} break;

		case PieceType_King:
		{
			v2i P = V2i(PieceP.x - 1, PieceP.y - 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y + 0);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x - 1, PieceP.y + 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 0, PieceP.y - 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 0, PieceP.y + 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y - 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 0);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			P = V2i(PieceP.x + 1, PieceP.y + 1);
			ADD_IF_IN_BOARD_AND_NO_PIECE_OF_COLOR(P, Piece->Color);

			//TODO(hugo) : Roque
		} break;

		InvalidDefaultCase;
	}

	return(Sentinel);
}

internal void
HighlightPossibleMoves(game_state* GameState, tile_list* TileList)
{
	for(tile_list* Tile = TileList; Tile; Tile = Tile->Next)
	{
		GameState->IsTileHighlighted[BOARD_COORD(Tile->P)] = true;
	}
}

internal bitmap 
GetPieceBitmap(game_state* GameState, chess_piece Piece)
{
	bitmap Result = GameState->PieceBitmaps[Piece.Color * PieceType_Count + Piece.Type];

	return(Result);
}

internal void
LoadPieceBitmaps(bitmap* Bitmaps)
{
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Pawn] = LoadBitmap("../data/WhitePawn.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Knight] = LoadBitmap("../data/WhiteKnight.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Bishop] = LoadBitmap("../data/WhiteBishop.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Rook] = LoadBitmap("../data/WhiteRook.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Queen] = LoadBitmap("../data/WhiteQueen.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_King] = LoadBitmap("../data/WhiteKing.png");

	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Pawn] = LoadBitmap("../data/BlackPawn.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Knight] = LoadBitmap("../data/BlackKnight.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Bishop] = LoadBitmap("../data/BlackBishop.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Rook] = LoadBitmap("../data/BlackRook.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Queen] = LoadBitmap("../data/BlackQueen.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_King] = LoadBitmap("../data/BlackKing.png");
}

internal v2i
GetClickedTile(board_tile* Chessboard, v2 MouseP)
{
	v2i Result = V2i(0, 0);
	v2 TileP = MouseP / (GlobalWindowHeight / 8);
	TileP.y = 8 - TileP.y;

	Result.x = u32(TileP.x);
	Result.y = u32(TileP.y);

	return(Result);
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
	for(u32 SquareX = 0; SquareX < 8; ++SquareX)
	{
		for(u32 SquareY = 0; SquareY < 8; ++SquareY)
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
SearchForKingCheck(board_tile* Chessboard, memory_arena* Arena)
{
	player_select Result = PlayerSelect_None;

	// TODO(hugo) : Maybe cache the result if time-critical ?
	kings_positions KingsPositions = FindKingsPositions(Chessboard);
	for(u32 SquareX = 0; SquareX < 8; ++SquareX)
	{
		for(u32 SquareY = 0; SquareY < 8; ++SquareY)
		{
			chess_piece* Piece = Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				temporary_memory TileListTempMemory = BeginTemporaryMemory(Arena);

				tile_list* AttackingTileList = GetAttackingTileList(Chessboard, Piece, V2i(SquareX, SquareY), Arena);
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

internal bool
IsPlayerCheckmate(board_tile* Chessboard, piece_color PlayerColor, memory_arena* Arena)
{
	bool SavingMoveFound = false;
	for(u32 SquareX = 0; (!SavingMoveFound) && (SquareX < 8); ++SquareX)
	{
		for(u32 SquareY = 0; (!SavingMoveFound) && (SquareY < 8); ++SquareY)
		{
			chess_piece* Piece = Chessboard[SquareX + 8 * SquareY];
			if(Piece && (Piece->Color == PlayerColor))
			{
				temporary_memory CheckMateTempMemory = BeginTemporaryMemory(Arena);

				v2i PieceP = V2i(SquareX, SquareY);
				tile_list* PossibleMoves = GetAttackingTileList(Chessboard, Piece, PieceP, Arena);
				for(tile_list* Move = PossibleMoves;
						Move;
						Move = Move->Next)
				{
					v2i PieceDest = Move->P;
					chess_piece* OldPieceAtDestSave = Chessboard[BOARD_COORD(PieceDest)];
					Chessboard[BOARD_COORD(PieceP)] = 0;
					Chessboard[BOARD_COORD(PieceDest)] = Piece;
					player_select NewCheckPlayer = SearchForKingCheck(Chessboard, Arena);
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
DeleteInvalidMoveDueToCheck(board_tile* Chessboard, chess_piece* Piece, v2i PieceP, tile_list** MoveSentinel, piece_color CheckPlayer, memory_arena* Arena)
{
	Assert(MoveSentinel);
	Assert(Piece);
	tile_list* PreviousMove = 0;
	for(tile_list* CurrentMove = *MoveSentinel; CurrentMove; CurrentMove = CurrentMove->Next)
	{
		bool DeleteMove = false;

		v2i PieceDest = CurrentMove->P;
		chess_piece* OldPieceAtDestSave = Chessboard[BOARD_COORD(PieceDest)];
		Chessboard[BOARD_COORD(PieceP)] = 0;
		Chessboard[BOARD_COORD(PieceDest)] = Piece;

		player_select NewCheckPlayer = SearchForKingCheck(Chessboard, Arena);
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
		Chessboard[BOARD_COORD(PieceP)] = Piece;
		Chessboard[BOARD_COORD(PieceDest)] = OldPieceAtDestSave;

		if(!DeleteMove)
		{
			PreviousMove = CurrentMove;
		}
	}
}

piece_color OtherColor(piece_color Color)
{
	Assert(Color != PieceColor_Count);
	piece_color Result = (piece_color)(1 - Color);
	return(Result);
}

internal void
DEBUGWriteConfigListToFile(chessboard_config_list* Sentinel)
{
	FILE* FileHandle = fopen("list_move.bin", "wb");
	Assert(FileHandle);

	for(chessboard_config_list* Config = Sentinel;
			Config;
			Config = Config->Next)
	{
		chessboard_config ChessboardConfig = Config->Config;
		fwrite(&ChessboardConfig, sizeof(ChessboardConfig), 1, FileHandle);
	}

	fclose(FileHandle);
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
IsDraw(game_state* GameState)
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

	// NOTE(hugo) : Checking for threefold repetition
	// TODO(hugo) : Check for right to castle or capture en passant
	chessboard_config CurrentConfig = WriteConfig(GameState->Chessboard);
	u32 ConfigMatchCounter = 0;
	for(chessboard_config_list* CurrentTestConfig = GameState->ChessboardConfigSentinel;
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

// TODO(hugo) : Get rid of the SDL_Renderer parameter in there
internal void
GameUpdateAndRender(game_memory* GameMemory, game_input* Input, SDL_Renderer* SDLRenderer)
{
	Assert(sizeof(game_state) <= GameMemory->StorageSize);
	game_state* GameState = (game_state*) GameMemory->Storage;
	if(!GameState->IsInitialised)
	{
		u64 RenderArenaSize = Megabytes(128);
		Assert(sizeof(game_state) + RenderArenaSize <= GameMemory->StorageSize);
		void* RenderArenaMemoryBase = (u8*)GameMemory->Storage + sizeof(game_state);

		// TODO(hugo) : Resizable window
		InitialiseRenderer(&GameState->Renderer, SDLRenderer, RenderArenaSize,
				V2(GlobalWindowWidth, GlobalWindowHeight), RenderArenaMemoryBase);

		u64 GameArenaSize = Megabytes(128);
		void* GameArenaMemoryBase = (u8*)GameMemory->Storage + sizeof(game_state) + RenderArenaSize;
		InitialiseArena(&GameState->GameArena, GameArenaSize, GameArenaMemoryBase);
		InitialiseChessboard(GameState->Chessboard, &GameState->GameArena);

		GameState->SquareSizeInPixels = 64;
		LoadPieceBitmaps(&GameState->PieceBitmaps[0]);

		GameState->PlayerToPlay = PieceColor_White;

		GameState->ChessboardConfigSentinel = 0;

		//DisplayChessboardToConsole(GameState->Chessboard);

		GameState->IsInitialised = true;
	}

	// NOTE(hugo) : Game update
	// {
	if(Pressed(Input->Mouse.Buttons[MouseButton_Left]))
	{
		GameState->ClickedTile = GetClickedTile(GameState->Chessboard, Input->Mouse.P);
		chess_piece* Piece = GameState->Chessboard[BOARD_COORD(GameState->ClickedTile)];
		if(Piece && Piece->Color == GameState->PlayerToPlay)
		{
			ClearTileHighlighted(GameState);


			temporary_memory HighlightingTileTempMemory = BeginTemporaryMemory(&GameState->GameArena);
			tile_list* AttackingTileList = GetAttackingTileList(GameState->Chessboard, Piece, GameState->ClickedTile, &GameState->GameArena);
			if(AttackingTileList)
			{
				DeleteInvalidMoveDueToCheck(GameState->Chessboard, Piece, GameState->ClickedTile, &AttackingTileList, GameState->PlayerToPlay, &GameState->GameArena);
			}

			HighlightPossibleMoves(GameState, AttackingTileList);
			EndTemporaryMemory(HighlightingTileTempMemory);


			GameState->SelectedPieceP = GameState->ClickedTile;
		}
		else
		{
			if(GameState->IsTileHighlighted[BOARD_COORD(GameState->ClickedTile)])
			{
				// NOTE(hugo): Make the intended move effective
				// if the player selects a valid tile
				chess_piece* SelectedPiece = GameState->Chessboard[BOARD_COORD(GameState->SelectedPieceP)];
				Assert(SelectedPiece);
				GameState->Chessboard[BOARD_COORD(GameState->SelectedPieceP)] = 0;
				GameState->Chessboard[BOARD_COORD(GameState->ClickedTile)] = SelectedPiece;

				// NOTE(hugo): Update the config list
				chessboard_config_list* NewChessboardConfigList = 
					PushStruct(&GameState->GameArena, chessboard_config_list);
				NewChessboardConfigList->Config = WriteConfig(GameState->Chessboard);
				NewChessboardConfigList->Next = GameState->ChessboardConfigSentinel;
				GameState->ChessboardConfigSentinel = NewChessboardConfigList;

				ClearTileHighlighted(GameState);

				GameState->PlayerCheck = SearchForKingCheck(GameState->Chessboard, &GameState->GameArena);
				bool IsCurrentPlayerCheckmate = IsPlayerCheckmate(GameState->Chessboard, OtherColor(GameState->PlayerToPlay), &GameState->GameArena);
				if(IsCurrentPlayerCheckmate)
				{
					printf("Checkmate !!\n");
					DEBUGWriteConfigListToFile(GameState->ChessboardConfigSentinel);
				}
				else if(IsDraw(GameState))
				{
					printf("PAT !!\n");
					DEBUGWriteConfigListToFile(GameState->ChessboardConfigSentinel);
				}

				GameState->PlayerToPlay = OtherColor(GameState->PlayerToPlay);
			}
		}
	}
	if(Pressed(Input->Mouse.Buttons[MouseButton_Right]))
	{
		ClearTileHighlighted(GameState);
	}
	// }

	renderer* Renderer = &GameState->Renderer;

	BeginRender(Renderer);

	PushClear(Renderer, V4(0.0f, 0.0f, 0.0f, 1.0f));

	// NOTE(hugo) : Render background
	// {
	u32 BoardSizeInPixels = 8 * GameState->SquareSizeInPixels;
	v2 BoardSize = V2(BoardSizeInPixels, BoardSizeInPixels);
	v2 BoardMin = 0.5f * V2(GlobalWindowWidth, GlobalWindowHeight) - 0.5f * BoardSize;
	rect2 BoardRect = RectFromMinSize(BoardMin, BoardSize);

	u32 SquareSizeInPixels = (u32)(BoardSizeInPixels / SQUARE_PER_SIDE);
	v2i SquareSize = V2i(SquareSizeInPixels, SquareSizeInPixels);

	for(u32 SquareY = 0; SquareY < SQUARE_PER_SIDE; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < SQUARE_PER_SIDE; ++SquareX)
		{
			v2 SquareMin = Hadamard(V2(SquareX, SquareY), V2(SquareSize)) + BoardMin;
			rect2 SquareRect = RectFromMinSize(SquareMin, V2(SquareSize));
			bool IsWhiteTile = (SquareX + SquareY) % 2 != 0;
			v4 SquareBackgroundColor = (IsWhiteTile) ? 
				V4(1.0f, 1.0f, 1.0f, 1.0f) : RGB8ToV4(RGB8(64, 146, 59));

			if(GameState->IsTileHighlighted[SquareX + SQUARE_PER_SIDE * SquareY])
			{
				SquareBackgroundColor = V4(0.0f, 1.0f, 1.0f, 1.0f);
			}

			PushRect(Renderer, SquareRect, SquareBackgroundColor);

			// NOTE(hugo) : Draw the possible piece
			chess_piece* Piece = GameState->Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				v2 SquareMin = Hadamard(V2(SquareX, SquareY), V2(SquareSize)) + BoardMin;
				bitmap PieceBitmap = GetPieceBitmap(GameState, *Piece);
				if(PieceBitmap.IsValid)
				{
					PushBitmap(&GameState->Renderer, PieceBitmap, SquareMin);
				}
			}
		}
	}
	// }

	// NOTE(hugo): Render the commands
	Render(Renderer);
	EndRender(Renderer);
}

s32 main(s32 ArgumentCount, char** Arguments)
{ 
	u32 SDLInitResult = SDL_Init(SDL_INIT_EVERYTHING);
	Assert(SDLInitResult == 0);

	bool Running = true;

	// TODO(hugo) : Should the window be resizable ?
	u32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	SDL_Window* Window = SDL_CreateWindow("The Road East", 
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			GlobalWindowWidth, GlobalWindowHeight, WindowFlags);
	Assert(Window);

	u32 RendererFlags = SDL_RENDERER_ACCELERATED;
	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, RendererFlags);
	Assert(Renderer);

	game_memory GameMemory = {};
	GameMemory.StorageSize = Megabytes(512);
	GameMemory.Storage = Allocate_(GameMemory.StorageSize);

	Assert(GameMemory.Storage);

	game_input Inputs[2] = {};
	game_input* NewInput = Inputs + 0;
	game_input* OldInput = Inputs + 1;

	// NOTE(hugo) : Timing info
	u32 LastCounter = SDL_GetTicks();
	u32 MonitorRefreshHz = 60;
	SDL_DisplayMode DisplayMode = {};
	if(SDL_GetCurrentDisplayMode(0, &DisplayMode) == 0)
	{
		if(DisplayMode.refresh_rate != 0)
		{
			MonitorRefreshHz = DisplayMode.refresh_rate;
		}
	}
	float GameUpdateHz = (MonitorRefreshHz / 2.0f);
	u32 TargetMSPerFrame = (u32)(1000.0f / GameUpdateHz);

	// TODO(hugo) : Maybe a little bit ugly to do this here
	NewInput->dtForFrame = 1.0f / GameUpdateHz;
	OldInput->dtForFrame = 1.0f / GameUpdateHz;

	while(Running)
	{
		//
		// NOTE(hugo) : Input gathering
		// {
		//
		SDL_Event Event;
		while(SDL_PollEvent(&Event))
		{
			switch(Event.type)
			{
				case SDL_QUIT:
					{
						Running = false;
					} break;
				default:
					{
					} break;
			}
		}
		SDLGetMouseInput(&NewInput->Mouse);
		SDLGetKeyboardInput(&NewInput->Keyboard);
		//
		// }
		//


		GameUpdateAndRender(&GameMemory, NewInput, Renderer);

		// NOTE(hugo) : Framerate computation
		u32 WorkMSElapsedForFrame = SDL_GetTicks() - LastCounter;

		// NOTE(hugo) : Setting the window title
		char WindowTitle[128];
		sprintf(WindowTitle, "synchess @ rivten - (%i, %i) - %i %i %i - %ims", (s32)NewInput->Mouse.P.x, (s32)NewInput->Mouse.P.y, (s32)NewInput->Mouse.Buttons[MouseButton_Left].IsDown, (s32)NewInput->Mouse.Buttons[MouseButton_Middle].IsDown, (s32)NewInput->Mouse.Buttons[MouseButton_Right].IsDown, WorkMSElapsedForFrame);
		SDL_SetWindowTitle(Window, WindowTitle);

		if(WorkMSElapsedForFrame < TargetMSPerFrame)
		{
			u32 SleepMS = TargetMSPerFrame - WorkMSElapsedForFrame;
			if(SleepMS > 0)
			{
				SDL_Delay(SleepMS);
			}
		}
		else
		{
			// TODO(hugo) : Missed framerate
		}

		// NOTE(hugo) : This must be at the very end
		LastCounter = SDL_GetTicks();

		// NOTE(hugo) : Switch inputs
		game_input* TempInput = NewInput;
		NewInput = OldInput;
		OldInput = TempInput;

		// TODO(hugo) : I think this is ok for perf but can we be sure ?
		// TODO(hugo): Is a full copy of the 512 scancodes even a good idea ?
		for(u32 ButtonIndex = 0; ButtonIndex < MouseButton_Count; ++ButtonIndex)
		{
			NewInput->Mouse.Buttons[ButtonIndex].WasDown = OldInput->Mouse.Buttons[ButtonIndex].IsDown;
		}
		for(u32 KeyButtonIndex = 0; KeyButtonIndex < ArrayCount(Inputs[0].Keyboard.Buttons); ++KeyButtonIndex)
		{
			NewInput->Keyboard.Buttons[KeyButtonIndex].WasDown = OldInput->Keyboard.Buttons[KeyButtonIndex].IsDown;
		}
	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);

	SDL_Quit();

	return(0);
}

