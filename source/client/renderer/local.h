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

#ifndef _R_LOCAL_H
#define _R_LOCAL_H

#include "public.h"
#include "../../common/shareddefs.h"
#include "../../common/hexen2defs.h"

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
#define SMP_FRAMES      2

#include "../../common/file_formats/bsp29.h"
#include "../../common/file_formats/bsp38.h"
#include "../../common/file_formats/bsp46.h"
#include "../../common/file_formats/bsp47.h"
#include "../../common/file_formats/mdl.h"
#include "../../common/file_formats/md2.h"
#include "../../common/file_formats/md3.h"
#include "../../common/file_formats/md4.h"
#include "../../common/file_formats/mdc.h"
#include "../../common/file_formats/mds.h"
#include "../../common/file_formats/mdm.h"
#include "../../common/file_formats/mdx.h"
#include "../../common/file_formats/spr.h"
#include "../../common/file_formats/sp2.h"

#include "qgl.h"
#include "images/image.h"
#include "shader.h"
#include "models/model.h"
#include "state.h"
#include "cvars.h"

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
const char* GLimp_GetSystemExtensionsString();
void* GLimp_GetProcAddress(const char* Name);

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void GLimp_SwapBuffers();

bool GLimp_SpawnRenderThread(void (* function)());
void* GLimp_RendererSleep();
void GLimp_FrontEndSleep();
void GLimp_WakeRenderer(void* data);

/*
====================================================================

init

====================================================================
*/

#define MAX_DRAWIMAGES          2048
#define MAX_LIGHTMAPS           256
#define MAX_MOD_KNOWN           2048

// 12 bits
// see QSORT_SHADERNUM_SHIFT
#define MAX_SHADERS             16384

#define MAX_SKINS               1024

#define FOG_TABLE_SIZE          256
#define FUNCTABLE_SIZE          4096
#define FUNCTABLE_MASK          (FUNCTABLE_SIZE - 1)
#define FUNCTABLE_SIZE2         12

// ydnar: optimizing diffuse lighting calculation with a table lookup
#define ENTITY_LIGHT_STEPS      16

#define MAX_PART_MODELS         5

#define MAX_WORLD_COORD         (128 * 1024)
#define MIN_WORLD_COORD         (-128 * 1024)

// ydnar: decals
#define MAX_WORLD_DECALS        1024
#define MAX_ENTITY_DECALS       128

struct dlight_t
{
	vec3_t origin;
	vec3_t color;				// range from 0.0 to 1.0, should be color normalized
	float radius;

	vec3_t transformed;			// origin in local coordinate system
	int additive;				// texture detail is lost tho when the lightmap is dark

	int overdraw;

	shader_t* shader;			//----(SA) adding a shader to dlights, so, if desired, we can change the blend or texture of a dlight

	bool forced;				//----(SA)	use this dlight when r_dynamiclight is either 1 or 2 (rather than just 1) for "important" gameplay lights (alarm lights, etc)

	float radiusInverseCubed;	// ydnar: attenuation optimization
	float intensity;			// 1.0 = fullbright, > 1.0 = overbright
	int flags;
};

struct lightstyle_t
{
	float rgb[3];				// 0.0 - 2.0
	float white;				// highest of rgb
};

struct particle_t
{
	vec3_t origin;
	byte rgba[4];
	float size;
	QParticleTexture Texture;
};

struct orientationr_t
{
	vec3_t origin;				// in world coordinates
	vec3_t axis[3];				// orientation in world
	vec3_t viewOrigin;			// viewParms->or.origin in local coordinates
	float modelMatrix[16];
};

struct viewParms_t
{
	orientationr_t orient;
	orientationr_t world;
	vec3_t pvsOrigin;				// may be different than or.origin for portals
	bool isPortal;					// true if this view is through a portal
	bool isMirror;					// the portal is a mirror, invert the face culling
	int frameSceneNum;				// copied from tr.frameSceneNum
	int frameCount;					// copied from tr.frameCount
	cplane_t portalPlane;			// clip anything behind this if mirroring
	int viewportX, viewportY, viewportWidth, viewportHeight;
	float fovX, fovY;
	float projectionMatrix[16];
	cplane_t frustum[5];			// ydnar: added farplane
	vec3_t visBounds[2];
	float zFar;

