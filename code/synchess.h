#pragma once

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

enum move_type
{
	MoveType_None,
	MoveType_Regular,
	MoveType_CastlingKingSide,
	MoveType_CastlingQueenSide,
	MoveType_EnPassant,

	MoveType_Count,
};

struct castling_rook_tracker
{
	bool IsFirstRank;
	bool HasMoved;
};

struct castling_piece_tracker
{
	bool KingHasMoved;
	castling_rook_tracker QueenRook;
	castling_rook_tracker KingRook;
};

struct chess_game_context
{
	board_tile Chessboard[64];
	chessboard_config_list* ChessboardConfigSentinel;
	castling_piece_tracker CastlingPieceTracker[2];

	player_select PlayerCheck;

};

struct game_state
{
	renderer Renderer;
	memory_arena GameArena;

	chess_game_context ChessContext;

	u32 SquareSizeInPixels;

	move_type TileHighlighted[64];
	bitmap PieceBitmaps[PieceType_Count * PieceColor_Count];
	v2i ClickedTile;
	v2i SelectedPieceP;

	piece_color PlayerToPlay;

	bool IsInitialised;
};

struct tile_list
{
	v2i P;
	move_type MoveType;
	tile_list* Next;
};
