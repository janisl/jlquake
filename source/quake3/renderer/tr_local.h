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

#define FULL_REFDEF

#include "../../client/client.h"
#include "../game/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "tr_public.h"
#include "qgl.h"

#define MAX_STATE_NAME 32

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

//======================================================================

int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);

//====================================================
extern	refimport_t		ri;

//
// cvars
//
extern QCvar	*r_inGameVideo;				// controls whether in game video should be draw
extern QCvar	*r_dlightBacks;			// dlight non-facing surfaces for continuity

extern	QCvar	*r_norefresh;			// bypasses the ref rendering
extern	QCvar	*r_drawentities;		// disable/enable entity rendering

extern  QCvar  *r_glDriver;

extern	QCvar	*r_noportals;
extern	QCvar	*r_portalOnly;

extern	QCvar	*r_debugSurface;

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

/*
** GL wrapper/helper functions
*/
void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_Shutdown( qboolean destroyWindow );

void    	R_Init( void );

//
// tr_shader.c
//
shader_t	*R_GetShaderByState( int index, long *cycleTime );

/*
============================================================

WORLD MAP

============================================================
*/

qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );

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

void RE_RenderScene( const refdef_t *fd );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );

#endif //TR_LOCAL_H
