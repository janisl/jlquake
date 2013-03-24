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
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

#define ON_EPSILON      0.1f			// point on plane side epsilon

#define SIDE_FRONT      0
#define SIDE_BACK       1
#define SIDE_ON         2

#define MAX_CLIP_VERTS  64

#define SKY_SUBDIVISIONS        8
#define HALF_SKY_SUBDIVISIONS   ( SKY_SUBDIVISIONS / 2 )

float speedscale;					// for top sky and bottom sky

static char skyname[ MAX_QPATH ];
static float skyrotate;
static vec3_t skyaxis;
static image_t* sky_images[ 6 ];

static float sky_mins[ 2 ][ 6 ], sky_maxs[ 2 ][ 6 ];
static float sky_min, sky_max;

static float s_cloudTexCoords[ 6 ][ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ][ 2 ];

static vec3_t s_skyPoints[ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ];
static float s_skyTexCoords[ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ][ 2 ];

// 3dstudio environment map names
static const char* suf[ 6 ] = {"rt", "bk", "lf", "ft", "up", "dn"};

static vec3_t sky_clip[ 6 ] =
{
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};

// 1 = s, 2 = t, 3 = 2048
static int st_to_vec[ 6 ][ 3 ] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down
};

// s = [0]/[2], t = [1]/[2]
static int vec_to_st[ 6 ][ 3 ] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

static int sky_texorder[ 6 ] = {0, 2, 1, 3, 4, 5};

//	A sky texture is 256*128, with the right side being a masked overlay
void R_InitSky( mbrush29_texture_t* mt ) {
	byte* src = ( byte* )mt + mt->offsets[ 0 ];

	// make an average value for the back to avoid
	// a fringe on the top level

	int r = 0;
	int g = 0;
	int b = 0;
	byte trans[ 128 * 128 * 4 ];
	for ( int i = 0; i < 128; i++ ) {
		for ( int j = 0; j < 128; j++ ) {
			int p = src[ i * 256 + j + 128 ];
			byte* rgba = r_palette[ p ];
			trans[ ( i * 128 + j ) * 4 + 0 ] = rgba[ 0 ];
			trans[ ( i * 128 + j ) * 4 + 1 ] = rgba[ 1 ];
			trans[ ( i * 128 + j ) * 4 + 2 ] = rgba[ 2 ];
			trans[ ( i * 128 + j ) * 4 + 3 ] = 255;
			r += rgba[ 0 ];
			g += rgba[ 1 ];
			b += rgba[ 2 ];
		}
	}

	byte transpix[ 4 ];
	transpix[ 0 ] = r / ( 128 * 128 );
	transpix[ 1 ] = g / ( 128 * 128 );
	transpix[ 2 ] = b / ( 128 * 128 );
	transpix[ 3 ] = 0;

	if ( !tr.solidskytexture ) {
		tr.solidskytexture = R_CreateImage( "*solidsky", trans, 128, 128, false, false, GL_REPEAT, false );
	} else {
		R_ReUploadImage( tr.solidskytexture, trans );
	}

	for ( int i = 0; i < 128; i++ ) {
		for ( int j = 0; j < 128; j++ ) {
			int p = src[ i * 256 + j ];
			byte* rgba;
			if ( p == 0 ) {
				rgba = transpix;
			} else {
				rgba = r_palette[ p ];
			}
			trans[ ( i * 128 + j ) * 4 + 0 ] = rgba[ 0 ];
			trans[ ( i * 128 + j ) * 4 + 1 ] = rgba[ 1 ];
			trans[ ( i * 128 + j ) * 4 + 2 ] = rgba[ 2 ];
			trans[ ( i * 128 + j ) * 4 + 3 ] = rgba[ 3 ];
		}
	}

	if ( !tr.alphaskytexture ) {
		tr.alphaskytexture = R_CreateImage( "*alphasky", trans, 128, 128, false, false, GL_REPEAT, false );
	} else {
		R_ReUploadImage( tr.alphaskytexture, trans );
	}
}

