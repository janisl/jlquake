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

#define WAVEVALUE( table, base, amplitude, phase, freq )  ( ( base ) + table[ idMath::FtoiFast( ( ( ( phase ) + tess.shaderTime * ( freq ) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * ( amplitude ) )

static int edgeVerts[ 6 ][ 2 ] =
{
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

static float* TableForFunc( genFunc_t func ) {
	switch ( func ) {
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	common->Error( "TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.shader->name );
	return NULL;
}

//	Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
static float EvalWaveForm( const waveForm_t* wf ) {
	float* table = TableForFunc( wf->func );

	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

static float EvalWaveFormClamped( const waveForm_t* wf ) {
	float glow = EvalWaveForm( wf );

	if ( glow < 0 ) {
		return 0;
	}

	if ( glow > 1 ) {
		return 1;
	}

	return glow;
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

//	Wiggle the normals for wavy environment mapping
static void RB_CalcDeformNormals( deformStage_t* ds ) {
	float* xyz = ( float* )tess.xyz;
	float* normal = ( float* )tess.normal;

	for ( int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
		float scale = 0.98f;
		scale = R_NoiseGet4f( xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
}

static void RB_CalcDeformVertexes( deformStage_t* ds ) {
	float* xyz = ( float* )tess.xyz;
	float* normal = ( float* )tess.normal;

	if ( ds->deformationWave.frequency < 0 ) {
		if ( VectorCompare( backEnd.currentEntity->e.fireRiseDir, vec3_origin ) ) {
			VectorSet( backEnd.currentEntity->e.fireRiseDir, 0, 0, 1 );
		}

		// get the world up vector in local coordinates
		vec3_t worldUp;
		if ( backEnd.currentEntity->e.hModel ) {
			// world surfaces dont have an axis
			VectorRotate( backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp );
		} else {
			VectorCopy( backEnd.currentEntity->e.fireRiseDir, worldUp );
		}
		// don't go so far if sideways, since they must be moving
		VectorScale( worldUp, 0.4 + 0.6 * idMath::Fabs( backEnd.currentEntity->e.fireRiseDir[ 2 ] ), worldUp );

		ds->deformationWave.frequency *= -1;
		bool inverse = false;
		if ( ds->deformationWave.frequency > 999 ) {
			// hack for negative Z deformation (ack)
			inverse = true;
			ds->deformationWave.frequency -= 999;
		}

		float* table = TableForFunc( ds->deformationWave.func );

		for ( int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
			float off = ( xyz[ 0 ] + xyz[ 1 ] + xyz[ 2 ] ) * ds->deformationSpread;

			float scale = WAVEVALUE( table, ds->deformationWave.base,
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );

			float dot = DotProduct( worldUp, normal );

			if ( dot * scale > 0 ) {
				if ( inverse ) {
					scale *= -1;
				}
				VectorMA( xyz, dot * scale, worldUp, xyz );
			}
		}

		if ( inverse ) {
			ds->deformationWave.frequency += 999;
		}
		ds->deformationWave.frequency *= -1;
	} else if ( ds->deformationWave.frequency == 0 ) {
		float scale = EvalWaveForm( &ds->deformationWave );

		for ( int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
			vec3_t offset;
			VectorScale( normal, scale, offset );

			xyz[ 0 ] += offset[ 0 ];
			xyz[ 1 ] += offset[ 1 ];
			xyz[ 2 ] += offset[ 2 ];
		}
	} else {
		float* table = TableForFunc( ds->deformationWave.func );

		for ( int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
			float off = ( xyz[ 0 ] + xyz[ 1 ] + xyz[ 2 ] ) * ds->deformationSpread;

			float scale = WAVEVALUE( table, ds->deformationWave.base,
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );

			vec3_t offset;
			VectorScale( normal, scale, offset );

			xyz[ 0 ] += offset[ 0 ];
			xyz[ 1 ] += offset[ 1 ];
			xyz[ 2 ] += offset[ 2 ];
		}
	}
}

static void RB_CalcBulgeVertexes( deformStage_t* ds ) {
	const float* st = ( const float* )tess.texCoords[ 0 ];
	float* xyz = ( float* )tess.xyz;
	float* normal = ( float* )tess.normal;

	float now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for ( int i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 ) {
		int off = ( int )( ( float )( FUNCTABLE_SIZE / idMath::TWO_PI ) * ( st[ 0 ] * ds->bulgeWidth + now ) );

		float scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;

		xyz[ 0 ] += normal[ 0 ] * scale;
		xyz[ 1 ] += normal[ 1 ] * scale;
		xyz[ 2 ] += normal[ 2 ] * scale;
	}
}

//	A deformation that can move an entire surface along a wave path
static void RB_CalcMoveVertexes( deformStage_t* ds ) {
	float* table = TableForFunc( ds->deformationWave.func );

	float scale = WAVEVALUE( table, ds->deformationWave.base,
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency );

	vec3_t offset;
	VectorScale( ds->moveVector, scale, offset );

	float* xyz = ( float* )tess.xyz;
	for ( int i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		VectorAdd( xyz, offset, xyz );
	}
}

//	Change a polygon into a bunch of text polygons
static void DeformText( const char* text ) {
	vec3_t height;
	height[ 0 ] = 0;
	height[ 1 ] = 0;
	height[ 2 ] = -1;
	vec3_t width;
	CrossProduct( tess.normal[ 0 ], height, width );

	// find the midpoint of the box
	vec3_t mid;
	VectorClear( mid );
	float bottom = 999999;
	float top = -999999;
	for ( int i = 0; i < 4; i++ ) {
		VectorAdd( tess.xyz[ i ], mid, mid );
		if ( tess.xyz[ i ][ 2 ] < bottom ) {
			bottom = tess.xyz[ i ][ 2 ];
		}
		if ( tess.xyz[ i ][ 2 ] > top ) {
			top = tess.xyz[ i ][ 2 ];
		}
	}
	vec3_t origin;
	VectorScale( mid, 0.25f, origin );

	// determine the individual character size
	height[ 0 ] = 0;
	height[ 1 ] = 0;
	height[ 2 ] = ( top - bottom ) * 0.5f;

	VectorScale( width, height[ 2 ] * -0.75f, width );

	// determine the starting position
	int len = String::Length( text );
	VectorMA( origin, ( len - 1 ), width, origin );

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	byte color[ 4 ];
	color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 255;

	// draw each character
	for ( int i = 0; i < len; i++ ) {
		int ch = text[ i ];
		ch &= 255;

		if ( ch != ' ' ) {
			int row = ch >> 4;
			int col = ch & 15;

			float frow = row * 0.0625f;
			float fcol = col * 0.0625f;
			float size = 0.0625f;

			RB_AddQuadStampExt( origin, width, height, color, fcol, frow, fcol + size, frow + size );
		}
		VectorMA( origin, -2, width, origin );
	}
}

void GlobalVectorToLocal( const vec3_t in, vec3_t out ) {
	out[ 0 ] = DotProduct( in, backEnd.orient.axis[ 0 ] );
	out[ 1 ] = DotProduct( in, backEnd.orient.axis[ 1 ] );
	out[ 2 ] = DotProduct( in, backEnd.orient.axis[ 2 ] );
}

//	Assuming all the triangles for this shader are independant quads, rebuild
// them as forward facing sprites
static void AutospriteDeform() {
	if ( tess.numVertexes & 3 ) {
		common->Printf( S_COLOR_YELLOW "Autosprite shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		common->Printf( S_COLOR_YELLOW "Autosprite shader %s had odd index count", tess.shader->name );
	}

	int oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	vec3_t leftDir, upDir;
	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.orient.axis[ 1 ], leftDir );
		GlobalVectorToLocal( backEnd.viewParms.orient.axis[ 2 ], upDir );
	} else {
		VectorCopy( backEnd.viewParms.orient.axis[ 1 ], leftDir );
		VectorCopy( backEnd.viewParms.orient.axis[ 2 ], upDir );
	}

	for ( int i = 0; i < oldVerts; i += 4 ) {
		// find the midpoint
		float* xyz = tess.xyz[ i ];

		vec3_t mid;
		mid[ 0 ] = 0.25f * ( xyz[ 0 ] + xyz[ 4 ] + xyz[ 8 ] + xyz[ 12 ] );
		mid[ 1 ] = 0.25f * ( xyz[ 1 ] + xyz[ 5 ] + xyz[ 9 ] + xyz[ 13 ] );
		mid[ 2 ] = 0.25f * ( xyz[ 2 ] + xyz[ 6 ] + xyz[ 10 ] + xyz[ 14 ] );

		vec3_t delta;
		VectorSubtract( xyz, mid, delta );
		float radius = VectorLength( delta ) * 0.707f;			// / sqrt(2)

		vec3_t left, up;
		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd.viewParms.isMirror ) {
			VectorSubtract( vec3_origin, left, left );
		}

		// compensate for scale in the axes if necessary
		if ( backEnd.currentEntity->e.nonNormalizedAxes ) {
			float axisLength = VectorLength( backEnd.currentEntity->e.axis[ 0 ] );
			if ( !axisLength ) {
				axisLength = 0;
			} else {
				axisLength = 1.0f / axisLength;
			}
			VectorScale( left, axisLength, left );
			VectorScale( up, axisLength, up );
		}

		RB_AddQuadStamp( mid, left, up, tess.vertexColors[ i ] );
	}
}

//	Autosprite2 will pivot a rectangular quad along the center of its long axis
static void Autosprite2Deform() {
	if ( tess.numVertexes & 3 ) {
		common->Printf( S_COLOR_YELLOW "Autosprite2 shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		common->Printf( S_COLOR_YELLOW "Autosprite2 shader %s had odd index count", tess.shader->name );
	}

	vec3_t forward;
	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.orient.axis[ 0 ], forward );
	} else {
		VectorCopy( backEnd.viewParms.orient.axis[ 0 ], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( int i = 0, indexes = 0; i < tess.numVertexes; i += 4, indexes += 6 ) {
		// find the midpoint
		float* xyz = tess.xyz[ i ];

		// identify the two shortest edges
		int nums[ 2 ];
		nums[ 0 ] = nums[ 1 ] = 0;
		float lengths[ 2 ];
		lengths[ 0 ] = lengths[ 1 ] = 999999;

		for ( int j = 0; j < 6; j++ ) {
			float* v1 = xyz + 4 * edgeVerts[ j ][ 0 ];
			float* v2 = xyz + 4 * edgeVerts[ j ][ 1 ];

			vec3_t temp;
			VectorSubtract( v1, v2, temp );

			float l = DotProduct( temp, temp );
			if ( l < lengths[ 0 ] ) {
				nums[ 1 ] = nums[ 0 ];
				lengths[ 1 ] = lengths[ 0 ];
				nums[ 0 ] = j;
				lengths[ 0 ] = l;
			} else if ( l < lengths[ 1 ] ) {
				nums[ 1 ] = j;
				lengths[ 1 ] = l;
			}
		}

		vec3_t mid[ 2 ];
		for ( int j = 0; j < 2; j++ ) {
			float* v1 = xyz + 4 * edgeVerts[ nums[ j ] ][ 0 ];
			float* v2 = xyz + 4 * edgeVerts[ nums[ j ] ][ 1 ];

			mid[ j ][ 0 ] = 0.5f * ( v1[ 0 ] + v2[ 0 ] );
			mid[ j ][ 1 ] = 0.5f * ( v1[ 1 ] + v2[ 1 ] );
			mid[ j ][ 2 ] = 0.5f * ( v1[ 2 ] + v2[ 2 ] );
		}

		// find the vector of the major axis
		vec3_t major;
		VectorSubtract( mid[ 1 ], mid[ 0 ], major );

		// cross this with the view direction to get minor axis
		vec3_t minor;
		CrossProduct( major, forward, minor );
		VectorNormalize( minor );

		// re-project the points
		for ( int j = 0; j < 2; j++ ) {
			float* v1 = xyz + 4 * edgeVerts[ nums[ j ] ][ 0 ];
			float* v2 = xyz + 4 * edgeVerts[ nums[ j ] ][ 1 ];

			float l = 0.5 * sqrt( lengths[ j ] );

			// we need to see which direction this edge
			// is used to determine direction of projection
			int k;
			for ( k = 0; k < 5; k++ ) {
				if ( tess.indexes[ indexes + k ] == ( glIndex_t )( i + edgeVerts[ nums[ j ] ][ 0 ] ) &&
					 tess.indexes[ indexes + k + 1 ] == ( glIndex_t )( i + edgeVerts[ nums[ j ] ][ 1 ] ) ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( mid[ j ], l, minor, v1 );
				VectorMA( mid[ j ], -l, minor, v2 );
			} else {
				VectorMA( mid[ j ], -l, minor, v1 );
				VectorMA( mid[ j ], l, minor, v2 );
			}
		}
	}
}

void RB_DeformTessGeometry() {
	for ( int i = 0; i < tess.shader->numDeforms; i++ ) {
		deformStage_t* ds = &tess.shader->deforms[ i ];

		switch ( ds->deformation ) {
		case DEFORM_NONE:
			break;

		case DEFORM_NORMALS:
			RB_CalcDeformNormals( ds );
			break;

		case DEFORM_WAVE:
			RB_CalcDeformVertexes( ds );
			break;

		case DEFORM_BULGE:
			RB_CalcBulgeVertexes( ds );
			break;

		case DEFORM_MOVE:
			RB_CalcMoveVertexes( ds );
			break;

		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;

		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;

		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;

		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			DeformText( backEnd.refdef.text[ ds->deformation - DEFORM_TEXT0 ] );
			break;
		}
	}
}

/*
====================================================================

COLORS

====================================================================
*/

// ydnar: faster, table-based version of this function
// saves about 1-2ms per frame on my machine with 64 x 1000 triangle models in scene
static void RB_CalcDiffuseColorET( unsigned char* colors ) {
	int i, dp, * colorsInt;
	float* normal;
	trRefEntity_t* ent;
	vec3_t lightDir;
	int numVertexes;

	ent = backEnd.currentEntity;
	VectorCopy( ent->lightDir, lightDir );

	normal = tess.normal[ 0 ];
	colorsInt = ( int* )colors;

	numVertexes = tess.numVertexes;
	for ( i = 0; i < numVertexes; i++, normal += 4, colorsInt++ ) {
		dp = idMath::FtoiFast( ENTITY_LIGHT_STEPS * DotProduct( normal, lightDir ) );

		// ydnar: enable this for twosided lighting
		//%	if( tess.shader->cullType == CT_TWO_SIDED )
		//%		dp = fabs( dp );

		if ( dp <= 0 ) {
			*colorsInt = ent->entityLightInt[ 0 ];
		} else if ( dp >= ENTITY_LIGHT_STEPS ) {
			*colorsInt = ent->entityLightInt[ ENTITY_LIGHT_STEPS - 1 ];
		} else {
			*colorsInt = ent->entityLightInt[ dp ];
		}
	}
}

void RB_CalcDiffuseColorWithCustomLights( byte* colors, vec3_t ambientLight, int ambientLightInt, vec3_t directedLight, vec3_t lightDir ) {
	float* normal = tess.normal[ 0 ];

	int numVertexes = tess.numVertexes;
	for ( int i = 0; i < numVertexes; i++, normal += 4 ) {
		float incoming = DotProduct( normal, lightDir );
		if ( incoming <= 0 ) {
			*( int* )&colors[ i * 4 ] = ambientLightInt;
			continue;
		}

		int j = idMath::FtoiFast( ambientLight[ 0 ] + incoming * directedLight[ 0 ] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 0 ] = j;

		j = idMath::FtoiFast( ambientLight[ 1 ] + incoming * directedLight[ 1 ] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 1 ] = j;

		j = idMath::FtoiFast( ambientLight[ 2 ] + incoming * directedLight[ 2 ] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 2 ] = j;

		colors[ i * 4 + 3 ] = 255;
	}
}

//	The basic vertex lighting calc
void RB_CalcDiffuseColor( byte* colors ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcDiffuseColorET( colors );
		return;
	}

	trRefEntity_t* ent = backEnd.currentEntity;
	int ambientLightInt = ent->ambientLightInt;
	vec3_t ambientLight;
	VectorCopy( ent->ambientLight, ambientLight );
	vec3_t directedLight;
	VectorCopy( ent->directedLight, directedLight );
	vec3_t lightDir;
	VectorCopy( ent->lightDir, lightDir );

	RB_CalcDiffuseColorWithCustomLights( colors, ambientLight, ambientLightInt, directedLight, lightDir );
}

void RB_CalcDiffuseOverBrightColor( byte* colors ) {
	trRefEntity_t* ent = backEnd.currentEntity;
	vec3_t ambientLight;
	VectorCopy( ent->ambientLight, ambientLight );
	vec3_t directedLight;
	VectorCopy( ent->directedLight, directedLight );
	vec3_t lightDir;
	VectorCopy( ent->lightDir, lightDir );

	float* normal = tess.normal[ 0 ];

	int numVertexes = tess.numVertexes;
	for ( int i = 0; i < numVertexes; i++, normal += 4 ) {
		float incoming = DotProduct( normal, lightDir );
		if ( incoming <= 0 ) {
			*( int* )&colors[ i * 4 ] = 0;
			continue;
		}

		int j = idMath::FtoiFast( ambientLight[ 0 ] + incoming * directedLight[ 0 ] );
		j -= 256;
		if ( j < 0 ) {
			j = 0;
		}
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 0 ] = j;

		j = idMath::FtoiFast( ambientLight[ 1 ] + incoming * directedLight[ 1 ] );
		j -= 256;
		if ( j < 0 ) {
			j = 0;
		}
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 1 ] = j;

		j = idMath::FtoiFast( ambientLight[ 2 ] + incoming * directedLight[ 2 ] );
		j -= 256;
		if ( j < 0 ) {
			j = 0;
		}
		if ( j > 255 ) {
			j = 255;
		}
		colors[ i * 4 + 2 ] = j;

		colors[ i * 4 + 3 ] = 255;
	}
}

void RB_CalcDiffuseColorEntityTopColour( byte* colors ) {
	trRefEntity_t* ent = backEnd.currentEntity;
	vec3_t topColour;
	topColour[ 0 ] = ent->e.topColour[ 0 ] / 255.0f;
	topColour[ 1 ] = ent->e.topColour[ 1 ] / 255.0f;
	topColour[ 2 ] = ent->e.topColour[ 2 ] / 255.0f;
	vec3_t ambientLight;
	ambientLight[ 0 ] = ent->ambientLight[ 0 ] * topColour[ 0 ];
	ambientLight[ 1 ] = ent->ambientLight[ 1 ] * topColour[ 1 ];
	ambientLight[ 2 ] = ent->ambientLight[ 2 ] * topColour[ 2 ];
	vec3_t directedLight;
	directedLight[ 0 ] = ent->directedLight[ 0 ] * topColour[ 0 ];
	directedLight[ 1 ] = ent->directedLight[ 1 ] * topColour[ 1 ];
	directedLight[ 2 ] = ent->directedLight[ 2 ] * topColour[ 2 ];
	vec3_t lightDir;
	VectorCopy( ent->lightDir, lightDir );

	int ambientLightInt;
	( ( byte* )&ambientLightInt )[ 0 ] = idMath::FtoiFast( ambientLight[ 0 ] );
	( ( byte* )&ambientLightInt )[ 1 ] = idMath::FtoiFast( ambientLight[ 1 ] );
	( ( byte* )&ambientLightInt )[ 2 ] = idMath::FtoiFast( ambientLight[ 2 ] );
	( ( byte* )&ambientLightInt )[ 3 ] = 0xff;

	RB_CalcDiffuseColorWithCustomLights( colors, ambientLight, ambientLightInt, directedLight, lightDir );
}

void RB_CalcDiffuseColorEntityBottomColour( byte* colors ) {
	trRefEntity_t* ent = backEnd.currentEntity;
	vec3_t bottomColour;
	bottomColour[ 0 ] = ent->e.bottomColour[ 0 ] / 255.0f;
	bottomColour[ 1 ] = ent->e.bottomColour[ 1 ] / 255.0f;
	bottomColour[ 2 ] = ent->e.bottomColour[ 2 ] / 255.0f;
	vec3_t ambientLight;
	ambientLight[ 0 ] = ent->ambientLight[ 0 ] * bottomColour[ 0 ];
	ambientLight[ 1 ] = ent->ambientLight[ 1 ] * bottomColour[ 1 ];
	ambientLight[ 2 ] = ent->ambientLight[ 2 ] * bottomColour[ 2 ];
	vec3_t directedLight;
	directedLight[ 0 ] = ent->directedLight[ 0 ] * bottomColour[ 0 ];
	directedLight[ 1 ] = ent->directedLight[ 1 ] * bottomColour[ 1 ];
	directedLight[ 2 ] = ent->directedLight[ 2 ] * bottomColour[ 2 ];
	vec3_t lightDir;
	VectorCopy( ent->lightDir, lightDir );

	int ambientLightInt;
	( ( byte* )&ambientLightInt )[ 0 ] = idMath::FtoiFast( ambientLight[ 0 ] );
	( ( byte* )&ambientLightInt )[ 1 ] = idMath::FtoiFast( ambientLight[ 1 ] );
	( ( byte* )&ambientLightInt )[ 2 ] = idMath::FtoiFast( ambientLight[ 2 ] );
	( ( byte* )&ambientLightInt )[ 3 ] = 0xff;

	RB_CalcDiffuseColorWithCustomLights( colors, ambientLight, ambientLightInt, directedLight, lightDir );
}

static void RB_CalcWaveColor( const waveForm_t* wf, byte* dstColors ) {
	float glow;
	if ( wf->func == GF_NOISE ) {
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	} else {
		glow = EvalWaveForm( wf ) * tr.identityLight;
	}

	if ( glow < 0 ) {
		glow = 0;
	} else if ( glow > 1 ) {
		glow = 1;
	}

	int v = idMath::FtoiFast( 255 * glow );
	byte color[ 4 ];
	color[ 0 ] = color[ 1 ] = color[ 2 ] = v;
	color[ 3 ] = 255;
	v = *( int* )color;

	int* colors = ( int* )dstColors;
	for ( int i = 0; i < tess.numVertexes; i++, colors++ ) {
		*colors = v;
	}
}

static void RB_CalcColorFromEntity( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	int c = *( int* )backEnd.currentEntity->e.shaderRGBA;
	int* pColors = ( int* )dstColors;

	for ( int i = 0; i < tess.numVertexes; i++, pColors++ ) {
		*pColors = c;
	}
}

static void RB_CalcColorFromOneMinusEntity( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	byte invModulate[ 4 ];
	invModulate[ 0 ] = 255 - backEnd.currentEntity->e.shaderRGBA[ 0 ];
	invModulate[ 1 ] = 255 - backEnd.currentEntity->e.shaderRGBA[ 1 ];
	invModulate[ 2 ] = 255 - backEnd.currentEntity->e.shaderRGBA[ 2 ];
	invModulate[ 3 ] = 255 - backEnd.currentEntity->e.shaderRGBA[ 3 ];	// this trashes alpha, but the AGEN block fixes it

	int c = *( int* )invModulate;
	int* pColors = ( int* )dstColors;

	for ( int i = 0; i < tess.numVertexes; i++, pColors++ ) {
		*pColors = c;
	}
}

static void RB_CalcColorFromEntityAbsoluteLight( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	int value = backEnd.currentEntity->e.absoluteLight * 255;
	if ( value > 255 ) {
		value = 255;
	}
	byte temp[ 4 ];
	temp[ 0 ] = value;
	temp[ 1 ] = value;
	temp[ 2 ] = value;
	temp[ 3 ] = 255;	// this trashes alpha, but the AGEN block fixes it
	int c = *( int* )temp;
	int* pColors = ( int* )dstColors;

	for ( int i = 0; i < tess.numVertexes; i++, pColors++ ) {
		*pColors = c;
	}
}

static void RB_CalcWaveAlpha( const waveForm_t* wf, byte* dstColors ) {
	float glow;
	if ( wf->func == GF_NOISE ) {
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	} else {
		glow = EvalWaveFormClamped( wf );
	}

	int v = 255 * glow;

	for ( int i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		dstColors[ 3 ] = v;
	}
}

//	Calculates specular coefficient and places it in the alpha channel
static void RB_CalcSpecularAlpha( byte* alphas ) {
	float* v = tess.xyz[ 0 ];
	float* normal = tess.normal[ 0 ];

	alphas += 3;

	int numVertexes = tess.numVertexes;
	for ( int i = 0; i < numVertexes; i++, v += 4, normal += 4, alphas += 4 ) {
		vec3_t lightDir;
		VectorSubtract( lightOrigin, v, lightDir );
		VectorNormalizeFast( lightDir );

		// calculate the specular color
		float d = DotProduct( normal, lightDir );

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		vec3_t reflected;
		reflected[ 0 ] = normal[ 0 ] * 2 * d - lightDir[ 0 ];
		reflected[ 1 ] = normal[ 1 ] * 2 * d - lightDir[ 1 ];
		reflected[ 2 ] = normal[ 2 ] * 2 * d - lightDir[ 2 ];

		vec3_t viewer;
		VectorSubtract( backEnd.orient.viewOrigin, v, viewer );
		float ilength = idMath::RSqrt( DotProduct( viewer, viewer ) );
		float l = DotProduct( reflected, viewer );
		l *= ilength;

		int b;
		if ( l < 0 ) {
			b = 0;
		} else {
			l = l * l;
			l = l * l;
			b = l * 255;
			if ( b > 255 ) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

static void RB_CalcAlphaFromEntity( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	dstColors += 3;

	for ( int i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		*dstColors = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	}
}

static void RB_CalcAlphaFromOneMinusEntity( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	dstColors += 3;

	for ( int i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[ 3 ];
	}
}

static void RB_CalcAlphaFromEntityConditionalTranslucent( byte* dstColors ) {
	if ( !backEnd.currentEntity ) {
		return;
	}

	dstColors += 3;
	byte alpha = backEnd.currentEntity->e.renderfx & RF_TRANSLUCENT ?
		backEnd.currentEntity->e.shaderRGBA[ 3 ] : 0xff;

	for ( int i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		*dstColors = alpha;
	}
}

static void RB_CalcModulateColorsByFogET( unsigned char* colors ) {
	// ydnar: no world, no fogging
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		if ( texCoords[ i ][ 0 ] <= 0.0f || texCoords[ i ][ 1 ] <= 0.0f ) {
			continue;
		}
		float f = 1.0f - ( texCoords[ i ][ 0 ] * texCoords[ i ][ 1 ] );
		if ( f <= 0.0f ) {
			colors[ 0 ] = 0;
			colors[ 1 ] = 0;
			colors[ 2 ] = 0;
		} else {
			colors[ 0 ] *= f;
			colors[ 1 ] *= f;
			colors[ 2 ] *= f;
		}
	}
}

static void RB_CalcModulateColorsByFog( byte* colors ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcModulateColorsByFogET( colors );
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[ i ][ 0 ], texCoords[ i ][ 1 ] );
		colors[ 0 ] *= f;
		colors[ 1 ] *= f;
		colors[ 2 ] *= f;
	}
}

static void RB_CalcModulateAlphasByFogET( unsigned char* colors ) {
	// ydnar: no world, no fogging
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		if ( texCoords[ i ][ 0 ] <= 0.0f || texCoords[ i ][ 1 ] <= 0.0f ) {
			continue;
		}
		float f = 1.0f - ( texCoords[ i ][ 0 ] * texCoords[ i ][ 1 ] );
		if ( f <= 0.0f ) {
			colors[ 3 ] = 0;
		} else {
			colors[ 3 ] *= f;
		}
	}
}

static void RB_CalcModulateAlphasByFog( byte* colors ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcModulateAlphasByFogET( colors );
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[ i ][ 0 ], texCoords[ i ][ 1 ] );
		colors[ 3 ] *= f;
	}
}

static void RB_CalcModulateRGBAsByFogET( unsigned char* colors ) {
	// ydnar: no world, no fogging
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		if ( texCoords[ i ][ 0 ] <= 0.0f || texCoords[ i ][ 1 ] <= 0.0f ) {
			continue;
		}
		float f = 1.0f - ( texCoords[ i ][ 0 ] * texCoords[ i ][ 1 ] );
		if ( f <= 0.0f ) {
			colors[ 0 ] = 0;
			colors[ 1 ] = 0;
			colors[ 2 ] = 0;
			colors[ 3 ] = 0;
		} else {
			colors[ 0 ] *= f;
			colors[ 1 ] *= f;
			colors[ 2 ] *= f;
			colors[ 3 ] *= f;
		}
	}
}

