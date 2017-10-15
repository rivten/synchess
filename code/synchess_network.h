#pragma once

#define SYNCHESS_PORT 1234
#define SYNCHESS_SERVER_IP "localhost"

// TODO(hugo) : This is probably a _very_ lame
// pattern for handling messages over the network
// but I guess it will do for now...

enum network_message_type
{
	NetworkMessageType_ConnectionEstablished,
	NetworkMessageType_Quit,
	NetworkMessageType_MoveDone,
	NetworkMessageType_NoRoomForClient,
	NetworkMessageType_ChessContextUpdate,

	NetworkMessageType_Count,
};

struct network_message_connection_establised
{
	piece_color Color;
};

struct network_message_move_done
{
	move_type Type;
	v2i InitialP;
	v2i DestP;
};

struct network_message_chess_context_update
{
	chessboard_config NewBoardConfig;

	// TODO(hugo) : OPTIM(hugo) :
	// In theory, we should only send each peer the piece_tracker 
	// that concerns himself
	castling_piece_tracker CastlingPieceTracker[2];

	player_select PlayerCheck;

	// TODO(hugo) : Necessary here ? Or other message to send
	// checkmate status ? What about draw ?
	player_select PlayerCheckmate;

	u32 LastDoubleStepCol;
};

struct network_synchess_message
{
	network_message_type Type;

	union
	{
		network_message_connection_establised ConnectionEstablished;
		network_message_move_done MoveDone;
		network_message_chess_context_update BoardUpdate;
	};
};