void EmitSkyPolys( mbrush29_surface_t* fa, shaderStage_t* pStage ) {
	for ( mbrush29_glpoly_t* p = fa->polys; p; p = p->next ) {
		float* v = p->verts[ 0 ];
		for ( int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE ) {
			vec3_t dir;
			VectorSubtract( v, tr.viewParms.orient.origin, dir );
			dir[ 2 ] *= 3;		// flatten the sphere

			float length = dir[ 0 ] * dir[ 0 ] + dir[ 1 ] * dir[ 1 ] + dir[ 2 ] * dir[ 2 ];
			length = sqrt( length );
			length = 6 * 63 / length;

			dir[ 0 ] *= length;
			dir[ 1 ] *= length;

			float s = ( speedscale + dir[ 0 ] ) * ( 1.0 / 128 );
			float t = ( speedscale + dir[ 1 ] ) * ( 1.0 / 128 );

			tess.svars.colors[ i ][ 0 ] = 255;
			tess.svars.colors[ i ][ 1 ] = 255;
			tess.svars.colors[ i ][ 2 ] = 255;
			tess.svars.colors[ i ][ 3 ] = 255;
			tess.svars.texcoords[ 0 ][ i ][ 0 ] = s;
			tess.svars.texcoords[ 0 ][ i ][ 1 ] = t;
			tess.xyz[ i ][ 0 ] = v[ 0 ];
			tess.xyz[ i ][ 1 ] = v[ 1 ];
			tess.xyz[ i ][ 2 ] = v[ 2 ];
		}
		EnableArrays( p->numverts );
		EmitPolyIndexesQ1( p );
		RB_IterateStagesGenericTemp( &tess, pStage );
		tess.numIndexes = 0;
		DisableArrays();
	}
}

void R_SetSky( const char* name, float rotate, vec3_t axis ) {
	String::NCpy( skyname, name, sizeof ( skyname ) - 1 );
	skyrotate = rotate;
	VectorCopy( axis, skyaxis );

	for ( int i = 0; i < 6; i++ ) {
		// chop down rotating skies for less memory
		if ( r_skymip->value || skyrotate ) {
			r_picmip->value++;
		}

		char pathname[ MAX_QPATH ];
		String::Sprintf( pathname, sizeof ( pathname ), "env/%s%s.tga", skyname, suf[ i ] );

		sky_images[ i ] = R_FindImageFile( pathname, false, false, GL_CLAMP );
		if ( !sky_images[ i ] ) {
			sky_images[ i ] = tr.defaultImage;
		}

		if ( r_skymip->value || skyrotate ) {
			// take less memory
			r_picmip->value--;
			sky_min = 1.0 / 256;
			sky_max = 255.0 / 256;
		} else {
			sky_min = 1.0 / 512;
			sky_max = 511.0 / 512;
		}
	}
}

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

static void AddSkyPolygon( int nump, vec3_t vecs ) {
	// decide which face it maps to
	vec3_t v;
	VectorCopy( vec3_origin, v );
	float* vp = vecs;
	for ( int i = 0; i < nump; i++, vp += 3 ) {
		VectorAdd( vp, v, v );
	}
	vec3_t av;
	av[ 0 ] = idMath::Fabs( v[ 0 ] );
	av[ 1 ] = idMath::Fabs( v[ 1 ] );
	av[ 2 ] = idMath::Fabs( v[ 2 ] );
	int axis;
	if ( av[ 0 ] > av[ 1 ] && av[ 0 ] > av[ 2 ] ) {
		if ( v[ 0 ] < 0 ) {
			axis = 1;
		} else {
			axis = 0;
		}
	} else if ( av[ 1 ] > av[ 2 ] && av[ 1 ] > av[ 0 ] )             {
		if ( v[ 1 ] < 0 ) {
			axis = 3;
		} else {
			axis = 2;
		}
	} else {
		if ( v[ 2 ] < 0 ) {
			axis = 5;
		} else {
			axis = 4;
		}
	}

	// project new texture coords
	for ( int i = 0; i < nump; i++, vecs += 3 ) {
		int j = vec_to_st[ axis ][ 2 ];
		float dv;
		if ( j > 0 ) {
			dv = vecs[ j - 1 ];
		} else {
			dv = -vecs[ -j - 1 ];
		}
		if ( dv < 0.001 ) {
			continue;	// don't divide by zero
		}
		j = vec_to_st[ axis ][ 0 ];
		float s;
		if ( j < 0 ) {
			s = -vecs[ -j - 1 ] / dv;
		} else {
			s = vecs[ j - 1 ] / dv;
		}
		j = vec_to_st[ axis ][ 1 ];
		float t;
		if ( j < 0 ) {
			t = -vecs[ -j - 1 ] / dv;
		} else {
			t = vecs[ j - 1 ] / dv;
		}

		if ( s < sky_mins[ 0 ][ axis ] ) {
			sky_mins[ 0 ][ axis ] = s;
		}
		if ( t < sky_mins[ 1 ][ axis ] ) {
			sky_mins[ 1 ][ axis ] = t;
		}
		if ( s > sky_maxs[ 0 ][ axis ] ) {
			sky_maxs[ 0 ][ axis ] = s;
		}
		if ( t > sky_maxs[ 1 ][ axis ] ) {
			sky_maxs[ 1 ][ axis ] = t;
		}
	}
}

