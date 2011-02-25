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

#include "cm_local.h"

/*

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/

extern const patchCollide_t	*debugPatchCollide;
extern const facet_t		*debugFacet;
extern vec3_t		debugBlockPoints[4];

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
void BotDrawDebugPolygons(void (*drawPoly)(int color, int numPoints, float *points), int value);

void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, float *points) ) {
	static QCvar	*cv;
	static QCvar	*cv2;
	const patchCollide_t	*pc;
	facet_t			*facet;
	winding_t		*w;
	int				i, j, k, n;
	int				curplanenum, planenum, curinward, inward;
	float			plane[4];
	vec3_t mins = {-15, -15, -28}, maxs = {15, 15, 28};
	//vec3_t mins = {0, 0, 0}, maxs = {0, 0, 0};
	vec3_t v1, v2;

	if ( !cv2 )
	{
		cv2 = Cvar_Get( "r_debugSurface", "0", 0 );
	}

	if (cv2->integer != 1)
	{
		BotDrawDebugPolygons(drawPoly, cv2->integer);
		return;
	}

	if ( !debugPatchCollide ) {
		return;
	}

	if ( !cv ) {
		cv = Cvar_Get( "cm_debugSize", "2", 0 );
	}
	pc = debugPatchCollide;

	for ( i = 0, facet = pc->facets ; i < pc->numFacets ; i++, facet++ ) {

		for ( k = 0 ; k < facet->numBorders + 1; k++ ) {
			//
			if (k < facet->numBorders) {
				planenum = facet->borderPlanes[k];
				inward = facet->borderInward[k];
			}
			else {
				planenum = facet->surfacePlane;
				inward = false;
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
			for (n = 0; n < 3; n++)
			{
				if (plane[n] > 0) v1[n] = maxs[n];
				else v1[n] = mins[n];
			} //end for
			VectorNegate(plane, v2);
			plane[3] += fabs(DotProduct(v1, v2));
			//*/

			w = CM46_BaseWindingForPlane( plane,  plane[3] );
			for ( j = 0 ; j < facet->numBorders + 1 && w; j++ ) {
				//
				if (j < facet->numBorders) {
					curplanenum = facet->borderPlanes[j];
					curinward = facet->borderInward[j];
				}
				else {
					curplanenum = facet->surfacePlane;
					curinward = false;
					//continue;
				}
				//
				if (curplanenum == planenum) continue;

				Vector4Copy( pc->planes[ curplanenum ].plane, plane );
				if ( !curinward ) {
					VectorSubtract( vec3_origin, plane, plane );
					plane[3] = -plane[3];
				}
		//			if ( !facet->borderNoAdjust[j] ) {
					plane[3] -= cv->value;
		//			}
				for (n = 0; n < 3; n++)
				{
					if (plane[n] > 0) v1[n] = maxs[n];
					else v1[n] = mins[n];
				} //end for
				VectorNegate(plane, v2);
				plane[3] -= fabs(DotProduct(v1, v2));

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
			}
			else
				GLog.Write("winding chopped away by border planes\n");
		}
	}

	// draw the debug block
	{
		vec3_t			v[3];

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
	vec3_t			v[4];

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
