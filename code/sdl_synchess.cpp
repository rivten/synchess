#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <rivten.h>
#include <rivten_math.h>

#include "input.h"

global_variable u32 GlobalWindowWidth = 1024;
global_variable u32 GlobalWindowHeight = 512;

struct camera
{
	v3 P;
#if 0
	// NOTE(hugo) : No need for them for now since the camera direction is fixed
	v3 XAxis;
	v3 ZAxis;
#endif
	float FocalLength;
};

SDL_Rect SDLRect(rect2 Rect)
{
	SDL_Rect Result = {};
	v2 Size = RectSize(Rect);
	Result.x = Rect.Min.x;
	Result.y = Rect.Min.y + Size.y;
	Result.w = Size.x;
	Result.h = Size.y;

	return(Result);
}

void SDLSetRenderColor(SDL_Renderer* Renderer, v4 Color)
{
	SDL_SetRenderDrawColor(Renderer, 255 * Color.r, 255 * Color.g, 255 * Color.b, 255 * Color.a);
}


#include "render.h"
#include "render.cpp"

struct game_memory
{
	u64 StorageSize;
	void* Storage;
};

struct game_state
{
	renderer Renderer;
	camera Camera;
	bool IsInitialized;
};

#define TILE_PER_SIDE 8

// TODO(hugo) : Get rid of the SDL_Renderer parameter in there
void GameUpdateAndRender(game_memory* GameMemory, game_input* Input, SDL_Renderer* SDLRenderer)
{
	Assert(sizeof(game_state) <= GameMemory->StorageSize);
	game_state* GameState = (game_state*) GameMemory->Storage;
	if(!GameState->IsInitialized)
	{
		u64 RenderArenaSize = Megabytes(128);
		Assert(sizeof(game_state) + RenderArenaSize <= GameMemory->StorageSize);
		void* RenderArenaMemoryBase = (u8*)GameMemory->Storage + sizeof(game_state);

		// TODO(hugo) : WHY 1/10 ?
		float PixelsToMeters = 1.0f / 10.0f;

		// TODO(hugo) : Resizable window
		InitRenderer(&GameState->Renderer, SDLRenderer, RenderArenaSize,
				V2(GlobalWindowWidth, GlobalWindowHeight), &GameState->Camera, PixelsToMeters, RenderArenaMemoryBase);

		GameState->IsInitialized = true;
	}

	renderer* Renderer = &GameState->Renderer;

	BeginRender(Renderer);

	PushClear(Renderer, V4(0.0f, 0.0f, 0.0f, 1.0f));

	BeginUI(Renderer);

	const u32 BoardSizeInPixels = 256;
	v2 BoardSize = V2(BoardSizeInPixels, BoardSizeInPixels);
	v2 BoardMin = 0.5f * V2(GlobalWindowWidth, GlobalWindowHeight) - 0.5f * BoardSize;
	rect2 BoardRect = RectFromMinSize(BoardMin, BoardSize);

	v2 TileSize = BoardSize / TILE_PER_SIDE;

	for(u32 TileY = 0; TileY < TILE_PER_SIDE; ++TileY)
	{
		for(u32 TileX = 0; TileX < TILE_PER_SIDE; ++TileX)
		{
			v2 TileMin = Hadamard(V2(TileX, TileY), TileSize) + BoardMin;
			rect2 TileRect = RectFromMinSize(TileMin, TileSize);
			bool IsWhiteTile = (TileX + TileY) % 2 == 0;
			v4 TileBackgroundColor = (IsWhiteTile) ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.0f, 1.0f, 0.0f, 1.0f);
			PushRect(Renderer, TileRect, TileBackgroundColor);
		}
	}

	EndUI(Renderer);


	// NOTE(hugo): Render the commands
	Render(Renderer);
	EndRender(Renderer);
}