static void ClipSkyPolygon( int nump, vec3_t vecs, int stage ) {
	if ( nump > MAX_CLIP_VERTS - 2 ) {
		common->Error( "ClipSkyPolygon: MAX_CLIP_VERTS" );
	}
	if ( stage == 6 ) {
		// fully clipped, so draw it
		AddSkyPolygon( nump, vecs );
		return;
	}

	bool front = false;
	bool back = false;
	float* norm = sky_clip[ stage ];
	int sides[ MAX_CLIP_VERTS ];
	float dists[ MAX_CLIP_VERTS ];
	float* v = vecs;
	for ( int i = 0; i < nump; i++, v += 3 ) {
		float d = DotProduct( v, norm );
		if ( d > ON_EPSILON ) {
			front = true;
			sides[ i ] = SIDE_FRONT;
		} else if ( d < -ON_EPSILON )     {
			back = true;
			sides[ i ] = SIDE_BACK;
		} else {
			sides[ i ] = SIDE_ON;
		}
		dists[ i ] = d;
	}

	if ( !front || !back ) {
		// not clipped
		ClipSkyPolygon( nump, vecs, stage + 1 );
		return;
	}

	// clip it
	sides[ nump ] = sides[ 0 ];
	dists[ nump ] = dists[ 0 ];
	VectorCopy( vecs, ( vecs + ( nump * 3 ) ) );
	int newc[ 2 ];
	newc[ 0 ] = newc[ 1 ] = 0;
	vec3_t newv[ 2 ][ MAX_CLIP_VERTS ];

	v = vecs;
	for ( int i = 0; i < nump; i++, v += 3 ) {
		switch ( sides[ i ] ) {
		case SIDE_FRONT:
			VectorCopy( v, newv[ 0 ][ newc[ 0 ] ] );
			newc[ 0 ]++;
			break;

		case SIDE_BACK:
			VectorCopy( v, newv[ 1 ][ newc[ 1 ] ] );
			newc[ 1 ]++;
			break;

		case SIDE_ON:
			VectorCopy( v, newv[ 0 ][ newc[ 0 ] ] );
			newc[ 0 ]++;
			VectorCopy( v, newv[ 1 ][ newc[ 1 ] ] );
			newc[ 1 ]++;
			break;
		}

		if ( sides[ i ] == SIDE_ON || sides[ i + 1 ] == SIDE_ON || sides[ i + 1 ] == sides[ i ] ) {
			continue;
		}

		float d = dists[ i ] / ( dists[ i ] - dists[ i + 1 ] );
		for ( int j = 0; j < 3; j++ ) {
			float e = v[ j ] + d * ( v[ j + 3 ] - v[ j ] );
			newv[ 0 ][ newc[ 0 ] ][ j ] = e;
			newv[ 1 ][ newc[ 1 ] ][ j ] = e;
		}
		newc[ 0 ]++;
		newc[ 1 ]++;
	}

	// continue
	ClipSkyPolygon( newc[ 0 ], newv[ 0 ][ 0 ], stage + 1 );
	ClipSkyPolygon( newc[ 1 ], newv[ 1 ][ 0 ], stage + 1 );
}

void R_ClearSkyBox() {
	for ( int i = 0; i < 6; i++ ) {
		sky_mins[ 0 ][ i ] = sky_mins[ 1 ][ i ] = 9999;
		sky_maxs[ 0 ][ i ] = sky_maxs[ 1 ][ i ] = -9999;
	}
}

void R_AddSkySurface( mbrush38_surface_t* fa ) {
	// calculate vertex values for sky box
	for ( mbrush38_glpoly_t* p = fa->polys; p; p = p->next ) {
		vec3_t verts[ MAX_CLIP_VERTS ];
		for ( int i = 0; i < p->numverts; i++ ) {
			VectorSubtract( p->verts[ i ], tr.viewParms.orient.origin, verts[ i ] );
		}
		ClipSkyPolygon( p->numverts, verts[ 0 ], 0 );
	}
}

