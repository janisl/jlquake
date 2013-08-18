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

#include "SurfaceFaceQ3.h"
#include "surfaces.h"
#include "backend.h"
#include "marks.h"
#include "cvars.h"
#include "../../common/common_defs.h"

idSurfaceFaceQ3::~idSurfaceFaceQ3() {
	Mem_Free( data );
}

cplane_t idSurfaceFaceQ3::GetPlane() const {
	cplane_t old;
	plane.ToOldCPlane( old );
	return old;
}

void idSurfaceFaceQ3::Draw() {
	srfSurfaceFace_t* surf = ( srfSurfaceFace_t* )data;
	RB_CHECKOVERFLOW( numVertexes, surf->numIndices );

	int dlightBits = this->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	unsigned* indices = ( unsigned* )( ( ( char* )surf ) + surf->ofsIndices );

	int Bob = tess.numVertexes;
	unsigned* tessIndexes = tess.indexes + tess.numIndexes;
	for ( int i = surf->numIndices - 1; i >= 0; i-- ) {
		tessIndexes[ i ] = indices[ i ] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	idWorldVertex* vert = vertexes;
	for ( int i = 0, ndx = tess.numVertexes; i < numVertexes; i++, vert++, ndx++ ) {
		vert->xyz.ToOldVec3( tess.xyz[ ndx ] );
		vert->normal.ToOldVec3( tess.normal[ ndx ] );
		vert->st.ToOldVec2( tess.texCoords[ ndx ][ 0 ] );
		vert->lightmap.ToOldVec2( tess.texCoords[ ndx ][ 1 ] );
		*( unsigned int* )&tess.vertexColors[ ndx ] = *( unsigned int* )vert->color;
		tess.vertexDlightBits[ ndx ] = dlightBits;
	}

	tess.numVertexes += numVertexes;
}

bool idSurfaceFaceQ3::CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const {
	// check if the surface has NOIMPACT or NOMARKS set
	if ( !R_ShaderCanHaveMarks( shader ) ) {
		return false;
	}
	// extra check for surfaces to avoid list overflows
	if ( plane.Normal() != vec3_origin ) {
		// the face plane should go through the box
		cplane_t old;
		plane.ToOldCPlane( old );
		int s = BoxOnPlaneSide( mins, maxs, &old );
		if ( s == 1 || s == 2 ) {
			return false;
		}
		float dot = DotProduct( old.normal, dir );
		if ( ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) && dot > -0.5 ) ||
			( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) && dot < -0.5 ) ) {
			// don't add faces that make sharp angles with the projection direction
			return false;
		}
	}
	return true;
}

void idSurfaceFaceQ3::MarkFragments( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) const {
	// check the normal of this face
	vec3_t old;
	plane.Normal().ToOldVec3( old );
	if ( DotProduct( old, projectionDir ) > -0.5 ) {
		return;
	}

	MarkFragmentsOldMapping( projectionDir, numPlanes, normals, dists, maxPoints, pointBuffer,
		maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs );
}

void idSurfaceFaceQ3::MarkFragmentsWolf( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const {
	if ( !oldMapping ) {
		MarkFragmentsWolfMapping( projectionDir, numPlanes, normals, dists, maxPoints, pointBuffer,
			maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs,
			center, radius, bestnormal, orientation, numPoints );
	} else {		// old mapping
		MarkFragmentsOldMapping( projectionDir, numPlanes, normals, dists, maxPoints, pointBuffer,
			maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs );
	}
}

void idSurfaceFaceQ3::MarkFragmentsOldMapping( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) const {
	srfSurfaceFace_t* surf = ( srfSurfaceFace_t* )GetBrush46Data();
	int* indexes = ( int* )( ( byte* )surf + surf->ofsIndices );
	for ( int k = 0; k < surf->numIndices; k += 3 ) {
		vec3_t clipPoints[ 2 ][ MAX_VERTS_ON_POLY ];
		for ( int j = 0; j < 3; j++ ) {
			( vertexes[ indexes[ k + j ] ].xyz + plane.Normal() * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ j ] );
		}
		// add the fragments of this face
		R_AddMarkFragments( 3, clipPoints,
			numPlanes, normals, dists,
			maxPoints, pointBuffer,
			maxFragments, fragmentBuffer,
			returnedPoints, returnedFragments, mins, maxs );
		if ( *returnedFragments == maxFragments ) {
			return;	// not enough space for more fragments
		}
	}
}

