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

cplane_t idSurfaceFaceQ3::GetPlane() const {
	return ( ( srfSurfaceFace_t* )data )->plane;
}

void idSurfaceFaceQ3::Draw() {
	srfSurfaceFace_t* surf = ( srfSurfaceFace_t* )data;
	RB_CHECKOVERFLOW( surf->numPoints, surf->numIndices );

	int dlightBits = this->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	unsigned* indices = ( unsigned* )( ( ( char* )surf ) + surf->ofsIndices );

	int Bob = tess.numVertexes;
	unsigned* tessIndexes = tess.indexes + tess.numIndexes;
	for ( int i = surf->numIndices - 1; i >= 0; i-- ) {
		tessIndexes[ i ] = indices[ i ] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	float* v = surf->points[ 0 ];

	int numPoints = surf->numPoints;

	if ( tess.shader->needsNormal ) {
		float* normal = surf->plane.normal;
		for ( int i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ ) {
			VectorCopy( normal, tess.normal[ ndx ] );
		}
	}

	v = surf->points[ 0 ];
	for ( int i = 0, ndx = tess.numVertexes; i < numPoints; i++, v += BRUSH46_VERTEXSIZE, ndx++ ) {
		VectorCopy( v, tess.xyz[ ndx ] );
		tess.texCoords[ ndx ][ 0 ][ 0 ] = v[ 3 ];
		tess.texCoords[ ndx ][ 0 ][ 1 ] = v[ 4 ];
		tess.texCoords[ ndx ][ 1 ][ 0 ] = v[ 5 ];
		tess.texCoords[ ndx ][ 1 ][ 1 ] = v[ 6 ];
		*( unsigned int* )&tess.vertexColors[ ndx ] = *( unsigned int* )&v[ 7 ];
		tess.vertexDlightBits[ ndx ] = dlightBits;
	}

	tess.numVertexes += surf->numPoints;
}
