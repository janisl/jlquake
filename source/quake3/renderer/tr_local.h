/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../../client/client.h"
#include "../game/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "tr_public.h"
#include "qgl.h"

#undef tr

// 12 bits
// see QSORT_SHADERNUM_SHIFT
#define	MAX_SHADERS				16384

#define MAX_STATE_NAME 32

// can't be increased without changing bit packing for drawsurfs


typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;


typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelMatrix[16];
} orientationr_t;

//===============================================================================

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	srfPoly_t	*polys;

	int			numDrawSurfs;
	drawSurf_t	*drawSurfs;


} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct {
	char		name[MAX_QPATH];
	shader_t	*shader;
} skinSurface_t;

typedef struct skin_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			numSurfaces;
	skinSurface_t	*surfaces[MD3_MAX_SURFACES];
} skin_t;


typedef struct {
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
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

//======================================================================

#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);
model_t		*R_GetModelByHandle( qhandle_t hModel );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);

//====================================================
extern	refimport_t		ri;

#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31	: sorted shader index
11 - 20	: entity index
2 - 6	: fog index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
0 - 1	: dlightmap index

	TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define	QSORT_SHADERNUM_SHIFT	17
#define	QSORT_ENTITYNUM_SHIFT	7
#define	QSORT_FOGNUM_SHIFT		2

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define FUNCTABLE_SIZE2		10


typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float	c_overDraw;
	
	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int		msec;			// total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
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
} backEndState_t;

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
struct trGlobals_t : trGlobals_base_t
{
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visCount;		// incremented every time a new vis cluster is entered
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						smpFrame;		// toggles from 0 to 1 every endFrame

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				worldMapLoaded;
	world_t					*world;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*flareImage;

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*projectionShadowShader;

	shader_t				*flareShader;
	shader_t				*sunShader;

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t					*currentModel;

	viewParms_t				viewParms;

	orientationr_t			orient;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];
};

extern backEndState_t	backEnd;
extern trGlobals_t	tr;


//
// cvars
//
extern QCvar	*r_flareSize;
extern QCvar	*r_flareFade;

extern QCvar	*r_railWidth;
extern QCvar	*r_railCoreWidth;
extern QCvar	*r_railSegmentLength;

extern QCvar	*r_ignore;				// used for debugging anything
extern QCvar	*r_ignoreFastPath;		// allows us to ignore our Tess fast paths

extern QCvar	*r_znear;				// near Z clip plane

extern QCvar	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern QCvar	*r_lodbias;				// push/pull LOD transitions
extern QCvar	*r_lodscale;

extern QCvar	*r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing

extern QCvar	*r_inGameVideo;				// controls whether in game video should be draw
extern QCvar	*r_fastsky;				// controls whether sky should be cleared or drawn
extern QCvar	*r_drawSun;				// controls drawing of sun quad
extern QCvar	*r_dynamiclight;		// dynamic lights enabled/disabled
extern QCvar	*r_dlightBacks;			// dlight non-facing surfaces for continuity

extern	QCvar	*r_norefresh;			// bypasses the ref rendering
extern	QCvar	*r_drawentities;		// disable/enable entity rendering
extern	QCvar	*r_drawworld;			// disable/enable world rendering
extern	QCvar	*r_speeds;				// various levels of information display
extern  QCvar	*r_detailTextures;		// enables/disables detail texturing stages
extern	QCvar	*r_novis;				// disable/enable usage of PVS
extern	QCvar	*r_nocull;
extern	QCvar	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	QCvar	*r_nocurves;
extern	QCvar	*r_showcluster;

extern	QCvar	*r_singleShader;				// make most world faces use default shader
extern	QCvar	*r_finish;
extern	QCvar	*r_drawBuffer;
extern  QCvar  *r_glDriver;
extern	QCvar	*r_offsetFactor;
extern	QCvar	*r_offsetUnits;

extern	QCvar	*r_fullbright;					// avoid lightmap pass
extern	QCvar	*r_lightmap;					// render lightmaps only
extern	QCvar	*r_vertexLight;					// vertex lighting mode for better performance
extern	QCvar	*r_uiFullScreen;				// ui is running fullscreen

extern	QCvar	*r_showtris;					// enables wireframe rendering of the world
extern	QCvar	*r_showsky;						// forces sky in front of all surfaces
extern	QCvar	*r_shownormals;					// draws wireframe normals
extern	QCvar	*r_clear;						// force screen clear every frame

extern	QCvar	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	QCvar	*r_flares;						// light flares

extern	QCvar	*r_lockpvs;
extern	QCvar	*r_noportals;
extern	QCvar	*r_portalOnly;

extern	QCvar	*r_subdivisions;
extern	QCvar	*r_lodCurveError;
extern	QCvar	*r_smp;
extern	QCvar	*r_showSmp;
extern	QCvar	*r_skipBackEnd;

extern	QCvar	*r_mapOverBrightBits;

extern	QCvar	*r_debugSurface;

extern	QCvar	*r_showImages;
extern	QCvar	*r_debugSort;

extern	QCvar	*r_printShaders;
extern	QCvar	*r_saveFontData;

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );


#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (vec3_t local, vec3_t world);
void R_LocalPointToWorld (vec3_t local, vec3_t world);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );

void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *orient );

/*
** GL wrapper/helper functions
*/
void	GL_SetDefaultState (void);
void	GL_Cull( int cullType );

void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void		RE_Shutdown( qboolean destroyWindow );

qboolean	R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );

void	R_SkinList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd( const void *data );
void	R_ScreenShot_f( void );

void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );


//
// tr_shader.c
//
qhandle_t		 RE_RegisterShaderLightMap( const char *name, int lightmapIndex );
qhandle_t		 RE_RegisterShader( const char *name );
qhandle_t		 RE_RegisterShaderNoMip( const char *name );
qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage);

shader_t	*R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t	*R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void		R_InitShaders( void );
void		R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_EndFrame( void );

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal );
void RB_AddDlightFlares( void );
void RB_RenderFlares (void);

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( mbrush46_model_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *orient );
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
								bsp46_drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

SCENE GENERATION

============================================================
*/

void R_ToggleSmpFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_RenderScene( const refdef_t *fd );

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MakeAnimModel( model_t *model );
void R_AddAnimSurfaces( trRefEntity_t *ent );
void RB_SurfaceAnim( md4Surface_t *surfType );

/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void	RB_CalcFogTexCoords( float *dstTexCoords );
void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void	RB_CalcColorFromEntity( unsigned char *dstColors );
void	RB_CalcColorFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcSpecularAlpha( unsigned char *alphas );
void	RB_CalcDiffuseColor( unsigned char *colors );

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

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} drawBufferCommand_t;

typedef struct {
	int		commandId;
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct {
	int		commandId;
} swapBuffersCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} endFrameCommand_t;

typedef struct {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef struct {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	qboolean jpeg;
} screenshotCommand_t;

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT
} renderCommand_t;


// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
typedef struct {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_ENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	renderCommandList_t	commands;
} backEndData_t;

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData[SMP_FRAMES];	// the second one may not be allocated

extern	volatile renderCommandList_t	*renderCommandList;

extern	volatile qboolean	renderThreadActive;


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_SyncRenderThread( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void RE_SetColor( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );

// font stuff
void R_InitFreeType();
void R_DoneFreeType();
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);


#endif //TR_LOCAL_H
