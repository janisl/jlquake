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
  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

#include "../cinematic/public.h"
#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

// Hex Color string support
#define gethex( ch ) ( ( ch ) > '9' ? ( ( ch ) >= 'a' ? ( ( ch ) - 'a' + 10 ) : ( ( ch ) - '7' ) ) : ( ( ch ) - '0' ) )
#define ishex( ch )  ( ( ch ) && ( ( ( ch ) >= '0' && ( ch ) <= '9' ) || ( ( ch ) >= 'A' && ( ch ) <= 'F' ) || ( ( ch ) >= 'a' && ( ch ) <= 'f' ) ) )
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString( p ) ( ishex( *( p ) ) && ishex( *( ( p ) + 1 ) ) && ishex( *( ( p ) + 2 ) ) && ishex( *( ( p ) + 3 ) ) && ishex( *( ( p ) + 4 ) ) && ishex( *( ( p ) + 5 ) ) )
#define Q_HexColorStringHasAlpha( p ) ( ishex( *( ( p ) + 6 ) ) && ishex( *( ( p ) + 7 ) ) )

shaderCommands_t tess;

bool setArraysOnce;

void EnableArrays( int numVertexes ) {
	qglVertexPointer( 3, GL_FLOAT, 16, tess.xyz );	// padded for SIMD
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[ 0 ] );
	if ( qglLockArraysEXT && numVertexes ) {
		qglLockArraysEXT( 0, numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}
}

void DisableArrays() {
	qglDisableClientState( GL_COLOR_ARRAY );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
}

