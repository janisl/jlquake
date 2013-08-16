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
//**
//**	This file does all of the processing necessary to turn a raw grid of
//**  points read from the map file into a srfGridMesh_t ready for rendering.
//**
//**	The level of detail solution is direction independent, based only on
//**  subdivided distance from the true curve.
//**
//**************************************************************************

#include "models/model.h"
#include "cvars.h"

static void LerpDrawVert( mem_drawVert_t* a, mem_drawVert_t* b, mem_drawVert_t* out ) {
	out->xyz[ 0 ] = 0.5f * ( a->xyz[ 0 ] + b->xyz[ 0 ] );
	out->xyz[ 1 ] = 0.5f * ( a->xyz[ 1 ] + b->xyz[ 1 ] );
	out->xyz[ 2 ] = 0.5f * ( a->xyz[ 2 ] + b->xyz[ 2 ] );

	out->st[ 0 ] = 0.5f * ( a->st[ 0 ] + b->st[ 0 ] );
	out->st[ 1 ] = 0.5f * ( a->st[ 1 ] + b->st[ 1 ] );

	out->lightmap[ 0 ] = 0.5f * ( a->lightmap[ 0 ] + b->lightmap[ 0 ] );
	out->lightmap[ 1 ] = 0.5f * ( a->lightmap[ 1 ] + b->lightmap[ 1 ] );

	out->color[ 0 ] = ( a->color[ 0 ] + b->color[ 0 ] ) >> 1;
	out->color[ 1 ] = ( a->color[ 1 ] + b->color[ 1 ] ) >> 1;
	out->color[ 2 ] = ( a->color[ 2 ] + b->color[ 2 ] ) >> 1;
	out->color[ 3 ] = ( a->color[ 3 ] + b->color[ 3 ] ) >> 1;
}

static void Transpose( int width, int height, mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	if ( width > height ) {
		for ( int i = 0; i < height; i++ ) {
			for ( int j = i + 1; j < width; j++ ) {
				if ( j < height ) {
					// swap the value
					mem_drawVert_t temp = ctrl[ j ][ i ];
					ctrl[ j ][ i ] = ctrl[ i ][ j ];
					ctrl[ i ][ j ] = temp;
				} else {
					// just copy
					ctrl[ j ][ i ] = ctrl[ i ][ j ];
				}
			}
		}
	} else {
		for ( int i = 0; i < width; i++ ) {
			for ( int j = i + 1; j < height; j++ ) {
				if ( j < width ) {
					// swap the value
					mem_drawVert_t temp = ctrl[ i ][ j ];
					ctrl[ i ][ j ] = ctrl[ j ][ i ];
					ctrl[ j ][ i ] = temp;
				} else {
					// just copy
					ctrl[ i ][ j ] = ctrl[ j ][ i ];
				}
			}
		}
	}

}

//	Handles all the complicated wrapping and degenerate cases.
static void MakeMeshNormals( int width, int height, mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	static int neighbors[ 8 ][ 2 ] =
	{
		{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	bool wrapWidth = true;
	for ( int i = 0; i < height; i++ ) {
		vec3_t delta;
		VectorSubtract( ctrl[ i ][ 0 ].xyz, ctrl[ i ][ width - 1 ].xyz, delta );
		float len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			wrapWidth = false;
			break;
		}
	}

	bool wrapHeight = true;
	for ( int i = 0; i < width; i++ ) {
		vec3_t delta;
		VectorSubtract( ctrl[ 0 ][ i ].xyz, ctrl[ height - 1 ][ i ].xyz, delta );
		float len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			wrapHeight = false;
			break;
		}
	}

	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			int count = 0;
			mem_drawVert_t* dv = &ctrl[ j ][ i ];
			vec3_t base;
			VectorCopy( dv->xyz, base );
			vec3_t around[ 8 ];
			bool good[ 8 ];
			for ( int k = 0; k < 8; k++ ) {
				VectorClear( around[ k ] );
				good[ k ] = false;

				for ( int dist = 1; dist <= 3; dist++ ) {
					int x = i + neighbors[ k ][ 0 ] * dist;
					int y = j + neighbors[ k ][ 1 ] * dist;
					if ( wrapWidth ) {
						if ( x < 0 ) {
							x = width - 1 + x;
						} else if ( x >= width ) {
							x = 1 + x - width;
						}
					}
					if ( wrapHeight ) {
						if ( y < 0 ) {
							y = height - 1 + y;
						} else if ( y >= height ) {
							y = 1 + y - height;
						}
					}

					if ( x < 0 || x >= width || y < 0 || y >= height ) {
						break;					// edge of patch
					}
					vec3_t temp;
					VectorSubtract( ctrl[ y ][ x ].xyz, base, temp );
					if ( VectorNormalize2( temp, temp ) == 0 ) {
						continue;				// degenerate edge, get more dist
					} else {
						good[ k ] = true;
						VectorCopy( temp, around[ k ] );
						break;					// good edge
					}
				}
			}

			vec3_t sum;
			VectorClear( sum );
			for ( int k = 0; k < 8; k++ ) {
				if ( !good[ k ] || !good[ ( k + 1 ) & 7 ] ) {
					continue;	// didn't get two points
				}
				vec3_t normal;
				CrossProduct( around[ ( k + 1 ) & 7 ], around[ k ], normal );
				if ( VectorNormalize2( normal, normal ) == 0 ) {
					continue;
				}
				VectorAdd( normal, sum, sum );
				count++;
			}
			if ( count == 0 ) {
				count = 1;
			}
			VectorNormalize2( sum, dv->normal );
		}
	}
}

