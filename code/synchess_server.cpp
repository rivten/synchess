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
#include "chess.cpp"

struct server_state
{
	memory_arena ServerArena;

	chess_game_context ChessContext;

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
			// NOTE(hugo) : Network init
			// {
			u32 MaxSocketCount = 3;
			ServerState->SocketSet = SDLNet_AllocSocketSet(MaxSocketCount);
			Assert(ServerState->SocketSet);

			//u32 MaxClientCount = MaxSocketCount - 1; // NOTE(hugo) : The server needs a socket.
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
			// }

			u64 ServerArenaSize = Megabytes(128);
			void* ServerArenaBase = (u8*)ServerMemory.Storage + sizeof(server_state);
			InitialiseArena(&ServerState->ServerArena, ServerArenaSize, ServerArenaBase);
			InitialiseChessContext(&ServerState->ChessContext, &ServerState->ServerArena);

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
				Message.ConnectionEstablished.GivenColor = (piece_color)(ServerState->CurrentClientCount);
				NetSendMessage(ServerState->ClientSockets[ServerState->CurrentClientCount], &Message);
				++ServerState->CurrentClientCount;

				printf("A new client connected !\n");

				if(ServerState->CurrentClientCount == ArrayCount(ServerState->ClientSockets))
				{
					// NOTE(hugo) : Broadcast to all that the game has started.
					network_synchess_message Message = {};
					Message.Type = NetworkMessageType_GameStarted;
					for(u32 SocketIndex = 0; 
							SocketIndex < ArrayCount(ServerState->ClientSockets); 
							++SocketIndex)
					{
						NetSendMessage(ServerState->ClientSockets[SocketIndex], &Message);
					}
				}
			}
			else
			{
				// NOTE(hugo) : No room for the incoming connexion. Tell him we are full
				TCPsocket TempSocket = SDLNet_TCP_Accept(ServerState->ServerSocket);
				Assert(TempSocket);
				network_synchess_message Message = {};
				Message.Type = NetworkMessageType_NoRoomForClient;
				NetSendMessage(TempSocket, &Message);
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
					//piece_color ClientColor = (piece_color)(ClientIndex);
					switch(Message.Type)
					{
						case NetworkMessageType_ConnectionEstablished:
						case NetworkMessageType_Quit:
						case NetworkMessageType_NoRoomForClient:
							{
								// NOTE(hugo) : Client should not send this.
								InvalidCodePath;
							} break;
						case NetworkMessageType_MoveDone:
							{
								// TODO(hugo) : Check that the move the client 
								// want to do is legal. (also check that
								// this is indeed the right client who
								// sent the move)
								ApplyMove(&ServerState->ChessContext, Message.MoveDone, &ServerState->ServerArena);
								network_synchess_message Message = {};
								Message.Type = NetworkMessageType_ChessContextUpdate;
								Message.ContextUpdate.NewBoardConfig = WriteConfig(ServerState->ChessContext.Chessboard);
								Message.ContextUpdate.CastlingPieceTracker[0] = ServerState->ChessContext.CastlingPieceTracker[0];
								Message.ContextUpdate.CastlingPieceTracker[1] = ServerState->ChessContext.CastlingPieceTracker[1];
								Message.ContextUpdate.PlayerCheck = ServerState->ChessContext.PlayerCheck;

								Message.ContextUpdate.LastDoubleStepCol = ServerState->ChessContext.LastDoubleStepCol;
								Message.ContextUpdate.PlayerToPlay = ServerState->ChessContext.PlayerToPlay;
								for(u32 ClientIndex = 0;
										ClientIndex < ArrayCount(ServerState->ClientSockets);
										++ClientIndex)
								{
									TCPsocket ClientSocket = ServerState->ClientSockets[ClientIndex];
									NetSendMessage(ClientSocket, &Message);

								}
								// TODO(hugo): Check for checkmate, draw, notify players 
								// for this and change internal game state.
							} break;

						InvalidDefaultCase;
					}
				}
			}
		}
	}

	SDLNet_Quit();

	return(0);
}
