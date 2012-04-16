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

//====================================================
extern refimport_t ri;

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );
void R_AddPolygonBufferSurfaces( void );

/*
** GL wrapper/helper functions
*/
void    GL_SetDefaultState( void );

void        R_BeginFrame( stereoFrame_t stereoFrame );
void        RE_BeginRegistration( glconfig_t *glconfig );
void        RE_Shutdown( qboolean destroyWindow );

float       R_ProcessLightmap( byte *pic, int in_padding, int width, int height, byte *pic_out ); // Arnout

void        R_Init( void );

void    R_ScreenShot_f( void );
void    R_ScreenShotJPEG_f( void );

/*
============================================================

WORLD MAP

============================================================
*/

void R_AddWorldSurfaces( void );

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

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

// Gordon: testing
#define MAX_POLYINDICIES 8192

void RB_ExecuteRenderCommands( const void *data );

void RE_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque );

// font stuff
void R_InitFreeType();
void R_DoneFreeType();
void RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );

/*
============================================================

GL FOG

============================================================
*/

qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );

#endif //TR_LOCAL_H (THIS MUST BE LAST!!)
