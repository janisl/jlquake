/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/



#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../../wolfclient/client.h"
#include "../../wolfclient/renderer/local.h"
#include "../game/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "tr_public.h"

// fast float to int conversion
#if id386 && !( ( defined __linux__ || defined __FreeBSD__ ) && ( defined __i386__ ) ) // rb010123
long myftol( float f );
#else
#define myftol( x ) ( (int)( x ) )
#endif


#define MAX_SHADER_STATES 2048
#define MAX_STATE_NAME 32

// can't be increased without changing bit packing for drawsurfs


// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct {
	refEntity_t e;

	float axisLength;           // compensate for non-normalized axis

	qboolean needDlights;       // true for bmodels that touch a dlight
	qboolean lightingCalculated;
	vec3_t lightDir;            // normalized direction towards light
	vec3_t ambientLight;        // color normalized to 0-255
	int ambientLightInt;            // 32 bit rgba packed
	vec3_t directedLight;
	float brightness;
} trRefEntity_t;

typedef struct {
	vec3_t origin;              // in world coordinates
	vec3_t axis[3];             // orientation in world
	vec3_t viewOrigin;          // viewParms->or.origin in local coordinates
	float modelMatrix[16];
} orientationr_t;

//===============================================================================

typedef struct corona_s {
	vec3_t origin;
	vec3_t color;               // range from 0.0 to 1.0, should be color normalized
	vec3_t transformed;         // origin in local coordinate system
	float scale;                // uses r_flaresize as the baseline (1.0)
	int id;
	int flags;                  // '1' is 'visible'
								// still send the corona request, even if not visible, for proper fading
} corona_t;

typedef struct dlight_s {
	vec3_t origin;
	vec3_t color;               // range from 0.0 to 1.0, should be color normalized
	float radius;

	vec3_t transformed;         // origin in local coordinate system

	// Ridah
	int overdraw;
	// done.

	shader_t    *dlshader;  //----(SA) adding a shader to dlights, so, if desired, we can change the blend or texture of a dlight

	qboolean forced;        //----(SA)	use this dlight when r_dynamiclight is either 1 or 2 (rather than just 1) for "important" gameplay lights (alarm lights, etc)
	//done

} dlight_t;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];             // transformation matrix

	int time;                       // time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;                    // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];
	qboolean areamaskModified;      // qtrue if areamask changed since last scene

	float floatTime;                // tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int num_entities;
	trRefEntity_t   *entities;

	int num_dlights;
	struct dlight_s *dlights;

	int num_coronas;
	struct corona_s *coronas;

	int numPolys;
	srfPoly_t* polys;

	int numDrawSurfs;
	drawSurf_t   *drawSurfs;
} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct {
	char name[MAX_QPATH];
	shader_t    *shader;
} skinSurface_t;

//----(SA) modified
#define MAX_PART_MODELS 5

typedef struct {
	char type[MAX_QPATH];           // md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[MAX_QPATH];          // lower.md3, belt1.md3, etc.
} skinModel_t;

typedef struct skin_s {
	char name[MAX_QPATH];               // game path, including extension
	int numSurfaces;
	int numModels;
	skinSurface_t   *surfaces[MD3_MAX_SURFACES];
	skinModel_t     *models[MAX_PART_MODELS];
	vec3_t scale;       //----(SA)	added
} skin_t;
//----(SA) end

typedef struct {
	orientationr_t  _or;
	orientationr_t world;
	vec3_t pvsOrigin;               // may be different than or.origin for portals
	qboolean isPortal;              // true if this view is through a portal
	qboolean isMirror;              // the portal is a mirror, invert the face culling
	int frameSceneNum;              // copied from tr.frameSceneNum
	int frameCount;                 // copied from tr.frameCount
	cplane_t portalPlane;           // clip anything behind this if mirroring
	int viewportX, viewportY, viewportWidth, viewportHeight;
	float fovX, fovY;
	float projectionMatrix[16];
	cplane_t frustum[4];
	vec3_t visBounds[2];
	float zFar;

	int dirty;

	glfog_t glFog;                  // fog parameters	//----(SA)	added

} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

