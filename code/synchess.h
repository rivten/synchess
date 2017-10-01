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

struct chessboard_config
{
	u8 Tiles[64];
};

struct chessboard_config_list
{
	chessboard_config Config;
	chessboard_config_list* Next;
};

enum castling_state
{
	CastlingState_None = 0,
	CastlingState_LeftRookMoved = 1 << 0,
	CastlingState_RightRookMoved = 1 << 1,
	CastlingState_KingMoved = 1 << 2,
};

enum move_type
{
	MoveType_None,
	MoveType_Regular,
	MoveType_Castling,
	MoveType_EnPassant,

	MoveType_Count,
};

// NOTE(hugo) : Maybe do this struct to hold data about the current game
// to avoid passing a lot of arguments to some functions (such as GetAttackingList and so on)
/*
struct chess_game_context
{
};
*/

struct game_state
{
	renderer Renderer;
	memory_arena GameArena;

	u32 SquareSizeInPixels;

	board_tile Chessboard[SQUARE_PER_SIDE * SQUARE_PER_SIDE];
	move_type TileHighlighted[SQUARE_PER_SIDE * SQUARE_PER_SIDE];
	bitmap PieceBitmaps[PieceType_Count * PieceColor_Count];
	v2i ClickedTile;
	v2i SelectedPieceP;

	piece_color PlayerToPlay;
	player_select PlayerCheck;

	castling_state PlayerCastlingState[2];

	chessboard_config_list* ChessboardConfigSentinel;

	bool IsInitialised;
};

struct tile_list
{
	v2i P;
	move_type MoveType;
	tile_list* Next;
};
