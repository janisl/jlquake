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

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

glconfig_t	glConfig;
glstate_t	glState;

QCvar*		r_logFile;

QCvar*		r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
QCvar*		r_stencilbits;
QCvar*		r_depthbits;
QCvar*		r_colorbits;
QCvar*		r_stereo;
QCvar*		r_displayRefresh;
QCvar*		r_fullscreen;

QCvar*		r_verbose;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

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
}