static void RB_ClipSkyPolygons( shaderCommands_t* input ) {
	R_ClearSkyBox();

	for ( int i = 0; i < input->numIndexes; i += 3 ) {
		vec3_t p[ 5 ];		// need one extra point for clipping
		for ( int j = 0; j < 3; j++ ) {
			VectorSubtract( input->xyz[ input->indexes[ i + j ] ], backEnd.viewParms.orient.origin, p[ j ] );
		}
		ClipSkyPolygon( 3, p[ 0 ], 0 );
	}
}

//	Parms: s, t range from -1 to 1
static void MakeSkyVec( float s, float t, int axis, float outSt[ 2 ], vec3_t outXYZ ) {
	float boxSize;
	if ( glfogsettings[ FOG_SKY ].registered ) {
		boxSize = glfogsettings[ FOG_SKY ].end;
	} else {
		boxSize = ( GGameType & GAME_Tech3 ) ? backEnd.viewParms.zFar / 1.75 : 2300;		// div sqrt(3)
	}
	// make sure the sky is not near clipped
	if ( boxSize < r_znear->value * 2.0 ) {
		boxSize = r_znear->value * 2.0;
	}

	vec3_t b;
	b[ 0 ] = s * boxSize;
	b[ 1 ] = t * boxSize;
	b[ 2 ] = boxSize;

	for ( int j = 0; j < 3; j++ ) {
		int k = st_to_vec[ axis ][ j ];
		if ( k < 0 ) {
			outXYZ[ j ] = -b[ -k - 1 ];
		} else {
			outXYZ[ j ] = b[ k - 1 ];
		}
	}

	// avoid bilerp seam
	s = ( s + 1 ) * 0.5;
	t = ( t + 1 ) * 0.5;

	if ( s < sky_min ) {
		s = sky_min;
	} else if ( s > sky_max )     {
		s = sky_max;
	}
	if ( t < sky_min ) {
		t = sky_min;
	} else if ( t > sky_max )     {
		t = sky_max;
	}

	t = 1.0 - t;

	if ( outSt ) {
		outSt[ 0 ] = s;
		outSt[ 1 ] = t;
	}
}

static void EmitSkyVertex( float s, float t, int axis ) {
	vec3_t v;
	float St[ 2 ];
	MakeSkyVec( s, t, axis, St, v );
	qglTexCoord2f( St[ 0 ], St[ 1 ] );
	qglVertex3fv( v );
}

void R_DrawSkyBoxQ2( surfaceType_t* dummy ) {
	if ( skyrotate ) {
		// check for no sky at all
		int i;
		for ( i = 0; i < 6; i++ ) {
			if ( sky_mins[ 0 ][ i ] < sky_maxs[ 0 ][ i ] &&
				 sky_mins[ 1 ][ i ] < sky_maxs[ 1 ][ i ] ) {
				break;
			}
		}
		if ( i == 6 ) {
			return;		// nothing visible
		}
	}

	qglPushMatrix();
	qglTranslatef( tr.viewParms.orient.origin[ 0 ], tr.viewParms.orient.origin[ 1 ], tr.viewParms.orient.origin[ 2 ] );
	qglRotatef( tr.refdef.floatTime * skyrotate, skyaxis[ 0 ], skyaxis[ 1 ], skyaxis[ 2 ] );

	for ( int i = 0; i < 6; i++ ) {
		if ( skyrotate ) {
			// hack, forces full sky to draw when rotating
			sky_mins[ 0 ][ i ] = -1;
			sky_mins[ 1 ][ i ] = -1;
			sky_maxs[ 0 ][ i ] = 1;
			sky_maxs[ 1 ][ i ] = 1;
		}

		if ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ||
			 sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) {
			continue;
		}

		GL_Bind( sky_images[ sky_texorder[ i ] ] );

		qglBegin( GL_QUADS );
		EmitSkyVertex( sky_mins[ 0 ][ i ], sky_mins[ 1 ][ i ], i );
		EmitSkyVertex( sky_mins[ 0 ][ i ], sky_maxs[ 1 ][ i ], i );
		EmitSkyVertex( sky_maxs[ 0 ][ i ], sky_maxs[ 1 ][ i ], i );
		EmitSkyVertex( sky_maxs[ 0 ][ i ], sky_mins[ 1 ][ i ], i );
		qglEnd();
	}
	qglPopMatrix();
}

