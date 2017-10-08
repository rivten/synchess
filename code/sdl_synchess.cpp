#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <rivten.h>
#include <rivten_math.h>

#include "input.h"

global_variable u32 GlobalWindowWidth = 512;
global_variable u32 GlobalWindowHeight = 512;

internal SDL_Rect
SDLRect(rect2 Rect)
{
	SDL_Rect Result = {};
	v2 Size = RectSize(Rect);
	Result.x = Rect.Min.x;
	Result.y = Rect.Min.y + Size.y;
	Result.w = Size.x;
	Result.h = Size.y;

	return(Result);
}

internal void
SDLSetRenderColor(SDL_Renderer* Renderer, v4 Color)
{
	SDL_SetRenderDrawColor(Renderer, 255 * Color.r, 255 * Color.g, 255 * Color.b, 255 * Color.a);
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
struct bitmap
{
	u32 Width;
	u32 Height;
	void* Data;

	// NOTE(hugo) : Offset is in percent relative to the bitmap size
	v2 Offset;
	bool IsValid;
};

u32 SafeCastToU32(s32 Value)
{
	Assert(Value >= 0);
	u32 Result = (u32)Value;
	return(Result);
}

bitmap LoadBitmap(char* Filename, v2 Offset = V2(0.0f, 0.0f))
{
	// TODO(hugo) : Load off an arena
	bitmap Result = {};
	s32 Depth = 0;
	s32 Width = 0;
	s32 Height = 0;
	Result.Data = stbi_load(Filename, &Width, &Height, &Depth, 4);
	Assert(Result.Data);

	Result.Width = SafeCastToU32(Width);
	Result.Height = SafeCastToU32(Height);
	Assert(Result.Width > 0);
	Assert(Result.Height > 0);

	Result.Offset = Offset;

	Result.IsValid = true;

	return(Result);
}

void FreeBitmap(bitmap Bitmap)
{
	stbi_image_free(Bitmap.Data);
}


#include "render.h"
#include "render.cpp"

struct game_memory
{
	u64 StorageSize;
	void* Storage;
};

#include "synchess.h"

struct rgb8
{
	u8 r;
	u8 g;
	u8 b;
};

internal rgb8
RGB8(u8 r, u8 g, u8 b)
{
	rgb8 Result = {r, g, b};
	return(Result);
}

internal v4
RGB8ToV4(rgb8 Color)
{
	v4 Result = V4(Color.r, Color.g, Color.b, 1.0f);
	Result = Result / 255.0f;

	return(Result);
}

internal void
DisplayChessboardToConsole(board_tile* Chessboard)
{
	for(s32 LineIndex = 7; LineIndex >= 0; --LineIndex)
	{
		for(u32 RowIndex = 0; RowIndex < 8; ++RowIndex)
		{
			chess_piece* Piece = Chessboard[RowIndex + 8 * LineIndex];
			if(Piece)
			{
				switch(Piece->Type)
				{
					case PieceType_Pawn:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("p");
							}
							else
							{
								printf("P");
							}
						} break;
					case PieceType_Knight:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("n");
							}
							else
							{
								printf("N");
							}
						} break;
					case PieceType_Bishop:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("b");
							}
							else
							{
								printf("B");
							}
						} break;
					case PieceType_Rook:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("r");
							}
							else
							{
								printf("R");
							}
						} break;
					case PieceType_Queen:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("q");
							}
							else
							{
								printf("Q");
							}
						} break;
					case PieceType_King:
						{
							if(Piece->Color == PieceColor_White)
							{
								printf("k");
							}
							else
							{
								printf("K");
							}
						} break;
					InvalidDefaultCase;
				}
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
	}
}

u8 SafeTruncateU32ToU8(u32 Value)
{
	Assert(Value <= 0xFF);
	u8 Result = (Value & 0xFF);

	return(Result);
}

internal chessboard_config
WriteConfig(board_tile* Chessboard)
{
	chessboard_config Result = {};
	for(u32 SquareX = 0; SquareX < 8; ++SquareX)
	{
		for(u32 SquareY = 0; SquareY < 8; ++SquareY)
		{
			u32 SquareIndex = SquareX + 8 * SquareY;
			Result.Tiles[SquareIndex] = u8(0x0000);
			chess_piece* Piece = Chessboard[SquareIndex];
			if(Piece)
			{
				Result.Tiles[SquareIndex] = SafeTruncateU32ToU8(1 + Piece->Type + PieceType_Count * Piece->Color);
			}
		}
	}
	return(Result);
}

#define BOARD_COORD(P) (P).x + 8 * (P).y

internal void
ClearTileHighlighted(game_state* GameState)
{
	for(u32 TileIndex = 0; TileIndex < ArrayCount(GameState->TileHighlighted); ++TileIndex)
	{
		GameState->TileHighlighted[TileIndex] = MoveType_None;
	}
}

