#pragma once


// TODO(hugo) : This is probably a _very_ lame
// pattern for handling messages over the network
// but I guess it will do for now...

enum network_message_type
{
	NetworkMessageType_None,
	NetworkMessageType_ConnectionEstablished,
	NetworkMessageType_Quit,
	NetworkMessageType_MoveDone,

	NetworkMessageType_Count,
};

struct network_synchess_message
{
	network_message_type Type;
};

