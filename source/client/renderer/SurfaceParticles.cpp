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

#include "SurfaceParticles.h"
#include "particle.h"
#include "backend.h"
#include "state.h"
#include "cvars.h"
#include "surfaces.h"
#include "../../common/common_defs.h"

idSurfaceParticles particlesSurface;

idSurfaceParticles::idSurfaceParticles() {
}

void idSurfaceParticles::DrawParticle( const particle_t* p, float s1, float t1, float s2, float t2 ) const {
	// hack a scale up to keep particles from disapearing
	float scale = ( p->origin[ 0 ] - tr.viewParms.orient.origin[ 0 ] ) * tr.viewParms.orient.axis[ 0 ][ 0 ] +
				  ( p->origin[ 1 ] - tr.viewParms.orient.origin[ 1 ] ) * tr.viewParms.orient.axis[ 0 ][ 1 ] +
				  ( p->origin[ 2 ] - tr.viewParms.orient.origin[ 2 ] ) * tr.viewParms.orient.axis[ 0 ][ 2 ];

	if ( scale < 20 ) {
		scale = p->size;
	} else {
		scale = p->size + scale * 0.004;
	}

	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += 3;
	tess.numIndexes += 3;

	tess.vertexColors[ numVerts ][ 0 ] = p->rgba[0];
	tess.vertexColors[ numVerts ][ 1 ] = p->rgba[1];
	tess.vertexColors[ numVerts ][ 2 ] = p->rgba[2];
	tess.vertexColors[ numVerts ][ 3 ] = p->rgba[3];
	tess.texCoords[ numVerts ][ 0 ][ 0 ] = s1;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = t1;
	tess.xyz[ numVerts ][ 0 ] = p->origin[ 0 ];
	tess.xyz[ numVerts ][ 1 ] = p->origin[ 1 ];
	tess.xyz[ numVerts ][ 2 ] = p->origin[ 2 ];
	tess.normal[ numVerts ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts ][ 2 ] = normal[ 2 ];

	tess.vertexColors[ numVerts + 1 ][ 0 ] = p->rgba[0];
	tess.vertexColors[ numVerts + 1 ][ 1 ] = p->rgba[1];
	tess.vertexColors[ numVerts + 1 ][ 2 ] = p->rgba[2];
	tess.vertexColors[ numVerts + 1 ][ 3 ] = p->rgba[3];
	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = s2;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = t1;
	tess.xyz[ numVerts + 1 ][ 0 ] = p->origin[ 0 ] + up[ 0 ] * scale;
	tess.xyz[ numVerts + 1 ][ 1 ] = p->origin[ 1 ] + up[ 1 ] * scale;
	tess.xyz[ numVerts + 1 ][ 2 ] = p->origin[ 2 ] + up[ 2 ] * scale;
	tess.normal[ numVerts + 1 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 1 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 1 ][ 2 ] = normal[ 2 ];

	tess.vertexColors[ numVerts + 2 ][ 0 ] = p->rgba[0];
	tess.vertexColors[ numVerts + 2 ][ 1 ] = p->rgba[1];
	tess.vertexColors[ numVerts + 2 ][ 2 ] = p->rgba[2];
	tess.vertexColors[ numVerts + 2 ][ 3 ] = p->rgba[3];
	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = s1;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = t2;
	tess.xyz[ numVerts + 2 ][ 0 ] = p->origin[ 0 ] + right[ 0 ] * scale;
	tess.xyz[ numVerts + 2 ][ 1 ] = p->origin[ 1 ] + right[ 1 ] * scale;
	tess.xyz[ numVerts + 2 ][ 2 ] = p->origin[ 2 ] + right[ 2 ] * scale;
	tess.normal[ numVerts + 2 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 2 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 2 ][ 2 ] = normal[ 2 ];

	tess.indexes[ numIndexes ] = numVerts;
	tess.indexes[ numIndexes + 1 ] = numVerts + 1;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
}

void idSurfaceParticles::DrawParticleTriangles() {
	VectorScale( backEnd.viewParms.orient.axis[ 2 ], 1.5, up );
	VectorScale( backEnd.viewParms.orient.axis[ 1 ], -1.5, right );
	VectorSubtract( oldvec3_origin, backEnd.viewParms.orient.axis[ 0 ], normal );

	const particle_t* p = backEnd.refdef.particles;
	for ( int i = 0; i < backEnd.refdef.num_particles; i++, p++ ) {
		RB_CHECKOVERFLOW( 3, 3 );

		switch ( p->Texture ) {
		case PARTTEX_Default:
			DrawParticle( p, 1 - 0.0625 / 2, 0.0625 / 2, 1 - 1.0625 / 2, 1.0625 / 2 );
			break;

		case PARTTEX_Snow1:
			DrawParticle( p, 1, 1, .18, .18 );
			break;

		case PARTTEX_Snow2:
			DrawParticle( p, 0, 0, .815, .815 );
			break;

		case PARTTEX_Snow3:
			DrawParticle( p, 1, 0, 0.5, 0.5 );
			break;

		case PARTTEX_Snow4:
			DrawParticle( p, 0, 1, 0.5, 0.5 );
			break;
		}
	}
}

void idSurfaceParticles::DrawParticlePoints() {
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	qglDisable( GL_TEXTURE_2D );

	qglPointSize( r_particle_size->value );

	qglBegin( GL_POINTS );
	const particle_t* p = backEnd.refdef.particles;
	for ( int i = 0; i < backEnd.refdef.num_particles; i++, p++ ) {
		qglColor4ubv( p->rgba );
		qglVertex3fv( p->origin );
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
}

void idSurfaceParticles::Draw() {
	if ( !tr.refdef.num_particles ) {
		return;
	}
	if ( ( GGameType & GAME_Quake2 ) && qglPointParameterfEXT ) {
		DrawParticlePoints();
	} else {
		DrawParticleTriangles();
	}
}