static void InvertCtrl( int width, int height, mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	for ( int i = 0; i < height; i++ ) {
		for ( int j = 0; j < width / 2; j++ ) {
			mem_drawVert_t temp = ctrl[ i ][ j ];
			ctrl[ i ][ j ] = ctrl[ i ][ width - 1 - j ];
			ctrl[ i ][ width - 1 - j ] = temp;
		}
	}
}

static void InvertErrorTable( float errorTable[ 2 ][ MAX_GRID_SIZE ], int width, int height ) {
	float copy[ 2 ][ MAX_GRID_SIZE ];
	Com_Memcpy( copy, errorTable, sizeof ( copy ) );

	for ( int i = 0; i < width; i++ ) {
		errorTable[ 1 ][ i ] = copy[ 0 ][ i ];	//[width-1-i];
	}

	for ( int i = 0; i < height; i++ ) {
		errorTable[ 0 ][ i ] = copy[ 1 ][ height - 1 - i ];
	}
}

static void PutPointsOnCurve( mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], int width, int height ) {
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 1; j < height; j += 2 ) {
			mem_drawVert_t prev, next;
			LerpDrawVert( &ctrl[ j ][ i ], &ctrl[ j + 1 ][ i ], &prev );
			LerpDrawVert( &ctrl[ j ][ i ], &ctrl[ j - 1 ][ i ], &next );
			LerpDrawVert( &prev, &next, &ctrl[ j ][ i ] );
		}
	}

	for ( int j = 0; j < height; j++ ) {
		for ( int i = 1; i < width; i += 2 ) {
			mem_drawVert_t prev, next;
			LerpDrawVert( &ctrl[ j ][ i ], &ctrl[ j ][ i + 1 ], &prev );
			LerpDrawVert( &ctrl[ j ][ i ], &ctrl[ j ][ i - 1 ], &next );
			LerpDrawVert( &prev, &next, &ctrl[ j ][ i ] );
		}
	}
}

static srfGridMesh_t* R_CreateSurfaceGridMesh( int width, int height,
	mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], float errorTable[ 2 ][ MAX_GRID_SIZE ] ) {
	// copy the results out to a grid
	int size = sizeof ( srfGridMesh_t ) + ( width * height - 1 ) * sizeof ( mem_drawVert_t );

	srfGridMesh_t* grid = ( srfGridMesh_t* )Mem_Alloc( size );
	Com_Memset( grid, 0, size );

	grid->widthLodError = new float[ width ];
	Com_Memcpy( grid->widthLodError, errorTable[ 0 ], width * 4 );

	grid->heightLodError = new float[ height ];
	Com_Memcpy( grid->heightLodError, errorTable[ 1 ], height * 4 );

	grid->width = width;
	grid->height = height;
	grid->surfaceType = SF_GRID;
	ClearBounds( grid->bounds[ 0 ], grid->bounds[ 1 ] );
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			mem_drawVert_t* vert = &grid->verts[ j * width + i ];
			*vert = ctrl[ j ][ i ];
			AddPointToBounds( vert->xyz, grid->bounds[ 0 ], grid->bounds[ 1 ] );
		}
	}

	// compute local origin and bounds
	VectorAdd( grid->bounds[ 0 ], grid->bounds[ 1 ], grid->localOrigin );
	VectorScale( grid->localOrigin, 0.5f, grid->localOrigin );
	vec3_t tmpVec;
	VectorSubtract( grid->bounds[ 0 ], grid->localOrigin, tmpVec );
	grid->radius = VectorLength( tmpVec );

	VectorCopy( grid->localOrigin, grid->lodOrigin );
	grid->lodRadius = grid->radius;

	return grid;
}