internal void
HighlightPossibleMoves(game_state* GameState, tile_list* TileList)
{
	for(tile_list* Tile = TileList; Tile; Tile = Tile->Next)
	{
		GameState->TileHighlighted[BOARD_COORD(Tile->P)] = Tile->MoveType;
	}
}

internal bitmap 
GetPieceBitmap(game_state* GameState, chess_piece Piece)
{
	bitmap Result = GameState->PieceBitmaps[Piece.Color * PieceType_Count + Piece.Type];

	return(Result);
}

internal void
LoadPieceBitmaps(bitmap* Bitmaps)
{
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Pawn] = LoadBitmap("../data/WhitePawn.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Knight] = LoadBitmap("../data/WhiteKnight.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Bishop] = LoadBitmap("../data/WhiteBishop.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Rook] = LoadBitmap("../data/WhiteRook.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_Queen] = LoadBitmap("../data/WhiteQueen.png");
	Bitmaps[PieceColor_White * PieceType_Count + PieceType_King] = LoadBitmap("../data/WhiteKing.png");

	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Pawn] = LoadBitmap("../data/BlackPawn.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Knight] = LoadBitmap("../data/BlackKnight.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Bishop] = LoadBitmap("../data/BlackBishop.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Rook] = LoadBitmap("../data/BlackRook.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_Queen] = LoadBitmap("../data/BlackQueen.png");
	Bitmaps[PieceColor_Black * PieceType_Count + PieceType_King] = LoadBitmap("../data/BlackKing.png");
}

internal v2i
GetClickedTile(board_tile* Chessboard, v2 MouseP)
{
	v2i Result = V2i(0, 0);
	v2 TileP = MouseP / (GlobalWindowHeight / 8);
	TileP.y = 8 - TileP.y;

	Result.x = u32(TileP.x);
	Result.y = u32(TileP.y);

	return(Result);
}

internal void
DEBUGWriteConfigListToFile(chessboard_config_list* Sentinel)
{
	FILE* FileHandle = fopen("list_move.bin", "wb");
	Assert(FileHandle);

	for(chessboard_config_list* Config = Sentinel;
			Config;
			Config = Config->Next)
	{
		chessboard_config ChessboardConfig = Config->Config;
		fwrite(&ChessboardConfig, sizeof(ChessboardConfig), 1, FileHandle);
	}

	fclose(FileHandle);
}

internal bool
ConfigMatch(chessboard_config* ConfigA, chessboard_config* ConfigB)
{
	bool Match = true;
	for(u32 TileIndex = 0;
			Match && (TileIndex < ArrayCount(ConfigA->Tiles));
			++TileIndex)
	{
		u8 TileA = ConfigA->Tiles[TileIndex];
		u8 TileB = ConfigB->Tiles[TileIndex];
		Match = (TileA == TileB);
	}

	return(Match);
}

#include "chess.cpp"

