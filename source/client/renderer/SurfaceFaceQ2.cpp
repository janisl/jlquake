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

#include "SurfaceFaceQ2.h"
#include "surfaces.h"
#include "main.h"

idSurfaceFaceQ2::idSurfaceFaceQ2() {
	Com_Memset( &surf, 0, sizeof( surf ) );
	data = &surf;
	texturechain = NULL;
}

void idSurfaceFaceQ2::Draw() {
	RB_CHECKOVERFLOW( numVertexes, surf.numIndexes );

	int numTessVerts = tess.numVertexes;
	int numTessIndexes = tess.numIndexes;

	tess.numVertexes += numVertexes;
	tess.numIndexes += surf.numIndexes;

	idWorldVertex* vert = vertexes;
	for ( int i = 0; i < numVertexes; i++, vert++ ) {
		vert->xyz.ToOldVec3( tess.xyz[ numTessVerts + i ] );
		vert->normal.ToOldVec3( tess.normal[ numTessVerts + i ] );
		vert->st.ToOldVec2( tess.texCoords[ numTessVerts + i ][ 0 ] );
		vert->lightmap.ToOldVec2( tess.texCoords[ numTessVerts + i ][ 1 ] );
	}
	for ( int i = 0; i < surf.numIndexes; i++ ) {
		tess.indexes[ numTessIndexes + i ] = numTessVerts + surf.indexes[ i ];
	}
	c_brush_polys++;
}

bool idSurfaceFaceQ2::DoCull( shader_t* shader ) const {
	vec3_t old;
	plane.Normal().ToOldVec3( old );
	double dot = DotProduct( tr.orient.viewOrigin, old ) - plane.Dist();
	if ( dot < 0 ) {
		return true;		// wrong side
	}
	return false;
}

int idSurfaceFaceQ2::DoMarkDynamicLights( int dlightBits ) {
	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
