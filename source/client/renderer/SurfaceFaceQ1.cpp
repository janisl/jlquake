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

#include "SurfaceFaceQ1.h"
#include "surfaces.h"
#include "backend.h"

#define BACKFACE_EPSILON    0.01

idSurfaceFaceQ1::idSurfaceFaceQ1() {
	Com_Memset( &surf, 0, sizeof( surf ) );
	data = &surf;
	texturechain = NULL;
}

void idSurfaceFaceQ1::Draw() {
	if ( !( surf.flags & ( BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWSKY ) ) &&
		!( backEnd.currentEntity->e.renderfx & ( RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT ) ) ) {
		R_RenderDynamicLightmaps( this );
	}

	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += surf.numVerts;
	tess.numIndexes += surf.numIndexes;

	float* v = surf.verts[ 0 ].v;
	for ( int i = 0; i < surf.numVerts; i++, v += BRUSH29_VERTEXSIZE ) {
		tess.xyz[ numVerts + i ][ 0 ] = v[ 0 ];
		tess.xyz[ numVerts + i ][ 1 ] = v[ 1 ];
		tess.xyz[ numVerts + i ][ 2 ] = v[ 2 ];
		tess.texCoords[ numVerts + i ][ 0 ][ 0 ] = v[ 3 ];
		tess.texCoords[ numVerts + i ][ 0 ][ 1 ] = v[ 4 ];
		tess.texCoords[ numVerts + i ][ 1 ][ 0 ] = v[ 5 ];
		tess.texCoords[ numVerts + i ][ 1 ][ 1 ] = v[ 6 ];
		if ( surf.flags & BRUSH29_SURF_PLANEBACK ) {
			tess.normal[ numVerts + i ][ 0 ] = -surf.plane->normal[ 0 ];
			tess.normal[ numVerts + i ][ 1 ] = -surf.plane->normal[ 1 ];
			tess.normal[ numVerts + i ][ 2 ] = -surf.plane->normal[ 2 ];
		} else {
			tess.normal[ numVerts + i ][ 0 ] = surf.plane->normal[ 0 ];
			tess.normal[ numVerts + i ][ 1 ] = surf.plane->normal[ 1 ];
			tess.normal[ numVerts + i ][ 2 ] = surf.plane->normal[ 2 ];
		}
	}
	for ( int i = 0; i < surf.numIndexes; i++ ) {
		tess.indexes[ numIndexes + i ] = numVerts + surf.indexes[ i ];
	}
}

bool idSurfaceFaceQ1::DoCull( shader_t* shader ) const {
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

	if ( dot < BACKFACE_EPSILON ) {
		return true;		// wrong side
	}
	return false;
}

int idSurfaceFaceQ1::DoMarkDynamicLights( int dlightBits ) {
	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
