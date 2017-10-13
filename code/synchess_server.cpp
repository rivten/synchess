#ifdef _WIN32
#include <SDL.h>
#include <SDL_net.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#endif

// NOTE(hugo) : The server for handling Synchess request.

#include "rivten.h"

// NOTE(hugo) : Basing most of this on : https://r3dux.org/2011/01/a-simple-sdl_net-chat-server-client/

#include "synchess_network.h"

s32 main(s32 ArgumentCount, char** Arguments)
{
	// NOTE(hugo) : Init SDL_Net
	s32 SDLNetInitResult = SDLNet_Init();
	Assert(SDLNetInitResult != -1);

	u32 MaxSocketCount = 3;
	SDLNet_SocketSet SocketSet = SDLNet_AllocSocketSet(MaxSocketCount);
	Assert(SocketSet);

	//u32 MaxClientCount = MaxSocketCount - 1; // NOTE(hugo) : The server needs a socket.
	TCPsocket ClientSockets[2];
	u32 CurrentClientCount = 0;

	for(u32 ClientIndex = 0; ClientIndex < ArrayCount(ClientSockets); ++ClientIndex)
	{
		ClientSockets[ClientIndex] = 0;
	}

	u32 ServerPort = 1234;

	IPaddress ServerAddress;
	s32 HostResolved = SDLNet_ResolveHost(&ServerAddress, 0, ServerPort);
	Assert(HostResolved != -1);

	printf("Successfully resolved host server to IP 0x%08x, port 0x%04x.\n", ServerAddress.host, ServerAddress.port);

	TCPsocket ServerSocket = SDLNet_TCP_Open(&ServerAddress);
	Assert(ServerSocket);

	s32 AddSocketResult = SDLNet_TCP_AddSocket(SocketSet, ServerSocket);
	Assert(AddSocketResult != -1);

	u32 SDLInitResult = SDL_Init(SDL_INIT_EVERYTHING);
	Assert(SDLInitResult == 0);

	bool Running = true;
	while(Running)
	{
		s32 ActiveSocketCount = SDLNet_CheckSockets(SocketSet, 0);
		Assert(ActiveSocketCount != -1);

		s32 ServerSocketActivity = SDLNet_SocketReady(ServerSocket);
		Assert(ServerSocketActivity != -1);
		if(ServerSocketActivity > 0)
		{
			// NOTE(hugo) : An incoming connexion is pending
			if(CurrentClientCount < ArrayCount(ClientSockets))
			{
				ClientSockets[CurrentClientCount] = SDLNet_TCP_Accept(ServerSocket);
				Assert(ClientSockets[CurrentClientCount]);

				s32 AddSocketResult = SDLNet_TCP_AddSocket(SocketSet, ClientSockets[CurrentClientCount]);
				Assert(AddSocketResult != -1);

				SDLNet_TCP_Send(ClientSockets[CurrentClientCount], "Hello, sailor !\n", 16);
				++CurrentClientCount;

				printf("A new client connected !\n");

				Running = false;
			}
			else
			{
				// NOTE(hugo) : No room for the incoming connexion. Tell him we are full
				TCPsocket TempSocket = SDLNet_TCP_Accept(ServerSocket);
				Assert(TempSocket);
				SDLNet_TCP_Send(TempSocket, "No room...\n", 11);
				SDLNet_TCP_Close(TempSocket);
			}
		}

		for(u32 ClientIndex = 0; ClientIndex < ArrayCount(ClientSockets); ++ClientIndex)
		{
			TCPsocket ClientSocket = ClientSockets[ClientIndex];
			if(!ClientSocket)
			{
				continue;
			}

			s32 ClientSocketActivity = SDLNet_SocketReady(ClientSocket);
			Assert(ClientSocketActivity != -1);
			if(ClientSocketActivity > 0)
			{
				char Buffer[1024] = {};
				s32 ReceivedBytes = SDLNet_TCP_Recv(ClientSocket, Buffer, ArrayCount(Buffer));
				Assert(ReceivedBytes != -1);
				Assert(ReceivedBytes < (s32)ArrayCount(Buffer));
				if(ReceivedBytes == 0)
				{
					// NOTE(hugo) : Connexion closed.
					SDLNet_TCP_DelSocket(SocketSet, ClientSocket);
					ClientSockets[ClientIndex] = 0;
				}
				else
				{
					// NOTE(hugo) : A message was received
					printf("Received from client #%i : %s\n", ClientIndex, Buffer);
				}
			}
		}
	}

	SDL_Delay(1000);
	network_synchess_message Message = {};
	Message.Type = NetworkMessageType_Quit;	
	SDLNet_TCP_Send(ClientSockets[0], &Message, sizeof(Message));

	SDLNet_FreeSocketSet(SocketSet);
	SDLNet_TCP_Close(ServerSocket);

	SDLNet_Quit();

	return(0);
}