void R_FreeSurfaceGridMesh( srfGridMesh_t* grid ) {
	delete[] grid->widthLodError;
	delete[] grid->heightLodError;
	Mem_Free( grid );
}

srfGridMesh_t* R_SubdividePatchToGrid( int width, int height,
	mem_drawVert_t points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ] ) {
	mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			ctrl[ j ][ i ] = points[ j * width + i ];
		}
	}

	float errorTable[ 2 ][ MAX_GRID_SIZE ];
	for ( int dir = 0; dir < 2; dir++ ) {
		for ( int j = 0; j < MAX_GRID_SIZE; j++ ) {
			errorTable[ dir ][ j ] = 0;
		}

		// horizontal subdivisions
		for ( int j = 0; j + 2 < width; j += 2 ) {
			// check subdivided midpoints against control points

			// FIXME: also check midpoints of adjacent patches against the control points
			// this would basically stitch all patches in the same LOD group together.

			float maxLen = 0;
			for ( int i = 0; i < height; i++ ) {
				// calculate the point on the curve
				vec3_t midxyz;
				for ( int l = 0; l < 3; l++ ) {
					midxyz[ l ] = ( ctrl[ i ][ j ].xyz[ l ] + ctrl[ i ][ j + 1 ].xyz[ l ] * 2 + ctrl[ i ][ j + 2 ].xyz[ l ] ) * 0.25f;
				}

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				vec3_t dir;
				VectorSubtract( midxyz, ctrl[ i ][ j ].xyz, midxyz );
				VectorSubtract( ctrl[ i ][ j + 2 ].xyz, ctrl[ i ][ j ].xyz, dir );
				VectorNormalize( dir );

				float d = DotProduct( midxyz, dir );
				vec3_t projected;
				VectorScale( dir, d, projected );
				vec3_t midxyz2;
				VectorSubtract( midxyz, projected, midxyz2 );
				float len = VectorLengthSquared( midxyz2 );				// we will do the sqrt later
				if ( len > maxLen ) {
					maxLen = len;
				}
			}

			maxLen = sqrt( maxLen );

			// if all the points are on the lines, remove the entire columns
			if ( maxLen < 0.1f ) {
				errorTable[ dir ][ j + 1 ] = 999;
				continue;
			}

			// see if we want to insert subdivided columns
			if ( width + 2 > MAX_GRID_SIZE ) {
				errorTable[ dir ][ j + 1 ] = 1.0f / maxLen;
				continue;	// can't subdivide any more
			}

			if ( maxLen <= r_subdivisions->value ) {
				errorTable[ dir ][ j + 1 ] = 1.0f / maxLen;
				continue;	// didn't need subdivision
			}

			errorTable[ dir ][ j + 2 ] = 1.0f / maxLen;

			// insert two columns and replace the peak
			width += 2;
			for ( int i = 0; i < height; i++ ) {
				mem_drawVert_t prev, next, mid;
				LerpDrawVert( &ctrl[ i ][ j ], &ctrl[ i ][ j + 1 ], &prev );
				LerpDrawVert( &ctrl[ i ][ j + 1 ], &ctrl[ i ][ j + 2 ], &next );
				LerpDrawVert( &prev, &next, &mid );

				for ( int k = width - 1; k > j + 3; k-- ) {
					ctrl[ i ][ k ] = ctrl[ i ][ k - 2 ];
				}
				ctrl[ i ][ j + 1 ] = prev;
				ctrl[ i ][ j + 2 ] = mid;
				ctrl[ i ][ j + 3 ] = next;
			}

			// back up and recheck this set again, it may need more subdivision
			j -= 2;
		}

		Transpose( width, height, ctrl );
		int t = width;
		width = height;
		height = t;
	}


	// put all the aproximating points on the curve
	PutPointsOnCurve( ctrl, width, height );

	// cull out any rows or columns that are colinear
	for ( int i = 1; i < width - 1; i++ ) {
		if ( errorTable[ 0 ][ i ] != 999 ) {
			continue;
		}
		for ( int j = i + 1; j < width; j++ ) {
			for ( int k = 0; k < height; k++ ) {
				ctrl[ k ][ j - 1 ] = ctrl[ k ][ j ];
			}
			errorTable[ 0 ][ j - 1 ] = errorTable[ 0 ][ j ];
		}
		width--;
	}

	for ( int i = 1; i < height - 1; i++ ) {
		if ( errorTable[ 1 ][ i ] != 999 ) {
			continue;
		}
		for ( int j = i + 1; j < height; j++ ) {
			for ( int k = 0; k < width; k++ ) {
				ctrl[ j - 1 ][ k ] = ctrl[ j ][ k ];
			}
			errorTable[ 1 ][ j - 1 ] = errorTable[ 1 ][ j ];
		}
		height--;
	}

