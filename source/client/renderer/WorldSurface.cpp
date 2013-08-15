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

#include "WorldSurface.h"
#include "decals.h"
#include "../../common/content_types.h"

void idWorldSurface::ProjectDecal( decalProjector_t* dp, mbrush46_model_t* bmodel ) {
}

bool idWorldSurface::ClipDecal( struct decalProjector_t* dp )
{
	//	early outs
	if ( dp->shader == NULL ) {
		return false;
	}
	if ( ( shader->surfaceFlags & ( BSP46SURF_NOIMPACT | BSP46SURF_NOMARKS ) ) ||
		 ( shader->contentFlags & BSP46CONTENTS_FOG ) ) {
		return false;
	}

	//	add to counts
	tr.pc.c_decalTestSurfaces++;

	//	get generic surface
	srfGeneric_t* gen = ( srfGeneric_t* )GetBrush46Data();

	//	test bounding sphere
	if ( !R_TestDecalBoundingSphere( dp, gen->localOrigin, ( gen->radius * gen->radius ) ) ) {
		return false;
	}

	//	planar surface
	if ( gen->plane.normal[ 0 ] || gen->plane.normal[ 1 ] || gen->plane.normal[ 2 ] ) {
		//	backface check
		float d = DotProduct( dp->planes[ 0 ], gen->plane.normal );
		if ( d < -0.0001 ) {
			return false;
		}

		//	plane-sphere check
		d = DotProduct( dp->center, gen->plane.normal ) - gen->plane.dist;
		if ( fabs( d ) >= dp->radius ) {
			return false;
		}
	}

	//	add to counts
	tr.pc.c_decalClipSurfaces++;
	return true;
}

bool idWorldSurface::CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) {
	return false;
}

void idWorldSurface::MarkFragments( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) {
}

void idWorldSurface::MarkFragmentsWolf( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) {
}
