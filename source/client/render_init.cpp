//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct vidmode_t
{
	const char*	description;
	int         width;
	int			height;
	float		pixelAspect;		// pixel width / height
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

glconfig_t	glConfig;
glstate_t	glState;

QCvar*		r_logFile;

QCvar*		r_mode;
QCvar*		r_fullscreen;
QCvar*		r_customwidth;
QCvar*		r_customheight;
QCvar*		r_customaspect;

QCvar*		r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
QCvar*		r_stencilbits;
QCvar*		r_depthbits;
QCvar*		r_colorbits;
QCvar*		r_stereo;
QCvar*		r_displayRefresh;

QCvar*		r_verbose;

QCvar*		r_ignorehwgamma;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480",		640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480 (wide)",856,	480,	1 }
};
static int		s_numVidModes = sizeof(r_vidModes) / sizeof(r_vidModes[0]);

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_GetModeInfo
//
//==========================================================================

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode)
{
    if (mode < -1)
	{
        return false;
	}
	if (mode >= s_numVidModes)
	{
		return false;
	}

	if (mode == -1)
	{
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return true;
	}

	vidmode_t* vm = &r_vidModes[mode];

	*width  = vm->width;
	*height = vm->height;
	*windowAspect = (float)vm->width / (vm->height * vm->pixelAspect);

	return true;
}

//==========================================================================
//
//	R_ModeList_f
//
//==========================================================================

static void R_ModeList_f()
{
	GLog.Write("\n");
	for (int i = 0; i < s_numVidModes; i++ )
	{
		GLog.Write("%s\n", r_vidModes[i].description);
	}
	GLog.Write("\n");
}

//==========================================================================
//
//	AssertCvarRange
//
//==========================================================================

void AssertCvarRange(QCvar* cv, float minVal, float maxVal, bool shouldBeIntegral)
{
	if (shouldBeIntegral)
	{
		if ((int)cv->value != cv->integer)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value);
			Cvar_Set(cv->name, va("%d", cv->integer));
		}
	}

	if (cv->value < minVal)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal);
		Cvar_Set(cv->name, va("%f", minVal));
	}
	else if (cv->value > maxVal)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal);
		Cvar_Set(cv->name, va("%f", maxVal));
	}
}

//==========================================================================
//
//	R_SharedRegister
//
//==========================================================================

void R_SharedRegister() 
{
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get( "r_verbose", "0", CVAR_CHEAT );

	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH2);
	r_colorbits = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stencilbits = Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH2);
	r_depthbits = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stereo = Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH2);
	AssertCvarRange(r_displayRefresh, 0, 200, true);
	r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customwidth = Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customheight = Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customaspect = Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_mode = Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH2);

	Cmd_AddCommand("modelist", R_ModeList_f);
}
