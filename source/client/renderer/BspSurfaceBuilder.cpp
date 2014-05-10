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

#include "BspSurfaceBuilder.h"
#include "../../common/Common.h"

//	Breaks a polygon up along axial 64 unit boundaries so that turbulent and
// sky warps can be done reasonably.
void idBspSurfaceBuilder::Subdivide( idSurfaceFaceQ1Q2* fa ) {
	polys = NULL;

	//
	// convert edges back to a normal polygon
	//
	idList<int> indexes;
	for ( int i = 0; i < fa->numVertexes; i++ ) {
		verts.Append( fa->vertexes[ i ].xyz );
		indexes.Append( i );
	}

	SubdividePolygon( indexes );

	delete[] fa->vertexes;
	fa->numVertexes = verts.Num();
	fa->vertexes = new idWorldVertex[ verts.Num() ];
	const idVec3* v = verts.Ptr();
	for ( int i = 0; i < verts.Num(); i++, v++ ) {
		fa->vertexes[ i ].xyz = *v;
		fa->vertexes[ i ].normal = fa->plane.Normal();
		fa->vertexes[ i ].st.x = DotProduct( *v, fa->textureInfo->vecs[ 0 ] ) / 64.0f;
		fa->vertexes[ i ].st.y = DotProduct( *v, fa->textureInfo->vecs[ 1 ] ) / 64.0f;
	}
	for ( glpoly_t* p = polys; p; p = p->next ) {
		fa->numIndexes += ( p->numverts - 2 ) * 3;
	}
	fa->indexes = new int[ fa->numIndexes ];
	int numIndexes = 0;
	for ( glpoly_t* p = polys; p; p = p->next ) {
		for ( int i = 0; i < p->numverts - 2; i++ ) {
			fa->indexes[ numIndexes + i * 3 + 0 ] = p->indexes[ 0 ];
			fa->indexes[ numIndexes + i * 3 + 1 ] = p->indexes[ i + 1 ];
			fa->indexes[ numIndexes + i * 3 + 2 ] = p->indexes[ i + 2 ];
		}
		numIndexes += ( p->numverts - 2 ) * 3;
	}

	glpoly_t* poly = polys;
	while ( poly ) {
		glpoly_t* tmp = poly;
		poly = poly->next;
		Mem_Free( tmp );
	}
	verts.Clear();
}

void idBspSurfaceBuilder::SubdividePolygon( const idList<int>& vertIndexes ) {
	idBounds bounds = BoundPoly( vertIndexes );

	for ( int i = 0; i < 3; i++ ) {
		float m = ( bounds[ 0 ][ i ] + bounds[ 1 ][ i ] ) * 0.5;
		m = SUBDIVIDE_SIZE * floor( m / SUBDIVIDE_SIZE + 0.5 );
		if ( bounds[ 1 ][ i ] - m < 8 ) {
			continue;
		}
		if ( m - bounds[ 0 ][ i ] < 8 ) {
			continue;
		}

		// cut it
		idList<float> dist;
		for ( int j = 0; j < vertIndexes.Num(); j++ ) {
			dist.Append( verts[ vertIndexes[ j ] ][ i ] - m );
		}

		// wrap cases
		dist.Append( dist[ 0 ] );

		idList<int> front, back;
		for ( int j = 0; j < vertIndexes.Num(); j++ ) {
			int index1 = vertIndexes[ j ];
			float dist1 = dist[ j ];
			if ( dist1 >= 0 ) {
				front.Append( index1 );
			}
			if ( dist1 <= 0 ) {
				back.Append( index1 );
			}
			float dist2 = dist[ j + 1 ];
			if ( dist1 == 0 || dist2 == 0 ) {
				continue;
			}
			if ( ( dist1 > 0 ) != ( dist2 > 0 ) ) {
				// clip point
				float frac = dist1 / ( dist1 - dist2 );
				int index2 = vertIndexes[ ( j + 1 ) % vertIndexes.Num() ];
				verts.Append( verts[ index1 ] + frac * ( verts[ index2 ] - verts[ index1 ] ) );
				front.Append( verts.Num() - 1 );
				back.Append( verts.Num() - 1 );
			}
		}

		SubdividePolygon( front );
		SubdividePolygon( back );
		return;
	}

	EmitPolygon( vertIndexes );
}

void idBspSurfaceBuilder::EmitPolygon( const idList<int>& vertIndexes ) {
	// add a point in the center to help keep warp valid
	glpoly_t* poly = ( glpoly_t* )Mem_Alloc( sizeof ( glpoly_t ) + ( ( vertIndexes.Num() - 4 ) + 2 ) * sizeof( int ) );
	poly->next = polys;
	polys = poly;
	poly->numverts = vertIndexes.Num() + 2;
	idVec3 total;
	total.Zero();
	for ( int i = 0; i < vertIndexes.Num(); i++ ) {
		poly->indexes[ i + 1 ] = vertIndexes[ i ];
		total += verts[ vertIndexes[ i ] ];
	}

	verts.Append( total / vertIndexes.Num() );

	poly->indexes[ 0 ] = verts.Num() - 1;
	poly->indexes[ vertIndexes.Num() + 1 ] = poly->indexes[ 1 ];
}

idBounds idBspSurfaceBuilder::BoundPoly( const idList<int>& vertIndexes ) {
	idBounds bounds;
	bounds.Clear();
	for ( int i = 0; i < vertIndexes.Num(); i++ ) {
		bounds.AddPoint( verts[ vertIndexes[ i ] ] );
	}
	return bounds;
}