#define MAX_FACE_POINTS     64

extern void( *rb_surfaceTable[SF_NUM_SURFACE_TYPES] ) ( void * );

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2

//======================================================================

typedef struct model_s {
	char name[MAX_QPATH];
	modtype_t type;
	int index;                      // model = tr.models[model->index]

	int q3_dataSize;                   // just for listing purposes
	mbrush46_model_t    *q3_bmodel;            // only if type == MOD_BRUSH46
	md3Header_t *q3_md3[MD3_MAX_LODS]; // only if type == MOD_MESH3
	mdsHeader_t *q3_mds;               // only if type == MOD_MDS
	mdcHeader_t *q3_mdc[MD3_MAX_LODS]; // only if type == MOD_MDC

	int q3_numLods;

// GR - model tessellation capability flag
	int q3_ATI_tess;
} model_t;


void        R_ModelInit( void );
model_t     *R_GetModelByHandle( qhandle_t hModel );
int         R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void        R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void        R_Modellist_f( void );

//====================================================
extern refimport_t ri;

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

(SA) modified for Wolf (11 bits of entity num)

old:

22 - 31	: sorted shader index
12 - 21	: entity index
3 - 7	: fog index
2		: used to be clipped flag
0 - 1	: dlightmap index

#define	QSORT_SHADERNUM_SHIFT	22
#define	QSORT_ENTITYNUM_SHIFT	12
#define	QSORT_FOGNUM_SHIFT		3

new:

22 - 31	: sorted shader index
11 - 21	: entity index
2 - 6	: fog index
removed	: used to be clipped flag
0 - 1	: dlightmap index

*/
#define QSORT_SHADERNUM_SHIFT   22
#define QSORT_ENTITYNUM_SHIFT   11
#define QSORT_FOGNUM_SHIFT      2

// GR - tessellation flag in bit 8
#define QSORT_ATI_TESS_SHIFT    8
// GR - TruForm flags
#define ATI_TESS_TRUFORM    1
#define ATI_TESS_NONE       0

extern int gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int c_leafs;
	int c_dlightSurfaces;
	int c_dlightSurfacesCulled;
} frontEndCounters_t;

