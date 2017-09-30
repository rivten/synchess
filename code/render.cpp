#pragma once

// TODO(hugo) : Parameter order is shit.
void InitialiseRenderer(renderer* Renderer, SDL_Renderer* SDLRenderer, u32 RenderArenaSize,
		v2 WindowSizeInPixels, void* BaseMemory)
{
	InitialiseArena(&Renderer->Arena, RenderArenaSize, BaseMemory);
	Renderer->CommandCount = 0;
	Renderer->TemporaryMemory = {};
	Renderer->SDLContext = SDLRenderer;
	Renderer->WindowSizeInPixels = WindowSizeInPixels;
}

void FlushCommands(renderer* Renderer)
{
	Renderer->CommandCount = 0;
	EndTemporaryMemory(Renderer->TemporaryMemory);
}

void FlushRenderCache(renderer* Renderer)
{
	for(u32 CacheIndex = 0; 
			CacheIndex < Renderer->CacheCount;
			++CacheIndex)
	{
		SDL_DestroyTexture(Renderer->CachedTextures[CacheIndex]);
	}

	Renderer->CacheCount = 0;
}

void BeginRender(renderer* Renderer)
{
	Renderer->TemporaryMemory = BeginTemporaryMemory(&Renderer->Arena);
}

void EndRender(renderer* Renderer)
{
	FlushCommands(Renderer);
	// TODO(hugo) : Find the right time to flush the cache
	//FlushRenderCache(Renderer);
}

void PushCommand(renderer* Renderer, render_command Command)
{
	render_command* PushedCommand = PushStruct(&Renderer->Arena, render_command);
	*PushedCommand = Command;
	++Renderer->CommandCount;
}

v2 SwitchBetweenWindowRenderCoords(renderer* Renderer, v2 P)
{
	v2 Result = {};
	Result.x = P.x;
	Result.y = Renderer->WindowSizeInPixels.y - P.y;

	return(Result);
}

SDL_Texture* FindTextureInCache(renderer* Renderer, void* Data)
{
	SDL_Texture* Found = 0;
	for(u32 DataIndex = 0;
			(!Found) && (DataIndex < Renderer->CacheCount);
			++DataIndex)
	{
		void* CachedData = Renderer->CachedData[DataIndex];
		if(CachedData == Data)
		{
			Found = Renderer->CachedTextures[DataIndex];
		}
	}

	return(Found);
}

void PushRenderCache(renderer* Renderer, SDL_Texture* Texture, void* Data)
{
	Assert(Renderer->CacheCount < ArrayCount(Renderer->CachedData));
	Renderer->CachedData[Renderer->CacheCount] = Data;
	Renderer->CachedTextures[Renderer->CacheCount] = Texture;
	++Renderer->CacheCount;
}

