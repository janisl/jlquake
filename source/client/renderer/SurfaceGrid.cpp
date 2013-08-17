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
#include "decals.h"
#include "marks.h"

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
				const idWorldVertex& dv = vertexes[ heightTable[ used + i ] * cv->width + widthTable[ j ] ];
				mem_drawVert_t* olddv = cv->verts + heightTable[ used + i ] * cv->width + widthTable[ j ];

				dv.xyz.ToOldVec3( xyz );
				texCoords[ 0 ] = olddv->st[ 0 ];
				texCoords[ 1 ] = olddv->st[ 1 ];
				texCoords[ 2 ] = olddv->lightmap[ 0 ];
				texCoords[ 3 ] = olddv->lightmap[ 1 ];
				if ( needsNormal ) {
					dv.normal.ToOldVec3( normal );
				}
				*( unsigned int* )color = *( unsigned int* ) olddv->color;
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

void idSurfaceGrid::ProjectDecal( decalProjector_t* dp, mbrush46_model_t* bmodel ) const {
	if ( !ClipDecal( dp ) ) {
		return;
	}

	//	get surface
	srfGridMesh_t* srf = ( srfGridMesh_t* )GetBrush46Data();

	//	walk mesh rows
	for ( int y = 0; y < ( srf->height - 1 ); y++ ) {
		//	walk mesh cols
		for ( int x = 0; x < ( srf->width - 1 ); x++ ) {
			//	get vertex
			const idWorldVertex* dv = vertexes + y * srf->width + x;

			vec3_t points[ 2 ][ MAX_DECAL_VERTS ];
			//	first triangle
			dv[ 0 ].xyz.ToOldVec3( points[ 0 ][ 0 ] );
			dv[ srf->width ].xyz.ToOldVec3( points[ 0 ][ 1 ] );
			dv[ 1 ].xyz.ToOldVec3( points[ 0 ][ 2 ] );
			ProjectDecalOntoWinding( dp, 3, points, this, bmodel );

			//	second triangle
			dv[ 1 ].xyz.ToOldVec3( points[ 0 ][ 0 ] );
			dv[ srf->width ].xyz.ToOldVec3( points[ 0 ][ 1 ] );
			dv[ srf->width + 1 ].xyz.ToOldVec3( points[ 0 ][ 2 ] );
			ProjectDecalOntoWinding( dp, 3, points, this, bmodel );
		}
	}
}

bool idSurfaceGrid::CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const {
	// check if the surface has NOIMPACT or NOMARKS set
	return R_ShaderCanHaveMarks( shader );
}

void idSurfaceGrid::MarkFragments( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) const {
	srfGridMesh_t* cv = ( srfGridMesh_t* )GetBrush46Data();
	for ( int m = 0; m < cv->height - 1; m++ ) {
		for ( int n = 0; n < cv->width - 1; n++ ) {
			// We triangulate the grid and chop all triangles within
			// the bounding planes of the to be projected polygon.
			// LOD is not taken into account, not such a big deal though.
			//
			// It's probably much nicer to chop the grid itself and deal
			// with this grid as a normal SF_GRID surface so LOD will
			// be applied. However the LOD of that chopped grid must
			// be synced with the LOD of the original curve.
			// One way to do this; the chopped grid shares vertices with
			// the original curve. When LOD is applied to the original
			// curve the unused vertices are flagged. Now the chopped curve
			// should skip the flagged vertices. This still leaves the
			// problems with the vertices at the chopped grid edges.
			//
			// To avoid issues when LOD applied to "hollow curves" (like
			// the ones around many jump pads) we now just add a 2 unit
			// offset to the triangle vertices.
			// The offset is added in the vertex normal vector direction
			// so all triangles will still fit together.
			// The 2 unit offset should avoid pretty much all LOD problems.

			int numClipPoints = 3;

			idWorldVertex* dv = vertexes + m * cv->width + n;

			vec3_t clipPoints[ 2 ][ MAX_VERTS_ON_POLY ];
			( dv[ 0 ].xyz + dv[ 0 ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 0 ] );
			( dv[ cv->width ].xyz + dv[ cv->width ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 1 ] );
			( dv[ 1 ].xyz + dv[ 1 ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 2 ] );
			// check the normal of this triangle
			vec3_t v1, v2;
			VectorSubtract( clipPoints[ 0 ][ 0 ], clipPoints[ 0 ][ 1 ], v1 );
			VectorSubtract( clipPoints[ 0 ][ 2 ], clipPoints[ 0 ][ 1 ], v2 );
			vec3_t normal;
			CrossProduct( v1, v2, normal );
			VectorNormalizeFast( normal );
			if ( DotProduct( normal, projectionDir ) < -0.1 ) {
				// add the fragments of this triangle
				R_AddMarkFragments( numClipPoints, clipPoints,
					numPlanes, normals, dists,
					maxPoints, pointBuffer,
					maxFragments, fragmentBuffer,
					returnedPoints, returnedFragments, mins, maxs );

				if ( *returnedFragments == maxFragments ) {
					return;	// not enough space for more fragments
				}
			}

			( dv[ 1 ].xyz + dv[ 1 ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 0 ] );
			( dv[ cv->width ].xyz + dv[ cv->width ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 1 ] );
			( dv[ cv->width + 1 ].xyz + dv[ cv->width + 1 ].normal * MARKER_OFFSET ).ToOldVec3( clipPoints[ 0 ][ 2 ] );
			// check the normal of this triangle
			VectorSubtract( clipPoints[ 0 ][ 0 ], clipPoints[ 0 ][ 1 ], v1 );
			VectorSubtract( clipPoints[ 0 ][ 2 ], clipPoints[ 0 ][ 1 ], v2 );
			CrossProduct( v1, v2, normal );
			VectorNormalizeFast( normal );
			if ( DotProduct( normal, projectionDir ) < -0.05 ) {
				// add the fragments of this triangle
				R_AddMarkFragments( numClipPoints, clipPoints,
					numPlanes, normals, dists,
					maxPoints, pointBuffer,
					maxFragments, fragmentBuffer,
					returnedPoints, returnedFragments, mins, maxs );

				if ( *returnedFragments == maxFragments ) {
					return;	// not enough space for more fragments
				}
			}
		}
	}
}