typedef struct {
	int c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float c_overDraw;

	int c_dlightVertexes;
	int c_dlightIndexes;

	int c_flareAdds;
	int c_flareTests;
	int c_flareRenders;

	int msec;               // total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	int smpFrame;
	trRefdef_t refdef;
	viewParms_t viewParms;
	orientationr_t  _or;
	backEndCounters_t pc;
	qboolean isHyperspace;
	trRefEntity_t   *currentEntity;
	qboolean skyRenderedThisView;       // flag for drawing sun

	qboolean projection2D;      // if qtrue, drawstretchpic doesn't need to change modes
	byte color2D[4];
	qboolean vertexes2D;        // shader needs to be finished
	trRefEntity_t entity2D;     // currentEntity will point at this when doing 2D rendering
} backEndState_t;

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct {
	qboolean registered;                    // cleared at shutdown, set at beginRegistration

	int visCount;                           // incremented every time a new vis cluster is entered
	int frameCount;                         // incremented every frame
	int sceneCount;                         // incremented every scene
	int viewCount;                          // incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int smpFrame;                           // toggles from 0 to 1 every endFrame

	int frameSceneNum;                      // zeroed at RE_BeginFrame

	qboolean worldMapLoaded;
	world_t                 *world;

	const byte              *externalVisData;   // from RE_SetWorldVisData, shared with CM_Load

	image_t                 *defaultImage;
	image_t                 *scratchImage[32];
	image_t                 *fogImage;
	image_t                 *dlightImage;   // inverse-square highlight for projective adding
	image_t                 *flareImage;
	image_t                 *whiteImage;            // full of 0xff
	image_t                 *identityLightImage;    // full of tr.identityLightByte

	shader_t                *defaultShader;
	shader_t                *shadowShader;
//	shader_t				*projectionShadowShader;
	shader_t                *dlightShader;      //----(SA) added

	shader_t                *flareShader;
	shader_t                *spotFlareShader;
	char                    *sunShaderName;
	shader_t                *sunShader;
	shader_t                *sunflareShader[6];  //----(SA) for the camera lens flare effect for sun

	int numLightmaps;
	image_t                 *lightmaps[MAX_LIGHTMAPS];

	trRefEntity_t           *currentEntity;
	trRefEntity_t worldEntity;                  // point currentEntity at this when rendering world
	int currentEntityNum;
	int shiftedEntityNum;                       // currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t                 *currentModel;

	viewParms_t viewParms;

	float identityLight;                        // 1.0 / ( 1 << overbrightBits )
	int identityLightByte;                      // identityLight * 255
	int overbrightBits;                         // r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t          _or;                 // for current entity

	trRefdef_t refdef;

	int viewCluster;

	vec3_t sunLight;                            // from the sky shader for this level
	vec3_t sunDirection;

//----(SA)	added
	float lightGridMulAmbient;          // lightgrid multipliers specified in sky shader
	float lightGridMulDirected;         //
//----(SA)	end

//	qboolean				levelGLFog;

	frontEndCounters_t pc;
	int frontEndMsec;                           // not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t                 *models[MAX_MOD_KNOWN];
	int numModels;

	int numImages;
	image_t                 *images[MAX_DRAWIMAGES];
	// Ridah
	int numCacheImages;

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int numShaders;
	shader_t                *shaders[MAX_SHADERS];
	shader_t                *sortedShaders[MAX_SHADERS];

	int numSkins;
	skin_t                  *skins[MAX_SKINS];

	float sinTable[FUNCTABLE_SIZE];
	float squareTable[FUNCTABLE_SIZE];
	float triangleTable[FUNCTABLE_SIZE];
	float sawToothTable[FUNCTABLE_SIZE];
	float inverseSawToothTable[FUNCTABLE_SIZE];
	float fogTable[FOG_TABLE_SIZE];

	// RF, temp var used while parsing shader only
	int allowCompress;

} trGlobals_t;

extern backEndState_t backEnd;
extern trGlobals_t tr;
extern glstate_t glState;           // outside of TR since it shouldn't be cleared during ref re-init

//====================================================================

float R_NoiseGet4f( float x, float y, float z, float t );
void  R_NoiseInit( void );

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_TagInfo_f( void );

void R_AddPolygonSurfaces( void );

// GR - add tessellation flag
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader,
					  int *fogNum, int *dlightMap, int *atiTess );

// GR - add tessellation flag
void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap, int atiTess );


void R_LocalNormalToWorld( vec3_t local, vec3_t world );
void R_LocalPointToWorld( vec3_t local, vec3_t world );
int R_CullLocalBox( vec3_t bounds[2] );
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );

void R_RotateForEntity( const trRefEntity_t * ent, const viewParms_t * viewParms, orientationr_t * _or );

/*
** GL wrapper/helper functions
*/
void    GL_Bind( image_t *image );
void    GL_SetDefaultState( void );
void    GL_SelectTexture( int unit );
void    GL_TextureMode( const char *string );
void    GL_CheckErrors( void );
void    GL_State( unsigned long stateVector );
void    GL_TexEnv( int env );
void    GL_Cull( int cullType );

#define GLS_SRCBLEND_ZERO                       0x00000001
#define GLS_SRCBLEND_ONE                        0x00000002
#define GLS_SRCBLEND_DST_COLOR                  0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR        0x00000004
#define GLS_SRCBLEND_SRC_ALPHA                  0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA        0x00000006
#define GLS_SRCBLEND_DST_ALPHA                  0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA        0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE             0x00000009
#define     GLS_SRCBLEND_BITS                   0x0000000f