void EnableMultitexturedArrays( int numVertexes ) {
	qglVertexPointer( 3, GL_FLOAT, 16, tess.xyz );	// padded for SIMD
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	GL_SelectTexture( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[ 0 ] );

	GL_SelectTexture( 1 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[ 1 ] );
	if ( qglLockArraysEXT && numVertexes ) {
		qglLockArraysEXT( 0, numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}
}

void DisableMultitexturedArrays() {
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	GL_SelectTexture( 0 );
	qglDisableClientState( GL_COLOR_ARRAY );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
	GL_SelectTexture( 1 );
}

//	This is just for OpenGL conformance testing, it should never be the fastest
static void APIENTRY R_ArrayElementDiscrete( GLint index ) {
	qglColor4ubv( tess.svars.colors[ index ] );
	if ( glState.currenttmu ) {
		qglMultiTexCoord2fARB( 0, tess.svars.texcoords[ 0 ][ index ][ 0 ], tess.svars.texcoords[ 0 ][ index ][ 1 ] );
		qglMultiTexCoord2fARB( 1, tess.svars.texcoords[ 1 ][ index ][ 0 ], tess.svars.texcoords[ 1 ][ index ][ 1 ] );
	} else   {
		qglTexCoord2fv( tess.svars.texcoords[ 0 ][ index ] );
	}
	qglVertex3fv( tess.xyz[ index ] );
}

static void R_DrawStripElements( int numIndexes, const glIndex_t* indexes, void ( APIENTRY* element )( GLint ) ) {
	if ( numIndexes <= 0 ) {
		return;
	}

	qglBegin( GL_TRIANGLE_STRIP );

	// prime the strip
	element( indexes[ 0 ] );
	element( indexes[ 1 ] );
	element( indexes[ 2 ] );

	glIndex_t last[ 3 ] = { -1, -1, -1 };
	last[ 0 ] = indexes[ 0 ];
	last[ 1 ] = indexes[ 1 ];
	last[ 2 ] = indexes[ 2 ];

	bool even = false;

	for ( int i = 3; i < numIndexes; i += 3 ) {
		// odd numbered triangle in potential strip
		if ( !even ) {
			// check previous triangle to see if we're continuing a strip
			if ( ( indexes[ i + 0 ] == last[ 2 ] ) && ( indexes[ i + 1 ] == last[ 1 ] ) ) {
				element( indexes[ i + 2 ] );
				assert( indexes[ i + 2 ] < ( glIndex_t )tess.numVertexes );
				even = true;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else {
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );

				element( indexes[ i + 0 ] );
				element( indexes[ i + 1 ] );
				element( indexes[ i + 2 ] );

				even = false;
			}
		} else   {
			// check previous triangle to see if we're continuing a strip
			if ( ( last[ 2 ] == indexes[ i + 1 ] ) && ( last[ 0 ] == indexes[ i + 0 ] ) ) {
				element( indexes[ i + 2 ] );

				even = false;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else {
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );

				element( indexes[ i + 0 ] );
				element( indexes[ i + 1 ] );
				element( indexes[ i + 2 ] );

				even = false;
			}
		}

		// cache the last three vertices
		last[ 0 ] = indexes[ i + 0 ];
		last[ 1 ] = indexes[ i + 1 ];
		last[ 2 ] = indexes[ i + 2 ];
	}

	qglEnd();
}

//	Optionally performs our own glDrawElements that looks for strip conditions
// instead of using the single glDrawElements call that may be inefficient
// without compiled vertex arrays.
static void R_DrawElements( int numIndexes, const glIndex_t* indexes ) {
	int primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if ( primitives == 0 ) {
		if ( qglLockArraysEXT ) {
			primitives = 2;
		} else   {
			primitives = 1;
		}
	}

	if ( primitives == 2 ) {
		qglDrawElements( GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, indexes );
		return;
	}

	if ( primitives == 1 ) {
		R_DrawStripElements( numIndexes,  indexes, qglArrayElement );
		return;
	}

	if ( primitives == 3 ) {
		R_DrawStripElements( numIndexes,  indexes, R_ArrayElementDiscrete );
		return;
	}

	// anything else will cause no drawing
}

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

static void R_BindAnimatedImage( textureBundle_t* bundle ) {
	if ( bundle->isVideoMap ) {
		CIN_RunCinematic( bundle->videoMapHandle );
		CIN_UploadCinematic( bundle->videoMapHandle );
		return;
	}

	if ( bundle->isLightmap && ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) ) {
		GL_Bind( tr.whiteImage );
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_Bind( bundle->image[ 0 ] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	int index = idMath::FtoiFast( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind( bundle->image[ index ] );
}

//	Draws triangle outlines for debugging
static void DrawTris( shaderCommands_t* input ) {
	GL_Bind( tr.whiteImage );

	const char* s = r_trisColor->string;
	vec4_t trisColor = { 1, 1, 1, 1 };
	if ( *s == '0' && ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) ) {
		s += 2;
		if ( Q_IsHexColorString( s ) ) {
			trisColor[ 0 ] = ( ( float )( gethex( *( s ) ) * 16 + gethex( *( s + 1 ) ) ) ) / 255.00;
			trisColor[ 1 ] = ( ( float )( gethex( *( s + 2 ) ) * 16 + gethex( *( s + 3 ) ) ) ) / 255.00;
			trisColor[ 2 ] = ( ( float )( gethex( *( s + 4 ) ) * 16 + gethex( *( s + 5 ) ) ) ) / 255.00;

			if ( Q_HexColorStringHasAlpha( s ) ) {
				trisColor[ 3 ] = ( ( float )( gethex( *( s + 6 ) ) * 16 + gethex( *( s + 7 ) ) ) ) / 255.00;
			}
		}
	} else   {
		for ( int i = 0; i < 4; i++ ) {
			char* token = String::Parse3( &s );
			if ( token ) {
				trisColor[ i ] = String::Atof( token );
			} else   {
				trisColor[ i ] = 1.f;
			}
		}

		if ( !trisColor[ 3 ] ) {
			trisColor[ 3 ] = 1.f;
		}
	}

	unsigned int stateBits = 0;
	if ( trisColor[ 3 ] < 1.f ) {
		stateBits |= ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	qglColor4fv( trisColor );

	if ( r_showtris->integer == 2 ) {
		stateBits |= GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;
		GL_State( stateBits );
		qglDepthRange( 0, 0 );
	} else   {
		stateBits |= GLS_POLYMODE_LINE;
		GL_State( stateBits );
		qglEnable( GL_POLYGON_OFFSET_LINE );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	qglDisableClientState( GL_COLOR_ARRAY );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );	// padded for SIMD

	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
	qglDepthRange( 0, 1 );
	qglDisable( GL_POLYGON_OFFSET_LINE );
}

//	Draws vertex normals for debugging
static void DrawNormals( shaderCommands_t* input ) {
	GL_Bind( tr.whiteImage );
	qglColor3f( 1, 1, 1 );
	qglDepthRange( 0, 0 );		// never occluded
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	if ( r_shownormals->integer == 2 ) {
		// ydnar: light direction
		trRefEntity_t* ent = backEnd.currentEntity;
		vec3_t temp2;
		if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
			VectorSubtract( ent->e.lightingOrigin, backEnd.orient.origin, temp2 );
		} else   {
			VectorClear( temp2 );
		}
		vec3_t temp;
		temp[ 0 ] = DotProduct( temp2, backEnd.orient.axis[ 0 ] );
		temp[ 1 ] = DotProduct( temp2, backEnd.orient.axis[ 1 ] );
		temp[ 2 ] = DotProduct( temp2, backEnd.orient.axis[ 2 ] );

		qglColor3f( ent->ambientLight[ 0 ] / 255, ent->ambientLight[ 1 ] / 255, ent->ambientLight[ 2 ] / 255 );
		qglPointSize( 5 );
		qglBegin( GL_POINTS );
		qglVertex3fv( temp );
		qglEnd();
		qglPointSize( 1 );

		if ( fabs( VectorLengthSquared( ent->lightDir ) - 1.0f ) > 0.2f ) {
			qglColor3f( 1, 0, 0 );
		} else   {
			qglColor3f( ent->directedLight[ 0 ] / 255, ent->directedLight[ 1 ] / 255, ent->directedLight[ 2 ] / 255 );
		}
		qglLineWidth( 3 );
		qglBegin( GL_LINES );
		qglVertex3fv( temp );
		VectorMA( temp, 32, ent->lightDir, temp );
		qglVertex3fv( temp );
		qglEnd();
		qglLineWidth( 1 );
	} else   {
		// ydnar: normals drawing
		qglBegin( GL_LINES );
		for ( int i = 0; i < input->numVertexes; i++ ) {
			qglVertex3fv( input->xyz[ i ] );
			vec3_t temp;
			VectorMA( input->xyz[ i ], r_normallength->value, input->normal[ i ], temp );
			qglVertex3fv( temp );
		}
		qglEnd();
	}

	qglDepthRange( 0, 1 );
}

//	We must set some things up before beginning any tesselation, because a
// surface may be forced to perform a RB_End due to overflow.
void RB_BeginSurface( shader_t* shader, int fogNum ) {
	shader_t* state = shader->remappedShader ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
	tess.ATI_tess = false;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if ( tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime ) {
		tess.shaderTime = tess.shader->clampTime;
	}
}

//	output = t0 * t1 or t0 + t1
//
//	t0 = most upstream according to spec
//	t1 = most downstream according to spec
static void DrawMultitextured( shaderCommands_t* input, int stage ) {
	shaderStage_t* pStage = tess.xstages[ stage ];

	if ( tess.shader->noFog && pStage->isFogged ) {
		R_FogOn();
	} else if ( tess.shader->noFog && !pStage->isFogged )     {
		R_FogOff();	// turn it back off
	} else   {
		// make sure it's on
		R_FogOn();
	}

	GL_State( pStage->stateBits );

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if ( backEnd.viewParms.isPortal ) {
		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[ 0 ] );
	R_BindAnimatedImage( &pStage->bundle[ 0 ] );

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else   {
		GL_TexEnv( tess.shader->multitextureEnv );
	}

	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[ 1 ] );

	R_BindAnimatedImage( &pStage->bundle[ 1 ] );

	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable( GL_TEXTURE_2D );

	GL_SelectTexture( 0 );
}

void DrawMultitexturedTemp( shaderCommands_t* input ) {
	R_DrawElements( input->numIndexes, input->indexes );
}

static void RB_IterateStagesGeneric( shaderCommands_t* input ) {
	for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t* pStage = tess.xstages[ stage ];

		if ( !pStage ) {
			break;
		}

		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce ) {
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}

		//
		// do multitexture
		//
		if ( pStage->bundle[ 1 ].image[ 0 ] != 0 ) {
			DrawMultitextured( input, stage );
		} else   {
			if ( !setArraysOnce ) {
				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[ 0 ] );
			}

			//
			// set state
			//
			if ( pStage->bundle[ 0 ].vertexLightmap && r_vertexLight->integer && !r_uiFullScreen->integer && r_lightmap->integer ) {
				GL_Bind( tr.whiteImage );
			} else   {
				R_BindAnimatedImage( &pStage->bundle[ 0 ] );
			}

			// Ridah, per stage fogging (detail textures)
			if ( tess.shader->noFog && pStage->isFogged ) {
				R_FogOn();
			} else if ( tess.shader->noFog && !pStage->isFogged )     {
				R_FogOff();	// turn it back off
			} else   {	// make sure it's on
				R_FogOn();
			}

			int fadeStart = backEnd.currentEntity->e.fadeStartTime;
			if ( fadeStart ) {
				int fadeEnd = backEnd.currentEntity->e.fadeEndTime;
				if ( fadeStart > tr.refdef.time ) {
					// has not started to fade yet
					GL_State( pStage->stateBits );
				} else   {
					if ( fadeEnd < tr.refdef.time ) {	// entity faded out completely
						continue;
					}

					float alphaval = ( float )( fadeEnd - tr.refdef.time ) / ( float )( fadeEnd - fadeStart );

					unsigned int tempState = pStage->stateBits;
					// remove the current blend, and don't write to Z buffer
					tempState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE );
					// set the blend to src_alpha, dst_one_minus_src_alpha
					tempState |= ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
					GL_State( tempState );
					GL_Cull( CT_FRONT_SIDED );
					// modulate the alpha component of each vertex in the render list
					for ( int i = 0; i < tess.numVertexes; i++ ) {
						tess.svars.colors[ i ][ 0 ] *= alphaval;
						tess.svars.colors[ i ][ 1 ] *= alphaval;
						tess.svars.colors[ i ][ 2 ] *= alphaval;
						tess.svars.colors[ i ][ 3 ] *= alphaval;
					}
				}
			} else if ( r_lightmap->integer && ( pStage->bundle[ 0 ].isLightmap || pStage->bundle[ 1 ].isLightmap ) )           {
				// ydnar: lightmap stages should be GL_ONE GL_ZERO so they can be seen
				unsigned int stateBits = ( pStage->stateBits & ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) |
										 ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
				GL_State( stateBits );
			} else   {
				GL_State( pStage->stateBits );
			}

			//
			// draw
			//
			R_DrawElements( input->numIndexes, input->indexes );
		}
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[ 0 ].isLightmap || pStage->bundle[ 1 ].isLightmap || pStage->bundle[ 0 ].vertexLightmap ) ) {
			break;
		}
	}
}

