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

static const patchCollide_t	*debugPatchCollide;
static const facet_t		*debugFacet;
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
================================================================================

TRACE TESTING

================================================================================
*/

/*
====================
CM_TracePointThroughPatchCollide

  special case for point traces because the patch collide "brushes" have no volume
====================
*/
void CM_TracePointThroughPatchCollide( traceWork_t *tw, const patchCollide_t* pc ) {
	qboolean	frontFacing[MAX_PATCH_PLANES];
	float		intersection[MAX_PATCH_PLANES];
	float		intersect;
	const patchPlane_t	*planes;
	const facet_t	*facet;
	int			i, j, k;
	float		offset;
	float		d1, d2;
	static QCvar *cv;

	if ( !cm_playerCurveClip->integer || !tw->isPoint ) {
		return;
	}

	// determine the trace's relationship to all planes
	planes = pc->planes;
	for ( i = 0 ; i < pc->numPlanes ; i++, planes++ ) {
		offset = DotProduct( tw->offsets[ planes->signbits ], planes->plane );
		d1 = DotProduct( tw->start, planes->plane ) - planes->plane[3] + offset;
		d2 = DotProduct( tw->end, planes->plane ) - planes->plane[3] + offset;
		if ( d1 <= 0 ) {
			frontFacing[i] = false;
		} else {
			frontFacing[i] = true;
		}
		if ( d1 == d2 ) {
			intersection[i] = 99999;
		} else {
			intersection[i] = d1 / ( d1 - d2 );
			if ( intersection[i] <= 0 ) {
				intersection[i] = 99999;
			}
		}
	}


	// see if any of the surface planes are intersected
	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		if ( !frontFacing[facet->surfacePlane] ) {
			continue;
		}
		intersect = intersection[facet->surfacePlane];
		if ( intersect < 0 ) {
			continue;		// surface is behind the starting point
		}
		if ( intersect > tw->trace.fraction ) {
			continue;		// already hit something closer
		}
		for ( j = 0 ; j < facet->numBorders ; j++ ) {
			k = facet->borderPlanes[j];
			if ( frontFacing[k] ^ facet->borderInward[j] ) {
				if ( intersection[k] > intersect ) {
					break;
				}
			} else {
				if ( intersection[k] < intersect ) {
					break;
				}
			}
		}
		if ( j == facet->numBorders ) {
			// we hit this facet
			if (!cv) {
				cv = Cvar_Get( "r_debugSurfaceUpdate", "1", 0 );
			}
			if (cv->integer) {
				debugPatchCollide = pc;
				debugFacet = facet;
			}
			planes = &pc->planes[facet->surfacePlane];

			// calculate intersection with a slight pushoff
			offset = DotProduct( tw->offsets[ planes->signbits ], planes->plane );
			d1 = DotProduct( tw->start, planes->plane ) - planes->plane[3] + offset;
			d2 = DotProduct( tw->end, planes->plane ) - planes->plane[3] + offset;
			tw->trace.fraction = ( d1 - SURFACE_CLIP_EPSILON ) / ( d1 - d2 );

			if ( tw->trace.fraction < 0 ) {
				tw->trace.fraction = 0;
			}

			VectorCopy( planes->plane,  tw->trace.plane.normal );
			tw->trace.plane.dist = planes->plane[3];
		}
	}
}

/*
====================
CM_CheckFacetPlane
====================
*/
int CM_CheckFacetPlane(float *plane, vec3_t start, vec3_t end, float *enterFrac, float *leaveFrac, int *hit) {
	float d1, d2, f;

	*hit = false;

	d1 = DotProduct( start, plane ) - plane[3];
	d2 = DotProduct( end, plane ) - plane[3];

	// if completely in front of face, no intersection with the entire facet
	if (d1 > 0 && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 )  ) {
		return false;
	}

	// if it doesn't cross the plane, the plane isn't relevent
	if (d1 <= 0 && d2 <= 0 ) {
		return true;
	}

	// crosses face
	if (d1 > d2) {	// enter
		f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
		if ( f < 0 ) {
			f = 0;
		}
		//always favor previous plane hits and thus also the surface plane hit
		if (f > *enterFrac) {
			*enterFrac = f;
			*hit = true;
		}
	} else {	// leave
		f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
		if ( f > 1 ) {
			f = 1;
		}
		if (f < *leaveFrac) {
			*leaveFrac = f;
		}
	}
	return true;
}