void idSurfaceFaceQ3::MarkFragmentsWolfMapping( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const {
	vec3_t axis[ 3 ];
	float texCoordScale, dot;
	vec3_t originalPoints[ 4 ];
	vec3_t newCenter, delta;
	int oldNumPoints;
	float epsilon = 0.5;
	// duplicated so we don't mess with the original clips for the curved surfaces
	vec3_t lnormals[ MAX_VERTS_ON_POLY + 2 ];
	float ldists[ MAX_VERTS_ON_POLY + 2 ];
	vec3_t lmins, lmaxs;
	vec3_t surfnormal;

	srfSurfaceFace_t* surf = ( srfSurfaceFace_t* )GetBrush46Data();

	plane.Normal().ToOldVec3( surfnormal );

	// Ridah, create a new clip box such that this decal surface is mapped onto
	// the current surface without distortion. To find the center of the new clip box,
	// we project the center of the original impact center out along the projection vector,
	// onto the current surface

	// find the center of the new decal
	dot = DotProduct( center, surfnormal );
	dot -= plane.Dist();
	// check the normal of this face
	if ( dot < -epsilon && DotProduct( surfnormal, projectionDir ) >= 0.01 ) {
		return;
	} else if ( idMath::Fabs( dot ) > radius ) {
		return;
	}
	// if the impact point is behind the surface, subtract the projection, otherwise add it
	VectorMA( center, -dot, bestnormal, newCenter );

	// recalc dot from the offset position
	dot = DotProduct( newCenter, surfnormal );
	dot -= plane.Dist();
	VectorMA( newCenter, -dot, surfnormal, newCenter );

	VectorMA( newCenter, MARKER_OFFSET, surfnormal, newCenter );

	// create the texture axis
	VectorNormalize2( surfnormal, axis[ 0 ] );
	PerpendicularVector( axis[ 1 ], axis[ 0 ] );
	RotatePointAroundVector( axis[ 2 ], axis[ 0 ], axis[ 1 ], ( float )orientation );
	CrossProduct( axis[ 0 ], axis[ 2 ], axis[ 1 ] );

	texCoordScale = 0.5 * 1.0 / radius;

	// create the full polygon
	for ( int j = 0; j < 3; j++ ) {
		originalPoints[ 0 ][ j ] = newCenter[ j ] - radius * axis[ 1 ][ j ] - radius * axis[ 2 ][ j ];
		originalPoints[ 1 ][ j ] = newCenter[ j ] + radius * axis[ 1 ][ j ] - radius * axis[ 2 ][ j ];
		originalPoints[ 2 ][ j ] = newCenter[ j ] + radius * axis[ 1 ][ j ] + radius * axis[ 2 ][ j ];
		originalPoints[ 3 ][ j ] = newCenter[ j ] - radius * axis[ 1 ][ j ] + radius * axis[ 2 ][ j ];
	}

	ClearBounds( lmins, lmaxs );

	// create the bounding planes for the to be projected polygon
	for ( int j = 0; j < 4; j++ ) {
		AddPointToBounds( originalPoints[ j ], lmins, lmaxs );

		vec3_t v1, v2;
		VectorSubtract( originalPoints[ ( j + 1 ) % numPoints ], originalPoints[ j ], v1 );
		VectorSubtract( originalPoints[ j ], surfnormal, v2 );
		VectorSubtract( originalPoints[ j ], v2, v2 );
		CrossProduct( v1, v2, lnormals[ j ] );
		VectorNormalize( lnormals[ j ] );
		ldists[ j ] = DotProduct( lnormals[ j ], originalPoints[ j ] );
	}
	numPlanes = numPoints;

	// done.

	int* indexes = ( int* )( ( byte* )surf + surf->ofsIndices );
	for ( int k = 0; k < surf->numIndices; k += 3 ) {
		vec3_t clipPoints[ 2 ][ MAX_VERTS_ON_POLY ];
		for ( int j = 0; j < 3; j++ ) {
			vec3_t v;
			vertexes[ indexes[ k + j ] ].xyz.ToOldVec3( v );
			VectorMA( v, MARKER_OFFSET, surfnormal, clipPoints[ 0 ][ j ] );
		}

		oldNumPoints = *returnedPoints;

		// add the fragments of this face
		R_AddMarkFragments( 3, clipPoints,
			numPlanes, lnormals, ldists,
			maxPoints, pointBuffer,
			maxFragments, fragmentBuffer,
			returnedPoints, returnedFragments, lmins, lmaxs );

		if ( oldNumPoints != *returnedPoints ) {
			// flag this surface as already having computed ST's
			fragmentBuffer[ *returnedFragments - 1 ].numPoints *= -1;

			// Ridah, calculate ST's
			for ( int j = 0; j < ( *returnedPoints - oldNumPoints ); j++ ) {
				VectorSubtract( ( float* )pointBuffer + 5 * ( oldNumPoints + j ), newCenter, delta );
				*( ( float* )pointBuffer + 5 * ( oldNumPoints + j ) + 3 ) = 0.5 + DotProduct( delta, axis[ 1 ] ) * texCoordScale;
				*( ( float* )pointBuffer + 5 * ( oldNumPoints + j ) + 4 ) = 0.5 + DotProduct( delta, axis[ 2 ] ) * texCoordScale;
			}
		}

		if ( *returnedFragments == maxFragments ) {
			return;	// not enough space for more fragments
		}
	}
}

bool idSurfaceFaceQ3::DoCull( shader_t* shader ) const {
	if ( shader->cullType == CT_TWO_SIDED ) {
		return false;
	}

	// face culling
	if ( !r_facePlaneCull->integer ) {
		return false;
	}

	vec3_t old;
	plane.Normal().ToOldVec3( old );
	float d = DotProduct( tr.orient.viewOrigin, old );

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here
	if ( shader->cullType == CT_FRONT_SIDED ) {
		if ( d < plane.Dist() - 8 ) {
			return true;
		}
	} else {
		if ( d > plane.Dist() + 8 ) {
			return true;
		}
	}

	return false;
}

int idSurfaceFaceQ3::DoMarkDynamicLights( int dlightBits ) {
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[ i ];
		vec3_t old;
		plane.Normal().ToOldVec3( old );
		float d = DotProduct( dl->origin, old ) - plane.Dist();
		if ( d < -dl->radius || d > dl->radius ) {
			// dlight doesn't reach the plane
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
