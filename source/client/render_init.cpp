//**************************************************************************
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
QCvar*		r_swapInterval;

QCvar*		r_verbose;

QCvar*		r_ext_compressed_textures;
QCvar*		r_ext_multitexture;
QCvar*		r_ext_texture_env_add;
QCvar*		r_ext_gamma_control;
QCvar*		r_ext_compiled_vertex_array;
QCvar*		r_ext_point_parameters;

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
QCvar*		r_textureMode;

QCvar*		r_ignoreGLErrors;

QCvar*		r_nobind;

QCvar*		r_mapOverBrightBits;
QCvar*		r_vertexLight;
QCvar*		r_lightmap;
QCvar*		r_fullbright;
QCvar*		r_singleShader;

QCvar*		r_subdivisions;

QCvar*		r_ignoreFastPath;
QCvar*		r_detailTextures;
QCvar*		r_uiFullScreen;
QCvar*		r_printShaders;

QCvar*		r_saveFontData;

QCvar*		r_smp;
QCvar*		r_maxpolys;
QCvar*		r_maxpolyverts;

QCvar*		r_dynamiclight;
QCvar*		r_znear;

QCvar*		r_nocull;

QCvar*		r_primitives;
QCvar*		r_vertex_arrays;
QCvar*		r_lerpmodels;
QCvar*		r_shadows;
QCvar*		r_debugSort;
QCvar*		r_showtris;
QCvar*		r_shownormals;
QCvar*		r_offsetFactor;
QCvar*		r_offsetUnits;
QCvar*		r_clear;

QCvar*		r_modulate;
QCvar*		r_ambientScale;
QCvar*		r_directedScale;
QCvar*		r_debugLight;

QCvar*		r_lightlevel;	// FIXME: This is a HACK to get the client's light level

QCvar*		r_lodbias;
QCvar*		r_lodscale;

QCvar*		r_skymip;
QCvar*		r_fastsky;
QCvar*		r_showsky;
QCvar*		r_drawSun;

QCvar*		r_lodCurveError;

QCvar*		r_ignore;

QCvar*		r_keeptjunctions;
QCvar*		r_texsort;

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

