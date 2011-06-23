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

#define MAX_DRAWIMAGES			2048
#define MAX_LIGHTMAPS			256
#define MAX_MOD_KNOWN			1500
//	Games must have their own constants because in Quake/Hexen 2 it's
// 64 and it's used by server.
#define MAX_LIGHTSTYLES			256

// 12 bits
// see QSORT_SHADERNUM_SHIFT
#define MAX_SHADERS				16384

#define MAX_SKINS				1024

#define FOG_TABLE_SIZE			256
#define FUNCTABLE_SIZE			1024
#define FUNCTABLE_MASK			(FUNCTABLE_SIZE - 1)
#define FUNCTABLE_SIZE2			10

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

struct orientationr_t
{
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelMatrix[16];
};

struct viewParms_t
{
	orientationr_t	orient;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	int			frameSceneNum;		// copied from tr.frameSceneNum
	int			frameCount;			// copied from tr.frameCount
	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	float		fovX, fovY;
	float		projectionMatrix[16];
	cplane_t	frustum[4];
	vec3_t		visBounds[2];
	float		zFar;
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

// skins allow models to be retextured without modifying the model file
struct skinSurface_t
{
	char		name[MAX_QPATH];
	shader_t*	shader;
};

struct skin_t
{
	char			name[MAX_QPATH];		// game path, including extension
	int				numSurfaces;
	skinSurface_t*	surfaces[MD3_MAX_SURFACES];
};

struct frontEndCounters_t
{
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
};

struct backEndCounters_t
{
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float	c_overDraw;
	
	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int		msec;			// total msec for backend run
};

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
struct trGlobals_t
{
	bool					registered;			// cleared at shutdown, set at beginRegistration

	int						frameCount;			// incremented every frame
	int						visCount;			// incremented every time a new vis cluster is entered
	int						sceneCount;			// incremented every scene
	int						viewCount;			// incremented every view (twice a scene if portaled)
												// and every R_MarkFragments call

	int						smpFrame;			// toggles from 0 to 1 every endFrame

	int						frameSceneNum;		// zeroed at RE_BeginFrame

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

	shader_t*				shadowShader;
	shader_t*				projectionShadowShader;

	shader_t*				flareShader;
	shader_t*				sunShader;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	trRefdef_t				refdef;

	trRefEntity_t*			currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t*				currentModel;

	viewParms_t				viewParms;

	orientationr_t			orient;				// for current entity

	int						viewCluster;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

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

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t*				shaders[MAX_SHADERS];
	shader_t*				sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t*					skins[MAX_SKINS];

	float					fogTable[FOG_TABLE_SIZE];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
};

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define MAX_RENDER_COMMANDS		0x40000

#define MAX_DRAWSURFS			0x10000
#define DRAWSURF_MASK			(MAX_DRAWSURFS-1)

#define MAX_ENTITIES			1023		// can't be increased without changing drawsurf bit packing
#define REF_ENTITYNUM_WORLD		MAX_ENTITIES

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define MAX_POLYS		600
#define MAX_POLYVERTS	3000

enum renderCommand_t
{
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT
};

struct renderCommandList_t
{
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
};

struct setColorCommand_t
{
	int		commandId;
	float	color[4];
};

struct stretchPicCommand_t
{
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
};

struct drawSurfsCommand_t
{
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
};

struct drawBufferCommand_t
{
	int		commandId;
	int		buffer;
};

struct swapBuffersCommand_t
{
	int		commandId;
};

struct screenshotCommand_t
{
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	qboolean jpeg;
};

// all state modified by the back end is seperated
// from the front end state
struct backEndState_t
{
	int			smpFrame;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	orient;
	backEndCounters_t	pc;
	qboolean	isHyperspace;
	trRefEntity_t	*currentEntity;
	qboolean	skyRenderedThisView;	// flag for drawing sun

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	byte		color2D[4];
	qboolean	vertexes2D;		// shader needs to be finished
	trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering
};

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
struct backEndData_t
{
	drawSurf_t			drawSurfs[MAX_DRAWSURFS];
	dlight_t			dlights[MAX_DLIGHTS];
	trRefEntity_t		entities[MAX_ENTITIES];
	srfPoly_t*			polys;//[MAX_POLYS];
	polyVert_t*			polyVerts;//[MAX_POLYVERTS];
	lightstyle_t		lightstyles[MAX_LIGHTSTYLES];
	renderCommandList_t	commands;
};

#define CULL_IN		0		// completely unclipped
#define CULL_CLIP	1		// clipped by one or more planes
#define CULL_OUT	2		// completely outside the clipping planes

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode);
void R_SharedRegister();
const char* R_GetTitleForWindow();
void R_CommonInitOpenGL();
void R_CommonInit1();
void R_CommonInit2();
void CommonGfxInfo_f();
void R_CommonShutdown(bool destroyWindow);