#define GLS_DSTBLEND_ZERO                       0x00000010
#define GLS_DSTBLEND_ONE                        0x00000020
#define GLS_DSTBLEND_SRC_COLOR                  0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR        0x00000040
#define GLS_DSTBLEND_SRC_ALPHA                  0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA        0x00000060
#define GLS_DSTBLEND_DST_ALPHA                  0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA        0x00000080
#define     GLS_DSTBLEND_BITS                   0x000000f0

#define GLS_DEPTHMASK_TRUE                      0x00000100

#define GLS_POLYMODE_LINE                       0x00001000

#define GLS_DEPTHTEST_DISABLE                   0x00010000
#define GLS_DEPTHFUNC_EQUAL                     0x00020000

#define GLS_FOG_DISABLE                         0x00020000  //----(SA)	added

#define GLS_ATEST_GT_0                          0x10000000
#define GLS_ATEST_LT_80                         0x20000000
#define GLS_ATEST_GE_80                         0x40000000
#define     GLS_ATEST_BITS                      0x70000000

#define GLS_DEFAULT         GLS_DEPTHMASK_TRUE

void    RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
void    RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );

void        RE_BeginFrame( stereoFrame_t stereoFrame );
void        RE_BeginRegistration( glconfig_t *glconfig );
void        RE_LoadWorldMap( const char *mapname );
void        RE_SetWorldVisData( const byte *vis );
qhandle_t   RE_RegisterModel( const char *name );
qhandle_t   RE_RegisterSkin( const char *name );
void        RE_Shutdown( qboolean destroyWindow );

qboolean    R_GetEntityToken( char *buffer, int size );

//----(SA)
qboolean    RE_GetSkinModel( qhandle_t skinid, const char *type, char *name );
qhandle_t   RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );    //----(SA)
//----(SA) end

model_t     *R_AllocModel( void );

void        R_Init( void );
image_t     *R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode );
image_t     *R_FindImageFileExt( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean characterMip, int glWrapClampMode ); //----(SA)	added

image_t     *R_CreateImage( const char *name, const byte *pic, int width, int height, qboolean mipmap
							, qboolean allowPicmip, int wrapClampMode );
//----(SA)	added (didn't want to modify all instances of R_CreateImage()
image_t     *R_CreateImageExt( const char *name, const byte *pic, int width, int height, qboolean mipmap
							   , qboolean allowPicmip, qboolean characterMip, int wrapClampMode );
//----(SA)	end

void        R_SetColorMappings( void );
void        R_GammaCorrect( byte *buffer, int bufSize );

void    R_ImageList_f( void );
void    R_SkinList_f( void );
void    R_ScreenShot_f( void );
void    R_ScreenShotJPEG_f( void );

void    R_InitFogTable( void );
float   R_FogFactor( float s, float t );
void    R_InitImages( void );
void    R_DeleteTextures( void );
int     R_SumOfUsedImages( void );
void    R_InitSkins( void );
skin_t  *R_GetSkinByHandle( qhandle_t hSkin );


//
// tr_shader.c
//
qhandle_t        RE_RegisterShaderLightMap( const char *name, int lightmapIndex );
qhandle_t        RE_RegisterShader( const char *name );
qhandle_t        RE_RegisterShaderNoMip( const char *name );
qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage );

shader_t    *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t    *R_GetShaderByHandle( qhandle_t hShader );
shader_t    *R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void        R_InitShaders( void );
void        R_ShaderList_f( void );
void    R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void        GLimp_Init( void );
void        GLimp_EndFrame( void );

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

extern shaderCommands_t tess;

