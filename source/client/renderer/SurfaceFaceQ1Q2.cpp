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

#include "SurfaceFaceQ1Q2.h"

//	Fills in s->texturemins[] and s->extents[]
void idSurfaceFaceQ1Q2::CalcSurfaceExtents() {
	float mins[ 2 ];
	mins[ 0 ] = mins[ 1 ] = 999999;
	float maxs[ 2 ];
	maxs[ 0 ] = maxs[ 1 ] = -99999;

	for ( int i = 0; i < numVertexes; i++ ) {
		vec3_t old;
		vertexes[ i ].xyz.ToOldVec3( old );
		for ( int j = 0; j < 2; j++ ) {
			float val = DotProduct( old, textureInfo->vecs[ j ] ) + textureInfo->vecs[ j ][ 3 ];
			if ( val < mins[ j ] ) {
				mins[ j ] = val;
			}
			if ( val > maxs[ j ] ) {
				maxs[ j ] = val;
			}
		}
	}

	int bmins[ 2 ], bmaxs[ 2 ];
	for ( int i = 0; i < 2; i++ ) {
		bmins[ i ] = floor( mins[ i ] / 16 );
		bmaxs[ i ] = ceil( maxs[ i ] / 16 );

		textureMins[ i ] = bmins[ i ] * 16;
		extents[ i ] = ( bmaxs[ i ] - bmins[ i ] ) * 16;
	}
}
