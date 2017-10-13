#ifdef _WIN32
#include <SDL.h>
#include <SDL_net.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#endif

// NOTE(hugo) : The server for handling Synchess request.

#include <rivten.h>
#include <rivten_math.h>

// NOTE(hugo) : Basing most of this on : https://r3dux.org/2011/01/a-simple-sdl_net-chat-server-client/

#include "synchess.h"
#include "synchess_network.h"

enum server_game_mode
{
	ServerGameMode_WaitingFor2PlayersToJoin,
	ServerGameMode_WaitingFor1PlayersToJoin,
	ServerGameMode_WaitingForWhiteToPlay,
	ServerGameMode_WaitingForBlackToPlay,
	ServerGameMode_GameEnded,

	ServerGameMode_Count,
};

struct server_state
{
	server_game_mode Mode;
	memory_arena ServerArena;

	chess_game_context ChessContext;

	piece_color PlayerToPlay;

	// NOTE(hugo) : Network stuff
	SDLNet_SocketSet SocketSet;
	TCPsocket ClientSockets[PieceColor_Count];
	TCPsocket ServerSocket;
	u32 CurrentClientCount;

	bool IsInitialised;
};

struct game_memory
{
	u64 StorageSize;
	void* Storage;
};

s32 main(s32 ArgumentCount, char** Arguments)
{
	// NOTE(hugo) : Init SDL_Net
	s32 SDLNetInitResult = SDLNet_Init();
	Assert(SDLNetInitResult != -1);

	game_memory ServerMemory = {};
	ServerMemory.StorageSize = Megabytes(512);
	ServerMemory.Storage = Allocate_(ServerMemory.StorageSize);
	Assert(ServerMemory.Storage);
	
	bool Running = true;
	while(Running)
	{
		Assert(sizeof(server_state) <= ServerMemory.StorageSize);
		server_state* ServerState = (server_state*) ServerMemory.Storage;
		if(!ServerState->IsInitialised)
		{
			u32 MaxSocketCount = 3;
			ServerState->SocketSet = SDLNet_AllocSocketSet(MaxSocketCount);
			Assert(ServerState->SocketSet);

			u32 MaxClientCount = MaxSocketCount - 1; // NOTE(hugo) : The server needs a socket.
			ServerState->CurrentClientCount = 0;

			for(u32 ClientIndex = 0; ClientIndex < ArrayCount(ServerState->ClientSockets); ++ClientIndex)
			{
				ServerState->ClientSockets[ClientIndex] = 0;
			}

			u32 ServerPort = SYNCHESS_PORT;

			IPaddress ServerAddress;
			s32 HostResolved = SDLNet_ResolveHost(&ServerAddress, 0, ServerPort);
			Assert(HostResolved != -1);

			printf("Successfully resolved host server to IP 0x%08x, port 0x%04x.\n", ServerAddress.host, ServerAddress.port);

			ServerState->ServerSocket = SDLNet_TCP_Open(&ServerAddress);
			Assert(ServerState->ServerSocket);

			s32 AddSocketResult = SDLNet_TCP_AddSocket(ServerState->SocketSet, ServerState->ServerSocket);
			Assert(AddSocketResult != -1);

			ServerState->IsInitialised = true;
		}

		// NOTE(hugo) : Server loop

		s32 ActiveSocketCount = SDLNet_CheckSockets(ServerState->SocketSet, 0);
		Assert(ActiveSocketCount != -1);

		s32 ServerSocketActivity = SDLNet_SocketReady(ServerState->ServerSocket);
		Assert(ServerSocketActivity != -1);
		if(ServerSocketActivity > 0)
		{
			// NOTE(hugo) : An incoming connexion is pending
			if(ServerState->CurrentClientCount < ArrayCount(ServerState->ClientSockets))
			{
				ServerState->ClientSockets[ServerState->CurrentClientCount] = SDLNet_TCP_Accept(ServerState->ServerSocket);
				Assert(ServerState->ClientSockets[ServerState->CurrentClientCount]);

				s32 AddSocketResult = SDLNet_TCP_AddSocket(ServerState->SocketSet, ServerState->ClientSockets[ServerState->CurrentClientCount]);
				Assert(AddSocketResult != -1);

				network_synchess_message Message = {};
				Message.Type = NetworkMessageType_ConnectionEstablished;
				Message.ConnectionEstablished.Color = (piece_color)(ServerState->CurrentClientCount);
				SDLNet_TCP_Send(ServerState->ClientSockets[ServerState->CurrentClientCount], &Message, sizeof(Message));
				++ServerState->CurrentClientCount;

				printf("A new client connected !\n");
			}
			else
			{
				// NOTE(hugo) : No room for the incoming connexion. Tell him we are full
				TCPsocket TempSocket = SDLNet_TCP_Accept(ServerState->ServerSocket);
				Assert(TempSocket);
				network_synchess_message Message = {};
				Message.Type = NetworkMessageType_NoRoomForClient;
				SDLNet_TCP_Send(TempSocket, &Message, sizeof(Message));
				SDLNet_TCP_Close(TempSocket);
			}
		}

		for(u32 ClientIndex = 0; ClientIndex < ArrayCount(ServerState->ClientSockets); ++ClientIndex)
		{
			TCPsocket ClientSocket = ServerState->ClientSockets[ClientIndex];
			if(!ClientSocket)
			{
				continue;
			}

			s32 ClientSocketActivity = SDLNet_SocketReady(ClientSocket);
			Assert(ClientSocketActivity != -1);
			if(ClientSocketActivity > 0)
			{
				network_synchess_message Message = {};
				s32 ReceivedBytes = SDLNet_TCP_Recv(ClientSocket, &Message, sizeof(Message));
				Assert(ReceivedBytes != -1);
				Assert(ReceivedBytes <= (s32)sizeof(Message));
				if(ReceivedBytes == 0)
				{
					// NOTE(hugo) : Connexion closed.
					SDLNet_TCP_DelSocket(ServerState->SocketSet, ClientSocket);
					ServerState->ClientSockets[ClientIndex] = 0;
				}
				else
				{
					// NOTE(hugo) : A message was received
					printf("Received from client #%i : %08x\n", ClientIndex, Message.Type);
				}
			}
		}
	}

	SDLNet_Quit();

	return(0);
}