s32 main(s32 ArgumentCount, char** Arguments)
{ 
	u32 SDLInitResult = SDL_Init(SDL_INIT_EVERYTHING);
	Assert(SDLInitResult == 0);

	bool Running = true;

	// TODO(hugo) : Should the window be resizable ?
	u32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	SDL_Window* Window = SDL_CreateWindow("The Road East", 
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			GlobalWindowWidth, GlobalWindowHeight, WindowFlags);
	Assert(Window);

	u32 RendererFlags = SDL_RENDERER_ACCELERATED;
	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, RendererFlags);
	Assert(Renderer);

	game_memory GameMemory = {};
	GameMemory.StorageSize = Megabytes(256);
	GameMemory.Storage = Allocate_(GameMemory.StorageSize);

	Assert(GameMemory.Storage);

	game_input Inputs[2] = {};
	game_input* NewInput = Inputs + 0;
	game_input* OldInput = Inputs + 1;

	// NOTE(hugo) : Timing info
	u32 LastCounter = SDL_GetTicks();
	u32 MonitorRefreshHz = 60;
	SDL_DisplayMode DisplayMode = {};
	if(SDL_GetCurrentDisplayMode(0, &DisplayMode) == 0)
	{
		if(DisplayMode.refresh_rate != 0)
		{
			MonitorRefreshHz = DisplayMode.refresh_rate;
		}
	}
	float GameUpdateHz = (MonitorRefreshHz / 2.0f);
	u32 TargetMSPerFrame = (u32)(1000.0f / GameUpdateHz);

	// TODO(hugo) : Maybe a little bit ugly to do this here
	NewInput->dtForFrame = 1.0f / GameUpdateHz;
	OldInput->dtForFrame = 1.0f / GameUpdateHz;

	while(Running)
	{
		//
		// NOTE(hugo) : Input gathering
		// {
		//
		SDL_Event Event;
		while(SDL_PollEvent(&Event))
		{
			switch(Event.type)
			{
				case SDL_QUIT:
					{
						Running = false;
					} break;
				default:
					{
					} break;
			}
		}
		SDLGetMouseInput(&NewInput->Mouse);
		SDLGetKeyboardInput(&NewInput->Keyboard);
		//
		// }
		//


		GameUpdateAndRender(&GameMemory, NewInput, Renderer);

		// NOTE(hugo) : Framerate computation
		u32 WorkMSElapsedForFrame = SDL_GetTicks() - LastCounter;

		// NOTE(hugo) : Setting the window title
		char WindowTitle[128];
		sprintf(WindowTitle, "synchess @ rivten - (%i, %i) - %i %i %i - %ims", (s32)NewInput->Mouse.P.x, (s32)NewInput->Mouse.P.y, (s32)NewInput->Mouse.Buttons[MouseButton_Left].IsDown, (s32)NewInput->Mouse.Buttons[MouseButton_Middle].IsDown, (s32)NewInput->Mouse.Buttons[MouseButton_Right].IsDown, WorkMSElapsedForFrame);
		SDL_SetWindowTitle(Window, WindowTitle);

		if(WorkMSElapsedForFrame < TargetMSPerFrame)
		{
			u32 SleepMS = TargetMSPerFrame - WorkMSElapsedForFrame;
			if(SleepMS > 0)
			{
				SDL_Delay(SleepMS);
			}
		}
		else
		{
			// TODO(hugo) : Missed framerate
		}

		// NOTE(hugo) : This must be at the very end
		LastCounter = SDL_GetTicks();

		// NOTE(hugo) : Switch inputs
		game_input* TempInput = NewInput;
		NewInput = OldInput;
		OldInput = TempInput;

		// TODO(hugo) : I think this is ok for perf but can we be sure ?
		// TODO(hugo): Is a full copy of the 512 scancodes even a good idea ?
		for(u32 ButtonIndex = 0; ButtonIndex < MouseButton_Count; ++ButtonIndex)
		{
			NewInput->Mouse.Buttons[ButtonIndex].WasDown = OldInput->Mouse.Buttons[ButtonIndex].IsDown;
		}
		for(u32 KeyButtonIndex = 0; KeyButtonIndex < ArrayCount(Inputs[0].Keyboard.Buttons); ++KeyButtonIndex)
		{
			NewInput->Keyboard.Buttons[KeyButtonIndex].WasDown = OldInput->Keyboard.Buttons[KeyButtonIndex].IsDown;
		}
	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);

	SDL_Quit();

	return(0);
}