static void RB_CalcModulateRGBAsByFog( byte* colors ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcModulateRGBAsByFogET( colors );
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[ SHADER_MAX_VERTEXES ][ 2 ];
	RB_CalcFogTexCoords( texCoords[ 0 ] );

	for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[ i ][ 0 ], texCoords[ i ][ 1 ] );
		colors[ 0 ] *= f;
		colors[ 1 ] *= f;
		colors[ 2 ] *= f;
		colors[ 3 ] *= f;
	}
}

void ComputeColors( shaderStage_t* pStage ) {
	//
	// rgbGen
	//
	switch ( pStage->rgbGen ) {
	case CGEN_IDENTITY:
		Com_Memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
		break;

	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
		break;

	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor( ( byte* )tess.svars.colors );
		break;

	case CGEN_LIGHTING_DIFFUSE_OVER_BRIGHT:
		RB_CalcDiffuseOverBrightColor( ( byte* )tess.svars.colors );
		break;

	case CGEN_LIGHTING_DIFFUSE_ENTITY_TOP_COLOUR:
		RB_CalcDiffuseColorEntityTopColour( ( byte* )tess.svars.colors );
		break;

	case CGEN_LIGHTING_DIFFUSE_ENTITY_BOTTOM_COLOUR:
		RB_CalcDiffuseColorEntityBottomColour( ( byte* )tess.svars.colors );
		break;

	case CGEN_EXACT_VERTEX:
		Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof ( tess.vertexColors[ 0 ] ) );
		break;

	case CGEN_CONST:
		for ( int i = 0; i < tess.numVertexes; i++ ) {
			*( int* )tess.svars.colors[ i ] = *( int* )pStage->constantColor;
		}
		break;

	case CGEN_VERTEX:
		if ( tr.identityLight == 1 ) {
			Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof ( tess.vertexColors[ 0 ] ) );
		} else {
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[ i ][ 0 ] = tess.vertexColors[ i ][ 0 ] * tr.identityLight;
				tess.svars.colors[ i ][ 1 ] = tess.vertexColors[ i ][ 1 ] * tr.identityLight;
				tess.svars.colors[ i ][ 2 ] = tess.vertexColors[ i ][ 2 ] * tr.identityLight;
				tess.svars.colors[ i ][ 3 ] = tess.vertexColors[ i ][ 3 ];
			}
		}
		break;

	case CGEN_ONE_MINUS_VERTEX:
		if ( tr.identityLight == 1 ) {
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[ i ][ 0 ] = 255 - tess.vertexColors[ i ][ 0 ];
				tess.svars.colors[ i ][ 1 ] = 255 - tess.vertexColors[ i ][ 1 ];
				tess.svars.colors[ i ][ 2 ] = 255 - tess.vertexColors[ i ][ 2 ];
			}
		} else {
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[ i ][ 0 ] = ( 255 - tess.vertexColors[ i ][ 0 ] ) * tr.identityLight;
				tess.svars.colors[ i ][ 1 ] = ( 255 - tess.vertexColors[ i ][ 1 ] ) * tr.identityLight;
				tess.svars.colors[ i ][ 2 ] = ( 255 - tess.vertexColors[ i ][ 2 ] ) * tr.identityLight;
			}
		}
		break;

	case CGEN_FOG:
	{
		mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

		for ( int i = 0; i < tess.numVertexes; i++ ) {
			*( int* )&tess.svars.colors[ i ] = GGameType & GAME_ET ? fog->shader->fogParms.colorInt : fog->colorInt;
		}
	}
	break;

	case CGEN_WAVEFORM:
		RB_CalcWaveColor( &pStage->rgbWave, ( byte* )tess.svars.colors );
		break;

	case CGEN_ENTITY:
		RB_CalcColorFromEntity( ( byte* )tess.svars.colors );
		break;

	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity( ( byte* )tess.svars.colors );
		break;

	case CGEN_ENTITY_ABSOLUTE_LIGHT:
		RB_CalcColorFromEntityAbsoluteLight( ( byte* )tess.svars.colors );
		break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen ) {
	case AGEN_SKIP:
		break;

	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( int i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[ i ][ 3 ] = 0xff;
				}
			}
		}
		break;

	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[ i ][ 3 ] = pStage->constantColor[ 3 ];
			}
		}
		break;

	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( byte* )tess.svars.colors );
		break;

	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( ( byte* )tess.svars.colors );
		break;

	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( byte* )tess.svars.colors );
		break;

	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( byte* )tess.svars.colors );
		break;

	case AGEN_ENTITY_CONDITIONAL_TRANSLUCENT:
		RB_CalcAlphaFromEntityConditionalTranslucent( ( byte* )tess.svars.colors );
		break;

	case AGEN_NORMALZFADE:
	{
		float alpha, range, lowest, highest, dot;
		vec3_t worldUp;
		qboolean zombieEffect = false;

		if ( VectorCompare( backEnd.currentEntity->e.fireRiseDir, vec3_origin ) ) {
			VectorSet( backEnd.currentEntity->e.fireRiseDir, 0, 0, 1 );
		}

		if ( backEnd.currentEntity->e.hModel ) {		// world surfaces dont have an axis
			VectorRotate( backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp );
		} else {
			VectorCopy( backEnd.currentEntity->e.fireRiseDir, worldUp );
		}

		lowest = pStage->zFadeBounds[ 0 ];
		if ( lowest == -1000 ) {		// use entity alpha
			lowest = backEnd.currentEntity->e.shaderTime;
			zombieEffect = true;
		}
		highest = pStage->zFadeBounds[ 1 ];
		if ( highest == -1000 ) {		// use entity alpha
			highest = backEnd.currentEntity->e.shaderTime;
			zombieEffect = true;
		}
		range = highest - lowest;
		for ( int i = 0; i < tess.numVertexes; i++ ) {
			dot = DotProduct( tess.normal[ i ], worldUp );

			// special handling for Zombie fade effect
			if ( zombieEffect ) {
				alpha = ( float )backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( dot + 1.0 ) / 2.0;
				alpha += ( 2.0 * ( float )backEnd.currentEntity->e.shaderRGBA[ 3 ] ) * ( 1.0 - ( dot + 1.0 ) / 2.0 );
				if ( alpha > 255.0 ) {
					alpha = 255.0;
				} else if ( alpha < 0.0 ) {
					alpha = 0.0;
				}
				tess.svars.colors[ i ][ 3 ] = ( byte )( alpha );
				continue;
			}

			if ( dot < highest ) {
				if ( dot > lowest ) {
					if ( dot < lowest + range / 2 ) {
						alpha = ( ( float )pStage->constantColor[ 3 ] * ( ( dot - lowest ) / ( range / 2 ) ) );
					} else {
						alpha = ( ( float )pStage->constantColor[ 3 ] * ( 1.0 - ( ( dot - lowest - range / 2 ) / ( range / 2 ) ) ) );
					}
					if ( alpha > 255.0 ) {
						alpha = 255.0;
					} else if ( alpha < 0.0 ) {
						alpha = 0.0;
					}

					// finally, scale according to the entity's alpha
					if ( backEnd.currentEntity->e.hModel ) {
						alpha *= ( float )backEnd.currentEntity->e.shaderRGBA[ 3 ] / 255.0;
					}

					tess.svars.colors[ i ][ 3 ] = ( byte )( alpha );
				} else {
					tess.svars.colors[ i ][ 3 ] = 0;
				}
			} else {
				tess.svars.colors[ i ][ 3 ] = 0;
			}
		}
	}
	break;

	case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[ i ][ 3 ] = tess.vertexColors[ i ][ 3 ];
			}
		}
		break;

	case AGEN_ONE_MINUS_VERTEX:
		for ( int i = 0; i < tess.numVertexes; i++ ) {
			tess.svars.colors[ i ][ 3 ] = 255 - tess.vertexColors[ i ][ 3 ];
		}
		break;

	case AGEN_PORTAL:
		for ( int i = 0; i < tess.numVertexes; i++ ) {
			vec3_t v;
			VectorSubtract( tess.xyz[ i ], backEnd.viewParms.orient.origin, v );
			float len = VectorLength( v );

			len /= tess.shader->portalRange;

			byte alpha;
			if ( len < 0 ) {
				alpha = 0;
			} else if ( len > 1 ) {
				alpha = 0xff;
			} else {
				alpha = len * 0xff;
			}

			tess.svars.colors[ i ][ 3 ] = alpha;
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum && ( !( GGameType & GAME_ET ) || !tess.shader->noFog ) ) {
		switch ( pStage->adjustColorsForFog ) {
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( byte* )tess.svars.colors );
			break;

		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( byte* )tess.svars.colors );
			break;

		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( byte* )tess.svars.colors );
			break;

		case ACFF_NONE:
			break;
		}
	}
}