void idSurfaceGrid::MarkFragmentsWolf( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const {
	MarkFragments( projectionDir, numPlanes, normals, dists, maxPoints, pointBuffer,
		maxFragments, fragmentBuffer, returnedPoints, returnedFragments, mins, maxs );
}

//	Returns true if the grid is completely culled away.
//	Also sets the clipped hint bit in tess
bool idSurfaceGrid::DoCull( shader_t* shader ) const {
	if ( r_nocurves->integer ) {
		return true;
	}

	srfGridMesh_t* cv = ( srfGridMesh_t* )GetBrush46Data();
	int sphereCull;
	if ( tr.currentEntityNum != REF_ENTITYNUM_WORLD ) {
		sphereCull = R_CullLocalPointAndRadius( cv->localOrigin, cv->radius );
	} else {
		sphereCull = R_CullPointAndRadius( cv->localOrigin, cv->radius );
	}

	// check for trivial reject
	if ( sphereCull == CULL_OUT ) {
		tr.pc.c_sphere_cull_patch_out++;
		return true;
	}
	// check bounding box if necessary
	else if ( sphereCull == CULL_CLIP ) {
		tr.pc.c_sphere_cull_patch_clip++;

		int boxCull = R_CullLocalBox( cv->bounds );

		if ( boxCull == CULL_OUT ) {
			tr.pc.c_box_cull_patch_out++;
			return true;
		} else if ( boxCull == CULL_IN ) {
			tr.pc.c_box_cull_patch_in++;
		} else {
			tr.pc.c_box_cull_patch_clip++;
		}
	} else {
		tr.pc.c_sphere_cull_patch_in++;
	}

	return false;
}

bool idSurfaceGrid::DoCullET( shader_t* shader, int* frontFace ) const {
	if ( r_nocurves->integer ) {
		return true;
	}
	return idWorldSurface::DoCullET( shader, frontFace );
}

int idSurfaceGrid::DoMarkDynamicLights( int dlightBits ) {
	srfGridMesh_t* grid = ( srfGridMesh_t* )GetBrush46Data();
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[ i ];
		if ( dl->origin[ 0 ] - dl->radius > grid->bounds[ 1 ][ 0 ] ||
			 dl->origin[ 0 ] + dl->radius <grid->bounds[ 0 ][ 0 ] ||
										   dl->origin[ 1 ] - dl->radius> grid->bounds[ 1 ][ 1 ] ||
			 dl->origin[ 1 ] + dl->radius <grid->bounds[ 0 ][ 1 ] ||
										   dl->origin[ 2 ] - dl->radius> grid->bounds[ 1 ][ 2 ] ||
			 dl->origin[ 2 ] + dl->radius < grid->bounds[ 0 ][ 2 ] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
