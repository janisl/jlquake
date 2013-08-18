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

#include "SurfaceEntity.h"
#include "surfaces.h"
#include "backend.h"
#include "cvars.h"
#include "state.h"
#include "../../common/common_defs.h"

idSurfaceEntity entitySurface;

idSurfaceEntity::idSurfaceEntity() {
}

void idSurfaceEntity::RB_SurfaceSprite() {
	// calculate the xyz locations for the four corners
	float radius = backEnd.currentEntity->e.radius;
	vec3_t left, up;
	if ( backEnd.currentEntity->e.rotation == 0 ) {
		VectorScale( backEnd.viewParms.orient.axis[ 1 ], radius, left );
		VectorScale( backEnd.viewParms.orient.axis[ 2 ], radius, up );
	} else {
		float ang = DEG2RAD( backEnd.currentEntity->e.rotation );
		float s = sin( ang );
		float c = cos( ang );

		VectorScale( backEnd.viewParms.orient.axis[ 1 ], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.orient.axis[ 2 ], left );

		VectorScale( backEnd.viewParms.orient.axis[ 2 ], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.orient.axis[ 1 ], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( oldvec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

void idSurfaceEntity::RB_SurfaceBeam() {
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t oldorigin;
	oldorigin[ 0 ] = e->oldorigin[ 0 ];
	oldorigin[ 1 ] = e->oldorigin[ 1 ];
	oldorigin[ 2 ] = e->oldorigin[ 2 ];

	vec3_t origin;
	origin[ 0 ] = e->origin[ 0 ];
	origin[ 1 ] = e->origin[ 1 ];
	origin[ 2 ] = e->origin[ 2 ];

	vec3_t direction, normalized_direction;
	normalized_direction[ 0 ] = direction[ 0 ] = oldorigin[ 0 ] - origin[ 0 ];
	normalized_direction[ 1 ] = direction[ 1 ] = oldorigin[ 1 ] - origin[ 1 ];
	normalized_direction[ 2 ] = direction[ 2 ] = oldorigin[ 2 ] - origin[ 2 ];

	if ( VectorNormalize( normalized_direction ) == 0 ) {
		return;
	}

	vec3_t perpvec;
	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	enum { NUM_BEAM_SEGS = 6 };
	vec3_t start_points[ NUM_BEAM_SEGS ], end_points[ NUM_BEAM_SEGS ];
	for ( int i = 0; i < NUM_BEAM_SEGS; i++ ) {
		RotatePointAroundVector( start_points[ i ], normalized_direction, perpvec, ( 360.0 / NUM_BEAM_SEGS ) * i );
		VectorAdd( start_points[ i ], direction, end_points[ i ] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	qglColor3f( 1, 0, 0 );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( int i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		qglVertex3fv( start_points[ i % NUM_BEAM_SEGS ] );
		qglVertex3fv( end_points[ i % NUM_BEAM_SEGS ] );
	}
	qglEnd();
}

void idSurfaceEntity::RB_SurfaceBeamQ2() {
	refEntity_t* e = &backEnd.currentEntity->e;

	enum { NUM_BEAM_SEGS = 6 };

	vec3_t oldorigin;
	oldorigin[ 0 ] = e->oldorigin[ 0 ];
	oldorigin[ 1 ] = e->oldorigin[ 1 ];
	oldorigin[ 2 ] = e->oldorigin[ 2 ];

	vec3_t origin;
	origin[ 0 ] = e->origin[ 0 ];
	origin[ 1 ] = e->origin[ 1 ];
	origin[ 2 ] = e->origin[ 2 ];

	vec3_t direction, normalized_direction;
	normalized_direction[ 0 ] = direction[ 0 ] = oldorigin[ 0 ] - origin[ 0 ];
	normalized_direction[ 1 ] = direction[ 1 ] = oldorigin[ 1 ] - origin[ 1 ];
	normalized_direction[ 2 ] = direction[ 2 ] = oldorigin[ 2 ] - origin[ 2 ];

	if ( VectorNormalize( normalized_direction ) == 0 ) {
		return;
	}

	vec3_t perpvec;
	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	vec3_t start_points[ NUM_BEAM_SEGS ], end_points[ NUM_BEAM_SEGS ];
	for ( int i = 0; i < 6; i++ ) {
		RotatePointAroundVector( start_points[ i ], normalized_direction, perpvec, ( 360.0 / NUM_BEAM_SEGS ) * i );
		VectorAdd( start_points[ i ], origin, start_points[ i ] );
		VectorAdd( start_points[ i ], direction, end_points[ i ] );
	}

	qglDisable( GL_TEXTURE_2D );
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	float r = ( d_8to24table[ e->skinNum & 0xFF ] ) & 0xFF;
	float g = ( d_8to24table[ e->skinNum & 0xFF ] >> 8 ) & 0xFF;
	float b = ( d_8to24table[ e->skinNum & 0xFF ] >> 16 ) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	qglColor4f( r, g, b, e->shaderRGBA[ 3 ] / 255.0 );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( int i = 0; i < NUM_BEAM_SEGS; i++ ) {
		qglVertex3fv( start_points[ i ] );
		qglVertex3fv( end_points[ i ] );
		qglVertex3fv( start_points[ ( i + 1 ) % NUM_BEAM_SEGS ] );
		qglVertex3fv( end_points[ ( i + 1 ) % NUM_BEAM_SEGS ] );
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
}

void idSurfaceEntity::DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth ) {
	// Gordon: configurable tile
	float t = len / ( GGameType & GAME_ET && backEnd.currentEntity->e.radius > 0 ? backEnd.currentEntity->e.radius : 256.f );

	int vbase = tess.numVertexes;

	float spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 0;
	if ( GGameType & GAME_ET ) {
		tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
		tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
		tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	} else {
		tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ] * 0.25;
		tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ] * 0.25;
		tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ] * 0.25;
	}
	tess.vertexColors[ tess.numVertexes ][ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 1;
	tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ][ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[ tess.numVertexes ] );

	tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = t;
	tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 0;
	tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ][ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = t;
	tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 1;
	tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ][ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ] = vbase;
	tess.indexes[ tess.numIndexes++ ] = vbase + 1;
	tess.indexes[ tess.numIndexes++ ] = vbase + 2;

	tess.indexes[ tess.numIndexes++ ] = vbase + 2;
	tess.indexes[ tess.numIndexes++ ] = vbase + 1;
	tess.indexes[ tess.numIndexes++ ] = vbase + 3;
}

void idSurfaceEntity::RB_SurfaceRailCore() {
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	vec3_t vec;
	VectorSubtract( end, start, vec );
	int len = VectorNormalize( vec );

	// compute side vector
	vec3_t v1;
	VectorSubtract( start, backEnd.viewParms.orient.origin, v1 );
	VectorNormalize( v1 );
	vec3_t v2;
	VectorSubtract( end, backEnd.viewParms.orient.origin, v2 );
	VectorNormalize( v2 );
	vec3_t right;
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	if ( GGameType & GAME_ET ) {
		DoRailCore( start, end, right, len, e->frame > 0 ? e->frame : 1 );
	} else {
		DoRailCore( start, end, right, len, r_railCoreWidth->integer );
	}
}

void idSurfaceEntity::DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up ) {
	if ( numSegs > 1 ) {
		numSegs--;
	}
	if ( !numSegs ) {
		return;
	}

	float scale = 0.25;

	int spanWidth = r_railWidth->integer;
	vec3_t pos[ 4 ];
	for ( int i = 0; i < 4; i++ ) {
		float c = cos( DEG2RAD( 45 + i * 90 ) );
		float s = sin( DEG2RAD( 45 + i * 90 ) );
		vec3_t v;
		v[ 0 ] = ( right[ 0 ] * c + up[ 0 ] * s ) * scale * spanWidth;
		v[ 1 ] = ( right[ 1 ] * c + up[ 1 ] * s ) * scale * spanWidth;
		v[ 2 ] = ( right[ 2 ] * c + up[ 2 ] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[ i ] );

		if ( numSegs > 1 ) {
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[ i ], dir, pos[ i ] );
		}
	}

	for ( int i = 0; i < numSegs; i++ ) {
		RB_CHECKOVERFLOW( 4, 6 );

		for ( int j = 0; j < 4; j++ ) {
			VectorCopy( pos[ j ], tess.xyz[ tess.numVertexes ] );
			tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = ( j < 2 );
			tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = ( j && j != 3 );
			tess.vertexColors[ tess.numVertexes ][ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
			tess.vertexColors[ tess.numVertexes ][ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
			tess.vertexColors[ tess.numVertexes ][ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
			tess.numVertexes++;

			VectorAdd( pos[ j ], dir, pos[ j ] );
		}

		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 0;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 1;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 3;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 3;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 1;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 2;
	}
}

void idSurfaceEntity::RB_SurfaceRailRings() {
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	// compute variables
	vec3_t vec;
	VectorSubtract( end, start, vec );
	int len = VectorNormalize( vec );
	vec3_t right, up;
	MakeNormalVectors( vec, right, up );
	int numSegs = len / r_railSegmentLength->value;
	if ( numSegs <= 0 ) {
		numSegs = 1;
	}

	VectorScale( vec, r_railSegmentLength->value, vec );

	DoRailDiscs( numSegs, start, vec, right, up );
}

void idSurfaceEntity::RB_SurfaceLightningBolt() {
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy( e->oldorigin, end );
	VectorCopy( e->origin, start );

	// compute variables
	vec3_t vec;
	VectorSubtract( end, start, vec );
	int len = VectorNormalize( vec );

	// compute side vector
	vec3_t v1;
	VectorSubtract( start, backEnd.viewParms.orient.origin, v1 );
	VectorNormalize( v1 );
	vec3_t v2;
	VectorSubtract( end, backEnd.viewParms.orient.origin, v2 );
	VectorNormalize( v2 );
	vec3_t right;
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	for ( int i = 0; i < 4; i++ ) {
		DoRailCore( start, end, right, len, 8 );
		vec3_t temp;
		RotatePointAroundVector( temp, vec, right, 45 );
		VectorCopy( temp, right );
	}
}

void idSurfaceEntity::RB_SurfaceSplash() {
	vec3_t left, up;
	float radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;

	VectorSet( left, -radius, 0, 0 );
	VectorSet( up, 0, radius, 0 );
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( oldvec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

//	Draws x/y/z lines from the origin for orientation debugging
void idSurfaceEntity::RB_SurfaceAxis() {
	GL_Bind( tr.whiteImage );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1, 0, 0 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 16, 0, 0 );
	qglColor3f( 0, 1, 0 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 0, 16, 0 );
	qglColor3f( 0, 0, 1 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 0, 0, 16 );
	qglEnd();
	qglLineWidth( 1 );
}

//	Entities that have a single procedurally generated surface
void idSurfaceEntity::Draw() {
	switch ( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;

	case RT_BEAM:
		RB_SurfaceBeam();
		break;

	case RT_BEAM_Q2:
		RB_SurfaceBeamQ2();
		break;

	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;

	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;

	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;

	case RT_SPLASH:
		RB_SurfaceSplash();
		break;

	default:
		RB_SurfaceAxis();
		break;
	}
}