// TODO(hugo) : Is it better to change the coordinate system when rendering
// or when a command is pushed in the renderer's buffer ?
void Render(renderer* Renderer)
{
	for(u32 CommandIndex = 0; CommandIndex < Renderer->CommandCount; ++CommandIndex)
	{
		render_command* Command = (render_command *)Renderer->Arena.Base + CommandIndex;
		switch(Command->Type)
		{
			case RenderCommand_Rect:
				{
					rect2 DrawRect = Command->Rect;
					SDL_Rect CommandRect = SDLRect(DrawRect);
					CommandRect.y = Renderer->WindowSizeInPixels.y - CommandRect.y;

					SDLSetRenderColor(Renderer->SDLContext, Command->Color);
					SDL_RenderFillRect(Renderer->SDLContext, &CommandRect);
				} break;

			case RenderCommand_Line:
				{
					v2 Begin = Command->Begin;
					v2 End = Command->End;

					SDLSetRenderColor(Renderer->SDLContext, Command->LineColor);
					SDL_RenderDrawLine(Renderer->SDLContext, 
							Begin.x, Renderer->WindowSizeInPixels.y - Begin.y,
							End.x, Renderer->WindowSizeInPixels.y - End.y);
				} break;

			case RenderCommand_RectBorder:
				{
					rect2 DrawRect = Command->Rect;
					SDL_Rect CommandRect = SDLRect(DrawRect);
					CommandRect.y = Renderer->WindowSizeInPixels.y - CommandRect.y;

					SDLSetRenderColor(Renderer->SDLContext, Command->Color);
					SDL_RenderDrawRect(Renderer->SDLContext, &CommandRect);
				} break;

			case RenderCommand_Clear:
				{
					SDLSetRenderColor(Renderer->SDLContext, Command->ClearColor);
					SDL_RenderClear(Renderer->SDLContext);
				} break;
			case RenderCommand_Bitmap:
				{
					Assert(Command->Bitmap.IsValid);

					v2 BitmapSize = V2(Command->Bitmap.Width, Command->Bitmap.Height);
					v2 OffsetP = Hadamard(Command->Bitmap.Offset, BitmapSize);
					rect2 DestRect = RectFromMinSize(Command->Pos - OffsetP, BitmapSize);

					u32 RedMask   = 0x000000FF;
					u32 GreenMask = 0x0000FF00;
					u32 BlueMask  = 0x00FF0000;
					u32 AlphaMask = 0xFF000000;

					u32 Depth = 32;
					u32 Pitch = 4 * Command->Bitmap.Width;

					SDL_Texture* BitmapTexture = FindTextureInCache(Renderer, Command->Bitmap.Data);
					if(!BitmapTexture)
					{
						// NOTE(hugo) : The bitmap is not in the cache. Add it
						SDL_Surface* BitmapSurface = SDL_CreateRGBSurfaceFrom(Command->Bitmap.Data,
								Command->Bitmap.Width, Command->Bitmap.Height, Depth, Pitch,
								RedMask, GreenMask, BlueMask, AlphaMask);
						Assert(BitmapSurface);

						BitmapTexture = SDL_CreateTextureFromSurface(Renderer->SDLContext,
								BitmapSurface);
						SDL_FreeSurface(BitmapSurface);
						Assert(BitmapTexture);

						PushRenderCache(Renderer, BitmapTexture, Command->Bitmap.Data);
					}

					SDL_Rect SDLDestRect = SDLRect(DestRect);
					SDLDestRect.y = Renderer->WindowSizeInPixels.y - SDLDestRect.y;

					SDL_RenderCopy(Renderer->SDLContext, BitmapTexture,
							0, &SDLDestRect);

				} break;
#if 0
			case RenderCommand_Hex:
				{
					SDLDrawHexagon(Renderer->SDLContext, 
							SwitchBetweenWindowRenderCoords(Renderer, Command->Center), 
							Command->Radius, Command->HexColor, Command->InitialAngle);
				} break;

			case RenderCommand_Circle:
				{
					Command->Circle.P = SwitchBetweenWindowRenderCoords(Renderer, Command->Circle.P);

					SDLDrawCircle(Renderer->SDLContext, Command->Circle, Command->CircleColor);
				} break;

			case RenderCommand_Text:
				{
					v2 TextP = Command->P;
					DrawText(Renderer->SDLContext, Command->Text, SwitchBetweenWindowRenderCoords(Renderer, TextP), Command->Font, Command->TextColor);
				} break;
#endif

			InvalidDefaultCase;
		}
	}

	SDL_RenderPresent(Renderer->SDLContext);
}

void PushLine(renderer* Renderer, v2 Begin, v2 End, v4 Color)
{
	render_command Command = {};
	Command.Type = RenderCommand_Line;
	Command.Begin = Begin;
	Command.End = End;
	Command.LineColor = Color;

	PushCommand(Renderer, Command);
}

void PushRect(renderer* Renderer, rect2 Rect, v4 Color)
{
	render_command Command = {};
	Command.Type = RenderCommand_Rect;
	Command.Rect = Rect;
	Command.Color = Color;

	PushCommand(Renderer, Command);
}

void PushRectBorder(renderer* Renderer, rect2 Rect, v4 Color)
{
	render_command Command = {};
	Command.Type = RenderCommand_RectBorder;
	Command.Rect = Rect;
	Command.Color = Color;

	PushCommand(Renderer, Command);
}

void PushClear(renderer* Renderer, v4 Color)
{
	render_command Command = {};
	Command.Type = RenderCommand_Clear;
	Command.ClearColor = Color;

	PushCommand(Renderer, Command);
}

internal void 
PushBitmap(renderer* Renderer, bitmap Bitmap, v2 P)
{
	render_command Command = {};
	Command.Type = RenderCommand_Bitmap;
	Command.Pos = P;
	Command.Bitmap = Bitmap;

	PushCommand(Renderer, Command);
}

#if 0
void PushText(renderer* Renderer, v2 P, char* Text, TTF_Font* Font, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f))
{
	render_command Command = {};
	Command.Type = RenderCommand_Text;
	Command.P = P;
	sprintf(Command.Text, "%s", Text);
	Command.TextColor = Color;
	Command.Font = Font;

	PushCommand(Renderer, Command);
}

void PushHex(renderer* Renderer, v2 Center, float Radius, v4 Color, float InitialAngle = 0)
{
	render_command Command = {};
	Command.Type = RenderCommand_Hex;
	Command.Center = Center;
	Command.Radius = Radius;
	Command.HexColor = Color;
	Command.InitialAngle = InitialAngle;

	PushCommand(Renderer, Command);
}

void PushCircle(renderer* Renderer, v2 Center, float Radius, v4 Color)
{
	render_command Command = {};
	Command.Type = RenderCommand_Circle;
	Command.Circle.P = Center;
	Command.Circle.Radius = Radius;
	Command.CircleColor = Color;

	PushCommand(Renderer, Command);
}

#endif
