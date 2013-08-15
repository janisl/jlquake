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

#include "SurfaceDecal.h"
#include "surfaces.h"

idSurfaceDecal::idSurfaceDecal() {
}

void idSurfaceDecal::Draw() {
	RB_CHECKOVERFLOW( surf.numVerts, 3 * ( surf.numVerts - 2 ) );

	// fan triangles into the tess array
	int numv = tess.numVertexes;
	for ( int i = 0; i < surf.numVerts; i++ ) {
		VectorCopy( surf.verts[ i ].xyz, tess.xyz[ numv ] );
		tess.texCoords[ numv ][ 0 ][ 0 ] = surf.verts[ i ].st[ 0 ];
		tess.texCoords[ numv ][ 0 ][ 1 ] = surf.verts[ i ].st[ 1 ];
		*( int* )&tess.vertexColors[ numv ] = *( int* )surf.verts[ i ].modulate;
		numv++;
	}

	//	generate fan indexes into the tess array
	for ( int i = 0; i < surf.numVerts - 2; i++ ) {
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}
