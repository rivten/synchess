#pragma once

#define SQUARE_PER_SIDE 8

enum piece_type
{
	PieceType_Pawn,
	PieceType_Knight,
	PieceType_Bishop,
	PieceType_Rook,
	PieceType_Queen,
	PieceType_King,

	PieceType_Count,
};

enum piece_color
{
	PieceColor_White,
	PieceColor_Black,

	PieceColor_Count,
};

struct chess_piece
{
	piece_type Type;
	piece_color Color;
};

typedef chess_piece* board_tile;

enum player_select
{
	PlayerSelect_None = 0,
	PlayerSelect_White = 1,
	PlayerSelect_Black = 2,
};

struct game_state
{
	renderer Renderer;
	memory_arena GameArena;

	u32 SquareSizeInPixels;

	board_tile Chessboard[SQUARE_PER_SIDE * SQUARE_PER_SIDE];
	bool IsTileHighlighted[SQUARE_PER_SIDE * SQUARE_PER_SIDE];
	bitmap PieceBitmaps[PieceType_Count * PieceColor_Count];
	v2i ClickedTile;
	v2i SelectedPieceP;

	piece_color PlayerToPlay;
	player_select PlayerCheck;

	bool IsInitialised;
};

struct tile_list
{
	v2i P;
	tile_list* Next;
};

