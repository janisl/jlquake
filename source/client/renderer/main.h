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

#ifndef _R_MAIN_H
#define _R_MAIN_H

#include "models/model.h"
#include "SurfacePoly.h"
#include "SurfacePolyBuffer.h"

struct skin_t;

#define MAX_DRAWIMAGES          2048
#define MAX_LIGHTMAPS           256
#define MAX_MOD_KNOWN           2048

// 12 bits
// see QSORT_SHADERNUM_SHIFT
#define MAX_SHADERS             16384

#define MAX_SKINS               1024

#define FOG_TABLE_SIZE          256
#define FUNCTABLE_SIZE          4096
#define FUNCTABLE_MASK          ( FUNCTABLE_SIZE - 1 )
#define FUNCTABLE_SIZE2         12

// ydnar: optimizing diffuse lighting calculation with a table lookup
#define ENTITY_LIGHT_STEPS      16

#define MAX_WORLD_COORD         ( 128 * 1024 )
#define MIN_WORLD_COORD         ( -128 * 1024 )

#define CULL_IN     0		// completely unclipped
#define CULL_CLIP   1		// clipped by one or more planes
#define CULL_OUT    2		// completely outside the clipping planes

struct dlight_t {
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

struct lightstyle_t {
	float rgb[ 3 ];					// 0.0 - 2.0
	float white;				// highest of rgb
};

struct particle_t {
	vec3_t origin;
	byte rgba[ 4 ];
	float size;
	QParticleTexture Texture;
};

struct orientationr_t {
	vec3_t origin;				// in world coordinates
	vec3_t axis[ 3 ];				// orientation in world
	vec3_t viewOrigin;			// viewParms->or.origin in local coordinates
	float modelMatrix[ 16 ];
};

struct viewParms_t {
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
	float projectionMatrix[ 16 ];
	cplane_t frustum[ 5 ];				// ydnar: added farplane
	vec3_t visBounds[ 2 ];
	float zFar;

	glfog_t glFog;					// fog parameters

	int dirty;
};

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
struct trRefEntity_t {
	refEntity_t e;

	float axisLength;			// compensate for non-normalized axis

	int dlightBits;				// for bmodels that touch a dlight
	bool lightingCalculated;
	vec3_t lightDir;			// normalized direction towards light
	vec3_t ambientLight;		// color normalized to 0-255
	int ambientLightInt;			// 32 bit rgba packed
	vec3_t directedLight;
	float brightness;
	int entityLightInt[ ENTITY_LIGHT_STEPS ];
};

struct corona_t {
	vec3_t origin;
	vec3_t color;			// range from 0.0 to 1.0, should be color normalized
	vec3_t transformed;		// origin in local coordinate system
	float scale;			// uses r_flaresize as the baseline (1.0)
	int id;
	int flags;				// still send the corona request, even if not visible, for proper fading
};

// ydnar: decal projection
struct decalProjector_t {
	shader_t* shader;
	byte color[ 4 ];
	int fadeStartTime, fadeEndTime;
	vec3_t mins, maxs;
	vec3_t center;
	float radius, radius2;
	qboolean omnidirectional;
	int numPlanes;					// either 5 or 6, for quad or triangle projectors
	vec4_t planes[ 6 ];
	vec4_t texMat[ 3 ][ 2 ];
};

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
struct trRefdef_t {
	int x;
	int y;
	int width;
	int height;
	float fov_x;
	float fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[ 3 ];					// transformation matrix

	int time;							// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;						// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[ MAX_MAP_AREA_BYTES ];
	bool areamaskModified;				// true if areamask changed since last scene

	float floatTime;					// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char text[ MAX_RENDER_STRINGS ][ MAX_RENDER_STRING_LENGTH ];

	int num_entities;
	trRefEntity_t* entities;

	int num_dlights;
	dlight_t* dlights;
	int dlightBits;					// ydnar: optimization

	int numPolys;
	idSurfacePoly* polys;

	int numDrawSurfs;
	drawSurf_t* drawSurfs;

	//	From Quake 2
	lightstyle_t* lightstyles;		// [MAX_LIGHTSTYLES]