// fast float to int conversion
#if id386 && !( (defined __linux__ || defined __FreeBSD__ ) && (defined __i386__ ) ) // rb010123
long myftol(float f);
#else
#define	myftol(x) ((int)(x))
#endif

void R_NoiseInit();
float R_NoiseGet4f(float x, float y, float z, float t);

void R_ToggleSmpFrame();
void R_CommonRenderScene(const refdef_t* fd);

void myGlMultMatrix(const float* a, const float* b, float* out);
void R_DecomposeSort(unsigned Sort, int* EntityNum, shader_t** Shader, int* FogNum, int* DLightMap);
void R_SetupProjection();
void R_SetupFrustum();
void R_RotateForViewer();
void R_LocalNormalToWorld(vec3_t local, vec3_t world);
void R_LocalPointToWorld(vec3_t local, vec3_t world);
void R_TransformModelToClip(const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
	vec4_t eye, vec4_t dst);
void R_TransformClipToWindow(const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window);
void R_RotateForEntity(const trRefEntity_t* ent, const viewParms_t* viewParms,
	orientationr_t* orient);
int R_CullLocalBox(vec3_t bounds[2]);
int R_CullPointAndRadius(vec3_t origin, float radius);
int R_CullLocalPointAndRadius(vec3_t origin, float radius);

// font stuff
void R_InitFreeType();
void R_DoneFreeType();

void R_InitBackEndData();
void R_FreeBackEndData();

void R_InitSkins();
skin_t* R_GetSkinByHandle(qhandle_t hSkin);
void R_SkinList_f();

int R_LightPointQ1(vec3_t p);
void R_LightPointQ2(vec3_t p, vec3_t color);
void R_SetupEntityLighting(const trRefdef_t* refdef, trRefEntity_t* ent);
int R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);

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

extern QCvar*	r_ignoreFastPath;		// allows us to ignore our Tess fast paths
extern QCvar*	r_detailTextures;		// enables/disables detail texturing stages
extern QCvar*	r_uiFullScreen;			// ui is running fullscreen
extern QCvar*	r_printShaders;

extern QCvar*	r_saveFontData;

extern QCvar*	r_smp;
extern QCvar*	r_maxpolys;
extern QCvar*	r_maxpolyverts;

extern QCvar*	r_dynamiclight;			// dynamic lights enabled/disabled
extern QCvar*	r_znear;				// near Z clip plane

extern QCvar*	r_nocull;

extern QCvar*	r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing
extern QCvar*	r_vertex_arrays;
extern QCvar*	r_lerpmodels;
extern QCvar*	r_shadows;				// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection

extern QCvar*	r_modulate;
extern QCvar*	r_ambientScale;
extern QCvar*	r_directedScale;
extern QCvar*	r_debugLight;

extern QCvar*	r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern trGlobals_t	tr;

extern backEndState_t	backEnd;
extern backEndData_t*	backEndData[SMP_FRAMES];	// the second one may not be allocated

extern int		max_polys;
extern int		max_polyverts;

extern int			r_firstSceneDrawSurf;
extern int			r_numentities;
extern int			r_firstSceneEntity;
extern int			r_numdlights;
extern int			r_firstSceneDlight;
extern int			r_numpolys;
extern int			r_firstScenePoly;
extern int			r_numpolyverts;

extern float		s_flipMatrix[16];

extern vec3_t		lightspot;

extern int			c_alias_polys;

/*
====================================================================

WAD files

====================================================================
*/

void R_LoadWadFile();
void* R_GetWadLumpByName(const char* name);

//	Temporarily must be defined in game.
void R_InitSkyTexCoords( float cloudLayerHeight );
void R_SyncRenderThread();
void R_InitSky(mbrush29_texture_t *mt);
void GL_CreateSurfaceLightmap (mbrush38_surface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);
void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

#endif
