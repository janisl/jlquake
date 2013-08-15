//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
// tr_marks.c -- polygon projection on the world polygons

#include "public.h"
#include "marks.h"
#include "main.h"
#include "../../common/common_defs.h"
#include "../../common/content_types.h"

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2

//	Out must have space for two more vertexes than in
static void R_ChopPolyBehindPlane( int numInPoints, const vec3_t inPoints[ MAX_VERTS_ON_POLY ],
	int* numOutPoints, vec3_t outPoints[ MAX_VERTS_ON_POLY ],
	const vec3_t normal, const vec_t dist, const vec_t epsilon ) {
	// don't clip if it might overflow
	if ( numInPoints >= MAX_VERTS_ON_POLY - 2 ) {
		*numOutPoints = 0;
		return;
	}

	int counts[ 3 ];
	counts[ 0 ] = counts[ 1 ] = counts[ 2 ] = 0;

	// determine sides for each point
	float dists[ MAX_VERTS_ON_POLY + 4 ];
	int sides[ MAX_VERTS_ON_POLY + 4 ];
	for ( int i = 0; i < numInPoints; i++ ) {
		float dot = DotProduct( inPoints[ i ], normal );
		dot -= dist;
		dists[ i ] = dot;
		if ( dot > epsilon ) {
			sides[ i ] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[ i ] = SIDE_BACK;
		} else {
			sides[ i ] = SIDE_ON;
		}
		counts[ sides[ i ] ]++;
	}
	sides[ numInPoints ] = sides[ 0 ];
	dists[ numInPoints ] = dists[ 0 ];

	*numOutPoints = 0;

	if ( !counts[ 0 ] ) {
		return;
	}
	if ( !counts[ 1 ] ) {
		*numOutPoints = numInPoints;
		Com_Memcpy( outPoints, inPoints, numInPoints * sizeof ( vec3_t ) );
		return;
	}

	for ( int i = 0; i < numInPoints; i++ ) {
		const float* p1 = inPoints[ i ];
		float* clip = outPoints[ *numOutPoints ];

		if ( sides[ i ] == SIDE_ON ) {
			VectorCopy( p1, clip );
			( *numOutPoints )++;
			continue;
		}

		if ( sides[ i ] == SIDE_FRONT ) {
			VectorCopy( p1, clip );
			( *numOutPoints )++;
			clip = outPoints[ *numOutPoints ];
		}

		if ( sides[ i + 1 ] == SIDE_ON || sides[ i + 1 ] == sides[ i ] ) {
			continue;
		}

		// generate a split point
		const float* p2 = inPoints[ ( i + 1 ) % numInPoints ];

		float d = dists[ i ] - dists[ i + 1 ];
		float dot;
		if ( d == 0 ) {
			dot = 0;
		} else {
			dot = dists[ i ] / d;
		}

		// clip xyz

		for ( int j = 0; j < 3; j++ ) {
			clip[ j ] = p1[ j ] + dot * ( p2[ j ] - p1[ j ] );
		}

		( *numOutPoints )++;
	}
}

bool R_ShaderCanHaveMarks( shader_t* shader ) {
	return !( shader->surfaceFlags & ( BSP46SURF_NOIMPACT | BSP46SURF_NOMARKS ) ) &&
		 !( shader->contentFlags & BSP46CONTENTS_FOG );
}

static void R_BoxSurfaces_r( mbrush46_node_t* node, vec3_t mins, vec3_t maxs,
	idWorldSurface** list, int listsize, int* listlength, vec3_t dir ) {
	// RF, if this node hasn't been rendered recently, ignore it
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) &&
		 node->visframe < tr.visCount - 2 ) {	// allow us to be a few frames behind
		return;
	}

	// do the tail recursion in a loop
	while ( node->contents == -1 ) {
		int s = BoxOnPlaneSide( mins, maxs, node->plane );
		if ( s == 1 ) {
			node = node->children[ 0 ];
		} else if ( s == 2 ) {
			node = node->children[ 1 ];
		} else {
			R_BoxSurfaces_r( node->children[ 0 ], mins, maxs, list, listsize, listlength, dir );
			node = node->children[ 1 ];
		}
	}

	// Ridah, don't mark alpha surfaces
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) &&
		 node->contents & BSP46CONTENTS_TRANSLUCENT ) {
		return;
	}

	// add the individual surfaces
	idWorldSurface** mark = node->firstmarksurface;
	int c = node->nummarksurfaces;
	while ( c-- ) {
		if ( *listlength >= listsize ) {
			break;
		}
		idWorldSurface* surf = *mark;
		if ( !surf->CheckAddMarks( mins, maxs, dir ) ) {
			surf->viewCount = tr.viewCount;
		}
		// check the viewCount because the surface may have
		// already been added if it spans multiple leafs
		if ( surf->viewCount != tr.viewCount ) {
			surf->viewCount = tr.viewCount;
			list[ *listlength ] = surf;
			( *listlength )++;
		}
		mark++;
	}
}