	int num_particles;
	particle_t* particles;

	int num_coronas;
	corona_t* coronas;

	int numPolyBuffers;
	idSurfacePolyBuffer* polybuffers;

	int decalBits;					// ydnar: optimization
	int numDecalProjectors;
	decalProjector_t* decalProjectors;

	int numDecals;
	srfDecal_t* decals;
};

struct frontEndCounters_t {
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

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
struct trGlobals_t {
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
	idRenderModel* worldModel;
	char* worldDir;		// ydnar: for referencing external lightmaps

	image_t* defaultImage;
	image_t* dlightImage;						// inverse-quare highlight for projective adding
	image_t* whiteImage;						// full of 0xff
	image_t* transparentImage;					// compltely transparent
	image_t* scratchImage[ 32 ];
	image_t* fogImage;
	image_t* particleImage;						// little dot for particles

	shader_t* defaultShader;

	shader_t* shadowShader;
	shader_t* projectionShadowShader;

	shader_t* flareShader;
	shader_t* spotFlareShader;
	char* sunShaderName;
	shader_t* sunShader;
	shader_t* sunflareShader;	//----(SA) for the camera lens flare effect for sun

	shader_t* particleShader;
	shader_t* colorShadeShader;
	shader_t* colorShellShader;

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
	idRenderModel* currentModel;
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
	idRenderModel* models[ MAX_MOD_KNOWN ];
	int numModels;

	int numLightmaps;
	image_t* lightmaps[ MAX_LIGHTMAPS ];

	int numImages;
	image_t* images[ MAX_DRAWIMAGES ];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int numShaders;
	shader_t* shaders[ MAX_SHADERS ];
	shader_t* sortedShaders[ MAX_SHADERS ];

	int numSkins;
	skin_t* skins[ MAX_SKINS ];

	float fogTable[ FOG_TABLE_SIZE ];

	float sinTable[ FUNCTABLE_SIZE ];
	float squareTable[ FUNCTABLE_SIZE ];
	float triangleTable[ FUNCTABLE_SIZE ];
	float sawToothTable[ FUNCTABLE_SIZE ];
	float inverseSawToothTable[ FUNCTABLE_SIZE ];
};

extern glconfig_t glConfig;			// outside of TR since it shouldn't be cleared during ref re-init

extern trGlobals_t tr;

extern int c_brush_polys;
extern int c_alias_polys;

extern int cl_numtransvisedicts;
extern int cl_numtranswateredicts;

extern float s_flipMatrix[ 16 ];

extern int gl_NormalFontBase;

extern glfog_t glfogsettings[ NUM_FOGS ];		// [0] never used (FOG_NONE)
extern glfogType_t glfogNum;				// fog type to use (from the mbrush46_fog_t enum list)
extern bool fogIsOn;

void myGlMultMatrix( const float* a, const float* b, float* out );
void R_DecomposeSort( unsigned sort, int* entityNum, shader_t** shader,
	int* fogNum, int* dlightMap, int* frontFace, int* atiTess );
void R_LocalNormalToWorld( vec3_t local, vec3_t world );
void R_LocalPointToWorld( vec3_t local, vec3_t world );
void R_TransformModelToClip( const vec3_t src, const float* modelMatrix, const float* projectionMatrix,
	vec4_t eye, vec4_t dst );
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window );
void R_RotateForEntity( const trRefEntity_t* ent, const viewParms_t* viewParms,
	orientationr_t* orient );
int R_CullLocalBox( vec3_t bounds[ 2 ] );
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );
void R_AddDrawSurfOld( surfaceType_t* surface, shader_t* shader, int fogIndex, int dlightMap, int frontFace, int atiTess, int forcedSortIndex );
void R_AddDrawSurf( idSurface* surface, shader_t* shader, int fogIndex, int dlightMap, int frontFace, int atiTess, int forcedSortIndex );
void R_RenderView( viewParms_t* parms );

void R_FogOn();
void R_FogOff();
void R_Fog( glfog_t* curfog );

void R_DebugText( const vec3_t org, float r, float g, float b, const char* text, bool neverOcclude );

#endif