/*
====================
CM_TraceThroughPatchCollide
====================
*/
void CM_TraceThroughPatchCollide( traceWork_t *tw, const patchCollide_t* pc ) {
	int i, j, hit, hitnum;
	float offset, enterFrac, leaveFrac, t;
	patchPlane_t *planes;
	facet_t	*facet;
	float plane[4], bestplane[4];
	vec3_t startp, endp;
	static QCvar *cv;

	if (tw->isPoint) {
		CM_TracePointThroughPatchCollide( tw, pc );
		return;
	}

	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		enterFrac = -1.0;
		leaveFrac = 1.0;
		hitnum = -1;
		//
		planes = &pc->planes[ facet->surfacePlane ];
		VectorCopy(planes->plane, plane);
		plane[3] = planes->plane[3];
		if ( tw->sphere.use ) {
			// adjust the plane distance apropriately for radius
			plane[3] += tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct( plane, tw->sphere.offset );
			if ( t > 0.0f ) {
				VectorSubtract( tw->start, tw->sphere.offset, startp );
				VectorSubtract( tw->end, tw->sphere.offset, endp );
			}
			else {
				VectorAdd( tw->start, tw->sphere.offset, startp );
				VectorAdd( tw->end, tw->sphere.offset, endp );
			}
		}
		else {
			offset = DotProduct( tw->offsets[ planes->signbits ], plane);
			plane[3] -= offset;
			VectorCopy( tw->start, startp );
			VectorCopy( tw->end, endp );
		}

		if (!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac, &hit)) {
			continue;
		}
		if (hit) {
			Vector4Copy(plane, bestplane);
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[ facet->borderPlanes[j] ];
			if (facet->borderInward[j]) {
				VectorNegate(planes->plane, plane);
				plane[3] = -planes->plane[3];
			}
			else {
				VectorCopy(planes->plane, plane);
				plane[3] = planes->plane[3];
			}
			if ( tw->sphere.use ) {
				// adjust the plane distance apropriately for radius
				plane[3] += tw->sphere.radius;

				// find the closest point on the capsule to the plane
				t = DotProduct( plane, tw->sphere.offset );
				if ( t > 0.0f ) {
					VectorSubtract( tw->start, tw->sphere.offset, startp );
					VectorSubtract( tw->end, tw->sphere.offset, endp );
				}
				else {
					VectorAdd( tw->start, tw->sphere.offset, startp );
					VectorAdd( tw->end, tw->sphere.offset, endp );
				}
			}
			else {
				// NOTE: this works even though the plane might be flipped because the bbox is centered
				offset = DotProduct( tw->offsets[ planes->signbits ], plane);
				plane[3] += fabs(offset);
				VectorCopy( tw->start, startp );
				VectorCopy( tw->end, endp );
			}

			if (!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac, &hit)) {
				break;
			}
			if (hit) {
				hitnum = j;
				Vector4Copy(plane, bestplane);
			}
		}
		if (j < facet->numBorders) continue;
		//never clip against the back side
		if (hitnum == facet->numBorders - 1) continue;

		if (enterFrac < leaveFrac && enterFrac >= 0) {
			if (enterFrac < tw->trace.fraction) {
				if (enterFrac < 0) {
					enterFrac = 0;
				}
				if (!cv) {
					cv = Cvar_Get( "r_debugSurfaceUpdate", "1", 0 );
				}
				if (cv && cv->integer) {
					debugPatchCollide = pc;
					debugFacet = facet;
				}

				tw->trace.fraction = enterFrac;
				VectorCopy( bestplane, tw->trace.plane.normal );
				tw->trace.plane.dist = bestplane[3];
			}
		}
	}
}


/*
=======================================================================

POSITION TEST

=======================================================================
*/

/*
====================
CM_PositionTestInPatchCollide
====================
*/
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const patchCollide_t* pc ) {
	int i, j;
	float offset, t;
	patchPlane_t *planes;
	facet_t	*facet;
	float plane[4];
	vec3_t startp;

	if (tw->isPoint) {
		return false;
	}
	//
	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		planes = &pc->planes[ facet->surfacePlane ];
		VectorCopy(planes->plane, plane);
		plane[3] = planes->plane[3];
		if ( tw->sphere.use ) {
			// adjust the plane distance apropriately for radius
			plane[3] += tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct( plane, tw->sphere.offset );
			if ( t > 0 ) {
				VectorSubtract( tw->start, tw->sphere.offset, startp );
			}
			else {
				VectorAdd( tw->start, tw->sphere.offset, startp );
			}
		}
		else {
			offset = DotProduct( tw->offsets[ planes->signbits ], plane);
			plane[3] -= offset;
			VectorCopy( tw->start, startp );
		}

		if ( DotProduct( plane, startp ) - plane[3] > 0.0f ) {
			continue;
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[ facet->borderPlanes[j] ];
			if (facet->borderInward[j]) {
				VectorNegate(planes->plane, plane);
				plane[3] = -planes->plane[3];
			}
			else {
				VectorCopy(planes->plane, plane);
				plane[3] = planes->plane[3];
			}
			if ( tw->sphere.use ) {
				// adjust the plane distance apropriately for radius
				plane[3] += tw->sphere.radius;

				// find the closest point on the capsule to the plane
				t = DotProduct( plane, tw->sphere.offset );
				if ( t > 0.0f ) {
					VectorSubtract( tw->start, tw->sphere.offset, startp );
				}
				else {
					VectorAdd( tw->start, tw->sphere.offset, startp );
				}
			}
			else {
				// NOTE: this works even though the plane might be flipped because the bbox is centered
				offset = DotProduct( tw->offsets[ planes->signbits ], plane);
				plane[3] += fabs(offset);
				VectorCopy( tw->start, startp );
			}

			if ( DotProduct( plane, startp ) - plane[3] > 0.0f ) {
				break;
			}
		}
		if (j < facet->numBorders) {
			continue;
		}
		// inside this patch facet
		return true;
	}
	return false;
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
