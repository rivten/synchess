#pragma once

enum render_mode
{
	RenderMode_None,
	RenderMode_UI,

	RenderMode_Count,
};

enum render_command_type
{
	RenderCommand_Rect,
	RenderCommand_Line,
	RenderCommand_RectBorder,
	RenderCommand_Clear,
	RenderCommand_Hex,
#if 0
	RenderCommand_Circle,
	RenderCommand_Text,
	RenderCommand_Bitmap,
#endif

	RenderCommand_Count,
};

enum render_command_flag
{
	RenderCommandFlag_UI = 1 << 0,
};

struct render_command
{
	render_command_type Type;
	render_command_flag Flags;
	union
	{
		struct // NOTE(hugo) : RenderCommand_Rect && RenderCommand_RectBorder
		{
			rect2 Rect;
			v4 Color;
		};
#if 0
		struct // NOTE(hugo) : RenderCommand_Text
		{
			v2 P;
			char Text[128];
			v4 TextColor;
			TTF_Font* Font;
		};
#endif
		struct // NOTE(hugo) : RenderCommand_Line
		{
			v2 Begin;
			v2 End;
			// TODO(hugo) : This name is shit because I use anonymous struct... hmmm
			v4 LineColor;
		};
#if 0
		struct // NOTE(hugo) : RenderCommand_Circle
		{
			circle Circle;
			v4 CircleColor;
		};
#endif
		struct // NOTE(hugo) : RenderCommand_Clear
		{
			v4 ClearColor;
		};
		struct // NOTE(hugo) : RenderCommand_Hex
		{
			v2 Center;
			float Radius;
			float InitialAngle;
			v4 HexColor;
		};
#if 0
		struct // NOTE(hugo) : RenderCommand_Bitmap
		{
			v2 Pos; // TODO(hugo) : Still shit because of anonymous struct... *sigh*
			bitmap Bitmap;
		};
#endif
	};
};

bool FlagSet(render_command_flag CommandFlags, render_command_flag Flag)
{
	bool Result = ((CommandFlags & Flag) == 1);

	return(Result);
}

void AddFlags(render_command* Command, render_command_flag Flags)
{
	Assert(!FlagSet(Command->Flags, Flags));

	// NOTE(hugo): Need to do a small hack because of the compiler :(
	// Otherwise I would have done Command->Flags |= Flags
	Command->Flags = (render_command_flag)(Command->Flags | Flags);
}

void DeleteFlags(render_command* Command, render_command_flag Flags)
{
	Assert(FlagSet(Command->Flags, Flags));

	Command->Flags = (render_command_flag)(Command->Flags & (!Flags));
}

struct renderer
{
	u32 CommandCount;
	memory_arena Arena;
	temporary_memory TemporaryMemory;
	SDL_Renderer* SDLContext;

	v2 WindowSizeInPixels;

	camera* Camera;

	render_mode Mode;

	// NOTE(hugo) : Caching to avoid 
	// over computation of SDL_Texture
	// WARNING(hugo) : VERY_IMPORTANT(hugo) :
	//    The textures in the cache are _never_ destroyed in SDL
	//    So if there is a cache-flush someday (when we hit the cache size limit)
	//    then I _must_ destroy the texture to avoid memory leaks
	void* CachedData[16];
	SDL_Texture* CachedTextures[16];
	u32 CacheCount;

	float PixelsToMeters;
};