void RB_IterateStagesGenericTemp( shaderCommands_t* input ) {
			//
			// draw
			//
			R_DrawElements( input->numIndexes, input->indexes );
}

//	Perform dynamic lighting with another rendering pass
static void ProjectDlightTexture() {
	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

	if ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
		// no dlights for snooper
		return;
	}

	for ( int l = 0; l < backEnd.refdef.num_dlights; l++ ) {
		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}
		float texCoordsArray[ SHADER_MAX_VERTEXES ][ 2 ];
		float* texCoords = texCoordsArray[ 0 ];
		byte colorArray[ SHADER_MAX_VERTEXES ][ 4 ];
		byte* colors = colorArray[ 0 ];

		dlight_t* dl = &backEnd.refdef.dlights[ l ];
		vec3_t origin;
		VectorCopy( dl->transformed, origin );
		float radius = dl->radius;
		float scale = 1.0f / radius;

		vec3_t floatColor;
		floatColor[ 0 ] = dl->color[ 0 ] * 255.0f;
		floatColor[ 1 ] = dl->color[ 1 ] * 255.0f;
		floatColor[ 2 ] = dl->color[ 2 ] * 255.0f;
		byte clipBits[ SHADER_MAX_VERTEXES ];
		for ( int i = 0; i < tess.numVertexes; i++, texCoords += 2, colors += 4 ) {
			backEnd.pc.c_dlightVertexes++;

			vec3_t dist;
			VectorSubtract( origin, tess.xyz[ i ], dist );
			texCoords[ 0 ] = 0.5f + dist[ 0 ] * scale;
			texCoords[ 1 ] = 0.5f + dist[ 1 ] * scale;

			int clip = 0;
			if ( texCoords[ 0 ] < 0.0f ) {
				clip |= 1;
			} else if ( texCoords[ 0 ] > 1.0f )       {
				clip |= 2;
			}
			if ( texCoords[ 1 ] < 0.0f ) {
				clip |= 4;
			} else if ( texCoords[ 1 ] > 1.0f )       {
				clip |= 8;
			}
			// modulate the strength based on the height and color
			float modulate;
			if ( dist[ 2 ] > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist[ 2 ] < -radius )       {
				clip |= 32;
				modulate = 0.0f;
			} else   {
				dist[ 2 ] = idMath::Fabs( dist[ 2 ] );
				if ( dist[ 2 ] < radius * 0.5f ) {
					modulate = 1.0f;
				} else   {
					modulate = 2.0f * ( radius - dist[ 2 ] ) * scale;
				}
			}
			clipBits[ i ] = clip;

			colors[ 0 ] = idMath::FtoiFast( floatColor[ 0 ] * modulate );
			colors[ 1 ] = idMath::FtoiFast( floatColor[ 1 ] * modulate );
			colors[ 2 ] = idMath::FtoiFast( floatColor[ 2 ] * modulate );
			colors[ 3 ] = 255;
		}

		// build a list of triangles that need light
		int numIndexes = 0;
		unsigned hitIndexes[ SHADER_MAX_INDEXES ];
		for ( int i = 0; i < tess.numIndexes; i += 3 ) {
			int a = tess.indexes[ i ];
			int b = tess.indexes[ i + 1 ];
			int c = tess.indexes[ i + 2 ];
			if ( clipBits[ a ] & clipBits[ b ] & clipBits[ c ] ) {
				continue;	// not lighted
			}
			hitIndexes[ numIndexes ] = a;
			hitIndexes[ numIndexes + 1 ] = b;
			hitIndexes[ numIndexes + 2 ] = c;
			numIndexes += 3;
		}

		if ( !numIndexes ) {
			continue;
		}

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[ 0 ] );

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		//	Creating dlight shader to allow for special blends or alternate dlight texture
		shader_t* dls = dl->shader;
		if ( dls ) {
			for ( int i = 0; i < dls->numUnfoggedPasses; i++ ) {
				shaderStage_t* stage = dls->stages[ i ];
				R_BindAnimatedImage( &dls->stages[ i ]->bundle[ 0 ] );
				GL_State( stage->stateBits | GLS_DEPTHFUNC_EQUAL );
				R_DrawElements( numIndexes, hitIndexes );
				backEnd.pc.c_totalIndexes += numIndexes;
				backEnd.pc.c_dlightIndexes += numIndexes;
			}
		} else   {
			R_FogOff();

			GL_Bind( tr.dlightImage );
			// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
			// where they aren't rendered
			if ( dl->additive ) {
				GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
			} else   {
				GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
			}
			R_DrawElements( numIndexes, hitIndexes );
			backEnd.pc.c_totalIndexes += numIndexes;
			backEnd.pc.c_dlightIndexes += numIndexes;

			// Ridah, overdraw lights several times, rather than sending
			//	multiple lights through
			for ( int i = 0; i < dl->overdraw; i++ ) {
				R_DrawElements( numIndexes, hitIndexes );
				backEnd.pc.c_totalIndexes += numIndexes;
				backEnd.pc.c_dlightIndexes += numIndexes;
			}

			R_FogOn();
		}
	}
}