static void AssertCvarRange(QCvar* cv, float minVal, float maxVal, bool shouldBeIntegral)
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
	r_ext_compressed_textures = Cvar_Get("r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_multitexture = Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_texture_env_add = Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_gamma_control = Cvar_Get("r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_compiled_vertex_array = Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_point_parameters = Cvar_Get("r_ext_point_parameters", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_overBrightBits = Cvar_Get("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_roundImagesDown = Cvar_Get("r_roundImagesDown", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_picmip = Cvar_Get("r_picmip", (GGameType & GAME_Quake3) ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH2);
	AssertCvarRange(r_picmip, 0, 16, true);
	r_texturebits = Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_simpleMipMaps = Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_vertexLight = Cvar_Get("r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_subdivisions = Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignoreFastPath = Cvar_Get("r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_detailTextures = Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	//	Disabled until I fix it on Linux.
	r_smp = Cvar_Get("r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH2);

	//
	// temporary latched variables that can only change over a restart
	//
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH2);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH2);
	AssertCvarRange(r_displayRefresh, 0, 200, true);
	r_intensity = Cvar_Get("r_intensity", "1", CVAR_LATCH2);
	r_colorMipLevels = Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH2);
	r_mapOverBrightBits = Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH2);
	r_fullbright = Cvar_Get("r_fullbright", "0", CVAR_LATCH2 | CVAR_CHEAT);
	r_singleShader = Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH2);

	//
	// archived variables that can change at any time
	//
	r_swapInterval = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_gamma = Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);
	r_wateralpha = Cvar_Get("r_wateralpha","0.4", CVAR_ARCHIVE);
	r_ignoreGLErrors = Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_textureMode = Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE);
	r_dynamiclight = Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_primitives = Cvar_Get("r_primitives", "0", CVAR_ARCHIVE);
	r_vertex_arrays = Cvar_Get("r_vertex_arrays", "0", CVAR_ARCHIVE);
	r_modulate = Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	if (GGameType & GAME_Quake3)
	{
		r_shadows = Cvar_Get("cg_shadows", "1", 0);
	}
	else
	{
		r_shadows = Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	}
	r_lodbias = Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_fastsky = Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);
	r_drawSun = Cvar_Get("r_drawSun", "0", CVAR_ARCHIVE);
	r_lodCurveError = Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT);
	r_keeptjunctions = Cvar_Get("r_keeptjunctions", "1", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get("r_verbose", "0", CVAR_CHEAT);
	r_nobind = Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_lightmap = Cvar_Get("r_lightmap", "0", 0);
	r_uiFullScreen = Cvar_Get("r_uifullscreen", "0", 0);
	r_printShaders = Cvar_Get("r_printShaders", "0", 0);
	r_saveFontData = Cvar_Get("r_saveFontData", "0", 0);
	r_maxpolys = Cvar_Get("r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = Cvar_Get("r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);
	r_znear = Cvar_Get("r_znear", "4", CVAR_CHEAT);
	AssertCvarRange(r_znear, 0.001f, 200, true);
	r_nocull = Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_lightlevel = Cvar_Get("r_lightlevel", "0", 0);
	r_lerpmodels = Cvar_Get("r_lerpmodels", "1", 0);
	r_ambientScale = Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_directedScale = Cvar_Get("r_directedScale", "1", CVAR_CHEAT);
	r_debugLight = Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort = Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_showtris = Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_shownormals = Cvar_Get("r_shownormals", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits = Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
	r_lodscale = Cvar_Get("r_lodscale", "5", CVAR_CHEAT);
	r_clear = Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_skymip = Cvar_Get("r_skymip", "0", 0);
	r_showsky = Cvar_Get("r_showsky", "0", CVAR_CHEAT);
	r_ignore = Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_texsort = Cvar_Get("r_texsort", "1", 0);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	Cmd_AddCommand("modelist", R_ModeList_f);
	Cmd_AddCommand("imagelist", R_ImageList_f);
	Cmd_AddCommand("shaderlist", R_ShaderList_f);
	Cmd_AddCommand("skinlist", R_SkinList_f);
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
//	R_CommonInitOpenGL
//
//	This is the OpenGL initialization function.  It is responsible for
// initialising OpenGL, setting extensions, creating a window of the
// appropriate size, doing fullscreen manipulations, etc.  Its overall
// responsibility is to make sure that a functional OpenGL subsystem is
// operating when it returns.
//
//==========================================================================

void R_CommonInitOpenGL()
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

//==========================================================================
//
//	R_InitFunctionTables
//
//==========================================================================

static void R_InitFunctionTables()
{
	//
	// init function tables
	//
	for (int i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)FUNCTABLE_SIZE)));
		tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = (float) i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}
}

//==========================================================================
//
//	R_CommonInit1
//
//==========================================================================

void R_CommonInit1()
{
	GLog.Write("----- R_Init -----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if ((qintptr)tess.xyz & 15)
	{
		GLog.Write("WARNING: tess.xyz not 16 byte aligned\n");
	}
	Com_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

	R_InitFunctionTables();

	R_InitFogTable();

	R_NoiseInit();
}

//==========================================================================
//
//	R_CommonInit2
//
//==========================================================================

void R_CommonInit2()
{
	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	int err = qglGetError();
	if (err != GL_NO_ERROR)
	{
		GLog.Write("glGetError() = 0x%x\n", err);
	}

	GLog.Write("----- finished R_Init -----\n");
}

//==========================================================================
//
//	R_CommonShutdown
//
//==========================================================================

void R_CommonShutdown(bool destroyWindow)
{
	GLog.Write("RE_Shutdown( %i )\n", destroyWindow);

	Cmd_RemoveCommand("modellist");
	Cmd_RemoveCommand("imagelist");
	Cmd_RemoveCommand("shaderlist");
	Cmd_RemoveCommand("skinlist");

	if (tr.registered)
	{
		R_FreeModels();

		R_FreeShaders();

		R_DeleteTextures();

		R_FreeBackEndData();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if (destroyWindow)
	{
		GLimp_Shutdown();

		// shutdown QGL subsystem
		QGL_Shutdown();
	}

	tr.registered = false;
}