	glfog_t glFog;					// fog parameters

	int dirty;
};

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
struct trRefEntity_t
{
	refEntity_t e;

	float axisLength;			// compensate for non-normalized axis

	int dlightBits;				// for bmodels that touch a dlight
	bool lightingCalculated;
	vec3_t lightDir;			// normalized direction towards light
	vec3_t ambientLight;		// color normalized to 0-255
	int ambientLightInt;			// 32 bit rgba packed
	vec3_t directedLight;
	float brightness;
	int entityLightInt[ENTITY_LIGHT_STEPS];
};

struct corona_t
{
	vec3_t origin;
	vec3_t color;			// range from 0.0 to 1.0, should be color normalized
	vec3_t transformed;		// origin in local coordinate system
	float scale;			// uses r_flaresize as the baseline (1.0)
	int id;
	int flags;				// still send the corona request, even if not visible, for proper fading
};

// ydnar: decal projection
struct decalProjector_t
{
	shader_t* shader;
	byte color[4];
	int fadeStartTime, fadeEndTime;
	vec3_t mins, maxs;
	vec3_t center;
	float radius, radius2;
	qboolean omnidirectional;
	int numPlanes;					// either 5 or 6, for quad or triangle projectors
	vec4_t planes[6];
	vec4_t texMat[3][2];
};

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
struct trRefdef_t
{
	int x;
	int y;
	int width;
	int height;
	float fov_x;
	float fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];					// transformation matrix

	int time;							// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;						// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];
	bool areamaskModified;				// true if areamask changed since last scene

	float floatTime;					// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int num_entities;
	trRefEntity_t* entities;

	int num_dlights;
	dlight_t* dlights;
	int dlightBits;					// ydnar: optimization

	int numPolys;
	srfPoly_t* polys;

	int numDrawSurfs;
	drawSurf_t* drawSurfs;

	//	From Quake 2
	lightstyle_t* lightstyles;		// [MAX_LIGHTSTYLES]

	int num_particles;
	particle_t* particles;

	int num_coronas;
	corona_t* coronas;

	int numPolyBuffers;
	srfPolyBuffer_t* polybuffers;

	int decalBits;					// ydnar: optimization
	int numDecalProjectors;
	decalProjector_t* decalProjectors;

	int numDecals;
	srfDecal_t* decals;
};

// skins allow models to be retextured without modifying the model file
struct skinSurface_t
{
	char name[MAX_QPATH];
	shader_t* shader;
	int hash;
};

struct skinModel_t
{
	char type[MAX_QPATH];		// md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[MAX_QPATH];		// lower.md3, belt1.md3, etc.
	int hash;
};

struct skin_t
{
	char name[MAX_QPATH];					// game path, including extension
	int numSurfaces;
	skinSurface_t* surfaces[MD3_MAX_SURFACES];
	int numModels;
	skinModel_t* models[MAX_PART_MODELS];
	vec3_t scale;		//----(SA)	added
};

struct frontEndCounters_t
{
	int c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int c_leafs;
	int c_dlightSurfaces;
	int c_dlightSurfacesCulled;

	int c_sphere_cull_in, c_sphere_cull_out;
	int c_plane_cull_in, c_plane_cull_out;

	int c_decalProjectors, c_decalTestSurfaces, c_decalClipSurfaces, c_decalSurfaces, c_decalSurfacesCreated;
};

struct backEndCounters_t
{
	int c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float c_overDraw;

	int c_dlightVertexes;
	int c_dlightIndexes;

	int c_flareAdds;
	int c_flareTests;
	int c_flareRenders;

	int msec;				// total msec for backend run
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
	bool registered;							// cleared at shutdown, set at beginRegistration

