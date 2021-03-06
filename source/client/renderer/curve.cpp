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
#include "SurfaceGrid.h"

static idWorldVertex LerpDrawVert( const idWorldVertex& a, const idWorldVertex& b ) {
	idWorldVertex out;
	out.xyz = 0.5f * ( a.xyz + b.xyz );
	out.st = 0.5f * ( a.st + b.st );
	out.lightmap = 0.5f * ( a.lightmap + b.lightmap );

	out.color[ 0 ] = ( a.color[ 0 ] + b.color[ 0 ] ) >> 1;
	out.color[ 1 ] = ( a.color[ 1 ] + b.color[ 1 ] ) >> 1;
	out.color[ 2 ] = ( a.color[ 2 ] + b.color[ 2 ] ) >> 1;
	out.color[ 3 ] = ( a.color[ 3 ] + b.color[ 3 ] ) >> 1;

	return out;
}

static void Transpose( int width, int height, idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	if ( width > height ) {
		for ( int i = 0; i < height; i++ ) {
			for ( int j = i + 1; j < width; j++ ) {
				if ( j < height ) {
					// swap the value
					idWorldVertex temp = ctrl[ j ][ i ];
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
					idWorldVertex temp = ctrl[ i ][ j ];
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
static void MakeMeshNormals( int width, int height, idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	static int neighbors[ 8 ][ 2 ] =
	{
		{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	bool wrapWidth = true;
	for ( int i = 0; i < height; i++ ) {
		float len = ( ctrl[ i ][ 0 ].xyz - ctrl[ i ][ width - 1 ].xyz ).LengthSqr();
		if ( len > 1.0 ) {
			wrapWidth = false;
			break;
		}
	}

	bool wrapHeight = true;
	for ( int i = 0; i < width; i++ ) {
		float len = ( ctrl[ 0 ][ i ].xyz - ctrl[ height - 1 ][ i ].xyz ).LengthSqr();
		if ( len > 1.0 ) {
			wrapHeight = false;
			break;
		}
	}

	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			int count = 0;
			idWorldVertex& dv = ctrl[ j ][ i ];
			idVec3 base = dv.xyz;
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
					idVec3 temp = ctrl[ y ][ x ].xyz - base;
					vec3_t oldtemp;
					temp.ToOldVec3( oldtemp );
					if ( VectorNormalize2( oldtemp, oldtemp ) == 0 ) {
						continue;				// degenerate edge, get more dist
					} else {
						good[ k ] = true;
						VectorCopy( oldtemp, around[ k ] );
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
			vec3_t old;
			VectorNormalize2( sum, old );
			dv.normal.FromOldVec3( old );
		}
	}
}

static void InvertCtrl( int width, int height, idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] ) {
	for ( int i = 0; i < height; i++ ) {
		for ( int j = 0; j < width / 2; j++ ) {
			idWorldVertex temp = ctrl[ i ][ j ];
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

static void PutPointsOnCurve( idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], int width, int height ) {
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 1; j < height; j += 2 ) {
			idWorldVertex prev = LerpDrawVert( ctrl[ j ][ i ], ctrl[ j + 1 ][ i ] );
			idWorldVertex next = LerpDrawVert( ctrl[ j ][ i ], ctrl[ j - 1 ][ i ] );
			ctrl[ j ][ i ] = LerpDrawVert( prev, next );
		}
	}

	for ( int j = 0; j < height; j++ ) {
		for ( int i = 1; i < width; i += 2 ) {
			idWorldVertex prev = LerpDrawVert( ctrl[ j ][ i ], ctrl[ j ][ i + 1 ] );
			idWorldVertex next= LerpDrawVert( ctrl[ j ][ i ], ctrl[ j ][ i - 1 ] );
			ctrl[ j ][ i ] = LerpDrawVert( prev, next );
		}
	}
}

static void R_CreateSurfaceGridMesh( idSurfaceGrid* surf, int width, int height,
	idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], float errorTable[ 2 ][ MAX_GRID_SIZE ] ) {
	// copy the results out to a grid
	surf->lodFixed = 0;
	surf->lodStitched = false;

	surf->widthLodError = new float[ width ];
	Com_Memcpy( surf->widthLodError, errorTable[ 0 ], width * 4 );

	surf->heightLodError = new float[ height ];
	Com_Memcpy( surf->heightLodError, errorTable[ 1 ], height * 4 );

	surf->width = width;
	surf->height = height;
	surf->vertexes = new idWorldVertex[ width * height ];
	surf->bounds.Clear();
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			idWorldVertex& vert = surf->vertexes[ j * width + i ];
			vert = ctrl[ j ][ i ];
			surf->bounds.AddPoint( vert.xyz );
		}
	}

	// compute local origin and bounds
	surf->boundingSphere = surf->bounds.ToSphere();
}

void R_FreeSurfaceGridMesh( idSurfaceGrid* grid ) {
	delete[] grid->widthLodError;
	delete[] grid->heightLodError;
}

void R_FreeSurfaceGridMeshAndVertexes( idSurfaceGrid* grid ) {
	R_FreeSurfaceGridMesh( grid );
	delete[] grid->vertexes;
}

void R_SubdividePatchToGrid( idSurfaceGrid* surf, int width, int height, idWorldVertex points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ] ) {
	idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
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
				idVec3 midxyz = ( ctrl[ i ][ j ].xyz + ctrl[ i ][ j + 1 ].xyz * 2 + ctrl[ i ][ j + 2 ].xyz ) * 0.25f;

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				midxyz = midxyz - ctrl[ i ][ j ].xyz;
				idVec3 dir = ctrl[ i ][ j + 2 ].xyz - ctrl[ i ][ j ].xyz;
				dir.Normalize();

				float d = midxyz * dir;
				idVec3 projected = dir * d;
				idVec3 midxyz2 = midxyz - projected;
				float len = midxyz2.LengthSqr();				// we will do the sqrt later
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
				idWorldVertex prev = LerpDrawVert( ctrl[ i ][ j ], ctrl[ i ][ j + 1 ] );
				idWorldVertex next = LerpDrawVert( ctrl[ i ][ j + 1 ], ctrl[ i ][ j + 2 ] );
				idWorldVertex mid = LerpDrawVert( prev, next );

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

	R_CreateSurfaceGridMesh( surf, width, height, ctrl, errorTable );
}

bool R_GridInsertColumn( idSurfaceGrid* grid, int column, int row, const idVec3& point, float loderror ) {
	int oldwidth = 0;
	int width = grid->width + 1;
	if ( width > MAX_GRID_SIZE ) {
		return false;
	}
	int height = grid->height;

	idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float errorTable[ 2 ][ MAX_GRID_SIZE ];
	for ( int i = 0; i < width; i++ ) {
		if ( i == column ) {
			//insert new column
			for ( int j = 0; j < grid->height; j++ ) {
				ctrl[ j ][ i ] = LerpDrawVert( grid->vertexes[ j * grid->width + i - 1 ], grid->vertexes[ j * grid->width + i ] );
				if ( j == row ) {
					ctrl[ j ][ i ].xyz = point;
				}
			}
			errorTable[ 0 ][ i ] = loderror;
			continue;
		}
		errorTable[ 0 ][ i ] = grid->widthLodError[ oldwidth ];
		for ( int j = 0; j < grid->height; j++ ) {
			ctrl[ j ][ i ] = grid->vertexes[ j * grid->width + oldwidth ];
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

	// free the old grid
	R_FreeSurfaceGridMeshAndVertexes( grid );
	// create a new grid
	R_CreateSurfaceGridMesh( grid, width, height, ctrl, errorTable );
	return true;
}

bool R_GridInsertRow( class idSurfaceGrid* grid, int row, int column, const idVec3& point, float loderror ) {
	int oldheight = 0;
	int width = grid->width;
	int height = grid->height + 1;
	if ( height > MAX_GRID_SIZE ) {
		return false;
	}

	idWorldVertex ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float errorTable[ 2 ][ MAX_GRID_SIZE ];
	for ( int i = 0; i < height; i++ ) {
		if ( i == row ) {
			//insert new row
			for ( int j = 0; j < grid->width; j++ ) {
				ctrl[ i ][ j ] = LerpDrawVert( grid->vertexes[ ( i - 1 ) * grid->width + j ], grid->vertexes[ i * grid->width + j ] );
				if ( j == column ) {
					ctrl[ i ][ j ].xyz = point;
				}
			}
			errorTable[ 1 ][ i ] = loderror;
			continue;
		}
		errorTable[ 1 ][ i ] = grid->heightLodError[ oldheight ];
		for ( int j = 0; j < grid->width; j++ ) {
			ctrl[ i ][ j ] = grid->vertexes[ oldheight * grid->width + j ];
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

	// free the old grid
	R_FreeSurfaceGridMeshAndVertexes( grid );
	// create a new grid
	R_CreateSurfaceGridMesh( grid, width, height, ctrl, errorTable );
	return true;
}
