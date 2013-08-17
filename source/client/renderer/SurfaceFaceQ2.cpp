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
	RB_CHECKOVERFLOW( surf.numVerts, surf.numIndexes );

	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += surf.numVerts;
	tess.numIndexes += surf.numIndexes;

	idWorldVertex* vert = vertexes;
	float* v = surf.verts[ 0 ].v;
	for ( int i = 0; i < surf.numVerts; i++, vert++, v += BRUSH38_VERTEXSIZE ) {
		vert->xyz.ToOldVec3( tess.xyz[ numVerts + i ] );
		vert->normal.ToOldVec3( tess.normal[ numVerts + i ] );
		tess.texCoords[ numVerts + i ][ 0 ][ 0 ] = v[ 0 ];
		tess.texCoords[ numVerts + i ][ 0 ][ 1 ] = v[ 1 ];
		tess.texCoords[ numVerts + i ][ 1 ][ 0 ] = v[ 2 ];
		tess.texCoords[ numVerts + i ][ 1 ][ 1 ] = v[ 3 ];
	}
	for ( int i = 0; i < surf.numIndexes; i++ ) {
		tess.indexes[ numIndexes + i ] = numVerts + surf.indexes[ i ];
	}
	c_brush_polys++;
}

bool idSurfaceFaceQ2::DoCull( shader_t* shader ) const {
	cplane_t* plane = surf.plane;

	double dot;
	switch ( plane->type ) {
	case PLANE_X:
		dot = tr.orient.viewOrigin[ 0 ] - plane->dist;
		break;
	case PLANE_Y:
		dot = tr.orient.viewOrigin[ 1 ] - plane->dist;
		break;
	case PLANE_Z:
		dot = tr.orient.viewOrigin[ 2 ] - plane->dist;
		break;
	default:
		dot = DotProduct( tr.orient.viewOrigin, plane->normal ) - plane->dist;
		break;
	}

	if ( surf.flags & BRUSH29_SURF_PLANEBACK ) {
		dot = -dot;
	}

	if ( dot < 0 ) {
		return true;		// wrong side
	}
	return false;
}

int idSurfaceFaceQ2::DoMarkDynamicLights( int dlightBits ) {
	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