	int frameCount;								// incremented every frame
	int visCount;								// incremented every time a new vis cluster is entered
	int sceneCount;								// incremented every scene
	int viewCount;								// incremented every view (twice a scene if portaled)
												// and every R_MarkFragments call

	int smpFrame;								// toggles from 0 to 1 every endFrame

	int frameSceneNum;							// zeroed at RE_BeginFrame

	bool worldMapLoaded;
	world_t* world;
	model_t* worldModel;
	char* worldDir;		// ydnar: for referencing external lightmaps

	image_t* defaultImage;
	image_t* scrapImage;						// for small graphics
	image_t* dlightImage;						// inverse-quare highlight for projective adding
	image_t* whiteImage;						// full of 0xff
	image_t* scratchImage[32];
	image_t* fogImage;
	image_t* particleImage;						// little dot for particles

	image_t* solidskytexture;
	image_t* alphaskytexture;

	shader_t* defaultShader;

	shader_t* shadowShader;
	shader_t* projectionShadowShader;

	shader_t* flareShader;
	shader_t* spotFlareShader;
	char* sunShaderName;
	shader_t* sunShader;
	shader_t* sunflareShader;	//----(SA) for the camera lens flare effect for sun

	float identityLight;						// 1.0 / ( 1 << overbrightBits )
	int identityLightByte;						// identityLight * 255
	int overbrightBits;							// r_overbrightBits->integer, but set to 0 if no hw gamma

	vec3_t sunLight;							// from the sky shader for this level
	vec3_t sunDirection;

	trRefdef_t refdef;

	trRefEntity_t* currentEntity;
	trRefEntity_t worldEntity;					// point currentEntity at this when rendering world
	int currentEntityNum;
	int shiftedEntityNum;						// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t* currentModel;
	mbrush46_model_t* currentBModel;	// only valid when rendering brush models

	viewParms_t viewParms;

	orientationr_t orient;						// for current entity

	int viewCluster;

	frontEndCounters_t pc;
	int frontEndMsec;							// not in pc due to clearing issue

	float lightGridMulAmbient;			// lightgrid multipliers specified in sky shader
	float lightGridMulDirected;			//

	// RF, temp var used while parsing shader only
	int allowCompress;

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t* models[MAX_MOD_KNOWN];
	int numModels;

	int numLightmaps;
	image_t* lightmaps[MAX_LIGHTMAPS];

	int numImages;
	image_t* images[MAX_DRAWIMAGES];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int numShaders;
	shader_t* shaders[MAX_SHADERS];
	shader_t* sortedShaders[MAX_SHADERS];

	int numSkins;
	skin_t* skins[MAX_SKINS];

	float fogTable[FOG_TABLE_SIZE];

	float sinTable[FUNCTABLE_SIZE];
	float squareTable[FUNCTABLE_SIZE];
	float triangleTable[FUNCTABLE_SIZE];
	float sawToothTable[FUNCTABLE_SIZE];
	float inverseSawToothTable[FUNCTABLE_SIZE];
};

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define MAX_RENDER_COMMANDS     0x40000

#define MAX_DRAWSURFS           0x40000
#define DRAWSURF_MASK           (MAX_DRAWSURFS - 1)

#define MAX_ENTITIES            1023		// can't be increased without changing drawsurf bit packing
#define REF_ENTITYNUM_WORLD     MAX_ENTITIES

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define MAX_POLYS       4096
#define MAX_POLYVERTS   8192

#define MAX_REF_PARTICLES       (8 * 1024)

#define MAX_CORONAS     32			//----(SA)	not really a reason to limit this other than trying to keep a reasonable count

// ydnar: max decal projectors per frame, each can generate lots of polys
#define MAX_DECAL_PROJECTORS    32	// uses bitmasks, don't increase
#define DECAL_PROJECTOR_MASK    (MAX_DECAL_PROJECTORS - 1)
#define MAX_DECALS              1024
#define DECAL_MASK              (MAX_DECALS - 1)

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

struct renderCommandList_t
{
	byte cmds[MAX_RENDER_COMMANDS];
	int used;
};

