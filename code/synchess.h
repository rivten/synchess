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
	MoveType_DoubleStepPawn,

	MoveType_Count,
};

// TODO(hugo) : We should make sure the rook is also still
// there and have not been eaten by the other player
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

struct move_params
{
	move_type Type;
	v2i InitialP;
	v2i DestP;
};

#define NO_PREVIOUS_DOUBLE_STEP 8
struct chess_game_context
{
	// NOTE(hugo): The decision is :
	// We had the choice between : 
	//  * either keeping the chessboard be a list of pointers,
	//        with null pointer meaning : no piece here
	//  * or making it a u8 Chessboard[64], with non meaning no piece here
	//  I decided to go with the first one because it would take too much time
	//  right now to make all the changes. This means that each client first creates a
	//  set of pieces the board will point to. But in the network message, we will send on the 
	//  compressed version.
	//  Then we will uncompress the board for each client.
	//  Maybe in the future, make everybody use u8 ? Even if this involves uncompressing all the time
	//  you want to perform a test ? What does the bandwidth tests say about that ?
	board_tile Chessboard[64];
	chessboard_config_list* ChessboardConfigSentinel;
	castling_piece_tracker CastlingPieceTracker[2];

	player_select PlayerCheck;

	u32 LastDoubleStepCol;

	piece_color PlayerToPlay;
};

enum user_mode
{
	UserMode_MakeMove,
	UserMode_PromotePawn,
};

struct tile_list
{
	v2i P;
	move_type MoveType;
	tile_list* Next;
};
