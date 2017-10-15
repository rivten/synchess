#ifdef _WIN32
#include <SDL.h>
#include <SDL_net.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
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

struct game_state
{
	renderer Renderer;
	memory_arena GameArena;

	chess_game_context ChessContext;

	u32 SquareSizeInPixels;

	user_mode UserMode;
	chess_piece* PawnToPromote;

	move_type TileHighlighted[64];
	bitmap PieceBitmaps[PieceType_Count * PieceColor_Count];
	v2i ClickedTile;
	v2i SelectedPieceP;

	// NOTE(hugo) : Network
	// TODO(hugo) : Are we sure we should put the network
	// stuff into the game state or into its own struct ?
	// Maybe do this into a platform_api struct in the future
	TCPsocket ClientSocket;

	bool IsInitialised;
};

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
	for(u32 SquareY = 0; SquareY < 8; ++SquareY)
	{
		for(u32 SquareX = 0; SquareX < 8; ++SquareX)
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

#include "chess.cpp"

internal void
ClearTileHighlighted(game_state* GameState)
{
	for(u32 TileIndex = 0; TileIndex < ArrayCount(GameState->TileHighlighted); ++TileIndex)
	{
		// TODO(hugo) : Not a cool convention because it forces us to have
		// a null move type which is not exciting.
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

#include "synchess_network.h"

// TODO(hugo) : Get rid of the SDL_Renderer parameter in there : 
// this can be done using the platform_api struct (see HandmadeHero for more)
internal void
GameUpdateAndRender(game_memory* GameMemory, game_input* Input, SDL_Renderer* SDLRenderer, TCPsocket ClientSocket)
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

		GameState->UserMode = UserMode_MakeMove;

		GameState->SquareSizeInPixels = 64;
		LoadPieceBitmaps(&GameState->PieceBitmaps[0]);

		GameState->ChessContext.PlayerToPlay = PieceColor_White;

		GameState->ClientSocket = ClientSocket;

		GameState->IsInitialised = true;
	}

	// NOTE(hugo) : Update network state
	// {

	s32 ClientSocketActivity = SDLNet_SocketReady(GameState->ClientSocket);
	Assert(ClientSocketActivity != -1);
	if(ClientSocketActivity > 0)
	{
		network_synchess_message Message = {};
		s32 ReceivedBytes = SDLNet_TCP_Recv(GameState->ClientSocket, &Message, sizeof(Message));

		switch(Message.Type)
		{
			case NetworkMessageType_ConnectionEstablished:
				{
					printf("Connection Established\n");
				} break;
			case NetworkMessageType_Quit:
				{
					printf("Quit\n");
				} break;
			case NetworkMessageType_MoveDone:
				{
					printf("Move Done\n");
				} break;
			case NetworkMessageType_NoRoomForClient:
				{
					printf("No Room for you...\n");
				} break;

			InvalidDefaultCase;
		}
	}


	// }

	// NOTE(hugo) : Game update
	// {
	switch(GameState->UserMode)
	{
		case UserMode_MakeMove:
			{
				if(Pressed(Input->Mouse.Buttons[MouseButton_Left]))
				{
					GameState->ClickedTile = GetClickedTile(GameState->ChessContext.Chessboard, Input->Mouse.P);
					Assert(IsInsideBoard(GameState->ClickedTile));
					chess_piece* Piece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)];
					if(Piece && (Piece->Color == GameState->ChessContext.PlayerToPlay))
					{
						ClearTileHighlighted(GameState);

						temporary_memory HighlightingTileTempMemory = BeginTemporaryMemory(&GameState->GameArena);
						tile_list* PossibleMoveList = GetPossibleMoveList(&GameState->ChessContext, Piece, 
								GameState->ClickedTile, &GameState->GameArena);
						if(PossibleMoveList)
						{
							DeleteInvalidMoveDueToCheck(&GameState->ChessContext, Piece, GameState->ClickedTile, &PossibleMoveList, GameState->ChessContext.PlayerToPlay, &GameState->GameArena);
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
								case MoveType_DoubleStepPawn:
									{
										chess_piece* SelectedPiece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->SelectedPieceP)];
										chess_piece* EatenPiece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)];
										Assert(SelectedPiece);
										GameState->ChessContext.Chessboard[BOARD_COORD(GameState->SelectedPieceP)] = 0;
										GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)] = SelectedPiece;
										if((SelectedPiece->Type == PieceType_Pawn) && 
												((GameState->ChessContext.PlayerToPlay == PieceColor_White && GameState->ClickedTile.y == 7)||
												  (GameState->ChessContext.PlayerToPlay == PieceColor_Black && GameState->ClickedTile.y == 0)))
										{
											chess_piece* PromotedPawn = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->ClickedTile)];
											GameState->UserMode = UserMode_PromotePawn;
											GameState->PawnToPromote = PromotedPawn;
										}


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
										castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->ChessContext.PlayerToPlay;
										if(SelectedPiece->Type == PieceType_King)
										{
											CastlingPieceTrackerPlayer->KingHasMoved = true;
										}
										if(SelectedPiece->Type == PieceType_Rook)
										{
											v2i SelectedPieceP = GameState->SelectedPieceP;
											if((SelectedPieceP.x == 0) &&
													((GameState->ChessContext.PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
													 (GameState->ChessContext.PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
											{
												CastlingPieceTrackerPlayer->QueenRook.HasMoved = true;
											}
											else if((SelectedPieceP.x == 7) &&
													((GameState->ChessContext.PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
													 (GameState->ChessContext.PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
											{
												CastlingPieceTrackerPlayer->KingRook.HasMoved = true;
											}
										}
										if(EatenPiece && EatenPiece->Type == PieceType_Rook)
										{
											v2i SelectedPieceP = GameState->ClickedTile;
											if((SelectedPieceP.x == 0) &&
													((GameState->ChessContext.PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
													 (GameState->ChessContext.PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
											{
												CastlingPieceTrackerPlayer->QueenRook.IsFirstRank = true;
											}
											else if((SelectedPieceP.x == 7) &&
													((GameState->ChessContext.PlayerToPlay == PieceColor_White) && (SelectedPieceP.y == 0) ||
													 (GameState->ChessContext.PlayerToPlay == PieceColor_Black) && (SelectedPieceP.y == 7)))
											{
												CastlingPieceTrackerPlayer->KingRook.IsFirstRank = true;
											}
										}
									} break;
								case MoveType_CastlingKingSide:
									{
										piece_color PlayerMovingColor = GameState->ChessContext.PlayerToPlay;
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
										castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->ChessContext.PlayerToPlay;
										CastlingPieceTrackerPlayer->KingHasMoved = true;
									} break;
								case MoveType_CastlingQueenSide:
									{
										piece_color PlayerMovingColor = GameState->ChessContext.PlayerToPlay;
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
										castling_piece_tracker* CastlingPieceTrackerPlayer = GameState->ChessContext.CastlingPieceTracker + GameState->ChessContext.PlayerToPlay;
										CastlingPieceTrackerPlayer->KingHasMoved = true;
									} break;
								case MoveType_EnPassant:
									{
										chess_piece* SelectedPiece = GameState->ChessContext.Chessboard[BOARD_COORD(GameState->SelectedPieceP)];
										Assert(SelectedPiece);
										piece_color PlayerMovingColor = GameState->ChessContext.PlayerToPlay;
										v2i PieceDestP = GameState->ClickedTile;
										v2i PieceP = GameState->SelectedPieceP;
										if(PlayerMovingColor == PieceColor_White)
										{
											Assert(u32(PieceDestP.x) == GameState->ChessContext.LastDoubleStepCol);
											Assert(PieceDestP.y == 5);
											Assert(!GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 5]);

											GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 4] = 0;
											GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 5] = SelectedPiece;
											GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)] = 0;
										}
										else if(PlayerMovingColor == PieceColor_Black)
										{
											Assert(u32(PieceDestP.x) == GameState->ChessContext.LastDoubleStepCol);
											Assert(PieceDestP.y == 2);
											Assert(!GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 2]);

											GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 3] = 0;
											GameState->ChessContext.Chessboard[GameState->ChessContext.LastDoubleStepCol + 8 * 2] = SelectedPiece;
											GameState->ChessContext.Chessboard[BOARD_COORD(PieceP)] = 0;
										}
										else
										{
											InvalidCodePath;
										}
									} break;
								InvalidDefaultCase;
							}
							GameState->ChessContext.LastDoubleStepCol = (ClickMoveType == MoveType_DoubleStepPawn) ? GameState->ClickedTile.x : NO_PREVIOUS_DOUBLE_STEP;

							// NOTE(hugo): Update the config list
							chessboard_config_list* NewChessboardConfigList = 
								PushStruct(&GameState->GameArena, chessboard_config_list);
							NewChessboardConfigList->Config = WriteConfig(GameState->ChessContext.Chessboard);
							NewChessboardConfigList->Next = GameState->ChessContext.ChessboardConfigSentinel;
							GameState->ChessContext.ChessboardConfigSentinel = NewChessboardConfigList;

							ClearTileHighlighted(GameState);

							// NOTE(hugo) : Resolve situation for the other player
							GameState->ChessContext.PlayerCheck = SearchForKingCheck(&GameState->ChessContext, &GameState->GameArena);
							bool IsCurrentPlayerCheckmate = false;
							if(IsPlayerUnderCheck(OtherColor(GameState->ChessContext.PlayerToPlay), GameState->ChessContext.PlayerCheck))
							{
								IsCurrentPlayerCheckmate = IsPlayerCheckmate(&GameState->ChessContext, OtherColor(GameState->ChessContext.PlayerToPlay), &GameState->GameArena);
							}
							if(IsCurrentPlayerCheckmate)
							{
								printf("Checkmate !!\n");
								DEBUGWriteConfigListToFile(GameState->ChessContext.ChessboardConfigSentinel);
							}
							else if(IsDraw(&GameState->ChessContext, &GameState->GameArena))
							{
								printf("PAT !!\n");
								DEBUGWriteConfigListToFile(GameState->ChessContext.ChessboardConfigSentinel);
							}

							GameState->ChessContext.PlayerToPlay = OtherColor(GameState->ChessContext.PlayerToPlay);
						}
					}
				}
				if(Pressed(Input->Mouse.Buttons[MouseButton_Right]))
				{
					ClearTileHighlighted(GameState);
				}
			} break;
		case UserMode_PromotePawn:
			{
				Assert(GameState->PawnToPromote);

				bool PromotionChosen = false;
				if(Pressed(Input->Keyboard.Buttons[SCANCODE_Q]))
				{
					GameState->PawnToPromote->Type = PieceType_Queen;
					PromotionChosen = true;
				}
				else if(Pressed(Input->Keyboard.Buttons[SCANCODE_R]))
				{
					GameState->PawnToPromote->Type = PieceType_Rook;
					PromotionChosen = true;
				}
				else if(Pressed(Input->Keyboard.Buttons[SCANCODE_B]))
				{
					GameState->PawnToPromote->Type = PieceType_Bishop;
					PromotionChosen = true;
				}
				else if(Pressed(Input->Keyboard.Buttons[SCANCODE_N]))
				{
					GameState->PawnToPromote->Type = PieceType_Knight;
					PromotionChosen = true;
				}

				if(PromotionChosen)
				{
					GameState->UserMode = UserMode_MakeMove;
					GameState->PawnToPromote = 0;
				}

			} break;
		InvalidDefaultCase;
	}

	if(Pressed(Input->Keyboard.Buttons[SCANCODE_E]))
	{
		Assert(ClientSocket);
		network_synchess_message Message = {};
		Message.Type = NetworkMessageType_NoRoomForClient;
		SDLNet_TCP_Send(ClientSocket, &Message, sizeof(Message));
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

	SDL_Window* Window = SDL_CreateWindow("synchess @ rivten", 
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

	// NOTE(hugo):  SDLNet Init
	// {
	s32 SDLNetInitResult = SDLNet_Init();
	Assert(SDLNetInitResult != -1);

	SDLNet_SocketSet SocketSet = SDLNet_AllocSocketSet(1);
	Assert(SocketSet);

	u32 ServerPort = SYNCHESS_PORT;
	char* ServerName = SYNCHESS_SERVER_IP;
	IPaddress ServerAddress;
	s32 HostResolvedResult = SDLNet_ResolveHost(&ServerAddress, ServerName, ServerPort);
	Assert(HostResolvedResult != -1);

	TCPsocket ClientSocket = SDLNet_TCP_Open(&ServerAddress);
	Assert(ClientSocket);

	s32 AddSocketResult = SDLNet_TCP_AddSocket(SocketSet, ClientSocket);
	Assert(AddSocketResult != -1);

	s32 ActiveSocketCount = SDLNet_CheckSockets(SocketSet, 5000);
	Assert(ActiveSocketCount != -1);

	s32 ClientSocketActivity = SDLNet_SocketReady(ClientSocket);
	Assert(ClientSocketActivity != -1);

	if(ClientSocketActivity > 0)
	{
		s32 MessageFromServer = SDLNet_SocketReady(ClientSocket);
		Assert(MessageFromServer != -1);
		if(MessageFromServer > 0)
		{
			network_synchess_message Message = {};
			s32 ReceivedBytes = SDLNet_TCP_Recv(ClientSocket, &Message, sizeof(Message));
			Assert(ReceivedBytes != -1);
			Assert(ReceivedBytes <= (s32)sizeof(Message));

			// NOTE(hugo) : This should be the welcoming message
			printf("0x%08X\n", *(u32*)&Message);
		}
	}
	// }

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

		SDLNet_CheckSockets(SocketSet, 0);

		GameUpdateAndRender(&GameMemory, NewInput, Renderer, ClientSocket);

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

