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

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

// HEADER FILES ------------------------------------------------------------

#include "local.h"

// MACROS ------------------------------------------------------------------

#define MAX_EDGE_DEFS   32

// TYPES -------------------------------------------------------------------

struct edgeDef_t {
	int i2;
	int facing;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static edgeDef_t edgeDefs[ SHADER_MAX_VERTEXES ][ MAX_EDGE_DEFS ];
static int numEdgeDefs[ SHADER_MAX_VERTEXES ];
static int facing[ SHADER_MAX_INDEXES / 3 ];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	RB_ProjectionShadowDeform
//
//==========================================================================

void RB_ProjectionShadowDeform() {
	float* xyz = ( float* )tess.xyz;

	vec3_t ground;
	ground[ 0 ] = backEnd.orient.axis[ 0 ][ 2 ];
	ground[ 1 ] = backEnd.orient.axis[ 1 ][ 2 ];
	ground[ 2 ] = backEnd.orient.axis[ 2 ][ 2 ];

	float groundDist = backEnd.orient.origin[ 2 ] - backEnd.currentEntity->e.shadowPlane;

	vec3_t lightDir;
	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	float d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, ( 0.5 - d ), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	vec3_t light;
	light[ 0 ] = lightDir[ 0 ] * d;
	light[ 1 ] = lightDir[ 1 ] * d;
	light[ 2 ] = lightDir[ 2 ] * d;

	for ( int i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		float h = DotProduct( xyz, ground ) + groundDist;

		xyz[ 0 ] -= light[ 0 ] * h;
		xyz[ 1 ] -= light[ 1 ] * h;
		xyz[ 2 ] -= light[ 2 ] * h;
	}
}

//==========================================================================
//
//	R_AddEdgeDef
//
//==========================================================================

static void R_AddEdgeDef( int i1, int i2, int facing ) {
	int c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}

//==========================================================================
//
//	R_RenderShadowEdges
//
//==========================================================================

static void R_RenderShadowEdges() {
#if 0
	// dumb way -- render every triangle's edges
	int numTris = tess.numIndexes / 3;

	for ( int i = 0; i < numTris; i++ ) {
		if ( !facing[ i ] ) {
			continue;
		}

		int i1 = tess.indexes[ i * 3 + 0 ];
		int i2 = tess.indexes[ i * 3 + 1 ];
		int i3 = tess.indexes[ i * 3 + 2 ];

		qglBegin( GL_TRIANGLE_STRIP );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( tess.xyz[ i1 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i2 ] );
		qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i3 ] );
		qglVertex3fv( tess.xyz[ i3 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( tess.xyz[ i1 + tess.numVertexes ] );
		qglEnd();
	}
#else
	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	int c_edges = 0;
	int c_rejected = 0;

	for ( int i = 0; i < tess.numVertexes; i++ ) {
		int c = numEdgeDefs[ i ];
		for ( int j = 0; j < c; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			int hit[ 2 ];
			hit[ 0 ] = 0;
			hit[ 1 ] = 0;

			int i2 = edgeDefs[ i ][ j ].i2;
			int c2 = numEdgeDefs[ i2 ];
			for ( int k = 0; k < c2; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( hit[ 1 ] == 0 ) {
				qglBegin( GL_TRIANGLE_STRIP );
				qglVertex3fv( tess.xyz[ i ] );
				qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				qglVertex3fv( tess.xyz[ i2 ] );
				qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
				qglEnd();
				c_edges++;
			} else   {
				c_rejected++;
			}
		}
	}
#endif
}

//==========================================================================
//
//	RB_ShadowTessEnd
//
//	triangleFromEdge[ v1 ][ v2 ]
//
//	set triangle from edge( v1, v2, tri )
//	if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
//	}
//
//==========================================================================

void RB_ShadowTessEnd() {
	int i;
	int numTris;
	vec3_t lightDir;

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return;
	}

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

	// project vertexes away from light direction
	for ( i = 0; i < tess.numVertexes; i++ ) {
		VectorMA( tess.xyz[ i ], -512, lightDir, tess.xyz[ i + tess.numVertexes ] );
	}

	// decide which triangles face the light
	Com_Memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0; i < numTris; i++ ) {
		int i1 = tess.indexes[ i * 3 + 0 ];
		int i2 = tess.indexes[ i * 3 + 1 ];
		int i3 = tess.indexes[ i * 3 + 2 ];

		float* v1 = tess.xyz[ i1 ];
		float* v2 = tess.xyz[ i2 ];
		float* v3 = tess.xyz[ i3 ];

		vec3_t d1, d2, normal;
		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		float d = DotProduct( normal, lightDir );
		if ( d > 0 ) {
			facing[ i ] = 1;
		} else   {
			facing[ i ] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	// draw the silhouette edges

	GL_Bind( tr.whiteImage );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	qglColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	GL_Cull( CT_BACK_SIDED );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

	R_RenderShadowEdges();

	GL_Cull( CT_FRONT_SIDED );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

	R_RenderShadowEdges();


	// reenable writing to the color buffer
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}

//==========================================================================
//
//	RB_ShadowFinish
//
//	Darken everything that is is a shadow volume. We have to delay this until
// everything has been shadowed, because otherwise shadows from different body
// parts would overlap and double darken.
//
//==========================================================================

void RB_ShadowFinish() {
	if ( r_shadows->integer != 2 ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 255 );

	qglDisable( GL_CLIP_PLANE0 );
	GL_Cull( CT_TWO_SIDED );

	GL_Bind( tr.whiteImage );

	qglLoadIdentity();

	qglColor3f( 0.6f, 0.6f, 0.6f );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f(1, 0, 0);
//	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	qglBegin( GL_QUADS );
	qglVertex3f( -100, 100, -10 );
	qglVertex3f( 100, 100, -10 );
	qglVertex3f( 100, -100, -10 );
	qglVertex3f( -100, -100, -10 );
	qglEnd();

	qglColor4f( 1, 1, 1, 1 );
	qglDisable( GL_STENCIL_TEST );
}