/*
====================================================================

TEX COORDS

====================================================================
*/

static void RB_CalcFogTexCoordsET( float* st ) {
	// get fog stuff
	mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;
	mbrush46_model_t* bmodel = tr.world->bmodels + fog->modelNum;

	// all fogging distance is based on world Z units
	vec3_t local;
	VectorSubtract( backEnd.orient.origin, backEnd.viewParms.orient.origin, local );
	vec4_t fogDistanceVector;
	fogDistanceVector[ 0 ] = -backEnd.orient.modelMatrix[ 2 ];
	fogDistanceVector[ 1 ] = -backEnd.orient.modelMatrix[ 6 ];
	fogDistanceVector[ 2 ] = -backEnd.orient.modelMatrix[ 10 ];
	fogDistanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orient.axis[ 0 ] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[ 0 ] *= fog->shader->fogParms.tcScale;
	fogDistanceVector[ 1 ] *= fog->shader->fogParms.tcScale;
	fogDistanceVector[ 2 ] *= fog->shader->fogParms.tcScale;
	fogDistanceVector[ 3 ] *= fog->shader->fogParms.tcScale;

	// offset view origin by fog brush origin (fixme: really necessary?)
	vec3_t viewOrigin;
	VectorCopy( backEnd.orient.viewOrigin, viewOrigin );

	// offset fog surface
	vec4_t fogSurface;
	VectorCopy( fog->surface, fogSurface );
	fogSurface[ 3 ] = fog->surface[ 3 ] + DotProduct( fogSurface, bmodel->orientation[ backEnd.smpFrame ].origin );

	// ydnar: general fog case
	if ( fog->originalBrushNumber >= 0 ) {
		// rotate the gradient vector for this orientation
		vec4_t fogDepthVector;
		float eyeT;
		if ( fog->hasSurface ) {
			fogDepthVector[ 0 ] = fogSurface[ 0 ] * backEnd.orient.axis[ 0 ][ 0 ] +
								  fogSurface[ 1 ] * backEnd.orient.axis[ 0 ][ 1 ] + fogSurface[ 2 ] * backEnd.orient.axis[ 0 ][ 2 ];
			fogDepthVector[ 1 ] = fogSurface[ 0 ] * backEnd.orient.axis[ 1 ][ 0 ] +
								  fogSurface[ 1 ] * backEnd.orient.axis[ 1 ][ 1 ] + fogSurface[ 2 ] * backEnd.orient.axis[ 1 ][ 2 ];
			fogDepthVector[ 2 ] = fogSurface[ 0 ] * backEnd.orient.axis[ 2 ][ 0 ] +
								  fogSurface[ 1 ] * backEnd.orient.axis[ 2 ][ 1 ] + fogSurface[ 2 ] * backEnd.orient.axis[ 2 ][ 2 ];
			fogDepthVector[ 3 ] = -fogSurface[ 3 ] + DotProduct( backEnd.orient.origin, fogSurface );

			// scale the fog vectors based on the fog's thickness
			fogDepthVector[ 0 ] *= fog->shader->fogParms.tcScale;
			fogDepthVector[ 1 ] *= fog->shader->fogParms.tcScale;
			fogDepthVector[ 2 ] *= fog->shader->fogParms.tcScale;
			fogDepthVector[ 3 ] *= fog->shader->fogParms.tcScale;

			eyeT = DotProduct( viewOrigin, fogDepthVector ) + fogDepthVector[ 3 ];
		} else {
			eyeT = 1;	// non-surface fog always has eye inside
			// Gordon: rarrrrr, stop stupid msvc debug thing
			fogDepthVector[ 0 ] = 0;
			fogDepthVector[ 1 ] = 0;
			fogDepthVector[ 2 ] = 0;
			fogDepthVector[ 3 ] = 0;
		}
		// see if the viewpoint is outside
		bool eyeInside = eyeT >= 0;

		// calculate density for each point
		float* v = tess.xyz[ 0 ];
		for ( int i = 0; i < tess.numVertexes; i++, v += 4 ) {
			// calculate the length in fog
			float s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[ 3 ];
			float t = DotProduct( v, fogDepthVector ) + fogDepthVector[ 3 ];

			if ( eyeInside ) {
				t += eyeT;
			}

			st[ 0 ] = s;
			st[ 1 ] = t;
			st += 2;
		}
	}
	// ydnar: optimized for level-wide fogging
	else {
		// calculate density for each point
		float* v = tess.xyz[ 0 ];
		for ( int i = 0; i < tess.numVertexes; i++, v += 4 ) {
			// calculate the length in fog (t is always 0 if eye is in fog)
			st[ 0 ] = DotProduct( v, fogDistanceVector ) + fogDistanceVector[ 3 ];
			st[ 1 ] = 1.0;
			st += 2;
		}
	}
}