struct setColorCommand_t
{
	int commandId;
	float color[4];
};

struct stretchPicCommand_t
{
	int commandId;
	shader_t* shader;
	float x, y;
	float w, h;
	float s1, t1;
	float s2, t2;

	byte gradientColor[4];	// color values 0-255
	int gradientType;
	float angle;
};

struct drawSurfsCommand_t
{
	int commandId;
	trRefdef_t refdef;
	viewParms_t viewParms;
	drawSurf_t* drawSurfs;
	int numDrawSurfs;
};

struct drawBufferCommand_t
{
	int commandId;
	int buffer;
};

struct swapBuffersCommand_t
{
	int commandId;
};

struct screenshotCommand_t
{
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char* fileName;
	qboolean jpeg;
};

struct poly2dCommand_t
{
	int commandId;
	polyVert_t* verts;
	int numverts;
	shader_t* shader;
};

struct renderToTextureCommand_t
{
	int commandId;
	image_t* image;
	int x;
	int y;
	int w;
	int h;
};

struct renderFinishCommand_t
{
	int commandId;
};

// all state modified by the back end is seperated
// from the front end state
struct backEndState_t
{
	int smpFrame;
	trRefdef_t refdef;
	viewParms_t viewParms;
	orientationr_t orient;
	backEndCounters_t pc;
	qboolean isHyperspace;
	trRefEntity_t* currentEntity;
	qboolean skyRenderedThisView;		// flag for drawing sun

	qboolean projection2D;		// if true, drawstretchpic doesn't need to change modes
	byte color2D[4];
	qboolean vertexes2D;		// shader needs to be finished
	trRefEntity_t entity2D;		// currentEntity will point at this when doing 2D rendering
};

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
struct backEndData_t
{
	drawSurf_t drawSurfs[MAX_DRAWSURFS];
	dlight_t dlights[MAX_DLIGHTS];
	trRefEntity_t entities[MAX_ENTITIES];
	srfPoly_t* polys;			//[MAX_POLYS];
	polyVert_t* polyVerts;			//[MAX_POLYVERTS];
	lightstyle_t lightstyles[MAX_LIGHTSTYLES];
	particle_t particles[MAX_REF_PARTICLES];
	corona_t coronas[MAX_CORONAS];
	srfPolyBuffer_t polybuffers[MAX_POLYS];
	decalProjector_t decalProjectors[MAX_DECAL_PROJECTORS];
	srfDecal_t decals[MAX_DECALS];
	renderCommandList_t commands;
};

#define CULL_IN     0		// completely unclipped
#define CULL_CLIP   1		// clipped by one or more planes
#define CULL_OUT    2		// completely outside the clipping planes

extern glconfig_t glConfig;			// outside of TR since it shouldn't be cleared during ref re-init

extern trGlobals_t tr;

extern int r_firstSceneDrawSurf;
extern int r_numentities;
extern int r_firstSceneEntity;
extern int r_numdlights;
extern int r_firstSceneDlight;
extern int r_numpolys;
extern int r_firstScenePoly;
extern int r_numpolyverts;
extern int r_numparticles;
extern int r_firstSceneParticle;
extern int r_numDecalProjectors;

extern float s_flipMatrix[16];

extern vec3_t lightspot;

extern int c_brush_polys;
extern int c_alias_polys;
extern int c_visible_textures;
extern int c_visible_lightmaps;

#define TURBSCALE (256.0 / (2 * idMath::PI))
extern float r_turbsin[256];

extern int gl_NormalFontBase;

extern glfog_t glfogsettings[NUM_FOGS];		// [0] never used (FOG_NONE)
extern glfogType_t glfogNum;				// fog type to use (from the mbrush46_fog_t enum list)
extern bool fogIsOn;

extern int skyboxportal;
extern int drawskyboxportal;

/*
============================================================

INIT

============================================================
*/

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode);
const char* R_GetTitleForWindow();

/*
============================================================

NOISE

============================================================
*/