#if 1
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width ) {
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		int t = width;
		width = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}
#endif

	// calculate normals
	MakeMeshNormals( width, height, ctrl );

	return R_CreateSurfaceGridMesh( width, height, ctrl, errorTable );
}

srfGridMesh_t* R_GridInsertColumn( srfGridMesh_t* grid, int column, int row, vec3_t point, float loderror ) {
	int oldwidth = 0;
	int width = grid->width + 1;
	if ( width > MAX_GRID_SIZE ) {
		return NULL;
	}
	int height = grid->height;

	mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float errorTable[ 2 ][ MAX_GRID_SIZE ];
	for ( int i = 0; i < width; i++ ) {
		if ( i == column ) {
			//insert new column
			for ( int j = 0; j < grid->height; j++ ) {
				LerpDrawVert( &grid->verts[ j * grid->width + i - 1 ], &grid->verts[ j * grid->width + i ], &ctrl[ j ][ i ] );
				if ( j == row ) {
					VectorCopy( point, ctrl[ j ][ i ].xyz );
				}
			}
			errorTable[ 0 ][ i ] = loderror;
			continue;
		}
		errorTable[ 0 ][ i ] = grid->widthLodError[ oldwidth ];
		for ( int j = 0; j < grid->height; j++ ) {
			ctrl[ j ][ i ] = grid->verts[ j * grid->width + oldwidth ];
		}
		oldwidth++;
	}
	for ( int j = 0; j < grid->height; j++ ) {
		errorTable[ 1 ][ j ] = grid->heightLodError[ j ];
	}
	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );
	// calculate normals
	MakeMeshNormals( width, height, ctrl );

	vec3_t lodOrigin;
	VectorCopy( grid->lodOrigin, lodOrigin );
	float lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh( grid );
	// create a new grid
	grid = R_CreateSurfaceGridMesh( width, height, ctrl, errorTable );
	grid->lodRadius = lodRadius;
	VectorCopy( lodOrigin, grid->lodOrigin );
	return grid;
}

srfGridMesh_t* R_GridInsertRow( srfGridMesh_t* grid, int row, int column, vec3_t point, float loderror ) {
	int oldheight = 0;
	int width = grid->width;
	int height = grid->height + 1;
	if ( height > MAX_GRID_SIZE ) {
		return NULL;
	}

	mem_drawVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float errorTable[ 2 ][ MAX_GRID_SIZE ];
	for ( int i = 0; i < height; i++ ) {
		if ( i == row ) {
			//insert new row
			for ( int j = 0; j < grid->width; j++ ) {
				LerpDrawVert( &grid->verts[ ( i - 1 ) * grid->width + j ], &grid->verts[ i * grid->width + j ], &ctrl[ i ][ j ] );
				if ( j == column ) {
					VectorCopy( point, ctrl[ i ][ j ].xyz );
				}
			}
			errorTable[ 1 ][ i ] = loderror;
			continue;
		}
		errorTable[ 1 ][ i ] = grid->heightLodError[ oldheight ];
		for ( int j = 0; j < grid->width; j++ ) {
			ctrl[ i ][ j ] = grid->verts[ oldheight * grid->width + j ];
		}
		oldheight++;
	}
	for ( int j = 0; j < grid->width; j++ ) {
		errorTable[ 0 ][ j ] = grid->widthLodError[ j ];
	}
	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );
	// calculate normals
	MakeMeshNormals( width, height, ctrl );

	vec3_t lodOrigin;
	VectorCopy( grid->lodOrigin, lodOrigin );
	float lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh( grid );
	// create a new grid
	grid = R_CreateSurfaceGridMesh( width, height, ctrl, errorTable );
	grid->lodRadius = lodRadius;
	VectorCopy( lodOrigin, grid->lodOrigin );
	return grid;
}