//	perform all dynamic lighting with a single rendering pass
static void DynamicLightSinglePass() {
	// early out
	if ( backEnd.refdef.num_dlights == 0 ) {
		return;
	}

	// clear colors
	Com_Memset( tess.svars.colors, 0, sizeof ( tess.svars.colors ) );

	// walk light list
	for ( int l = 0; l < backEnd.refdef.num_dlights; l++ ) {
		// early out
		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;
		}

		// setup
		dlight_t* dl = &backEnd.refdef.dlights[ l ];
		vec3_t origin;
		VectorCopy( dl->transformed, origin );
		float radius = dl->radius;
		float radiusInverseCubed = dl->radiusInverseCubed;
		float intensity = dl->intensity;
		vec3_t floatColor;
		floatColor[ 0 ] = dl->color[ 0 ] * 255.0f;
		floatColor[ 1 ] = dl->color[ 1 ] * 255.0f;
		floatColor[ 2 ] = dl->color[ 2 ] * 255.0f;

		// directional lights have max intensity and washout remainder intensity
		float remainder;
		if ( dl->flags & REF_DIRECTED_DLIGHT ) {
			remainder = intensity * 0.125;
		} else   {
			remainder = 0.0f;
		}

		// illuminate vertexes
		byte* colors = tess.svars.colors[ 0 ];
		for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
			backEnd.pc.c_dlightVertexes++;

			float modulate;
			// directional dlight, origin is a directional normal
			if ( dl->flags & REF_DIRECTED_DLIGHT ) {
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct( dl->origin, tess.normal[ i ] );
				if ( tess.shader->cullType == CT_TWO_SIDED ) {
					modulate = fabs( modulate );
				}
				modulate += remainder;
			}
			// ball dlight
			else {
				vec3_t dir;
				dir[ 0 ] = radius - fabs( origin[ 0 ] - tess.xyz[ i ][ 0 ] );
				if ( dir[ 0 ] <= 0.0f ) {
					continue;
				}
				dir[ 1 ] = radius - fabs( origin[ 1 ] - tess.xyz[ i ][ 1 ] );
				if ( dir[ 1 ] <= 0.0f ) {
					continue;
				}
				dir[ 2 ] = radius - fabs( origin[ 2 ] - tess.xyz[ i ][ 2 ] );
				if ( dir[ 2 ] <= 0.0f ) {
					continue;
				}

				modulate = intensity * dir[ 0 ] * dir[ 1 ] * dir[ 2 ] * radiusInverseCubed;
			}

			// optimizations
			if ( modulate < ( 1.0f / 128.0f ) ) {
				continue;
			} else if ( modulate > 1.0f )     {
				modulate = 1.0f;
			}

			// add to color
			int color = colors[ 0 ] + idMath::FtoiFast( floatColor[ 0 ] * modulate );
			colors[ 0 ] = color > 255 ? 255 : color;
			color = colors[ 1 ] + idMath::FtoiFast( floatColor[ 1 ] * modulate );
			colors[ 1 ] = color > 255 ? 255 : color;
			color = colors[ 2 ] + idMath::FtoiFast( floatColor[ 2 ] * modulate );
			colors[ 2 ] = color > 255 ? 255 : color;
		}
	}

	// build a list of triangles that need light
	int* intColors = ( int* )tess.svars.colors;
	int numIndexes = 0;
	unsigned hitIndexes[ SHADER_MAX_INDEXES ];
	for ( int i = 0; i < tess.numIndexes; i += 3 ) {
		int a = tess.indexes[ i ];
		int b = tess.indexes[ i + 1 ];
		int c = tess.indexes[ i + 2 ];
		if ( !( intColors[ a ] | intColors[ b ] | intColors[ c ] ) ) {
			continue;
		}
		hitIndexes[ numIndexes++ ] = a;
		hitIndexes[ numIndexes++ ] = b;
		hitIndexes[ numIndexes++ ] = c;
	}

	if ( numIndexes == 0 ) {
		return;
	}

	// debug code
	//%	for( i = 0; i < numIndexes; i++ )
	//%		intColors[ hitIndexes[ i ] ] = 0x000000FF;

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	// render the dynamic light pass
	R_FogOff();
	GL_Bind( tr.whiteImage );
	GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
	R_DrawElements( numIndexes, hitIndexes );
	backEnd.pc.c_totalIndexes += numIndexes;
	backEnd.pc.c_dlightIndexes += numIndexes;
	R_FogOn();
}

