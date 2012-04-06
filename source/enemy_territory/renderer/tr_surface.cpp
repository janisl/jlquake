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

// tr_surf.c
#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

void RB_SurfacePolychain( srfPoly_t *p );
void RB_SurfaceTriangles( srfTriangles_t *srf );
void RB_SurfaceFoliage(srfFoliage_t* srf);
void RB_SurfaceFace( srfSurfaceFace_t *surf );
void RB_SurfaceGrid( srfGridMesh_t *cv );
void RB_SurfaceEntity( surfaceType_t *surfType );
void RB_SurfaceBad( surfaceType_t *surfType );
void RB_SurfaceFlare( srfFlare_t *surf );
void RB_SurfaceDisplayList( srfDisplayList_t *surf );
void RB_SurfacePolyBuffer( srfPolyBuffer_t *surf );
void RB_SurfaceDecal( srfDecal_t *srf );
void RB_SurfaceSkip( void *surf );


void( *rb_surfaceTable[SF_NUM_SURFACE_TYPES] ) ( void * ) = {
	( void( * ) ( void* ) )RB_SurfaceBad,          // SF_BAD,
	( void( * ) ( void* ) )RB_SurfaceSkip,         // SF_SKIP,
	( void( * ) ( void* ) )RB_SurfaceFace,         // SF_FACE,
	( void( * ) ( void* ) )RB_SurfaceGrid,         // SF_GRID,
	( void( * ) ( void* ) )RB_SurfaceTriangles,    // SF_TRIANGLES,
	( void( * ) ( void* ) )RB_SurfaceFoliage,      // SF_FOLIAGE,
	( void( * ) ( void* ) )RB_SurfacePolychain,    // SF_POLY,
	( void( * ) ( void* ) )RB_SurfaceMesh,         // SF_MD3,
	NULL,//SF_MD4
	( void( * ) ( void* ) )RB_SurfaceCMesh,        // SF_MDC,
	( void( * ) ( void* ) )RB_SurfaceAnimMds,      // SF_MDS,
	( void( * ) ( void* ) )RB_MDM_SurfaceAnim,     // SF_MDM,
	( void( * ) ( void* ) )RB_SurfaceFlare,        // SF_FLARE,
	( void( * ) ( void* ) )RB_SurfaceEntity,       // SF_ENTITY
	( void( * ) ( void* ) )RB_SurfaceDisplayList,  // SF_DISPLAY_LIST
	( void( * ) ( void* ) )RB_SurfacePolyBuffer,   // SF_POLYBUFFER
	( void( * ) ( void* ) )RB_SurfaceDecal,        // SF_DECAL
};
