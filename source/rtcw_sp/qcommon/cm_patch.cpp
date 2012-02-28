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


#include "cm_local.h"
#include "cm_patch.h"

int c_totalPatchBlocks;
int c_totalPatchSurfaces;
int c_totalPatchEdges;

extern const patchCollide_t *debugPatchCollide;
extern const facet_t        *debugFacet;
extern bool debugBlock;
extern vec3_t debugBlockPoints[4];

/*
=================
CM_ClearLevelPatches
=================
*/
void CM_ClearLevelPatches( void ) {
	debugPatchCollide = NULL;
	debugFacet = NULL;
}

/*
================================================================================

PATCH COLLIDE GENERATION

================================================================================
*/

extern int cm_patch_numPlanes;
extern patchPlane_t cm_patch_planes[MAX_PATCH_PLANES];

extern int cm_patch_numFacets;
extern facet_t cm_patch_facets[MAX_PATCH_PLANES];          //maybe MAX_FACETS ??

#define NORMAL_EPSILON  0.0001
#define DIST_EPSILON    0.02

typedef enum {
	EN_TOP,
	EN_RIGHT,
	EN_BOTTOM,
	EN_LEFT
} edgeName_t;

/*
===================
CM_GeneratePatchCollide

Creates an internal structure that will be used to perform
collision detection with a patch mesh.

Points is packed as concatenated rows.
===================
*/
patchCollide_t* CM_GeneratePatchCollide( int width, int height, vec3_t *points ) {
	patchCollide_t  *pf;
	MAC_STATIC cGrid_t grid;
	int i, j;

	if ( width <= 2 || height <= 2 || !points ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)",
				   width, height, points );
	}

	if ( !( width & 1 ) || !( height & 1 ) ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes" );
	}

	if ( width > MAX_GRID_SIZE || height > MAX_GRID_SIZE ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE" );
	}

	// build a grid
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = qfalse;
	grid.wrapHeight = qfalse;
	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			VectorCopy( points[j * width + i], grid.points[i][j] );
		}
	}

	// subdivide the grid
	grid.SetWrapWidth();
	grid.SubdivideColumns();
	grid.RemoveDegenerateColumns();

	grid.Transpose();

	grid.SetWrapWidth();
	grid.SubdivideColumns();
	grid.RemoveDegenerateColumns();

	// we now have a grid of points exactly on the curve
	// the aproximate surface defined by these points will be
	// collided against
	pf = (patchCollide_t*)Hunk_Alloc( sizeof( *pf ), h_high );
	ClearBounds( pf->bounds[0], pf->bounds[1] );
	for ( i = 0 ; i < grid.width ; i++ ) {
		for ( j = 0 ; j < grid.height ; j++ ) {
			AddPointToBounds( grid.points[i][j], pf->bounds[0], pf->bounds[1] );
		}
	}

	c_totalPatchBlocks += ( grid.width - 1 ) * ( grid.height - 1 );

	// generate a bsp tree for the surface
	pf->FromGrid( &grid );

	// expand by one unit for epsilon purposes
	pf->bounds[0][0] -= 1;
	pf->bounds[0][1] -= 1;
	pf->bounds[0][2] -= 1;

	pf->bounds[1][0] += 1;
	pf->bounds[1][1] += 1;
	pf->bounds[1][2] += 1;

	return pf;
}

/*
=======================================================================

DEBUGGING

=======================================================================
*/


/*
==================
CM_DrawDebugSurface

Called from the renderer
==================
*/
#ifndef BSPC
void BotDrawDebugPolygons( void ( *drawPoly )( int color, int numPoints, float *points ), int value );
#endif