//	To do the clipped fog plane really correctly, we should use projected
// textures, but I don't trust the drivers and it doesn't fit our shader data.
void RB_CalcFogTexCoords( float* st ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcFogTexCoordsET( st );
		return;
	}

	mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	vec3_t local;
	VectorSubtract( backEnd.orient.origin, backEnd.viewParms.orient.origin, local );
	vec4_t fogDistanceVector;
	fogDistanceVector[ 0 ] = -backEnd.orient.modelMatrix[ 2 ];
	fogDistanceVector[ 1 ] = -backEnd.orient.modelMatrix[ 6 ];
	fogDistanceVector[ 2 ] = -backEnd.orient.modelMatrix[ 10 ];
	fogDistanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orient.axis[ 0 ] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[ 0 ] *= fog->tcScale;
	fogDistanceVector[ 1 ] *= fog->tcScale;
	fogDistanceVector[ 2 ] *= fog->tcScale;
	fogDistanceVector[ 3 ] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	vec4_t fogDepthVector;
	float eyeT;
	if ( fog->hasSurface ) {
		fogDepthVector[ 0 ] = fog->surface[ 0 ] * backEnd.orient.axis[ 0 ][ 0 ] +
							  fog->surface[ 1 ] * backEnd.orient.axis[ 0 ][ 1 ] + fog->surface[ 2 ] * backEnd.orient.axis[ 0 ][ 2 ];
		fogDepthVector[ 1 ] = fog->surface[ 0 ] * backEnd.orient.axis[ 1 ][ 0 ] +
							  fog->surface[ 1 ] * backEnd.orient.axis[ 1 ][ 1 ] + fog->surface[ 2 ] * backEnd.orient.axis[ 1 ][ 2 ];
		fogDepthVector[ 2 ] = fog->surface[ 0 ] * backEnd.orient.axis[ 2 ][ 0 ] +
							  fog->surface[ 1 ] * backEnd.orient.axis[ 2 ][ 1 ] + fog->surface[ 2 ] * backEnd.orient.axis[ 2 ][ 2 ];
		fogDepthVector[ 3 ] = -fog->surface[ 3 ] + DotProduct( backEnd.orient.origin, fog->surface );

		eyeT = DotProduct( backEnd.orient.viewOrigin, fogDepthVector ) + fogDepthVector[ 3 ];
	} else {
		eyeT = 1;	// non-surface fog always has eye inside

		//JL Don't leave memory uninitialised.
		fogDepthVector[ 0 ] = 0;
		fogDepthVector[ 1 ] = 0;
		fogDepthVector[ 2 ] = 0;
		fogDepthVector[ 3 ] = 0;
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	bool eyeOutside;
	if ( eyeT < 0 ) {
		eyeOutside = true;
	} else {
		eyeOutside = false;
	}

	fogDistanceVector[ 3 ] += 1.0 / 512;

	// calculate density for each point
	float* v = tess.xyz[ 0 ];
	for ( int i = 0; i < tess.numVertexes; i++, v += 4 ) {
		// calculate the length in fog
		float s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[ 3 ];
		float t = DotProduct( v, fogDepthVector ) + fogDepthVector[ 3 ];

		// partially clipped fogs use the T axis
		if ( eyeOutside ) {
			if ( t < 1.0 ) {
				t = 1.0 / 32;	// point is outside, so no fogging
			} else {
				t = 1.0 / 32 + 30.0 / 32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		} else {
			if ( t < 0 ) {
				t = 1.0 / 32;	// point is outside, so no fogging
			} else {
				t = 31.0 / 32;
			}
		}

		st[ 0 ] = s;
		st[ 1 ] = t;
		st += 2;
	}
}

static void RB_CalcEnvironmentTexCoordsET( float* st ) {
	int i;
	float d2, * v, * normal, sAdjust, tAdjust;
	vec3_t viewOrigin, ia1, ia2, viewer, reflected;


	// setup
	v = tess.xyz[ 0 ];
	normal = tess.normal[ 0 ];
	VectorCopy( backEnd.orient.viewOrigin, viewOrigin );

	// ydnar: origin of entity affects its environment map (every 256 units)
	// this is similar to racing game hacks where the env map seems to move
	// as the car passes through the world
	sAdjust = VectorLength( backEnd.orient.origin ) * 0.00390625;
	//%	 sAdjust = backEnd.orient.origin[ 0 ] * 0.00390625;
	sAdjust = 0.5 -  ( sAdjust - floor( sAdjust ) );

	tAdjust = backEnd.orient.origin[ 2 ] * 0.00390625;
	tAdjust = 0.5 - ( tAdjust - floor( tAdjust ) );

	// ydnar: the final reflection vector must be converted into world-space again
	// we just assume here that all transformations are rotations, so the inverse
	// of the transform matrix (the 3x3 part) is just the transpose
	// additionally, we don't need all 3 rows, so we just calculate 2
	// and we also scale by 0.5 to eliminate two per-vertex multiplies
	ia1[ 0 ] = backEnd.orient.axis[ 0 ][ 1 ] * 0.5;
	ia1[ 1 ] = backEnd.orient.axis[ 1 ][ 1 ] * 0.5;
	ia1[ 2 ] = backEnd.orient.axis[ 2 ][ 1 ] * 0.5;
	ia2[ 0 ] = backEnd.orient.axis[ 0 ][ 2 ] * 0.5;
	ia2[ 1 ] = backEnd.orient.axis[ 1 ][ 2 ] * 0.5;
	ia2[ 2 ] = backEnd.orient.axis[ 2 ][ 2 ] * 0.5;

	// walk verts
	for ( i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2 ) {
		VectorSubtract( viewOrigin, v, viewer );
		VectorNormalizeFast( viewer );

		d2 = 2.0 * DotProduct( normal, viewer );

		reflected[ 0 ] = normal[ 0 ] * d2 - viewer[ 0 ];
		reflected[ 1 ] = normal[ 1 ] * d2 - viewer[ 1 ];
		reflected[ 2 ] = normal[ 2 ] * d2 - viewer[ 2 ];

		st[ 0 ] = sAdjust + DotProduct( reflected, ia1 );
		st[ 1 ] = tAdjust - DotProduct( reflected, ia2 );
	}
}

static void RB_CalcEnvironmentTexCoords( float* st ) {
	if ( GGameType & GAME_ET ) {
		RB_CalcEnvironmentTexCoordsET( st );
		return;
	}

	float* v = tess.xyz[ 0 ];
	float* normal = tess.normal[ 0 ];

	for ( int i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2 ) {
		vec3_t viewer;
		VectorSubtract( backEnd.orient.viewOrigin, v, viewer );
		VectorNormalizeFast( viewer );

		float d = DotProduct( normal, viewer );

		vec3_t reflected;
		reflected[ 0 ] = normal[ 0 ] * 2 * d - viewer[ 0 ];
		reflected[ 1 ] = normal[ 1 ] * 2 * d - viewer[ 1 ];
		reflected[ 2 ] = normal[ 2 ] * 2 * d - viewer[ 2 ];

		st[ 0 ] = 0.5 + reflected[ 1 ] * 0.5;
		st[ 1 ] = 0.5 - reflected[ 2 ] * 0.5;
	}
}

static void RB_CalcFireRiseEnvTexCoords( float* st ) {
	float* v = tess.xyz[ 0 ];
	float* normal = tess.normal[ 0 ];
	vec3_t viewer;
	VectorNegate( backEnd.currentEntity->e.fireRiseDir, viewer );
	VectorNormalizeFast( viewer );

	for ( int i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2 ) {
		float d = DotProduct( normal, viewer );

		vec3_t reflected;
		reflected[ 0 ] = normal[ 0 ] * 2 * d - viewer[ 0 ];
		reflected[ 1 ] = normal[ 1 ] * 2 * d - viewer[ 1 ];
		reflected[ 2 ] = normal[ 2 ] * 2 * d - viewer[ 2 ];

		st[ 0 ] = 0.5 + reflected[ 1 ] * 0.5;
		st[ 1 ] = 0.5 - reflected[ 2 ] * 0.5;
	}
}

static void RB_CalcQuakeSkyTexCoords( float* st ) {
	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		vec3_t dir;
		VectorSubtract( tess.xyz[ i ], tr.viewParms.orient.origin, dir );
		dir[ 2 ] *= 3;		// flatten the sphere

		float length = dir[ 0 ] * dir[ 0 ] + dir[ 1 ] * dir[ 1 ] + dir[ 2 ] * dir[ 2 ];
		length = sqrt( length );
		length = 6 * 63 / length * ( 1.0 / 128 );

		st[ 0 ] = dir[ 0 ] * length;
		st[ 1 ] = dir[ 1 ] * length;
	}
}