//	Perform dynamic lighting with multiple rendering passes
static void DynamicLightPass() {
	// early out
	if ( backEnd.refdef.num_dlights == 0 ) {
		return;
	}

	// walk light list
	for ( int l = 0; l < backEnd.refdef.num_dlights; l++ ) {
		// early out
		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;
		}

		// clear colors
		Com_Memset( tess.svars.colors, 0, sizeof ( tess.svars.colors ) );

		// setup
		dlight_t* dl = &backEnd.refdef.dlights[ l ];
		vec3_t origin;
		VectorCopy( dl->transformed, origin );
		float radius = dl->radius;
		float radiusInverseCubed = dl->radiusInverseCubed;
		float intensity = dl->intensity;
		vec3_t floatColor;
		floatColor[ 0 ] = dl->color[ 0 ] * 255.0f;
		floatColor[ 1 ] = dl->color[ 1 ] * 255.0f;
		floatColor[ 2 ] = dl->color[ 2 ] * 255.0f;

		// directional lights have max intensity and washout remainder intensity
		float remainder;
		if ( dl->flags & REF_DIRECTED_DLIGHT ) {
			remainder = intensity * 0.125;
		} else   {
			remainder = 0.0f;
		}

		// illuminate vertexes
		byte* colors = tess.svars.colors[ 0 ];
		for ( int i = 0; i < tess.numVertexes; i++, colors += 4 ) {
			backEnd.pc.c_dlightVertexes++;

			// directional dlight, origin is a directional normal
			float modulate;
			if ( dl->flags & REF_DIRECTED_DLIGHT ) {
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct( dl->origin, tess.normal[ i ] );
				if ( tess.shader->cullType == CT_TWO_SIDED ) {
					modulate = fabs( modulate );
				}
				modulate += remainder;
			}
			// ball dlight
			else {
				vec3_t dir;
				dir[ 0 ] = radius - fabs( origin[ 0 ] - tess.xyz[ i ][ 0 ] );
				if ( dir[ 0 ] <= 0.0f ) {
					continue;
				}
				dir[ 1 ] = radius - fabs( origin[ 1 ] - tess.xyz[ i ][ 1 ] );
				if ( dir[ 1 ] <= 0.0f ) {
					continue;
				}
				dir[ 2 ] = radius - fabs( origin[ 2 ] - tess.xyz[ i ][ 2 ] );
				if ( dir[ 2 ] <= 0.0f ) {
					continue;
				}

				modulate = intensity * dir[ 0 ] * dir[ 1 ] * dir[ 2 ] * radiusInverseCubed;
			}

			// optimizations
			if ( modulate < ( 1.0f / 128.0f ) ) {
				continue;
			} else if ( modulate > 1.0f )     {
				modulate = 1.0f;
			}

			// set color
			int color = idMath::FtoiFast( floatColor[ 0 ] * modulate );
			colors[ 0 ] = color > 255 ? 255 : color;
			color = idMath::FtoiFast( floatColor[ 1 ] * modulate );
			colors[ 1 ] = color > 255 ? 255 : color;
			color = idMath::FtoiFast( floatColor[ 2 ] * modulate );
			colors[ 2 ] = color > 255 ? 255 : color;
		}

		// build a list of triangles that need light
		int* intColors = ( int* )tess.svars.colors;
		int numIndexes = 0;
		unsigned hitIndexes[ SHADER_MAX_INDEXES ];
		for ( int i = 0; i < tess.numIndexes; i += 3 ) {
			int a = tess.indexes[ i ];
			int b = tess.indexes[ i + 1 ];
			int c = tess.indexes[ i + 2 ];
			if ( !( intColors[ a ] | intColors[ b ] | intColors[ c ] ) ) {
				continue;
			}
			hitIndexes[ numIndexes++ ] = a;
			hitIndexes[ numIndexes++ ] = b;
			hitIndexes[ numIndexes++ ] = c;
		}

		if ( numIndexes == 0 ) {
			continue;
		}

		// debug code (fixme, there's a bug in this function!)
		//%	for( i = 0; i < numIndexes; i++ )
		//%		intColors[ hitIndexes[ i ] ] = 0x000000FF;

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		R_FogOff();
		GL_Bind( tr.whiteImage );
		GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		R_DrawElements( numIndexes, hitIndexes );
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
		R_FogOn();
	}
}