void R_NoiseInit();
float R_NoiseGet4f(float x, float y, float z, float t);

/*
============================================================

SCENE

============================================================
*/

void R_ToggleSmpFrame();

/*
============================================================

MAIN

============================================================
*/

void myGlMultMatrix(const float* a, const float* b, float* out);
void R_DecomposeSort(unsigned sort, int* entityNum, shader_t** shader,
	int* fogNum, int* dlightMap, int* frontFace, int* atiTess);
void R_SetupProjection();
void R_LocalNormalToWorld(vec3_t local, vec3_t world);
void R_LocalPointToWorld(vec3_t local, vec3_t world);
void R_TransformModelToClip(const vec3_t src, const float* modelMatrix, const float* projectionMatrix,
	vec4_t eye, vec4_t dst);
void R_TransformClipToWindow(const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window);
void R_RotateForEntity(const trRefEntity_t* ent, const viewParms_t* viewParms,
	orientationr_t* orient);
int R_CullLocalBox(vec3_t bounds[2]);
int R_CullPointAndRadius(vec3_t origin, float radius);
int R_CullLocalPointAndRadius(vec3_t origin, float radius);
void R_AddDrawSurf(surfaceType_t* surface, shader_t* shader, int fogIndex, int dlightMap, int frontFace, int atiTess);
void R_RenderView(viewParms_t* parms);

void R_FogOn();
void R_FogOff();
void R_Fog(glfog_t* curfog);

void R_DebugText(const vec3_t org, float r, float g, float b, const char* text, bool neverOcclude);

extern int cl_numtransvisedicts;
extern int cl_numtranswateredicts;

/*
============================================================

FONTS

============================================================
*/

// font stuff
void R_InitFreeType();
void R_DoneFreeType();

/*
============================================================

BACK END

============================================================
*/

void R_InitBackEndData();
void R_FreeBackEndData();
void RB_BeginDrawingView();
void RB_SetGL2D();
void RB_ShowImages();
void RB_ExecuteRenderCommands(const void* data);
void RB_RenderThread();

extern backEndState_t backEnd;
extern backEndData_t* backEndData[SMP_FRAMES];		// the second one may not be allocated

extern int max_polys;
extern int max_polyverts;

extern volatile bool renderThreadActive;

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

void R_InitCommandBuffers();
void R_ShutdownCommandBuffers();
void R_IssueRenderCommands(bool runPerformanceCounters);
void R_SyncRenderThread();
void* R_GetCommandBuffer(int bytes);
void R_AddDrawSurfCmd(drawSurf_t* drawSurfs, int numDrawSurfs);

/*
============================================================

SKINS

============================================================
*/

void R_InitSkins();
skin_t* R_GetSkinByHandle(qhandle_t hSkin);
void R_SkinList_f();

/*
============================================================

LIGHTS

============================================================
*/

#define DLIGHT_CUTOFF       64

int R_LightPointQ1(vec3_t p);
void R_LightPointQ2(vec3_t p, vec3_t color);
void R_SetupEntityLighting(const trRefdef_t* refdef, trRefEntity_t* ent);
void R_MarkLightsQ1(dlight_t* light, int bit, mbrush29_node_t* node);
void R_PushDlightsQ1();
void R_MarkLightsQ2(dlight_t* light, int bit, mbrush38_node_t* node);
void R_PushDlightsQ2();
void R_TransformDlights(int count, dlight_t* dl, orientationr_t* orient);
void R_DlightBmodel(mbrush46_model_t* bmodel);
void R_CullDlights();

/*
====================================================================

WAD files

====================================================================
*/

void R_LoadWadFile();
void* R_GetWadLumpByName(const char* name);

/*
====================================================================

Surfaces

====================================================================
*/

struct glRect_t
{
	byte l, t, w, h;
};

#define LIGHTMAP_BYTES 4

struct gllightmapstate_t
{
	int current_lightmap_texture;

	mbrush38_surface_t* lightmap_surfaces[MAX_LIGHTMAPS];

