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

	idWorldVertex* vert = vertexes;
	for ( int i = 0; i < surf.numVerts; i++, vert++ ) {
		vert->xyz.ToOldVec3( tess.xyz[ numVerts + i ] );
		vert->normal.ToOldVec3( tess.normal[ numVerts + i ] );
		vert->st.ToOldVec2( tess.texCoords[ numVerts + i ][ 0 ] );
		vert->lightmap.ToOldVec2( tess.texCoords[ numVerts + i ][ 1 ] );
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