//	Blends a fog texture on top of everything else
static void RB_FogPass() {
	if ( tr.refdef.rdflags & RDF_SNOOPERVIEW ) {
		// no fog pass in snooper
		return;
	}

	if ( GGameType & GAME_ET ) {
		if ( tess.shader->noFog || !r_wolffog->integer ) {
			return;
		}

		// ydnar: no world, no fogging
		if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
			return;
		}
	}

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[ 0 ] );

	mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

	for ( int i = 0; i < tess.numVertexes; i++ ) {
		*( int* )&tess.svars.colors[ i ] = GGameType & GAME_ET ? fog->shader->fogParms.colorInt : fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float* )tess.svars.texcoords[ 0 ] );

	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else   {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );
}

//	Set the fog parameters for this pass.
static void SetIteratorFog() {
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
		R_FogOff();
		return;
	}

	if ( backEnd.refdef.rdflags & RDF_DRAWINGSKY ) {
		if ( glfogsettings[ FOG_SKY ].registered ) {
			R_Fog( &glfogsettings[ FOG_SKY ] );
		} else   {
			R_FogOff();
		}
		return;
	}

	if ( skyboxportal && backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) {
		if ( glfogsettings[ FOG_PORTALVIEW ].registered ) {
			R_Fog( &glfogsettings[ FOG_PORTALVIEW ] );
		} else   {
			R_FogOff();
		}
	} else   {
		if ( glfogNum > FOG_NONE ) {
			R_Fog( &glfogsettings[ FOG_CURRENT ] );
		} else   {
			R_FogOff();
		}
	}
}

