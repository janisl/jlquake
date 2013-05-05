//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef __UI_H__
#define __UI_H__

#include "../../common/command_buffer.h"
#include "../renderer/public.h"

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

struct vrect_t {
	int x;
	int y;
	int width;
	int height;
};

struct viddef_t {
	int width;
	int height;
};

extern viddef_t viddef;					// global video state
extern qhandle_t char_texture;
extern qhandle_t char_smalltexture;
extern vec4_t g_color_table[ 32 ];

extern Cvar* scr_viewsize;
extern Cvar* crosshair;
extern float h2_introTime;
extern vrect_t scr_vrect;			// position of render window
extern Cvar* scr_showpause;
extern int scr_draw_loading;
extern Cvar* scr_netgraph;
extern Cvar* cl_debuggraph;
extern Cvar* cl_timegraph;
extern bool scr_initialized;			// ready to draw
extern Cvar* clq2_stereo_separation;
extern qhandle_t scrGfxPause;
extern qhandle_t scrGfxLoading;

void UI_AdjustFromVirtualScreen( float* x, float* y, float* w, float* h );
void UI_DrawPicShader( int x, int y, qhandle_t shader );
void UI_DrawStretchPicShader( int x, int y, int w, int h, qhandle_t shader );
void UI_DrawSubPic( int x, int y, qhandle_t shader, int srcx, int srcy, int width, int height );
void UI_TileClear( int x, int y, int w, int h, qhandle_t pic );
void UI_FillPal( int x, int y, int w, int h, int c );
void UI_DrawCharBase( int x, int y, int num, int w, int h, qhandle_t shader, int numberOfColumns,
	int numberOfRows );
void UI_DrawChar( int x, int y, int num );
void UI_DrawString( int x, int y, const char* str, int mask = 0 );
void UI_DrawSmallString( int x, int y, const char* str );

void SCR_FillRect( float x, float y, float width, float height, const float* color );
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void SCR_DrawSmallChar( int x, int y, int ch );
void SCR_DrawStringExt( int x, int y, float size, const char* string, float* setColor, bool forceColor );
void SCR_DrawBigString( int x, int y, const char* s, float alpha );			// draws a string with embedded color control characters with fade
void SCR_DrawSmallString( int x, int y, const char* string );

void SCR_Init();
void SCR_DrawDebugGraph();
void SCR_CenterPrint( const char* str );
void SCR_CheckDrawCenterString();
void SCR_ClearCenterString();
void SCR_CalcVrect();
void SCR_DrawNet();
void SCRQH_InitShaders();
void SCRQ2_InitShaders();
void SCR_DrawFPS();
void SCR_TileClear();
void SCR_UpdateScreen();

bool Field_KeyDownEvent( field_t* edit, int key );
void Field_CharEvent( field_t* edit, int ch );
void Field_Draw( field_t* edit, int x, int y, bool showCursor );
void Field_BigDraw( field_t* edit, int x, int y, bool showCursor );

void V_Init();
float CalcFov( float fovX, float width, float height );
void R_PolyBlend( refdef_t* fd, float* blendColour );

void UI_Init();
void UI_KeyEvent( int key, bool down );
void UI_KeyDownEvent( int key );
void UI_CharEvent( int key );
void UI_MouseEvent( int dx, int dy );
void UI_DrawMenu();
bool UI_IsFullscreen();
void UI_ForceMenuOff();
void UI_SetMainMenu();
void UI_SetInGameMenu();
void CL_InitRenderer();

#endif
