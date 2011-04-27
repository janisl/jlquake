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

trGlobals_base_t*	tr_shared;

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

QCvar*		r_gamma;
QCvar*		r_ignorehwgamma;
QCvar*		r_intensity;
QCvar*		r_overBrightBits;

QCvar*		r_wateralpha;
QCvar*		r_roundImagesDown;
QCvar*		r_picmip;
QCvar*		r_texturebits;
QCvar*		r_colorMipLevels;
QCvar*		r_simpleMipMaps;

QCvar*		r_ignoreGLErrors;

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
	//
	// latched and archived variables
	//
	r_mode = Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH2);
	r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customwidth = Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customheight = Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customaspect = Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_colorbits = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stencilbits = Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH2);
	r_depthbits = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stereo = Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_overBrightBits = Cvar_Get("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_roundImagesDown = Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_picmip = Cvar_Get("r_picmip", (GGameType & GAME_Quake3) ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH2);
	AssertCvarRange(r_picmip, 0, 16, true);
	r_texturebits = Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_simpleMipMaps = Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH2);

	//
	// temporary latched variables that can only change over a restart
	//
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH2);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH2);
	AssertCvarRange(r_displayRefresh, 0, 200, true);
	r_intensity = Cvar_Get("r_intensity", "1", CVAR_LATCH2);
	r_colorMipLevels = Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH2);

	//
	// archived variables that can change at any time
	//
	r_gamma = Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);
	r_wateralpha = Cvar_Get("r_wateralpha","0.4", CVAR_ARCHIVE);
	r_ignoreGLErrors = Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get("r_verbose", "0", CVAR_CHEAT);

	Cmd_AddCommand("modelist", R_ModeList_f);
}

//==========================================================================
//
//	R_GetTitleForWindow
//
//==========================================================================

const char* R_GetTitleForWindow()
{
	if (GGameType & GAME_QuakeWorld)
	{
		return "QuakeWorld";
	}
	if (GGameType & GAME_Quake)
	{
		return "Quake";
	}
	if (GGameType & GAME_HexenWorld)
	{
		return "HexenWorld";
	}
	if (GGameType & GAME_Hexen2)
	{
		return "Hexen II";
	}
	if (GGameType & GAME_Quake2)
	{
		return "Quake 2";
	}
	if (GGameType & GAME_Quake3)
	{
		return "Quake 3: Arena";
	}
	return "Unknown";
}

//==========================================================================
//
//	R_SetMode
//
//==========================================================================

static void R_SetMode()
{
	rserr_t err = GLimp_SetMode(r_mode->integer, r_colorbits->integer, !!r_fullscreen->integer);
	if (err == RSERR_OK)
	{
		return;
	}

	if (err == RSERR_INVALID_FULLSCREEN)
	{
		GLog.Write("...WARNING: fullscreen unavailable in this mode\n");

		Cvar_SetValue("r_fullscreen", 0);
		r_fullscreen->modified = false;

		err = GLimp_SetMode(r_mode->integer, r_colorbits->integer, false);
		if (err == RSERR_OK)
		{
			return;
		}
	}

	GLog.Write("...WARNING: could not set the given mode (%d)\n", r_mode->integer);

	// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
	// try it again but with a 16-bit desktop
	if (r_colorbits->integer != 16 || r_fullscreen->integer == 0 || r_mode->integer != 3)
	{
		err = GLimp_SetMode(3, 16, true);
		if (err == RSERR_OK)
		{
			return;
		}
		GLog.Write("...WARNING: could not set default 16-bit fullscreen mode\n");
	}

	// try setting it back to something safe
	err = GLimp_SetMode(3, r_colorbits->integer, false);
	if (err == RSERR_OK)
	{
		return;
	}

	GLog.Write("...WARNING: could not revert to safe mode\n");
	throw QException("R_SetMode() - could not initialise OpenGL subsystem\n" );
}

//==========================================================================
//
//	R_CommonInit
//
//	This is the OpenGL initialization function.  It is responsible for
// initialising OpenGL, setting extensions, creating a window of the
// appropriate size, doing fullscreen manipulations, etc.  Its overall
// responsibility is to make sure that a functional OpenGL subsystem is
// operating when it returns.
//
//==========================================================================

void R_CommonInit()
{	
	GLog.Write("Initializing OpenGL subsystem\n");

	//	Ceate the window and set up the context.
	R_SetMode();

	// 	Initialise our QGL dynamic bindings
	QGL_Init();

	//	Needed for Quake 3 UI vm.
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	//	Get our config strings.
	QStr::NCpyZ(glConfig.vendor_string, (char*)qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));
	QStr::NCpyZ(glConfig.renderer_string, (char*)qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));
	QStr::NCpyZ(glConfig.version_string, (char*)qglGetString(GL_VERSION), sizeof(glConfig.version_string));
	QStr::NCpyZ(glConfig.extensions_string, (char*)qglGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));

	// OpenGL driver constants
	GLint temp;
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
	glConfig.maxTextureSize = temp;

	//	Load palette used by 8-bit graphics files.
	if (GGameType & GAME_QuakeHexen)
	{
		R_InitQ1Palette();
	}
	else if (GGameType & GAME_Quake2)
	{
		R_InitQ2Palette();
	}
}

//==========================================================================
//
//	CommonGfxInfo_f
//
//==========================================================================

void CommonGfxInfo_f()
{
	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	GLog.Write("\nGL_VENDOR: %s\n", glConfig.vendor_string);
	GLog.Write("GL_RENDERER: %s\n", glConfig.renderer_string);
	GLog.Write("GL_VERSION: %s\n", glConfig.version_string);

	GLog.WriteLine("GL_EXTENSIONS:");
	QArray<QStr> Exts;
	QStr(glConfig.extensions_string).Split(' ', Exts);
	for (int i = 0; i < Exts.Num(); i++)
	{
		GLog.WriteLine(" %s", *Exts[i]);
	}

	GLog.Write("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	GLog.Write("GL_MAX_ACTIVE_TEXTURES: %d\n", glConfig.maxActiveTextures );
	GLog.Write("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	GLog.Write("MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1]);
	if (glConfig.displayFrequency)
	{
		GLog.Write("%d\n", glConfig.displayFrequency);
	}
	else
	{
		GLog.Write("N/A\n");
	}
}