void RB_StageIteratorGeneric() {
	shaderCommands_t* input = &tess;

	RB_DeformTessGeometry();

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va( "--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name ) );
	}

	// set GL fog
	SetIteratorFog();

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		// RF< so we can send the normals as an array
		qglEnableClientState( GL_NORMAL_ARRAY );
		qglEnable( GL_PN_TRIANGLES_ATI );	// ATI PN-Triangles extension
	}

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 || input->shader->multitextureEnv ) {
		setArraysOnce = false;
		qglDisableClientState( GL_COLOR_ARRAY );
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	} else   {
		setArraysOnce = true;

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[ 0 ] );
	}

	// RF, send normals only if required
	// This must be done first, since we can't change the arrays once they have been
	// locked
	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

	//
	// lock XYZ
	//
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );	// padded for SIMD
	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if ( !setArraysOnce ) {
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnableClientState( GL_COLOR_ARRAY );
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

	//
	// now do any dynamic lighting needed
	//
	//
	// now do any dynamic lighting needed
	//
	if ( GGameType & GAME_ET ) {
		if ( tess.dlightBits && tess.shader->fogPass &&
			 !( tess.shader->surfaceFlags & ( BSP46SURF_NODLIGHT | BSP46SURF_SKY ) ) ) {
			if ( r_dynamiclight->integer == 2 ) {
				DynamicLightPass();
			} else   {
				DynamicLightSinglePass();
			}
		}
	} else   {
		if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE &&
			 !( tess.shader->surfaceFlags & ( BSP46SURF_NODLIGHT | BSP46SURF_SKY ) ) ) {
			ProjectDlightTexture();
		}
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	// turn truform back off
	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglDisable( GL_PN_TRIANGLES_ATI );		// ATI PN-Triangles extension
		qglDisableClientState( GL_NORMAL_ARRAY );
	}
}

