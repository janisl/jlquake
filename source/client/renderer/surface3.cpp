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

/*
  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

#include "surfaces.h"
#include "main.h"
#include "backend.h"
#include "particle.h"
#include "sky.h"
#include "cvars.h"
#include "state.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"

void RB_CheckOverflow( int verts, int indexes ) {
	if ( tess.numVertexes + verts < SHADER_MAX_VERTEXES &&
		 tess.numIndexes + indexes < SHADER_MAX_INDEXES ) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		common->Error( "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		common->Error( "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface( tess.shader, tess.fogNum );
}

void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte* color, float s1, float t1, float s2, float t2 ) {
	RB_CHECKOVERFLOW( 4, 6 );

	int ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ ndx ][ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx ][ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx ][ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 1 ][ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx + 1 ][ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx + 1 ][ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 2 ][ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 2 ][ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 2 ][ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];

	tess.xyz[ ndx + 3 ][ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 3 ][ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 3 ][ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];

	// constant normal all the way around
	vec3_t normal;
	VectorSubtract( vec3_origin, backEnd.viewParms.orient.axis[ 0 ], normal );

	tess.normal[ ndx ][ 0 ] = tess.normal[ ndx + 1 ][ 0 ] = tess.normal[ ndx + 2 ][ 0 ] = tess.normal[ ndx + 3 ][ 0 ] = normal[ 0 ];
	tess.normal[ ndx ][ 1 ] = tess.normal[ ndx + 1 ][ 1 ] = tess.normal[ ndx + 2 ][ 1 ] = tess.normal[ ndx + 3 ][ 1 ] = normal[ 1 ];
	tess.normal[ ndx ][ 2 ] = tess.normal[ ndx + 1 ][ 2 ] = tess.normal[ ndx + 2 ][ 2 ] = tess.normal[ ndx + 3 ][ 2 ] = normal[ 2 ];

	// standard square texture coordinates
	tess.texCoords[ ndx ][ 0 ][ 0 ] = tess.texCoords[ ndx ][ 1 ][ 0 ] = s1;
	tess.texCoords[ ndx ][ 0 ][ 1 ] = tess.texCoords[ ndx ][ 1 ][ 1 ] = t1;

	tess.texCoords[ ndx + 1 ][ 0 ][ 0 ] = tess.texCoords[ ndx + 1 ][ 1 ][ 0 ] = s2;
	tess.texCoords[ ndx + 1 ][ 0 ][ 1 ] = tess.texCoords[ ndx + 1 ][ 1 ][ 1 ] = t1;

	tess.texCoords[ ndx + 2 ][ 0 ][ 0 ] = tess.texCoords[ ndx + 2 ][ 1 ][ 0 ] = s2;
	tess.texCoords[ ndx + 2 ][ 0 ][ 1 ] = tess.texCoords[ ndx + 2 ][ 1 ][ 1 ] = t2;

	tess.texCoords[ ndx + 3 ][ 0 ][ 0 ] = tess.texCoords[ ndx + 3 ][ 1 ][ 0 ] = s1;
	tess.texCoords[ ndx + 3 ][ 0 ][ 1 ] = tess.texCoords[ ndx + 3 ][ 1 ][ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	*( unsigned int* )&tess.vertexColors[ ndx ] =
		*( unsigned int* )&tess.vertexColors[ ndx + 1 ] =
			*( unsigned int* )&tess.vertexColors[ ndx + 2 ] =
				*( unsigned int* )&tess.vertexColors[ ndx + 3 ] =
					*( unsigned int* )color;

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte* color ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}