	int allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte lightmap_buffer[4 * BLOCK_WIDTH * BLOCK_HEIGHT];
};

void GL_BuildLightmaps();
void R_RenderBrushPolyQ1(mbrush29_surface_t* fa, bool override);
void R_DrawFullBrightPoly(mbrush29_surface_t* s);
void R_DrawSequentialPoly(mbrush29_surface_t* s);
void R_BlendLightmapsQ1();
void EmitWaterPolysQ1(mbrush29_surface_t* fa);
void DrawTextureChainsQ1();
void R_DrawWaterSurfaces();

void GL_BeginBuildingLightmaps(model_t* m);
void GL_CreateSurfaceLightmapQ2(mbrush38_surface_t* surf);
void GL_EndBuildingLightmaps();
image_t* R_TextureAnimationQ2(mbrush38_texinfo_t* tex);
void R_RenderBrushPolyQ2(mbrush38_surface_t* fa);
void DrawTextureChainsQ2();
void R_BlendLightmapsQ2();
void GL_RenderLightmappedPoly(mbrush38_surface_t* surf);
void R_DrawAlphaSurfaces();
void R_DrawTriangleOutlines();

void RB_CheckOverflow(int verts, int indexes);
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES) {RB_CheckOverflow(v,i); }
void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, byte* color);
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte* color, float s1, float t1, float s2, float t2);

extern mbrush29_glpoly_t* lightmap_polys[MAX_LIGHTMAPS];
extern mbrush29_leaf_t* r_viewleaf;
extern mbrush29_leaf_t* r_oldviewleaf;
extern mbrush29_surface_t* skychain;
extern mbrush29_surface_t* waterchain;
extern int skytexturenum;		// index in cl.loadmodel, not gl texture object

extern gllightmapstate_t gl_lms;
extern mbrush38_surface_t* r_alpha_surfaces;
extern int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern void(*rb_surfaceTable[SF_NUM_SURFACE_TYPES]) (void*);

/*
====================================================================

Skies

====================================================================
*/

void R_InitSky(mbrush29_texture_t* mt);
void EmitSkyPolys(mbrush29_surface_t* fa);
void EmitBothSkyLayers(mbrush29_surface_t* fa);
void R_DrawSkyChain(mbrush29_surface_t* s);
void R_ClearSkyBox();
void R_AddSkySurface(mbrush38_surface_t* fa);
void R_DrawSkyBoxQ2();
void R_InitSkyTexCoords(float cloudLayerHeight);
void RB_StageIteratorSky();
void RB_DrawSun();

extern float speedscale;		// for top sky and bottom sky

/*
====================================================================

WORLD MAP

====================================================================
*/

#define BACKFACE_EPSILON    0.01

void R_DrawBrushModelQ1(trRefEntity_t* e, bool Translucent);
void R_DrawWorldQ1();
void R_DrawBrushModelQ2(trRefEntity_t* e);
void R_DrawWorldQ2();
void R_AddBrushModelSurfaces(trRefEntity_t* e);
void R_AddWorldSurfaces();

/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares();
void RB_RenderFlares();

/*
============================================================

Screenshots

============================================================
*/

const void* RB_TakeScreenshotCmd(const void* data);
void R_ScreenShot_f();
void R_ScreenShotJPEG_f();

/*
============================================================

Particles

============================================================
*/

void R_InitParticleTexture();
void R_DrawParticles();

/*
============================================================

DECALS - ydnar

============================================================
*/

void R_AddModelShadow(const refEntity_t* ent);
void R_TransformDecalProjector(decalProjector_t * in, vec3_t axis[3], vec3_t origin, decalProjector_t * out);
bool R_TestDecalBoundingBox(decalProjector_t* dp, vec3_t mins, vec3_t maxs);
void R_ProjectDecalOntoSurface(decalProjector_t* dp, mbrush46_surface_t* surf, mbrush46_model_t* bmodel);
void R_AddDecalSurfaces(mbrush46_model_t* bmodel);
void R_CullDecalProjectors();

#endif
