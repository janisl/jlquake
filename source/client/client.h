//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _CLIENT_H
#define _CLIENT_H

#include "../core/core.h"

#include "sound/public.h"
#include "renderer/public.h"
#include "input/keycodes.h"
#include "input/public.h"
#include "cinematic/public.h"
#include "ui/ui.h"
#include "hexen2clientdefs.h"
#include "quake2clientdefs.h"
#include "game/particles.h"

extern Cvar*		cl_inGameVideo;

void CL_SharedInit();
int CL_ScaledMilliseconds();
void CL_CalcQuakeSkinTranslation(int top, int bottom, byte* translate);
void CL_CalcHexen2SkinTranslation(int top, int bottom, int playerClass, byte* translate);

extern byte* playerTranslation;
extern int color_offsets[MAX_PLAYER_CLASS];

struct clientStaticCommon_t
{
	int framecount;
	int frametime;			// msec since last frame

	// rendering info
	glconfig_t glconfig;
	qhandle_t charSetShader;
	qhandle_t whiteShader;
	qhandle_t consoleShader;
};

struct clientActiveCommon_t
{
	int			serverTime;			// may be paused during play
};

extern clientStaticCommon_t* cls_common;
extern clientActiveCommon_t* cl_common;

char* Sys_GetClipboardData();	// note that this isn't journaled...

//	Called by Windows driver.
void Key_ClearStates();

float frand();	// 0 to 1
float crand();	// -1 to 1

#endif