static void RB_CalcTurbulentTexCoords( const waveForm_t* wf, float* st ) {
	float now = wf->phase + tess.shaderTime * wf->frequency;

	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		float s = st[ 0 ];
		float t = st[ 1 ];

		st[ 0 ] = s + tr.sinTable[ ( ( int )( ( ( tess.xyz[ i ][ 0 ] + tess.xyz[ i ][ 2 ] ) * 1.0 / 128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * wf->amplitude;
		st[ 1 ] = t + tr.sinTable[ ( ( int )( ( tess.xyz[ i ][ 1 ] * 1.0 / 128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * wf->amplitude;
	}
}

static void RB_CalcOldTurbulentTexCoords( const waveForm_t* wf, float* st ) {
	float now = wf->phase + tess.shaderTime * wf->frequency;

	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		float s = st[ 0 ];
		float t = st[ 1 ];

		st[ 0 ] = s + tr.sinTable[ idMath::FtoiFast( ( t * 8 / idMath::TWO_PI + now ) * FUNCTABLE_SIZE ) & FUNCTABLE_MASK ] * wf->amplitude;
		st[ 1 ] = t + tr.sinTable[ idMath::FtoiFast( ( s * 8 / idMath::TWO_PI + now ) * FUNCTABLE_SIZE ) & FUNCTABLE_MASK ] * wf->amplitude;
	}
}

static void RB_CalcScrollTexCoords( const float scrollSpeed[ 2 ], float* st ) {
	float timeScale = tess.shaderTime;

	float adjustedScrollS = scrollSpeed[ 0 ] * timeScale;
	float adjustedScrollT = scrollSpeed[ 1 ] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floor( adjustedScrollT );

	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		st[ 0 ] += adjustedScrollS;
		st[ 1 ] += adjustedScrollT;
	}
}

static void RB_CalcScaleTexCoords( const float scale[ 2 ], float* st ) {
	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		st[ 0 ] *= scale[ 0 ];
		st[ 1 ] *= scale[ 1 ];
	}
}

static void RB_CalcTransformTexCoords( const texModInfo_t* tmi, float* st ) {
	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		float s = st[ 0 ];
		float t = st[ 1 ];

		st[ 0 ] = s * tmi->matrix[ 0 ][ 0 ] + t * tmi->matrix[ 1 ][ 0 ] + tmi->translate[ 0 ];
		st[ 1 ] = s * tmi->matrix[ 0 ][ 1 ] + t * tmi->matrix[ 1 ][ 1 ] + tmi->translate[ 1 ];
	}
}

static void RB_CalcStretchTexCoords( const waveForm_t* wf, float* st ) {
	float p = 1.0f / EvalWaveForm( wf );

	texModInfo_t tmi;
	tmi.matrix[ 0 ][ 0 ] = p;
	tmi.matrix[ 1 ][ 0 ] = 0;
	tmi.translate[ 0 ] = 0.5f - 0.5f * p;

	tmi.matrix[ 0 ][ 1 ] = 0;
	tmi.matrix[ 1 ][ 1 ] = p;
	tmi.translate[ 1 ] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tmi, st );
}

static void RB_CalcRotateTexCoords( float degsPerSecond, float* st ) {
	float timeScale = tess.shaderTime;
	texModInfo_t tmi;

	float degs = -degsPerSecond * timeScale;
	int index = ( int )( degs * ( FUNCTABLE_SIZE / 360.0f ) );

	float sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	float cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[ 0 ][ 0 ] = cosValue;
	tmi.matrix[ 1 ][ 0 ] = -sinValue;
	tmi.translate[ 0 ] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[ 0 ][ 1 ] = sinValue;
	tmi.matrix[ 1 ][ 1 ] = cosValue;
	tmi.translate[ 1 ] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords( &tmi, st );
}

static void RB_CalcSwapTexCoords( float* st ) {
	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		float s = st[ 0 ];
		float t = st[ 1 ];

		st[ 0 ] = t;
		st[ 1 ] = 1.0 - s;		// err, flaming effect needs this
	}
}

void ComputeTexCoords( shaderStage_t* pStage ) {
	for ( int b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[ b ].tcGen ) {
		case TCGEN_IDENTITY:
			Com_Memset( tess.svars.texcoords[ b ], 0, sizeof ( float ) * 2 * tess.numVertexes );
			break;

		case TCGEN_TEXTURE:
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.texcoords[ b ][ i ][ 0 ] = tess.texCoords[ i ][ 0 ][ 0 ];
				tess.svars.texcoords[ b ][ i ][ 1 ] = tess.texCoords[ i ][ 0 ][ 1 ];
			}
			break;

		case TCGEN_LIGHTMAP:
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.texcoords[ b ][ i ][ 0 ] = tess.texCoords[ i ][ 1 ][ 0 ];
				tess.svars.texcoords[ b ][ i ][ 1 ] = tess.texCoords[ i ][ 1 ][ 1 ];
			}
			break;

		case TCGEN_VECTOR:
			for ( int i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.texcoords[ b ][ i ][ 0 ] = DotProduct( tess.xyz[ i ], pStage->bundle[ b ].tcGenVectors[ 0 ] );
				tess.svars.texcoords[ b ][ i ][ 1 ] = DotProduct( tess.xyz[ i ], pStage->bundle[ b ].tcGenVectors[ 1 ] );
			}
			break;

		case TCGEN_FOG:
			RB_CalcFogTexCoords( ( float* )tess.svars.texcoords[ b ] );
			break;

		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords( ( float* )tess.svars.texcoords[ b ] );
			break;

		case TCGEN_FIRERISEENV_MAPPED:
			RB_CalcFireRiseEnvTexCoords( ( float* )tess.svars.texcoords[ b ] );
			break;

		case TCGEN_QUAKE_SKY:
			RB_CalcQuakeSkyTexCoords( ( float* )tess.svars.texcoords[ b ] );
			break;

		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for ( int tm = 0; tm < pStage->bundle[ b ].numTexMods; tm++ ) {
			switch ( pStage->bundle[ b ].texMods[ tm ].type ) {
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[ b ].texMods[ tm ].wave,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_TURBULENT_OLD:
				RB_CalcOldTurbulentTexCoords( &pStage->bundle[ b ].texMods[ tm ].wave,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[ b ].texMods[ tm ].scroll,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[ b ].texMods[ tm ].scale,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[ b ].texMods[ tm ].wave,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[ b ].texMods[ tm ],
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[ b ].texMods[ tm ].rotateSpeed,
					( float* )tess.svars.texcoords[ b ] );
				break;

			case TMOD_SWAP:
				RB_CalcSwapTexCoords( ( float* )tess.svars.texcoords[ b ] );
				break;

			default:
				common->Error( "ERROR: unknown texmod '%d' in shader '%s'\n",
					pStage->bundle[ b ].texMods[ tm ].type, tess.shader->name );
				break;
			}
		}
	}
}