void CM_DrawDebugSurface( void ( *drawPoly )( int color, int numPoints, float *points ) ) {
	static Cvar   *cv;
#ifndef BSPC
	static Cvar   *cv2;
#endif
	const patchCollide_t    *pc;
	facet_t         *facet;
	winding_t       *w;
	int i, j, k, n;
	int curplanenum, planenum, curinward, inward;
	float plane[4];
	vec3_t mins = {-15, -15, -28}, maxs = {15, 15, 28};
	//vec3_t mins = {0, 0, 0}, maxs = {0, 0, 0};
	vec3_t v1, v2;

#ifndef BSPC
	if ( !cv2 ) {
		cv2 = Cvar_Get( "r_debugSurface", "0", 0 );
	}

	if ( cv2->integer != 1 ) {
		BotDrawDebugPolygons( drawPoly, cv2->integer );
		return;
	}
#endif

	if ( !debugPatchCollide ) {
		return;
	}

#ifndef BSPC
	if ( !cv ) {
		cv = Cvar_Get( "cm_debugSize", "2", 0 );
	}
#endif
	pc = debugPatchCollide;

	for ( i = 0, facet = pc->facets ; i < pc->numFacets ; i++, facet++ ) {

		for ( k = 0 ; k < facet->numBorders + 1; k++ ) {
			//
			if ( k < facet->numBorders ) {
				planenum = facet->borderPlanes[k];
				inward = facet->borderInward[k];
			} else {
				planenum = facet->surfacePlane;
				inward = qfalse;
				//continue;
			}

			Vector4Copy( pc->planes[ planenum ].plane, plane );

			//planenum = facet->surfacePlane;
			if ( inward ) {
				VectorSubtract( vec3_origin, plane, plane );
				plane[3] = -plane[3];
			}

			plane[3] += cv->value;
			//*
			for ( n = 0; n < 3; n++ )
			{
				if ( plane[n] > 0 ) {
					v1[n] = maxs[n];
				} else { v1[n] = mins[n];}
			} //end for
			VectorNegate( plane, v2 );
			plane[3] += fabs( DotProduct( v1, v2 ) );
			//*/

			w = CM46_BaseWindingForPlane( plane,  plane[3] );
			for ( j = 0 ; j < facet->numBorders + 1 && w; j++ ) {
				//
				if ( j < facet->numBorders ) {
					curplanenum = facet->borderPlanes[j];
					curinward = facet->borderInward[j];
				} else {
					curplanenum = facet->surfacePlane;
					curinward = qfalse;
					//continue;
				}
				//
				if ( curplanenum == planenum ) {
					continue;
				}

				Vector4Copy( pc->planes[ curplanenum ].plane, plane );
				if ( !curinward ) {
					VectorSubtract( vec3_origin, plane, plane );
					plane[3] = -plane[3];
				}
				//			if ( !facet->borderNoAdjust[j] ) {
				plane[3] -= cv->value;
				//			}
				for ( n = 0; n < 3; n++ )
				{
					if ( plane[n] > 0 ) {
						v1[n] = maxs[n];
					} else { v1[n] = mins[n];}
				} //end for
				VectorNegate( plane, v2 );
				plane[3] -= fabs( DotProduct( v1, v2 ) );

				CM46_ChopWindingInPlace( &w, plane, plane[3], 0.1f );
			}
			if ( w ) {
				if ( facet == debugFacet ) {
					drawPoly( 4, w->numpoints, w->p[0] );
					//Com_Printf("blue facet has %d border planes\n", facet->numBorders);
				} else {
					drawPoly( 1, w->numpoints, w->p[0] );
				}
				CM46_FreeWinding( w );
			} else {
				Com_Printf( "winding chopped away by border planes\n" );
			}
		}
	}

	// draw the debug block
	{
		vec3_t v[3];

		VectorCopy( debugBlockPoints[0], v[0] );
		VectorCopy( debugBlockPoints[1], v[1] );
		VectorCopy( debugBlockPoints[2], v[2] );
		drawPoly( 2, 3, v[0] );

		VectorCopy( debugBlockPoints[2], v[0] );
		VectorCopy( debugBlockPoints[3], v[1] );
		VectorCopy( debugBlockPoints[0], v[2] );
		drawPoly( 2, 3, v[0] );
	}

#if 0
	vec3_t v[4];

	v[0][0] = pc->bounds[1][0];
	v[0][1] = pc->bounds[1][1];
	v[0][2] = pc->bounds[1][2];

	v[1][0] = pc->bounds[1][0];
	v[1][1] = pc->bounds[0][1];
	v[1][2] = pc->bounds[1][2];

	v[2][0] = pc->bounds[0][0];
	v[2][1] = pc->bounds[0][1];
	v[2][2] = pc->bounds[1][2];

	v[3][0] = pc->bounds[0][0];
	v[3][1] = pc->bounds[1][1];
	v[3][2] = pc->bounds[1][2];

	drawPoly( 4, v[0] );
#endif
}