void RB_StageIteratorVertexLitTexture() {
	shaderCommands_t* input = &tess;

	//
	// compute colors
	//
	RB_CalcDiffuseColor( ( byte* )tess.svars.colors );

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va( "--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name ) );
	}

	// set GL fog
	SetIteratorFog();

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set arrays and lock
	//
	qglEnableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglEnable( GL_PN_TRIANGLES_ATI );	// ATI PN-Triangles extension
		qglEnableClientState( GL_NORMAL_ARRAY );			// RF< so we can send the normals as an array
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[ 0 ][ 0 ] );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	//
	// call special shade routine
	//
	R_BindAnimatedImage( &tess.xstages[ 0 ]->bundle[ 0 ] );
	GL_State( tess.xstages[ 0 ]->stateBits );
	R_DrawElements( input->numIndexes, input->indexes );

	//
	// now do any dynamic lighting needed
	//
	if ( GGameType & GAME_ET ) {
		if ( tess.dlightBits && tess.shader->fogPass &&
			 !( tess.shader->surfaceFlags & ( BSP46SURF_NODLIGHT | BSP46SURF_SKY ) ) ) {
			if ( r_dynamiclight->integer == 2 ) {
				DynamicLightPass();
			} else   {
				DynamicLightSinglePass();
			}
		}
	} else   {
		if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
			ProjectDlightTexture();
		}
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglDisable( GL_PN_TRIANGLES_ATI );		// ATI PN-Triangles extension
	}
}

void RB_StageIteratorLightmappedMultitexture() {
	shaderCommands_t* input = &tess;

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va( "--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name ) );
	}

	// set GL fog
	SetIteratorFog();

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set color, pointers, and lock
	//
	GL_State( GLS_DEFAULT );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglEnable( GL_PN_TRIANGLES_ATI );		// ATI PN-Triangles extension
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.constantColor255 );

	//
	// select base stage
	//
	GL_SelectTexture( 0 );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	R_BindAnimatedImage( &tess.xstages[ 0 ]->bundle[ 0 ] );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[ 0 ][ 0 ] );

	//
	// configure second stage
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else   {
		GL_TexEnv( GL_MODULATE );
	}
	if ( tess.xstages[ 0 ]->bundle[ 1 ].isLightmap && ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) ) {
		GL_Bind( tr.whiteImage );
	} else   {
		R_BindAnimatedImage( &tess.xstages[ 0 ]->bundle[ 1 ] );
	}
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[ 0 ][ 1 ] );

	//
	// lock arrays
	//
	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	qglDisable( GL_TEXTURE_2D );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	GL_SelectTexture( 0 );

	//
	// now do any dynamic lighting needed
	//
	if ( GGameType & GAME_ET ) {
		if ( tess.dlightBits && tess.shader->fogPass &&
			 !( tess.shader->surfaceFlags & ( BSP46SURF_NODLIGHT | BSP46SURF_SKY ) ) ) {
			if ( r_dynamiclight->integer == 2 ) {
				DynamicLightPass();
			} else   {
				DynamicLightSinglePass();
			}
		}
	} else   {
		if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
			ProjectDlightTexture();
		}
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglDisable( GL_PN_TRIANGLES_ATI );		// ATI PN-Triangles extension
	}
}

void RB_EndSurface() {
	shaderCommands_t* input = &tess;

	if ( input->numIndexes == 0 ) {
		return;
	}

	if ( input->indexes[ SHADER_MAX_INDEXES - 1 ] != 0 ) {
		common->Error( "RB_EndSurface() - SHADER_MAX_INDEXES hit" );
	}
	if ( input->xyz[ SHADER_MAX_VERTEXES - 1 ][ 0 ] != 0 ) {
		common->Error( "RB_EndSurface() - SHADER_MAX_VERTEXES hit" );
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	if ( GGameType & GAME_WolfSP && skyboxportal ) {
		// world
		if ( !( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) ) {
			if ( tess.currentStageIteratorFunc == RB_StageIteratorSky ) {
				// don't process these tris at all
				return;
			}
		}
		// portal sky
		else {
			if ( !drawskyboxportal ) {
				if ( !( tess.currentStageIteratorFunc == RB_StageIteratorSky ) ) {
					// /only/ process sky tris
					return;
				}
			}
		}
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris( input );
	}
	if ( r_shownormals->integer ) {
		DrawNormals( input );
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

	QGL_LogComment( "----------\n" );
}