void R_AddMarkFragments( int numClipPoints, vec3_t clipPoints[ 2 ][ MAX_VERTS_ON_POLY ],
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) {
	// chop the surface by all the bounding planes of the to be projected polygon
	int pingPong = 0;

	for ( int i = 0; i < numPlanes; i++ ) {
		R_ChopPolyBehindPlane( numClipPoints, clipPoints[ pingPong ],
			&numClipPoints, clipPoints[ !pingPong ], normals[ i ], dists[ i ], 0.5 );
		pingPong ^= 1;
		if ( numClipPoints == 0 ) {
			break;
		}
	}
	// completely clipped away?
	if ( numClipPoints == 0 ) {
		return;
	}

	// add this fragment to the returned list
	if ( numClipPoints + ( *returnedPoints ) > maxPoints ) {
		return;	// not enough space for this polygon
	}

	markFragment_t* mf = fragmentBuffer + *returnedFragments;
	mf->firstPoint = *returnedPoints;
	mf->numPoints = numClipPoints;
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		for ( int i = 0; i < numClipPoints; i++ ) {
			VectorCopy( clipPoints[ pingPong ][ i ], ( float* )pointBuffer + 5 * ( *returnedPoints + i ) );
		}
	} else {
		Com_Memcpy( pointBuffer + ( *returnedPoints ) * 3, clipPoints[ pingPong ], numClipPoints * sizeof ( vec3_t ) );
	}

	( *returnedPoints ) += numClipPoints;
	( *returnedFragments )++;
}