void RB_BeginSurface( shader_t *shader, int fogNum );
void RB_EndSurface( void );
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW( v,i ) if ( tess.numVertexes + ( v ) >= SHADER_MAX_VERTEXES || tess.numIndexes + ( i ) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow( v,i );}

void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );
void RB_AddQuadStampFadingCornersExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, float scale, vec3_t normal, int id, int flags ); // TTimo updated prototype
void RB_AddDlightFlares( void );
void RB_RenderFlares( void );

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( mbrush46_model_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t * dl, orientationr_t * _or );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData( shaderCommands_t *shader );
void R_InitSkyTexCoords( float cloudLayerHeight );
void R_DrawSkyBox( shaderCommands_t *shader );
void RB_DrawSun( void );
void RB_ClipSkyPolygons( shaderCommands_t *shader );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
									   bsp46_drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int orientation, const vec3_t *points, const vec3_t projection,
					 int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

SCENE GENERATION

============================================================
*/

void R_ToggleSmpFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
// Ridah
void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
// done.
// Ridah
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
// done.
//----(SA)
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags );
//----(SA)
void RE_RenderScene( const refdef_t *fd );

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MakeAnimModel( model_t *model );
void R_AddAnimSurfaces( trRefEntity_t *ent );
void RB_SurfaceAnim( mdsSurface_t *surfType );
int R_GetBoneTag( orientation_t *outTag, mdsHeader_t *mds, int startTagIndex, const refEntity_t *refent, const char *tagName );

/*
=============================================================
=============================================================
*/
void    R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
								vec4_t eye, vec4_t dst );
void    R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void    RB_DeformTessGeometry( void );

void    RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void    RB_CalcFireRiseEnvTexCoords( float *st );
void    RB_CalcFogTexCoords( float *dstTexCoords );
void    RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void    RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void    RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void    RB_CalcSwapTexCoords( float *dstTexCoords );
void    RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void    RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
void    RB_CalcModulateColorsByFog( unsigned char *dstColors );
void    RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void    RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void    RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
void    RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors );
void    RB_CalcAlphaFromEntity( unsigned char *dstColors );
void    RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void    RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void    RB_CalcColorFromEntity( unsigned char *dstColors );
void    RB_CalcColorFromOneMinusEntity( unsigned char *dstColors );
void    RB_CalcSpecularAlpha( unsigned char *alphas );
void    RB_CalcDiffuseColor( unsigned char *colors );

void    RB_ZombieFXInit( void );
void    RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir );


/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_RenderThread( void );
void RB_ExecuteRenderCommands( const void *data );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

typedef struct {
	byte cmds[MAX_RENDER_COMMANDS];
	int used;
} renderCommandList_t;

typedef struct {
	int commandId;
	float color[4];
} setColorCommand_t;

typedef struct {
	int commandId;
	int buffer;
} drawBufferCommand_t;

typedef struct {
	int commandId;
	image_t *image;
	int width;
	int height;
	void    *data;
} subImageCommand_t;

typedef struct {
	int commandId;
} swapBuffersCommand_t;

typedef struct {
	int commandId;
	int buffer;
} endFrameCommand_t;

typedef struct {
	int commandId;
	shader_t    *shader;
	float x, y;
	float w, h;
	float s1, t1;
	float s2, t2;

	byte gradientColor[4];      // color values 0-255
	int gradientType;       //----(SA)	added
} stretchPicCommand_t;

typedef struct {
	int commandId;
	trRefdef_t refdef;
	viewParms_t viewParms;
	drawSurf_t *drawSurfs;
	int numDrawSurfs;
} drawSurfsCommand_t;

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
typedef struct {
	drawSurf_t drawSurfs[MAX_DRAWSURFS];
	dlight_t dlights[MAX_DLIGHTS];
	corona_t coronas[MAX_CORONAS];          //----(SA)
	trRefEntity_t entities[MAX_ENTITIES];
	srfPoly_t polys[MAX_POLYS];
	polyVert_t polyVerts[MAX_POLYVERTS];
	renderCommandList_t commands;
} backEndData_t;

