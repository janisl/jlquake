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

#ifndef _R_COMMANDS_H
#define _R_COMMANDS_H

#include "main.h"

#define MAX_RENDER_COMMANDS     0x40000

enum renderCommand_t
{
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_STRETCH_PIC_GRADIENT,
	RC_ROTATED_PIC,
	RC_2DPOLYS,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_RENDERTOTEXTURE,
	RC_FINISH,
	RC_SCREENSHOT
};

struct renderCommandList_t {
	byte cmds[ MAX_RENDER_COMMANDS ];
	int used;
};

struct setColorCommand_t {
	int commandId;
	float color[ 4 ];
};

struct stretchPicCommand_t {
	int commandId;
	shader_t* shader;
	float x, y;
	float w, h;
	float s1, t1;
	float s2, t2;

	byte gradientColor[ 4 ];	// color values 0-255
	int gradientType;
	float angle;
};

struct drawSurfsCommand_t {
	int commandId;
	trRefdef_t refdef;
	viewParms_t viewParms;
	drawSurf_t* drawSurfs;
	int numDrawSurfs;
};

struct drawBufferCommand_t {
	int commandId;
	int buffer;
};

struct swapBuffersCommand_t {
	int commandId;
};

struct screenshotCommand_t {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char* fileName;
	qboolean jpeg;
};

struct poly2dCommand_t {
	int commandId;
	polyVert_t* verts;
	int numverts;
	shader_t* shader;
};

struct renderToTextureCommand_t {
	int commandId;
	image_t* image;
	int x;
	int y;
	int w;
	int h;
};

struct renderFinishCommand_t {
	int commandId;
};

void R_InitCommandBuffers();
void R_ShutdownCommandBuffers();
void R_SyncRenderThread();
void R_IssueRenderCommands( bool runPerformanceCounters );
void* R_GetCommandBuffer( int bytes );
void R_AddDrawSurfCmd( drawSurf_t* drawSurfs, int numDrawSurfs );

#endif