int R_MarkFragments( int numPoints, const vec3_t* points, const vec3_t projection,
	int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer ) {
	//increment view count for double check prevention
	tr.viewCount++;

	vec3_t projectionDir;
	VectorNormalize2( projection, projectionDir );

	// find all the brushes that are to be considered
	vec3_t mins, maxs;
	ClearBounds( mins, maxs );
	for ( int i = 0; i < numPoints; i++ ) {
		AddPointToBounds( points[ i ], mins, maxs );
		vec3_t temp;
		VectorAdd( points[ i ], projection, temp );
		AddPointToBounds( temp, mins, maxs );
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA( points[ i ], -20, projectionDir, temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if ( numPoints > MAX_VERTS_ON_POLY ) {
		numPoints = MAX_VERTS_ON_POLY;
	}
	// create the bounding planes for the to be projected polygon
	vec3_t normals[ MAX_VERTS_ON_POLY + 2 ];
	float dists[ MAX_VERTS_ON_POLY + 2 ];
	for ( int i = 0; i < numPoints; i++ ) {
		vec3_t v1, v2;
		VectorSubtract( points[ ( i + 1 ) % numPoints ], points[ i ], v1 );
		VectorAdd( points[ i ], projection, v2 );
		VectorSubtract( points[ i ], v2, v2 );
		CrossProduct( v1, v2, normals[ i ] );
		VectorNormalizeFast( normals[ i ] );
		dists[ i ] = DotProduct( normals[ i ], points[ i ] );
	}

	// add near and far clipping planes for projection
	VectorCopy( projectionDir, normals[ numPoints ] );
	dists[ numPoints ] = DotProduct( normals[ numPoints ], points[ 0 ] ) - 32;
	VectorCopy( projectionDir, normals[ numPoints + 1 ] );
	VectorInverse( normals[ numPoints + 1 ] );
	dists[ numPoints + 1 ] = DotProduct( normals[ numPoints + 1 ], points[ 0 ] ) - 20;
	int numPlanes = numPoints + 2;

	int numsurfaces = 0;
	idWorldSurface* surfaces[ 64 ];
	R_BoxSurfaces_r( tr.world->nodes, mins, maxs, surfaces, 64, &numsurfaces, projectionDir );
	//assert(numsurfaces <= 64);
	//assert(numsurfaces != 64);

	int returnedPoints = 0;
	int returnedFragments = 0;

	for ( int i = 0; i < numsurfaces; i++ ) {
		surfaces[ i ]->MarkFragments( projectionDir,
				numPlanes, normals, dists,
				maxPoints, pointBuffer,
				maxFragments, fragmentBuffer,
				&returnedPoints, &returnedFragments, mins, maxs );
		if ( returnedFragments == maxFragments ) {
			break;	// not enough space for more fragments
		}
	}
	return returnedFragments;
}

int R_MarkFragmentsWolf( int orientation, const vec3_t* points, const vec3_t projection,
	int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer ) {
	int numPlanes;
	int i;
	vec3_t mins, maxs;
	int returnedFragments;
	int returnedPoints;
	vec3_t normals[ MAX_VERTS_ON_POLY + 2 ];
	float dists[ MAX_VERTS_ON_POLY + 2 ];
	vec3_t projectionDir;
	vec3_t v1, v2;
	float radius;
	vec3_t center;			// center of original mark
	int numPoints = 4;				// Ridah, we were only ever passing in 4, so I made this local and used the parameter for the orientation
	qboolean oldMapping = false;

	//increment view count for double check prevention
	tr.viewCount++;

	// RF, negative maxFragments means we want original mapping
	if ( maxFragments < 0 ) {
		maxFragments = -maxFragments;
		oldMapping = true;
	}

	VectorClear( center );
	for ( i = 0; i < numPoints; i++ ) {
		VectorAdd( points[ i ], center, center );
	}
	VectorScale( center, 1.0 / numPoints, center );
	//
	radius = VectorNormalize2( projection, projectionDir ) / 2.0;
	vec3_t bestnormal;
	VectorNegate( projectionDir, bestnormal );
	// find all the brushes that are to be considered
	ClearBounds( mins, maxs );
	for ( i = 0; i < numPoints; i++ ) {
		vec3_t temp;

		AddPointToBounds( points[ i ], mins, maxs );
		VectorMA( points[ i ], 1 * ( 1 + oldMapping * radius * 4 ), projection, temp );
		AddPointToBounds( temp, mins, maxs );
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA( points[ i ], -20 * ( 1.0 + ( float )oldMapping * ( radius / 20.0 ) * 4 ), projectionDir, temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if ( numPoints > MAX_VERTS_ON_POLY ) {
		numPoints = MAX_VERTS_ON_POLY;
	}
	// create the bounding planes for the to be projected polygon
	for ( i = 0; i < numPoints; i++ ) {
		VectorSubtract( points[ ( i + 1 ) % numPoints ], points[ i ], v1 );
		VectorAdd( points[ i ], projection, v2 );
		VectorSubtract( points[ i ], v2, v2 );
		CrossProduct( v1, v2, normals[ i ] );
		VectorNormalize( normals[ i ] );
		dists[ i ] = DotProduct( normals[ i ], points[ i ] );
	}
	// add near and far clipping planes for projection
	VectorCopy( projectionDir, normals[ numPoints ] );
	dists[ numPoints ] = DotProduct( normals[ numPoints ], points[ 0 ] ) - radius * ( 1 + oldMapping * 10 );
	VectorCopy( projectionDir, normals[ numPoints + 1 ] );
	VectorInverse( normals[ numPoints + 1 ] );
	dists[ numPoints + 1 ] = DotProduct( normals[ numPoints + 1 ], points[ 0 ] ) - radius * ( 1 + oldMapping * 10 );
	numPlanes = numPoints + 2;

	int numsurfaces = 0;
	idWorldSurface* surfaces[ 4096 ];
	R_BoxSurfaces_r( tr.world->nodes, mins, maxs, surfaces, 4096, &numsurfaces, projectionDir );

	returnedPoints = 0;
	returnedFragments = 0;

	// find the closest surface to center the decal there, and wrap around other surfaces
	if ( !oldMapping ) {
		VectorNegate( bestnormal, bestnormal );
	}

	for ( i = 0; i < numsurfaces; i++ ) {
		surfaces[ i ]->MarkFragmentsWolf( projectionDir,
				numPlanes, normals, dists,
				maxPoints, pointBuffer,
				maxFragments, fragmentBuffer,
				&returnedPoints, &returnedFragments, mins, maxs,
				oldMapping, center, radius, bestnormal, orientation, numPoints );
		if ( returnedFragments == maxFragments ) {
			break;	// not enough space for more fragments
		}
	}
	return returnedFragments;
}