extern int max_polys;
extern int max_polyverts;

extern backEndData_t   *backEndData[SMP_FRAMES];    // the second one may not be allocated

extern volatile renderCommandList_t    *renderCommandList;

extern volatile qboolean renderThreadActive;


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_SyncRenderThread( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void RE_SetColor( const float *rgba );
void RE_StretchPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_StretchPicGradient( float x, float y, float w, float h,
							float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void SaveJPG( char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer );

// font stuff
void R_InitFreeType();
void R_DoneFreeType();
void RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );

// Ridah, caching system
// NOTE: to disable this for development, set "r_cache 0" in autoexec.cfg
void R_InitTexnumImages( qboolean force );

void *R_CacheModelAlloc( int size );
void R_CacheModelFree( void *ptr );
void R_PurgeModels( int count );
void R_BackupModels( void );
qboolean R_FindCachedModel( const char *name, model_t *newmod );
void R_LoadCacheModels( void );

void *R_CacheImageAlloc( int size );
void R_CacheImageFree( void *ptr );
qboolean R_TouchImage( image_t *inImage );
image_t *R_FindCachedImage( const char *name, int hash );
void R_FindFreeTexnum( image_t *image );
void R_LoadCacheImages( void );
void R_PurgeBackupImages( int purgeCount );
void R_BackupImages( void );

void *R_CacheShaderAlloc( int size );
void R_CacheShaderFree( void *ptr );
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash );
void R_BackupShaders( void );
void R_PurgeShaders( int count );
void R_LoadCacheShaders( void );
// done.

//------------------------------------------------------------------------------
// Ridah, mesh compression
#define NUMMDCVERTEXNORMALS  256

extern float r_anormals[NUMMDCVERTEXNORMALS][3];

// NOTE: MDC_MAX_ERROR is effectively the compression level. the lower this value, the higher
// the accuracy, but with lower compression ratios.
#define MDC_MAX_ERROR       0.1     // if any compressed vert is off by more than this from the
									// actual vert, make this a baseframe

#define MDC_DIST_SCALE      0.05    // lower for more accuracy, but less range

// note: we are locked in at 8 or less bits since changing to byte-encoded normals
#define MDC_BITS_PER_AXIS   8
#define MDC_MAX_OFS         127.0   // to be safe

#define MDC_MAX_DIST        ( MDC_MAX_OFS * MDC_DIST_SCALE )

#if 0
void R_MDC_DecodeXyzCompressed( mdcXyzCompressed_t *xyzComp, vec3_t out, vec3_t normal );
#else   // optimized version
#define R_MDC_DecodeXyzCompressed( ofsVec, out, normal ) \
	( out )[0] = ( (float)( ( ofsVec ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
	( out )[1] = ( (float)( ( ofsVec >> 8 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
	( out )[2] = ( (float)( ( ofsVec >> 16 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
	VectorCopy( ( r_anormals )[( ofsVec >> 24 )], normal );
#endif

void R_AddMDCSurfaces( trRefEntity_t *ent );
// done.
//------------------------------------------------------------------------------

void R_LatLongToNormal( vec3_t outNormal, short latLong );


/*
============================================================

GL FOG

============================================================
*/

//extern glfog_t		glfogCurrent;
extern glfog_t glfogsettings[NUM_FOGS];         // [0] never used (FOG_NONE)
extern glfogType_t glfogNum;                    // fog type to use (from the mbrush46_fog_t enum list)

extern qboolean fogIsOn;

extern void         R_FogOff( void );
extern void         R_FogOn( void );

extern void R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );

extern int skyboxportal;
extern int drawskyboxportal;

// Ridah, virtual memory
void *R_Hunk_Begin( void );
void R_Hunk_End( void );
void R_FreeImageBuffer( void );

#endif //TR_LOCAL_H (THIS MUST BE LAST!!)
