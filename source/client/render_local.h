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

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define	SMP_FRAMES		2

#include "../core/bsp29file.h"
#include "../core/bsp38file.h"
#include "../core/bsp46file.h"
#include "../core/mdlfile.h"
#include "../core/md2file.h"
#include "../core/md3file.h"
#include "../core/md4file.h"
#include "../core/sprfile.h"
#include "../core/sp2file.h"

#include "render_qgl.h"
#include "render_image.h"
#include "render_shader.h"
#include "render_model.h"
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

struct dlight_t
{
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
};

struct lightstyle_t
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
};

struct particle_t
{
	vec3_t	origin;
	int		color;
	float	alpha;
};

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
struct trRefEntity_t
{
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	bool		needDlights;	// true for bmodels that touch a dlight
	bool		lightingCalculated;
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
};

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
struct trRefdef_t
{
	int				x;
	int				y;
	int				width;
	int				height;
	float			fov_x;
	float			fov_y;
	vec3_t			vieworg;
	vec3_t			viewaxis[3];		// transformation matrix

	int				time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int				rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte			areamask[MAX_MAP_AREA_BYTES];
	bool			areamaskModified;	// true if areamask changed since last scene

	float			floatTime;			// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char			text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int				num_entities;
	trRefEntity_t*	entities;

	int				num_dlights;
	dlight_t*		dlights;

	int				numPolys;
	srfPoly_t*		polys;

	int				numDrawSurfs;
	drawSurf_t*		drawSurfs;

	//	From Quake 2
	lightstyle_t*	lightstyles;	// [MAX_LIGHTSTYLES]

	int				num_particles;
	particle_t*		particles;
};

#define MAX_DRAWIMAGES			2048
#define MAX_LIGHTMAPS			256
#define MAX_MOD_KNOWN			1500

#define FOG_TABLE_SIZE			256
#define FUNCTABLE_SIZE			1024
#define FUNCTABLE_MASK			(FUNCTABLE_SIZE - 1)

struct trGlobals_base_t
{
	int						frameCount;			// incremented every frame

	bool					worldMapLoaded;
	world_t*				world;
	model_t*				worldModel;

	image_t*				defaultImage;
	image_t*				scrapImage;			// for small graphics
	image_t*				dlightImage;		// inverse-quare highlight for projective adding
	image_t*				whiteImage;			// full of 0xff
	image_t*				scratchImage[32];
	image_t*				fogImage;

	shader_t*				defaultShader;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	trRefdef_t				refdef;

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t*				models[MAX_MOD_KNOWN];
	int						numModels;

	int						numLightmaps;
	image_t*				lightmaps[MAX_LIGHTMAPS];

	int						numImages;
	image_t*				images[MAX_DRAWIMAGES];

	float					fogTable[FOG_TABLE_SIZE];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
};

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode);
void AssertCvarRange(QCvar* cv, float minVal, float maxVal, bool shouldBeIntegral);
void R_SharedRegister();
const char* R_GetTitleForWindow();
void R_CommonInit();
void CommonGfxInfo_f();
void R_InitFunctionTables();

// fast float to int conversion
#if id386 && !( (defined __linux__ || defined __FreeBSD__ ) && (defined __i386__ ) ) // rb010123
long myftol(float f);
#else
#define	myftol(x) ((int)(x))
#endif

void R_NoiseInit();
float R_NoiseGet4f(float x, float y, float z, float t);

void R_CommonRenderScene(const refdef_t* fd);

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
extern QCvar*	r_ext_gamma_control;
extern QCvar*	r_ext_compiled_vertex_array;
extern QCvar*	r_ext_point_parameters;

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

extern QCvar*	r_mapOverBrightBits;
extern QCvar*	r_vertexLight;			// vertex lighting mode for better performance
extern QCvar*	r_lightmap;				// render lightmaps only
extern QCvar*	r_fullbright;			// avoid lightmap pass
extern QCvar*	r_singleShader;			// make most world faces use default shader

extern QCvar*	r_subdivisions;

extern trGlobals_base_t*	tr_shared;
#define tr		(*tr_shared)

/*
====================================================================

WAD files

====================================================================
*/

void R_LoadWadFile();
void* R_GetWadLumpByName(const char* name);

//	Temporarily must be defined in game.
void R_InitSkyTexCoords( float cloudLayerHeight );
shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
void R_SyncRenderThread();
void R_InitSky(mbrush29_texture_t *mt);
void GL_CreateSurfaceLightmap (mbrush38_surface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

#endif