// TODO(hugo) : Get rid of the SDL_Renderer parameter in there
internal void
GameUpdateAndRender(game_memory* GameMemory, game_input* Input, SDL_Renderer* SDLRenderer)
{
	Assert(sizeof(game_state) <= GameMemory->StorageSize);
	game_state* GameState = (game_state*) GameMemory->Storage;
	if(!GameState->IsInitialised)
	{
		u64 RenderArenaSize = Megabytes(128);
		Assert(sizeof(game_state) + RenderArenaSize <= GameMemory->StorageSize);
		void* RenderArenaMemoryBase = (u8*)GameMemory->Storage + sizeof(game_state);

		// TODO(hugo) : Resizable window
		InitialiseRenderer(&GameState->Renderer, SDLRenderer, RenderArenaSize,
				V2(GlobalWindowWidth, GlobalWindowHeight), RenderArenaMemoryBase);

		u64 GameArenaSize = Megabytes(128);
		void* GameArenaMemoryBase = (u8*)GameMemory->Storage + sizeof(game_state) + RenderArenaSize;
		InitialiseArena(&GameState->GameArena, GameArenaSize, GameArenaMemoryBase);
		InitialiseChessContext(&GameState->ChessContext, &GameState->GameArena);

		GameState->SquareSizeInPixels = 64;
		LoadPieceBitmaps(&GameState->PieceBitmaps[0]);

		GameState->PlayerToPlay = PieceColor_White;

		GameState->IsInitialised = true;
	}

	// NOTE(hugo) : Game update
	// {
	if(Pressed(Input->Mouse.Buttons[MouseButton_Left]))
	{
		GameState->ClickedTile = GetClickedTile(GameState->ChessContext.Chessboard, Input->Mouse.P);
		Assert(IsInsideBoard(GameState->ClickedTile));
		chess_piece* Piece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)];
		if(Piece && (Piece->Color == GameState->PlayerToPlay))
		{
			ClearTileHighlighted(GameState);

			temporary_memory HighlightingTileTempMemory = BeginTemporaryMemory(&GameState->GameArena);
			tile_list* PossibleMoveList = GetPossibleMoveList(&GameState->ChessContext, Piece, 
					GameState->ClickedTile, &GameState->GameArena);
			if(PossibleMoveList)
			{
				DeleteInvalidMoveDueToCheck(&GameState->ChessContext, Piece, GameState->ClickedTile, &PossibleMoveList, GameState->PlayerToPlay, &GameState->GameArena);
			}

			HighlightPossibleMoves(GameState, PossibleMoveList);
			EndTemporaryMemory(HighlightingTileTempMemory);


			GameState->SelectedPieceP = GameState->ClickedTile;
		}
		else
		{
			move_type ClickMoveType = GameState->TileHighlighted[BOARD_COORD(GameState->ClickedTile)];
			if(ClickMoveType != MoveType_None)
			{
				// NOTE(hugo): Make the intended move effective
				// if the player selects a valid tile
				switch(ClickMoveType)
				{
					case MoveType_Regular:
						{
							chess_piece* SelectedPiece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->SelectedPieceP)];
							chess_piece* EatenPiece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)];
							Assert(SelectedPiece);
							GameState->ChessContext.Chessboard[BOARD_COORD(GameState->SelectedPieceP)] = 0;
							GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)] = SelectedPiece;

							// TODO(hugo) : Make sure we chage the castling_state
							// of the player if a rook or the king is moved
							// maybe a possibility to do this efficiently would be :
							// in the chess_game_context track the exact location
							// of each piece (or a pointer to them) so that,
							// if a pawn is promoted to a tower, we don't have issues
							// TODO(hugo) : NOTE(hugo) : This is not fully exhaustive because it does not take into account the fact
							// that a rook could have been into.
							// What I mean is that we don't need to have an exact castling_piece_tracker since, if some parameters are
							// correct, then the castling will be disabled for a player. Therefore it is useless to have the other
							// parameters be exact since the firsts are already meaningful to the decision.
							castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->PlayerToPlay;
							if(SelectedPiece->Type == PieceType_King)
							{
								CastlingPieceTrackerPlayer->KingHasMoved = true;
							}
							if(SelectedPiece->Type == PieceType_Rook)
							{
								v2i SelectedPieceP = GameState->SelectedPieceP;
								if((SelectedPieceP.x == 0) &&
										((GameState->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
										 (GameState->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
								{
									CastlingPieceTrackerPlayer->QueenRook.HasMoved = true;
								}
								else if((SelectedPieceP.x == 7) &&
										((GameState->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
										 (GameState->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
								{
									CastlingPieceTrackerPlayer->KingRook.HasMoved = true;
								}
							}
							if(EatenPiece && EatenPiece->Type == PieceType_Rook)
							{
								v2i SelectedPieceP = GameState->ClickedTile;
								if((SelectedPieceP.x == 0) &&
										((GameState->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
										 (GameState->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
								{
									CastlingPieceTrackerPlayer->QueenRook.IsFirstRank = true;
								}
								else if((SelectedPieceP.x == 7) &&
										((GameState->PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
										 (GameState->PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
								{
									CastlingPieceTrackerPlayer->KingRook.IsFirstRank = true;
								}
							}
						} break;
					case MoveType_CastlingKingSide:
						{
							piece_color PlayerMovingColor = GameState->PlayerToPlay;
							s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
							chess_piece* KingRook = GameState->ChessContext.Chessboard[BOARD_COORD(V2i(7, LineIndex))];
							Assert(KingRook->Type == PieceType_Rook);
							v2i PieceP = GameState->SelectedPieceP;
							v2i PieceDestP = GameState->ClickedTile;
							chess_piece* King = GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)];
							Assert(King->Type == PieceType_King);
							GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)] = 0;
							Assert(GameState->ChessContext.Chessboard[BOARD_COORD(PieceDestP)] == 0);
							GameState->ChessContext.Chessboard[BOARD_COORD(PieceDestP)] = King;
							GameState->ChessContext.Chessboard[BOARD_COORD(V2i(7, LineIndex))] = 0;
							GameState->ChessContext.Chessboard[BOARD_COORD(V2i(5, LineIndex))] = KingRook;

							// NOTE(hugo) : Disabling future castling possibilities
							castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->PlayerToPlay;
							CastlingPieceTrackerPlayer->KingHasMoved = true;
						} break;
					case MoveType_CastlingQueenSide:
						{
							piece_color PlayerMovingColor = GameState->PlayerToPlay;
							s32 LineIndex = (PlayerMovingColor == PieceColor_White) ? 0 : 7;
							chess_piece* QueenRook = GameState->ChessContext.Chessboard[BOARD_COORD(V2i(0, LineIndex))];
							Assert(QueenRook->Type == PieceType_Rook);
							v2i PieceP = GameState->SelectedPieceP;
							v2i PieceDestP = GameState->ClickedTile;
							chess_piece* King = GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)];
							Assert(King->Type == PieceType_King);
							GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)] = 0;
							Assert(GameState->ChessContext.Chessboard[BOARD_COORD(PieceDestP)] == 0);
							GameState->ChessContext.Chessboard[BOARD_COORD(PieceDestP)] = King;
							GameState->ChessContext.Chessboard[BOARD_COORD(V2i(0, LineIndex))] = 0;
							GameState->ChessContext.Chessboard[BOARD_COORD(V2i(3, LineIndex))] = QueenRook;

							// NOTE(hugo) : Disabling future castling possibilities
							castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->PlayerToPlay;
							CastlingPieceTrackerPlayer->KingHasMoved = true;
						} break;
					case MoveType_EnPassant:
						{
							// TODO(hugo): Implement
							InvalidCodePath;
						} break;
					InvalidDefaultCase;
				}

				// NOTE(hugo): Update the config list
				chessboard_config_list* NewChessboardConfigList = 
					PushStruct(&GameState->GameArena, chessboard_config_list);
				NewChessboardConfigList->Config = WriteConfig(GameState->ChessContext.Chessboard);
				NewChessboardConfigList->Next = GameState->ChessContext.ChessboardConfigSentinel;
				GameState->ChessContext.ChessboardConfigSentinel = NewChessboardConfigList;

				ClearTileHighlighted(GameState);

				// NOTE(hugo) : Resolve situation for the other player
				GameState->ChessContext.PlayerCheck = SearchForKingCheck(&GameState->ChessContext, &GameState->GameArena);
				bool IsCurrentPlayerCheckmate = IsPlayerCheckmate(&GameState->ChessContext, OtherColor(GameState->PlayerToPlay), &GameState->GameArena);
				if(IsCurrentPlayerCheckmate)
				{
					printf("Checkmate !!\n");
					DEBUGWriteConfigListToFile(GameState->ChessContext.ChessboardConfigSentinel);
				}
				else if(IsDraw(GameState))
				{
					printf("PAT !!\n");
					DEBUGWriteConfigListToFile(GameState->ChessContext.ChessboardConfigSentinel);
				}

				GameState->PlayerToPlay = OtherColor(GameState->PlayerToPlay);
			}
		}
	}
	if(Pressed(Input->Mouse.Buttons[MouseButton_Right]))
	{
		ClearTileHighlighted(GameState);
	}
	// }

	renderer* Renderer = &GameState->Renderer;

	BeginRender(Renderer);

	PushClear(Renderer, V4(0.0f, 0.0f, 0.0f, 1.0f));

	// NOTE(hugo) : Render background
	// {
	u32 BoardSizeInPixels = 8 * GameState->SquareSizeInPixels;
	v2 BoardSize = V2(BoardSizeInPixels, BoardSizeInPixels);
	v2 BoardMin = 0.5f * V2(GlobalWindowWidth, GlobalWindowHeight) - 0.5f * BoardSize;
	rect2 BoardRect = RectFromMinSize(BoardMin, BoardSize);

	u32 SquareSizeInPixels = (u32)(BoardSizeInPixels / 8);
	v2i SquareSize = V2i(SquareSizeInPixels, SquareSizeInPixels);

	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
		{
			v2 SquareMin = Hadamard(V2(SquareX, SquareY), V2(SquareSize)) + BoardMin;
			rect2 SquareRect = RectFromMinSize(SquareMin, V2(SquareSize));
			bool IsWhiteTile = (SquareX + SquareY) % 2 != 0;
			v4 SquareBackgroundColor = (IsWhiteTile) ? 
				V4(1.0f, 1.0f, 1.0f, 1.0f) : RGB8ToV4(RGB8(64, 146, 59));

			if(GameState->TileHighlighted[SquareX + 8 * SquareY] != MoveType_None)
			{
				SquareBackgroundColor = V4(0.0f, 1.0f, 1.0f, 1.0f);
			}

			PushRect(Renderer, SquareRect, SquareBackgroundColor);

			// NOTE(hugo) : Draw the possible piece
			chess_piece* Piece = GameState->ChessContext.Chessboard[SquareX + 8 * SquareY];
			if(Piece)
			{
				v2 SquareMin = Hadamard(V2(SquareX, SquareY), V2(SquareSize)) + BoardMin;
				bitmap PieceBitmap = GetPieceBitmap(GameState, *Piece);
				if(PieceBitmap.IsValid)
				{
					PushBitmap(&GameState->Renderer, PieceBitmap, SquareMin);
				}
			}
		}
	}
	// }

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
	GameMemory.StorageSize = Megabytes(512);
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

