/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/



#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../../wolfclient/client.h"
#include "../../wolfclient/renderer/local.h"
#include "../game/q_shared.h"
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

#define MAX_FACE_POINTS     64

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

void        R_ModelInit( void );
int         R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void        R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void        R_Modellist_f( void );

//====================================================
extern refimport_t ri;

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_TagInfo_f( void );

void R_AddPolygonSurfaces( void );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );


void R_LocalNormalToWorld( vec3_t local, vec3_t world );
void R_LocalPointToWorld( vec3_t local, vec3_t world );
int R_CullLocalBox( vec3_t bounds[2] );
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );

/*
** GL wrapper/helper functions
*/
void    GL_SetDefaultState( void );

void        RE_BeginFrame( stereoFrame_t stereoFrame );
void        RE_BeginRegistration( glconfig_t *glconfig );
void        RE_LoadWorldMap( const char *mapname );
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

void    R_SkinList_f( void );
void    R_ScreenShot_f( void );
void    R_ScreenShotJPEG_f( void );

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
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( mbrush46_model_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );


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
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
//----(SA)
void RE_RenderScene( const refdef_t *fd );

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MakeAnimModel( model_t *model );
void R_AddAnimSurfaces( trRefEntity_t *ent );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

void *R_GetCommandBuffer( int bytes );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void RE_SetColor( const float *rgba );
void RE_StretchPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_RotatedPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );       // NERVE - SMF
void RE_StretchPicGradient( float x, float y, float w, float h,
							float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
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

void *R_CacheShaderAlloc( int size );
void R_CacheShaderFree( void *ptr );
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash );
void R_BackupShaders( void );
void R_PurgeShaders( int count );
void R_LoadCacheShaders( void );
// done.

//------------------------------------------------------------------------------

void R_AddMDCSurfaces( trRefEntity_t *ent );
// done.
//------------------------------------------------------------------------------

/*
============================================================

GL FOG

============================================================
*/

// Ridah, virtual memory
void *R_Hunk_Begin( void );
void R_Hunk_End( void );

#endif //TR_LOCAL_H (THIS MUST BE LAST!!)
