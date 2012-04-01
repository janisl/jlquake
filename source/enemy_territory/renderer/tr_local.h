/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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

#define MAX_SHADER_STATES 2048
#define MAX_STATE_NAME 32

// can't be increased without changing bit packing for drawsurfs

/*
==============================================================================

SURFACES

==============================================================================
*/

#define MAX_FACE_POINTS     1024

// ydnar: decals
#define MAX_WORLD_DECALS        1024
#define MAX_ENTITY_DECALS       128

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

// ydnar: optimization
#define WORLD_MAX_SKY_NODES 32

//======================================================================

void        R_ModelInit( void );
model_t     *R_GetModelByHandle( qhandle_t hModel );
int         R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void        R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void        R_Modellist_f( void );

//====================================================
extern refimport_t ri;

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
void R_AddPolygonBufferSurfaces( void );


void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader,
					  int *fogNum, int *frontFace, int *dlightMap );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogNum, int frontFace, int dlightMap );


void R_LocalNormalToWorld( vec3_t local, vec3_t world );
void R_LocalPointToWorld( vec3_t local, vec3_t world );
int R_CullLocalBox( vec3_t bounds[2] );
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );


void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *orientation );

/*
** GL wrapper/helper functions
*/
void    GL_SetDefaultState( void );
void    GL_TextureMode( const char *string );
void    GL_TextureAnisotropy( float anisotropy );

void    RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
void    RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );

void        RE_BeginFrame( stereoFrame_t stereoFrame );
void        RE_BeginRegistration( glconfig_t *glconfig );
void        RE_LoadWorldMap( const char *mapname );
qhandle_t   RE_RegisterModel( const char *name );
qhandle_t   RE_RegisterSkin( const char *name );
void        RE_Shutdown( qboolean destroyWindow );

qboolean    R_GetEntityToken( char *buffer, int size );

float       R_ProcessLightmap( byte *pic, int in_padding, int width, int height, byte *pic_out ); // Arnout

//----(SA)
qboolean    RE_GetSkinModel( qhandle_t skinid, const char *type, char *name );
qhandle_t   RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );    //----(SA)
//----(SA) end

model_t     *R_AllocModel( void );

void        R_Init( void );

void    R_ImageList_f( void );
void    R_SkinList_f( void );
void    R_ScreenShot_f( void );
void    R_ScreenShotJPEG_f( void );

void    R_DeleteTextures( void );
int     R_SumOfUsedImages( void );
void    R_InitSkins( void );
skin_t  *R_GetSkinByHandle( qhandle_t hSkin );

void    R_DebugText( const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude );

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
//bani
qboolean RE_LoadDynamicShader( const char *shadername, const char *shadertext );

// fretn - renderToTexture
void RE_RenderToTexture( int textureid, int x, int y, int w, int h );
// bani
void RE_Finish( void );
int R_GetTextureId( const char *name );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

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

void RB_DrawBounds( vec3_t mins, vec3_t maxs ); // ydnar


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

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, float scale, vec3_t normal, int id, qboolean visible );    //----(SA)	added scale.  added id.  added visible
void RB_AddDlightFlares( void );
void RB_RenderFlares( void );

/*
============================================================

LIGHTS

============================================================
*/

void R_TransformDlights( int count, dlight_t *dl, orientationr_t *orientation );
void R_CullDlights( void );
void R_DlightBmodel( mbrush46_model_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
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

DECALS - ydnar

============================================================
*/

void RE_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime );
void RE_ClearDecals( void );

void R_AddModelShadow( refEntity_t *ent );

void R_TransformDecalProjector( decalProjector_t * in, vec3_t axis[ 3 ], vec3_t origin, decalProjector_t * out );
qboolean R_TestDecalBoundingBox( decalProjector_t *dp, vec3_t mins, vec3_t maxs );
qboolean R_TestDecalBoundingSphere( decalProjector_t *dp, vec3_t center, float radius2 );

void R_ProjectDecalOntoSurface( decalProjector_t *dp, mbrush46_surface_t *surf, mbrush46_model_t *bmodel );

void R_AddDecalSurface( mbrush46_decal_t *decal );
void R_AddDecalSurfaces( mbrush46_model_t *bmodel );
void R_CullDecalProjectors( void );


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
void RE_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer );
// ydnar: modified dlight system to support seperate radius & intensity
// void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
void RE_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
//----(SA)
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
//----(SA)
void RE_RenderScene( const refdef_t *fd );
void RE_SaveViewParms();
void RE_RestoreViewParms();

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MakeAnimModel( model_t *model );
void R_AddAnimSurfaces( trRefEntity_t *ent );
void RB_SurfaceAnim( mdsSurface_t *surfType );
int R_GetBoneTag( orientation_t *outTag, mdsHeader_t *mds, int startTagIndex, const refEntity_t *refent, const char *tagName );

//
// MDM / MDX
//
void R_MDM_MakeAnimModel( model_t *model );
void R_MDM_AddAnimSurfaces( trRefEntity_t *ent );
void RB_MDM_SurfaceAnim( mdmSurface_t *surfType );
int R_MDM_GetBoneTag( orientation_t *outTag, mdmHeader_t *mdm, int startTagIndex, const refEntity_t *refent, const char *tagName );

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

// Gordon: testing
#define MAX_POLYINDICIES 8192

extern volatile renderCommandList_t    *renderCommandList;

void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_SyncRenderThread( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void RE_SetColor( const float *rgba );
void RE_StretchPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_RotatedPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );       // NERVE - SMF
void RE_StretchPicGradient( float x, float y, float w, float h,
							float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
void RE_2DPolyies( polyVert_t* verts, int numverts, qhandle_t hShader );
void RE_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );

// font stuff
void R_InitFreeType();
void R_DoneFreeType();
void RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );

void *R_CacheModelAlloc( int size );
void R_CacheModelFree( void *ptr );
void R_PurgeModels( int count );
void R_BackupModels( void );
qboolean R_FindCachedModel( const char *name, model_t *newmod );
void R_LoadCacheModels( void );

void R_CacheImageFree( void *ptr );
void R_PurgeBackupImages( int purgeCount );
void R_BackupImages( void );

//void *R_CacheShaderAlloc( int size );
//void R_CacheShaderFree( void *ptr );

void R_CacheShaderFreeExt( const char* name, void *ptr, const char* file, int line );
void* R_CacheShaderAllocExt( const char* name, int size, const char* file, int line );

#define R_CacheShaderAlloc( name, size ) R_CacheShaderAllocExt( name, size, __FILE__, __LINE__ )
#define R_CacheShaderFree( name, ptr ) R_CacheShaderFreeExt( name, ptr, __FILE__, __LINE__ )

shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash );
void R_BackupShaders( void );
void R_PurgeShaders( int count );
void R_PurgeLightmapShaders( void );
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


// Ridah, virtual memory
void *R_Hunk_Begin( void );
void R_Hunk_End( void );
void R_FreeImageBuffer( void );

qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );

#endif //TR_LOCAL_H (THIS MUST BE LAST!!)
