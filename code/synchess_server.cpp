#ifdef _WIN32
#include <SDL.h>
#include <SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#include <SDL2/SDL.h>
#endif

// NOTE(hugo) : The server for handling Synchess request.

#include "rivten.h"

// NOTE(hugo) : Basing most of this on : https://r3dux.org/2011/01/a-simple-sdl_net-chat-server-client/

s32 main(s32 ArgumentCount, char** Arguments)
{
	// NOTE(hugo) : Init SDL_Net
	s32 SDLNetInitResult = SDLNet_Init();
	Assert(SDLNetInitResult != -1);

	u32 MaxSocketCount = 3;
	SDLNet_SocketSet SocketSet = SDLNet_AllocSocketSet(MaxSocketCount);
	Assert(SocketSet);

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
		// NOTE(hugo) : Input polling
		SDL_Event Event;
		while(SDL_PollEvent(&Event))
		{
			switch(Event.type)
			{
				case SDL_QUIT:
					{
						Running = false;
					} break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					{
						printf("Key pressed\n");
						if(Event.key.keysym.scancode == SDL_SCANCODE_Q)
						{
							Running = false;
						}
					} break;
				default:
					{
					} break;
			}
		}

		s32 ActiveSocketCount = SDLNet_CheckSockets(SocketSet, 0);
		Assert(ActiveSocketCount != -1);
		if(ActiveSocketCount > 0)
		{
		}
	}

	return(0);
}
