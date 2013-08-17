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

#include "SurfaceTriangles.h"
#include "surfaces.h"
#include "backend.h"
#include "decals.h"
#include "marks.h"
#include "../../common/common_defs.h"

cplane_t idSurfaceTriangles::GetPlane() const {
	srfTriangles_t* tri = ( srfTriangles_t* )data;
	idWorldVertex* v1 = vertexes + tri->indexes[ 0 ];
	idWorldVertex* v2 = vertexes + tri->indexes[ 1 ];
	idWorldVertex* v3 = vertexes + tri->indexes[ 2 ];
	vec4_t plane4;
	vec3_t oldv1, oldv2, oldv3;
	v1->xyz.ToOldVec3( oldv1 );
	v2->xyz.ToOldVec3( oldv2 );
	v3->xyz.ToOldVec3( oldv3 );
	PlaneFromPoints( plane4, oldv1, oldv2, oldv3 );
	cplane_t plane;
	VectorCopy( plane4, plane.normal );
	plane.dist = plane4[ 3 ];
	return plane;
}

void idSurfaceTriangles::Draw() {
	srfTriangles_t* srf = ( srfTriangles_t* ) data;
	// ydnar: moved before overflow so dlights work properly
	RB_CHECKOVERFLOW( srf->numVerts, srf->numIndexes );

	int dlightBits = this->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	for ( int i = 0; i < srf->numIndexes; i += 3 ) {
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}
	tess.numIndexes += srf->numIndexes;

	idWorldVertex* vert = vertexes;
	mem_drawVert_t* dv = srf->verts;
	float* xyz = tess.xyz[ tess.numVertexes ];
	float* normal = tess.normal[ tess.numVertexes ];
	float* texCoords = tess.texCoords[ tess.numVertexes ][ 0 ];
	byte* color = tess.vertexColors[ tess.numVertexes ];
	bool needsNormal = tess.shader->needsNormal;

	for ( int i = 0; i < srf->numVerts; i++, vert++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4 ) {
		vert->xyz.ToOldVec3( xyz );

		if ( needsNormal ) {
			vert->normal.ToOldVec3( normal );
		}

		texCoords[ 0 ] = dv->st[ 0 ];
		texCoords[ 1 ] = dv->st[ 1 ];

		texCoords[ 2 ] = dv->lightmap[ 0 ];
		texCoords[ 3 ] = dv->lightmap[ 1 ];

		*( int* )color = *( int* )dv->color;
	}

	for ( int i = 0; i < srf->numVerts; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}

void idSurfaceTriangles::ProjectDecal( decalProjector_t* dp, mbrush46_model_t* bmodel ) const {
	if ( !ClipDecal( dp ) ) {
		return;
	}

	//	get surface
	srfTriangles_t* srf = ( srfTriangles_t* )GetBrush46Data();

	//	walk triangle list
	for ( int i = 0; i < srf->numIndexes; i += 3 ) {
		//	make triangle
		vec3_t points[ 2 ][ MAX_DECAL_VERTS ];
		vertexes[ srf->indexes[ i ] ].xyz.ToOldVec3( points[ 0 ][ 0 ] );
		vertexes[ srf->indexes[ i + 1 ] ].xyz.ToOldVec3( points[ 0 ][ 1 ] );
		vertexes[ srf->indexes[ i + 2 ] ].xyz.ToOldVec3( points[ 0 ][ 2 ] );

		//	chop it
		ProjectDecalOntoWinding( dp, 3, points, this, bmodel );
	}
}

bool idSurfaceTriangles::CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const {
	return ( GGameType & GAME_ET ) && R_ShaderCanHaveMarks( shader );
}

void idSurfaceTriangles::MarkFragmentsWolf( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const {
	// duplicated so we don't mess with the original clips for the curved surfaces

	if ( !oldMapping ) {
		vec3_t lnormals[ MAX_VERTS_ON_POLY + 2 ];
		float ldists[ MAX_VERTS_ON_POLY + 2 ];
		for ( int k = 0; k < numPoints; k++ ) {
			VectorNegate( normals[ k ], lnormals[ k ] );
			ldists[ k ] = -dists[ k ];
		}
		VectorNegate( normals[ numPoints ], lnormals[ numPoints ] );
		ldists[ numPoints ] = dists[ numPoints + 1 ];
		VectorNegate( normals[ numPoints + 1 ], lnormals[ numPoints + 1 ] );
		ldists[ numPoints + 1 ] = dists[ numPoints ];

		MarkFragmentsOldMapping( numPlanes, lnormals, ldists, maxPoints, pointBuffer,
			maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs );
	} else {
		MarkFragmentsOldMapping( numPlanes, normals, dists, maxPoints, pointBuffer,
			maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs );
	}
}

void idSurfaceTriangles::MarkFragmentsOldMapping( int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) const {
	srfTriangles_t* cts = ( srfTriangles_t* )GetBrush46Data();
	int* indexes = cts->indexes;
	for ( int k = 0; k < cts->numIndexes; k += 3 ) {
		vec3_t clipPoints[ 2 ][ MAX_VERTS_ON_POLY ];
		for ( int j = 0; j < 3; j++ ) {
			( vertexes[ indexes[ k + j ] ].xyz + vertexes[ indexes[ k + j ] ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ j ] );
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

bool idSurfaceTriangles::DoCull( shader_t* shader ) const {
	srfTriangles_t* cv = ( srfTriangles_t* )GetBrush46Data();
	int boxCull = R_CullLocalBox( cv->bounds );

	return boxCull == CULL_OUT;
}

int idSurfaceTriangles::DoMarkDynamicLights( int dlightBits ) {
	// FIXME: more dlight culling to trisurfs...
	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
