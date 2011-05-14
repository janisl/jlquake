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

#ifndef _R_LOCAL_H
#define _R_LOCAL_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#ifndef APIENTRY
#define APIENTRY
#endif

#include "render_qgl.h"
#include "render_image.h"
#include "render_state.h"

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

enum rserr_t
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
};

rserr_t GLimp_SetMode(int mode, int colorbits, bool fullscreen);
void GLimp_Shutdown();
void* GLimp_GetProcAddress(const char* Name);

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void GLimp_SwapBuffers();

bool GLimp_SpawnRenderThread(void (*function)());
void* GLimp_RendererSleep();
void GLimp_FrontEndSleep();
void GLimp_WakeRenderer(void* data);

/*
====================================================================

init

====================================================================
*/

#define MAX_DRAWIMAGES			2048

#define FOG_TABLE_SIZE			256

struct trGlobals_base_t
{
	int						frameCount;			// incremented every frame

	image_t*				defaultImage;
	image_t*				scrapImage;			// for small graphics
	image_t*				dlightImage;		// inverse-quare highlight for projective adding
	image_t*				whiteImage;			// full of 0xff
	image_t*				scratchImage[32];
	image_t*				fogImage;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	int						numImages;
	image_t*				images[MAX_DRAWIMAGES];

	float					fogTable[FOG_TABLE_SIZE];
};

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode);
void AssertCvarRange(QCvar* cv, float minVal, float maxVal, bool shouldBeIntegral);
void R_SharedRegister();
const char* R_GetTitleForWindow();
void R_CommonInit();
void CommonGfxInfo_f();

extern glconfig_t	glConfig;		// outside of TR since it shouldn't be cleared during ref re-init

extern QCvar*	r_logFile;				// number of frames to emit GL logs
extern QCvar*	r_verbose;				// used for verbose debug spew

extern QCvar*	r_mode;					// video mode
extern QCvar*	r_fullscreen;
extern QCvar*	r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
extern QCvar*	r_stencilbits;			// number of desired stencil bits
extern QCvar*	r_depthbits;			// number of desired depth bits
extern QCvar*	r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern QCvar*	r_stereo;				// desired pixelformat stereo flag
extern QCvar*	r_displayRefresh;		// optional display refresh option
extern QCvar*	r_swapInterval;

extern QCvar*	r_ext_compressed_textures;	// these control use of specific extensions
extern QCvar*	r_ext_multitexture;
extern QCvar*	r_ext_texture_env_add;
extern QCvar*	r_ext_compiled_vertex_array;

extern QCvar*	r_gamma;
extern QCvar*	r_ignorehwgamma;		// overrides hardware gamma capabilities
extern QCvar*	r_intensity;
extern QCvar*	r_overBrightBits;

extern QCvar*	r_wateralpha;
extern QCvar*	r_roundImagesDown;
extern QCvar*	r_picmip;				// controls picmip values
extern QCvar*	r_texturebits;			// number of desired texture bits
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error
extern QCvar*	r_colorMipLevels;		// development aid to see texture mip usage
extern QCvar*	r_simpleMipMaps;
extern QCvar*	r_textureMode;

extern QCvar*	r_ignoreGLErrors;

extern QCvar*	r_nobind;				// turns off binding to appropriate textures

extern trGlobals_base_t*	tr_shared;
#define tr		(*tr_shared)

/*
====================================================================

WAD files

====================================================================
*/

void R_LoadWadFile();
void* R_GetWadLumpByName(const char* name);

#endif
