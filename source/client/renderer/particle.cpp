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

#include "local.h"
#include "../../common/common_defs.h"

static byte dottexture[ 16 ][ 16 ] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//1
	{0,0,0,1,0,1,0,0,0,0,0,0,1,1,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,0},
	{0,1,0,1,1,1,0,1,0,0,0,1,1,1,1,0},	//4
	{0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,0},
	{0,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0},	//8
	{0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,0,0,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,0,1,0,0,0,0,1,1,1,1,1,0,1,0},	//12
	{0,1,1,1,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,1,1,1,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//16
};

void R_InitParticleTexture() {
	//
	// particle texture
	//
	byte data[ 16 ][ 16 ][ 4 ];
	for ( int x = 0; x < 16; x++ ) {
		for ( int y = 0; y < 16; y++ ) {
			data[ y ][ x ][ 0 ] = 255;
			data[ y ][ x ][ 1 ] = 255;
			data[ y ][ x ][ 2 ] = 255;
			data[ y ][ x ][ 3 ] = dottexture[ y ][ x ] * 255;
		}
	}
	tr.particleImage = R_CreateImage( "*particle", ( byte* )data, 16, 16, true, false, GL_CLAMP, false );
}

static void R_DrawParticle( const particle_t* p, const vec3_t up, const vec3_t right,
	float s1, float t1, float s2, float t2 ) {
	// hack a scale up to keep particles from disapearing
	float scale = ( p->origin[ 0 ] - tr.viewParms.orient.origin[ 0 ] ) * tr.viewParms.orient.axis[ 0 ][ 0 ] +
				  ( p->origin[ 1 ] - tr.viewParms.orient.origin[ 1 ] ) * tr.viewParms.orient.axis[ 0 ][ 1 ] +
				  ( p->origin[ 2 ] - tr.viewParms.orient.origin[ 2 ] ) * tr.viewParms.orient.axis[ 0 ][ 2 ];

	if ( scale < 20 ) {
		scale = p->size;
	} else   {
		scale = p->size + scale * 0.004;
	}

	qglColor4ubv( p->rgba );

	qglTexCoord2f( s1, t1 );
	tess.xyz[ 0 ][ 0 ] = p->origin[ 0 ];
	tess.xyz[ 0 ][ 1 ] = p->origin[ 1 ];
	tess.xyz[ 0 ][ 2 ] = p->origin[ 2 ];
	R_ArrayElement( 0 );

	qglTexCoord2f( s2, t1 );
	tess.xyz[ 1 ][ 0 ] = p->origin[ 0 ] + up[ 0 ] * scale;
	tess.xyz[ 1 ][ 1 ] = p->origin[ 1 ] + up[ 1 ] * scale;
	tess.xyz[ 1 ][ 2 ] = p->origin[ 2 ] + up[ 2 ] * scale;
	R_ArrayElement( 1 );

	qglTexCoord2f( s1, t2 );
	tess.xyz[ 2 ][ 0 ] = p->origin[ 0 ] + right[ 0 ] * scale;
	tess.xyz[ 2 ][ 1 ] = p->origin[ 1 ] + right[ 1 ] * scale;
	tess.xyz[ 2 ][ 2 ] = p->origin[ 2 ] + right[ 2 ] * scale;
	R_ArrayElement( 2 );
}

static void R_DrawParticleTriangles() {
	GL_Bind( tr.particleImage );
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );		// no z buffering
	qglBegin( GL_TRIANGLES );

	vec3_t up, right;
	VectorScale( tr.viewParms.orient.axis[ 2 ], 1.5, up );
	VectorScale( tr.viewParms.orient.axis[ 1 ], -1.5, right );

	const particle_t* p = tr.refdef.particles;
	for ( int i = 0; i < tr.refdef.num_particles; i++, p++ ) {
		switch ( p->Texture ) {
		case PARTTEX_Default:
			R_DrawParticle( p, up, right, 1 - 0.0625 / 2, 0.0625 / 2, 1 - 1.0625 / 2, 1.0625 / 2 );
			break;

		case PARTTEX_Snow1:
			R_DrawParticle( p, up, right, 1, 1, .18, .18 );
			break;

		case PARTTEX_Snow2:
			R_DrawParticle( p, up, right, 0, 0, .815, .815 );
			break;

		case PARTTEX_Snow3:
			R_DrawParticle( p, up, right, 1, 0, 0.5, 0.5 );
			break;

		case PARTTEX_Snow4:
			R_DrawParticle( p, up, right, 0, 1, 0.5, 0.5 );
			break;
		}
	}

	qglEnd();
	GL_State( GLS_DEPTHMASK_TRUE );			// back to normal Z buffering
	qglColor4f( 1, 1, 1, 1 );
}

static void R_DrawParticlePoints() {
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	qglDisable( GL_TEXTURE_2D );

	qglPointSize( r_particle_size->value );

	qglBegin( GL_POINTS );
	const particle_t* p = tr.refdef.particles;
	for ( int i = 0; i < tr.refdef.num_particles; i++, p++ ) {
		qglColor4ubv( p->rgba );
		qglVertex3fv( p->origin );
	}
	qglEnd();

	qglColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
	GL_State( GLS_DEPTHMASK_TRUE );
	qglEnable( GL_TEXTURE_2D );
}

void R_DrawParticles(void*) {
	if ( !tr.refdef.num_particles ) {
		return;
	}
	if ( ( GGameType & GAME_Quake2 ) && qglPointParameterfEXT ) {
		R_DrawParticlePoints();
	} else   {
		R_DrawParticleTriangles();
	}
}
