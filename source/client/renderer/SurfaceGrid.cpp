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

#include "SurfaceGrid.h"
#include "surfaces.h"
#include "backend.h"
#include "cvars.h"

//	Just copy the grid of points and triangulate
void idSurfaceGrid::Draw() {
	srfGridMesh_t* cv = ( srfGridMesh_t* )data;
	int dlightBits = this->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	float lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	int widthTable[ MAX_GRID_SIZE ];
	widthTable[ 0 ] = 0;
	int lodWidth = 1;
	for ( int i = 1; i < cv->width - 1; i++ ) {
		if ( cv->widthLodError[ i ] <= lodError ) {
			widthTable[ lodWidth ] = i;
			lodWidth++;
		}
	}
	widthTable[ lodWidth ] = cv->width - 1;
	lodWidth++;

	int heightTable[ MAX_GRID_SIZE ];
	heightTable[ 0 ] = 0;
	int lodHeight = 1;
	for ( int i = 1; i < cv->height - 1; i++ ) {
		if ( cv->heightLodError[ i ] <= lodError ) {
			heightTable[ lodHeight ] = i;
			lodHeight++;
		}
	}
	heightTable[ lodHeight ] = cv->height - 1;
	lodHeight++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	int used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		int irows, vrows;
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface( tess.shader, tess.fogNum );
				tess.dlightBits |= dlightBits;	// ydnar: for proper dlighting
			} else {
				break;
			}
		} while ( 1 );

		int rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		int numVertexes = tess.numVertexes;

		float* xyz = tess.xyz[ numVertexes ];
		float* normal = tess.normal[ numVertexes ];
		float* texCoords = tess.texCoords[ numVertexes ][ 0 ];
		byte* color = tess.vertexColors[ numVertexes ];
		int* vDlightBits = &tess.vertexDlightBits[ numVertexes ];
		bool needsNormal = tess.shader->needsNormal;

		for ( int i = 0; i < rows; i++ ) {
			for ( int j = 0; j < lodWidth; j++ ) {
				bsp46_drawVert_t* dv = cv->verts + heightTable[ used + i ] * cv->width + widthTable[ j ];

				xyz[ 0 ] = dv->xyz[ 0 ];
				xyz[ 1 ] = dv->xyz[ 1 ];
				xyz[ 2 ] = dv->xyz[ 2 ];
				texCoords[ 0 ] = dv->st[ 0 ];
				texCoords[ 1 ] = dv->st[ 1 ];
				texCoords[ 2 ] = dv->lightmap[ 0 ];
				texCoords[ 3 ] = dv->lightmap[ 1 ];
				if ( needsNormal ) {
					normal[ 0 ] = dv->normal[ 0 ];
					normal[ 1 ] = dv->normal[ 1 ];
					normal[ 2 ] = dv->normal[ 2 ];
				}
				*( unsigned int* )color = *( unsigned int* )dv->color;
				*vDlightBits++ = dlightBits;
				xyz += 4;
				normal += 4;
				texCoords += 4;
				color += 4;
			}
		}

		// add the indexes
		int h = rows - 1;
		int w = lodWidth - 1;
		int numIndexes = tess.numIndexes;
		for ( int i = 0; i < h; i++ ) {
			for ( int j = 0; j < w; j++ ) {
				// vertex order to be reckognized as tristrips
				int v1 = numVertexes + i * lodWidth + j + 1;
				int v2 = v1 - 1;
				int v3 = v2 + lodWidth;
				int v4 = v3 + 1;

				tess.indexes[ numIndexes ] = v2;
				tess.indexes[ numIndexes + 1 ] = v3;
				tess.indexes[ numIndexes + 2 ] = v1;

				tess.indexes[ numIndexes + 3 ] = v1;
				tess.indexes[ numIndexes + 4 ] = v3;
				tess.indexes[ numIndexes + 5 ] = v4;
				numIndexes += 6;
			}
		}

		tess.numIndexes = numIndexes;

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}

float idSurfaceGrid::LodErrorForVolume( vec3_t local, float radius ) {
	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	vec3_t world;
	world[ 0 ] = local[ 0 ] * backEnd.orient.axis[ 0 ][ 0 ] + local[ 1 ] * backEnd.orient.axis[ 1 ][ 0 ] +
				 local[ 2 ] * backEnd.orient.axis[ 2 ][ 0 ] + backEnd.orient.origin[ 0 ];
	world[ 1 ] = local[ 0 ] * backEnd.orient.axis[ 0 ][ 1 ] + local[ 1 ] * backEnd.orient.axis[ 1 ][ 1 ] +
				 local[ 2 ] * backEnd.orient.axis[ 2 ][ 1 ] + backEnd.orient.origin[ 1 ];
	world[ 2 ] = local[ 0 ] * backEnd.orient.axis[ 0 ][ 2 ] + local[ 1 ] * backEnd.orient.axis[ 1 ][ 2 ] +
				 local[ 2 ] * backEnd.orient.axis[ 2 ][ 2 ] + backEnd.orient.origin[ 2 ];

	VectorSubtract( world, backEnd.viewParms.orient.origin, world );
	float d = DotProduct( world, backEnd.viewParms.orient.axis[ 0 ] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}