static void DrawSkySide( image_t* image, const int mins[ 2 ], const int maxs[ 2 ] ) {
	GL_Bind( image );

	for ( int t = mins[ 1 ] + HALF_SKY_SUBDIVISIONS; t < maxs[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
		qglBegin( GL_TRIANGLE_STRIP );

		for ( int s = mins[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= maxs[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
			qglTexCoord2fv( s_skyTexCoords[ t ][ s ] );
			qglVertex3fv( s_skyPoints[ t ][ s ] );

			qglTexCoord2fv( s_skyTexCoords[ t + 1 ][ s ] );
			qglVertex3fv( s_skyPoints[ t + 1 ][ s ] );
		}

		qglEnd();
	}
}

static void DrawSkySideInner( image_t* image, const int mins[ 2 ], const int maxs[ 2 ] ) {
	GL_Bind( image );

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );
	for ( int t = mins[ 1 ] + HALF_SKY_SUBDIVISIONS; t < maxs[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
		qglBegin( GL_TRIANGLE_STRIP );

		for ( int s = mins[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= maxs[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
			qglTexCoord2fv( s_skyTexCoords[ t ][ s ] );
			qglVertex3fv( s_skyPoints[ t ][ s ] );

			qglTexCoord2fv( s_skyTexCoords[ t + 1 ][ s ] );
			qglVertex3fv( s_skyPoints[ t + 1 ][ s ] );
		}

		qglEnd();
	}
}

static void DrawSkyBox( shader_t* shader ) {
	sky_min = 0;
	sky_max = 1;

	Com_Memset( s_skyTexCoords, 0, sizeof ( s_skyTexCoords ) );

	for ( int i = 0; i < 6; i++ ) {
		sky_mins[ 0 ][ i ] = floor( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[ 1 ][ i ] = floor( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 0 ][ i ] = ceil( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 1 ][ i ] = ceil( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ) ||
			 ( sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) ) {
			continue;
		}

		int sky_mins_subd[ 2 ], sky_maxs_subd[ 2 ];
		sky_mins_subd[ 0 ] = sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[ 1 ] = sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 0 ] = sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 1 ] = sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;

		if ( sky_mins_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_mins_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_mins_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_mins_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_mins_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_mins_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_maxs_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_maxs_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_maxs_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_maxs_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_maxs_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		//
		// iterate through the subdivisions
		//
		for ( int t = sky_mins_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
			for ( int s = sky_mins_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS,
					( t - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS, i, s_skyTexCoords[ t ][ s ], s_skyPoints[ t ][ s ] );
			}
		}

		DrawSkySide( shader->sky.outerbox[ sky_texorder[ i ] ], sky_mins_subd, sky_maxs_subd );
	}
}

static void DrawSkyBoxInner( shader_t* shader ) {
	Com_Memset( s_skyTexCoords, 0, sizeof ( s_skyTexCoords ) );

	for ( int i = 0; i < 6; i++ ) {
		sky_mins[ 0 ][ i ] = floor( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[ 1 ][ i ] = floor( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 0 ][ i ] = ceil( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 1 ][ i ] = ceil( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ) ||
			 ( sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) ) {
			continue;
		}

		int sky_mins_subd[ 2 ], sky_maxs_subd[ 2 ];
		sky_mins_subd[ 0 ] = sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[ 1 ] = sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 0 ] = sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 1 ] = sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;

		if ( sky_mins_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_mins_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_mins_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_mins_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_mins_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_mins_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_maxs_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_maxs_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_maxs_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_maxs_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_maxs_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		//
		// iterate through the subdivisions
		//
		for ( int t = sky_mins_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
			for ( int s = sky_mins_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS,
					( t - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS,
					i, s_skyTexCoords[ t ][ s ], s_skyPoints[ t ][ s ] );
			}
		}

		DrawSkySideInner( shader->sky.innerbox[ sky_texorder[ i ] ], sky_mins_subd, sky_maxs_subd );
	}
}

/*
===================================================================================

CLOUD VERTEX GENERATION

===================================================================================
*/

//	Called when a sky shader is parsed
void R_InitSkyTexCoords( float heightCloud ) {
	float radiusWorld = 4096;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for ( int i = 0; i < 6; i++ ) {
		for ( int t = 0; t <= SKY_SUBDIVISIONS; t++ ) {
			for ( int s = 0; s <= SKY_SUBDIVISIONS; s++ ) {
				// compute vector from view origin to sky side integral point
				vec3_t skyVec;
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS,
					( t - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS, i, NULL, skyVec );

				// compute parametric value 'p' that intersects with cloud layer
				float p = ( 1.0f / ( 2 * DotProduct( skyVec, skyVec ) ) ) *
						  ( -2 * skyVec[ 2 ] * radiusWorld +
							2 * sqrt( Square( skyVec[ 2 ] ) * Square( radiusWorld ) +
								2 * Square( skyVec[ 0 ] ) * radiusWorld * heightCloud +
								Square( skyVec[ 0 ] ) * Square( heightCloud ) +
								2 * Square( skyVec[ 1 ] ) * radiusWorld * heightCloud +
								Square( skyVec[ 1 ] ) * Square( heightCloud ) +
								2 * Square( skyVec[ 2 ] ) * radiusWorld * heightCloud +
								Square( skyVec[ 2 ] ) * Square( heightCloud ) ) );

				// compute intersection point based on p
				vec3_t v;
				VectorScale( skyVec, p, v );
				v[ 2 ] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize( v );

				float sRad = idMath::ACos( v[ 0 ] );
				float tRad = idMath::ACos( v[ 1 ] );

				s_cloudTexCoords[ i ][ t ][ s ][ 0 ] = sRad;
				s_cloudTexCoords[ i ][ t ][ s ][ 1 ] = tRad;
			}
		}
	}
}

static void FillCloudySkySide( const int mins[ 2 ], const int maxs[ 2 ], bool addIndexes ) {
	int vertexStart = tess.numVertexes;

	for ( int t = mins[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= maxs[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
		for ( int s = mins[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= maxs[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
			VectorAdd( s_skyPoints[ t ][ s ], backEnd.viewParms.orient.origin, tess.xyz[ tess.numVertexes ] );
			tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = s_skyTexCoords[ t ][ s ][ 0 ];
			tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = s_skyTexCoords[ t ][ s ][ 1 ];

			tess.numVertexes++;

			if ( tess.numVertexes >= SHADER_MAX_VERTEXES ) {
				common->Error( "SHADER_MAX_VERTEXES hit in FillCloudySkySide()\n" );
			}
		}
	}

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if ( addIndexes ) {
		int tHeight = maxs[ 1 ] - mins[ 1 ] + 1;
		int sWidth = maxs[ 0 ] - mins[ 0 ] + 1;
		if ( tess.numIndexes + ( sWidth - 1 ) * ( tHeight - 1 ) * 6 >= SHADER_MAX_INDEXES ) {
			common->Error( "SHADER_MAX_INDEXES hit in FillCloudySkySide()\n" );
		}
		for ( int t = 0; t < tHeight - 1; t++ ) {
			for ( int s = 0; s < sWidth - 1; s++ ) {
				tess.indexes[ tess.numIndexes ] = vertexStart + s + t * sWidth;
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + ( t + 1 ) * sWidth;
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + t * sWidth;
				tess.numIndexes++;

				tess.indexes[ tess.numIndexes ] = vertexStart + s + ( t + 1 ) * sWidth;
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + ( t + 1 ) * sWidth;
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + t * sWidth;
				tess.numIndexes++;
			}
		}
	}
}

static void FillCloudBox( const shader_t* shader, int stage ) {
	for ( int i = 0; i < 6; i++ ) {
		float MIN_T;
		if ( 1 ) {	// FIXME? shader->sky.fullClouds )
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if ( i == 5 ) {
				continue;
			}
		} else {
			switch ( i ) {
			case 0:
			case 1:
			case 2:
			case 3:
				MIN_T = -1;
				break;

			case 5:
				// don't draw clouds beneath you
				continue;

			case 4:		// top
			default:
				MIN_T = -HALF_SKY_SUBDIVISIONS;
				break;
			}
		}

		sky_mins[ 0 ][ i ] = floor( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[ 1 ][ i ] = floor( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 0 ][ i ] = ceil( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 1 ][ i ] = ceil( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ) ||
			 ( sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) ) {
			continue;
		}

		int sky_mins_subd[ 2 ], sky_maxs_subd[ 2 ];
		sky_mins_subd[ 0 ] = idMath::FtoiFast( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_mins_subd[ 1 ] = idMath::FtoiFast( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_maxs_subd[ 0 ] = idMath::FtoiFast( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_maxs_subd[ 1 ] = idMath::FtoiFast( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS );

		if ( sky_mins_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_mins_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_mins_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_mins_subd[ 1 ] < MIN_T ) {
			sky_mins_subd[ 1 ] = MIN_T;
		} else if ( sky_mins_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_mins_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS ) {
			sky_maxs_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		} else if ( sky_maxs_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}
		if ( sky_maxs_subd[ 1 ] < MIN_T ) {
			sky_maxs_subd[ 1 ] = MIN_T;
		} else if ( sky_maxs_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )       {
			sky_maxs_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		//
		// iterate through the subdivisions
		//
		for ( int t = sky_mins_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ ) {
			for ( int s = sky_mins_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ ) {
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS,
					( t - HALF_SKY_SUBDIVISIONS ) / ( float )HALF_SKY_SUBDIVISIONS, i, NULL, s_skyPoints[ t ][ s ] );

				s_skyTexCoords[ t ][ s ][ 0 ] = s_cloudTexCoords[ i ][ t ][ s ][ 0 ];
				s_skyTexCoords[ t ][ s ][ 1 ] = s_cloudTexCoords[ i ][ t ][ s ][ 1 ];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, stage == 0 );
	}
}

static void R_BuildCloudData( shaderCommands_t* input ) {
	shader_t* shader = input->shader;

	assert( shader->isSky );

	sky_min = 1.0 / 256.0f;		// FIXME: not correct?
	sky_max = 255.0 / 256.0f;

	// set up for drawing
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	if ( input->shader->sky.cloudHeight ) {
		if ( GGameType & GAME_ET ) {
			// ok, this is really wierd. it's iterating through shader stages here,
			// which is unecessary for a multi-stage sky shader, as far as i can tell
			// nuking this
			FillCloudBox( input->shader, 0 );
		} else {
			for ( int i = 0; i < MAX_SHADER_STAGES; i++ ) {
				if ( !tess.xstages[ i ] ) {
					break;
				}
				FillCloudBox( input->shader, i );
			}
		}
	}
}

//	All of the visible sky triangles are in tess
//
//	Other things could be stuck in here, like birds in the sky, etc
void RB_StageIteratorSky() {
	if ( r_fastsky->integer ) {
		return;
	}

	// when portal sky exists, only render skybox for the portal sky scene
	if ( skyboxportal && !( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) ) {
		return;
	}

	// does the current fog require fastsky?
	if ( backEnd.viewParms.glFog.registered ) {
		if ( !backEnd.viewParms.glFog.drawsky ) {
			return;
		}
	} else if ( glfogNum > FOG_NONE )     {
		if ( !glfogsettings[ FOG_CURRENT ].drawsky ) {
			return;
		}
	}

	backEnd.refdef.rdflags |= RDF_DRAWINGSKY;

	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	RB_ClipSkyPolygons( &tess );

	// r_showsky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in
	if ( r_showsky->integer ) {
		qglDepthRange( 0.0, 0.0 );
	} else {
		qglDepthRange( 1.0, 1.0 );
	}

	// draw the outer skybox
	if ( tess.shader->sky.outerbox[ 0 ] && tess.shader->sky.outerbox[ 0 ] != tr.defaultImage ) {
		qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

		qglPushMatrix();
		GL_State( 0 );
		qglTranslatef( backEnd.viewParms.orient.origin[ 0 ], backEnd.viewParms.orient.origin[ 1 ], backEnd.viewParms.orient.origin[ 2 ] );

		DrawSkyBox( tess.shader );

		qglPopMatrix();
	}

	// generate the vertexes for all the clouds, which will be drawn
	// by the generic shader routine
	R_BuildCloudData( &tess );

	RB_StageIteratorGeneric();

	// draw the inner skybox
	if ( tess.shader->sky.innerbox[ 0 ] && tess.shader->sky.innerbox[ 0 ] != tr.defaultImage ) {
		qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

		qglPushMatrix();
		GL_State( 0 );
		qglTranslatef( backEnd.viewParms.orient.origin[ 0 ], backEnd.viewParms.orient.origin[ 1 ], backEnd.viewParms.orient.origin[ 2 ] );

		DrawSkyBoxInner( tess.shader );

		qglPopMatrix();
	}

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	backEnd.refdef.rdflags &= ~RDF_DRAWINGSKY;

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = true;
}

//	(SA) FIXME: sun should render behind clouds, so passing dark areas cover it up
void RB_DrawSun() {
	if ( !tr.sunShader ) {
		return;
	}
	if ( !backEnd.skyRenderedThisView ) {
		return;
	}
	if ( !r_drawSun->integer ) {
		return;
	}
	qglPushMatrix();
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	qglTranslatef( backEnd.viewParms.orient.origin[ 0 ], backEnd.viewParms.orient.origin[ 1 ], backEnd.viewParms.orient.origin[ 2 ] );

	float dist = backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	float size = dist * ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ? 0.2 : 0.4 );

	vec3_t origin, vec1, vec2;
	VectorScale( tr.sunDirection, dist, origin );
	PerpendicularVector( vec1, tr.sunDirection );
	CrossProduct( tr.sunDirection, vec1, vec2 );

	VectorScale( vec1, size, vec1 );
	VectorScale( vec2, size, vec2 );

	// farthest depth range
	qglDepthRange( 1.0, 1.0 );

	RB_BeginSurface( tr.sunShader, tess.fogNum );
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		// (SA) simpler sun drawing
		byte color[ 4 ];
		color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 255;
		RB_AddQuadStamp( origin, vec1, vec2, color );
	} else {
		// FIXME: use quad stamp

		vec3_t temp;
		VectorCopy( origin, temp );
		VectorSubtract( temp, vec1, temp );
		VectorSubtract( temp, vec2, temp );
		VectorCopy( temp, tess.xyz[ tess.numVertexes ] );
		tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 0;
		tess.vertexColors[ tess.numVertexes ][ 0 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 1 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 2 ] = 255;
		tess.numVertexes++;

		VectorCopy( origin, temp );
		VectorAdd( temp, vec1, temp );
		VectorSubtract( temp, vec2, temp );
		VectorCopy( temp, tess.xyz[ tess.numVertexes ] );
		tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 1;
		tess.vertexColors[ tess.numVertexes ][ 0 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 1 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 2 ] = 255;
		tess.numVertexes++;

		VectorCopy( origin, temp );
		VectorAdd( temp, vec1, temp );
		VectorAdd( temp, vec2, temp );
		VectorCopy( temp, tess.xyz[ tess.numVertexes ] );
		tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 1;
		tess.vertexColors[ tess.numVertexes ][ 0 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 1 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 2 ] = 255;
		tess.numVertexes++;

		VectorCopy( origin, temp );
		VectorSubtract( temp, vec1, temp );
		VectorAdd( temp, vec2, temp );
		VectorCopy( temp, tess.xyz[ tess.numVertexes ] );
		tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = 0;
		tess.vertexColors[ tess.numVertexes ][ 0 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 1 ] = 255;
		tess.vertexColors[ tess.numVertexes ][ 2 ] = 255;
		tess.numVertexes++;

		tess.indexes[ tess.numIndexes++ ] = 0;
		tess.indexes[ tess.numIndexes++ ] = 1;
		tess.indexes[ tess.numIndexes++ ] = 2;
		tess.indexes[ tess.numIndexes++ ] = 0;
		tess.indexes[ tess.numIndexes++ ] = 2;
		tess.indexes[ tess.numIndexes++ ] = 3;
	}

	RB_EndSurface();

	if ( r_drawSun->integer > 1 ) {
		// draw flare effect
		// (SA) FYI:	This is cheezy and was only a test so far.
		//				If we decide to use the flare business I will /definatly/ improve all this

		// get a point a little closer
		dist = dist * 0.7;
		VectorScale( tr.sunDirection, dist, origin );

		// and make the flare a little smaller
		VectorScale( vec1, 0.5f, vec1 );
		VectorScale( vec2, 0.5f, vec2 );

		// add the vectors to give an 'off angle' result
		vec3_t temp;
		VectorAdd( tr.sunDirection, backEnd.viewParms.orient.axis[ 0 ], temp );
		VectorNormalize( temp );

		// amplify the result
		origin[ 0 ] += temp[ 0 ] * 500.0;
		origin[ 1 ] += temp[ 1 ] * 500.0;
		origin[ 2 ] += temp[ 2 ] * 500.0;

		// (SA) FIXME: todo: flare effect should render last (on top of everything else) and only when sun is in view (sun moving out of camera past degree n should start to cause flare dimming until view angle to sun is off by angle n + x.

		// draw the flare
		RB_BeginSurface( tr.sunflareShader, tess.fogNum );
		byte color[ 4 ];
		color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 255;
		RB_AddQuadStamp( origin, vec1, vec2, color );
		RB_EndSurface();
	}

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );
	qglPopMatrix();
}
