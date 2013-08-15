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

#include "SurfacePolyBuffer.h"
#include "surfaces.h"

idSurfacePolyBuffer::idSurfacePolyBuffer() {
}

void idSurfacePolyBuffer::Draw() {
	RB_EndSurface();

	RB_BeginSurface( tess.shader, tess.fogNum );

	// ===================================================
	//	Originally tess was pointed to different arrays.
	tess.numIndexes =   surf.pPolyBuffer->numIndicies;
	tess.numVertexes =  surf.pPolyBuffer->numVerts;

	Com_Memcpy( tess.xyz, surf.pPolyBuffer->xyz, tess.numVertexes * sizeof ( vec4_t ) );
	for ( int i = 0; i < tess.numVertexes; i++ ) {
		tess.texCoords[ i ][ 0 ][ 0 ] = surf.pPolyBuffer->st[ i ][ 0 ];
		tess.texCoords[ i ][ 0 ][ 1 ] = surf.pPolyBuffer->st[ i ][ 1 ];
	}
	Com_Memcpy( tess.indexes, surf.pPolyBuffer->indicies, tess.numIndexes * sizeof ( glIndex_t ) );
	Com_Memcpy( tess.vertexColors, surf.pPolyBuffer->color, tess.numVertexes * sizeof ( color4ub_t ) );
	// ===================================================

	RB_EndSurface();
}
