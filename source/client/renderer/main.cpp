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

#include "main.h"
#include "backend.h"
#include "commands.h"
#include "decals.h"
#include "world.h"
#include "surfaces.h"
#include "cvars.h"
#include "state.h"
#include "light.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/content_types.h"

struct sortedent_t {
	trRefEntity_t* ent;
	vec_t len;
};

glconfig_t glConfig;

trGlobals_t tr;

int c_brush_polys;
int c_alias_polys;

int cl_numtransvisedicts;
int cl_numtranswateredicts;

void ( * BotDrawDebugPolygonsFunc )( void ( * drawPoly )( int color, int numPoints, float* points ), int value );

float s_flipMatrix[ 16 ] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

int gl_NormalFontBase = 0;

// fog stuff
glfog_t glfogsettings[ NUM_FOGS ];
glfogType_t glfogNum = FOG_NONE;
bool fogIsOn = false;

static sortedent_t cl_transvisedicts[ MAX_ENTITIES ];
static sortedent_t cl_transwateredicts[ MAX_ENTITIES ];

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
static surfaceType_t entitySurface = SF_ENTITY;
static surfaceType_t particlesSurface = SF_PARTICLES;

void myGlMultMatrix( const float* a, const float* b, float* out ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ] +
				a [ i * 4 + 1 ] * b [ 1 * 4 + j ] +
				a [ i * 4 + 2 ] * b [ 2 * 4 + j ] +
				a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

void R_DecomposeSort( unsigned sort, int* entityNum, shader_t** shader,
	int* fogNum, int* dlightMap, int* frontFace, int* atiTess ) {
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & ( MAX_SHADERS - 1 ) ];
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & 1023;
	if ( GGameType & GAME_ET ) {
		*frontFace = ( sort >> QSORT_FRONTFACE_SHIFT ) & 1;
		*dlightMap = sort & 1;
	} else {
		*frontFace = 0;
		*dlightMap = sort & 3;
	}
	*atiTess = ( sort >> QSORT_ATI_TESS_SHIFT ) & 1;
}

static void R_SetFrameFog() {
	// Arnout: new style global fog transitions
	if ( tr.world->globalFogTransEndTime ) {
		if ( tr.world->globalFogTransEndTime >= tr.refdef.time ) {
			int fadeTime = tr.world->globalFogTransEndTime - tr.world->globalFogTransStartTime;
			float lerpPos = ( float )( tr.refdef.time - tr.world->globalFogTransStartTime ) / ( float )fadeTime;
			if ( lerpPos > 1 ) {
				lerpPos = 1;
			}

			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 0 ] = tr.world->globalTransStartFog[ 0 ] + ( ( tr.world->globalTransEndFog[ 0 ] - tr.world->globalTransStartFog[ 0 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 1 ] = tr.world->globalTransStartFog[ 1 ] + ( ( tr.world->globalTransEndFog[ 1 ] - tr.world->globalTransStartFog[ 1 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 2 ] = tr.world->globalTransStartFog[ 2 ] + ( ( tr.world->globalTransEndFog[ 2 ] - tr.world->globalTransStartFog[ 2 ] ) * lerpPos );

			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.colorInt = ColorBytes4( tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 0 ] * tr.identityLight,
				tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 1 ] * tr.identityLight,
				tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 2 ] * tr.identityLight, 1.0 );

			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque = tr.world->globalTransStartFog[ 3 ] + ( ( tr.world->globalTransEndFog[ 3 ] - tr.world->globalTransStartFog[ 3 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.tcScale = 1.0f / ( tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque * 8 );
		} else {
			// transition complete
			VectorCopy( tr.world->globalTransEndFog, tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color );
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.colorInt = ColorBytes4( tr.world->globalTransEndFog[ 0 ] * tr.identityLight,
				tr.world->globalTransEndFog[ 1 ] * tr.identityLight,
				tr.world->globalTransEndFog[ 2 ] * tr.identityLight, 1.0 );
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque = tr.world->globalTransEndFog[ 3 ];
			tr.world->fogs[ tr.world->globalFog ].shader->fogParms.tcScale = 1.0f / ( tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque * 8 );

			tr.world->globalFogTransEndTime = 0;
		}
	}

	if ( r_speeds->integer == 5 ) {
		if ( !glfogsettings[ FOG_TARGET ].registered ) {
			common->Printf( "no fog - calc zFar: %0.1f\n", tr.viewParms.zFar );
			return;
		}
	}

	// DHM - Nerve :: If fog is not valid, don't use it
	if ( !glfogsettings[ FOG_TARGET ].registered ) {
		return;
	}

	// still fading
	if ( glfogsettings[ FOG_TARGET ].finishTime && glfogsettings[ FOG_TARGET ].finishTime >= tr.refdef.time ) {
		// transitioning from density to distance
		if ( glfogsettings[ FOG_LAST ].mode == GL_EXP && glfogsettings[ FOG_TARGET ].mode == GL_LINEAR ) {
			// for now just fast transition to the target when dissimilar fogs are
			Com_Memcpy( &glfogsettings[ FOG_CURRENT ], &glfogsettings[ FOG_TARGET ], sizeof ( glfog_t ) );
			glfogsettings[ FOG_TARGET ].finishTime = 0;
		}
		// transitioning from distance to density
		else if ( glfogsettings[ FOG_LAST ].mode == GL_LINEAR && glfogsettings[ FOG_TARGET ].mode == GL_EXP ) {
			Com_Memcpy( &glfogsettings[ FOG_CURRENT ], &glfogsettings[ FOG_TARGET ], sizeof ( glfog_t ) );
			glfogsettings[ FOG_TARGET ].finishTime = 0;
		}
		// transitioning like fog modes
		else {
			int fadeTime = glfogsettings[ FOG_TARGET ].finishTime - glfogsettings[ FOG_TARGET ].startTime;
			if ( fadeTime <= 0 ) {
				fadeTime = 1;	// avoid divide by zero
			}
			float lerpPos = ( float )( tr.refdef.time - glfogsettings[ FOG_TARGET ].startTime ) / ( float )fadeTime;
			if ( lerpPos > 1 ) {
				lerpPos = 1;
			}

			// lerp near/far
			glfogsettings[ FOG_CURRENT ].start = glfogsettings[ FOG_LAST ].start + ( ( glfogsettings[ FOG_TARGET ].start - glfogsettings[ FOG_LAST ].start ) * lerpPos );
			glfogsettings[ FOG_CURRENT ].end = glfogsettings[ FOG_LAST ].end + ( ( glfogsettings[ FOG_TARGET ].end - glfogsettings[ FOG_LAST ].end ) * lerpPos );

			// lerp color
			glfogsettings[ FOG_CURRENT ].color[ 0 ] = glfogsettings[ FOG_LAST ].color[ 0 ] + ( ( glfogsettings[ FOG_TARGET ].color[ 0 ] - glfogsettings[ FOG_LAST ].color[ 0 ] ) * lerpPos );
			glfogsettings[ FOG_CURRENT ].color[ 1 ] = glfogsettings[ FOG_LAST ].color[ 1 ] + ( ( glfogsettings[ FOG_TARGET ].color[ 1 ] - glfogsettings[ FOG_LAST ].color[ 1 ] ) * lerpPos );
			glfogsettings[ FOG_CURRENT ].color[ 2 ] = glfogsettings[ FOG_LAST ].color[ 2 ] + ( ( glfogsettings[ FOG_TARGET ].color[ 2 ] - glfogsettings[ FOG_LAST ].color[ 2 ] ) * lerpPos );

			glfogsettings[ FOG_CURRENT ].density = glfogsettings[ FOG_TARGET ].density;
			glfogsettings[ FOG_CURRENT ].mode = glfogsettings[ FOG_TARGET ].mode;
			glfogsettings[ FOG_CURRENT ].registered = true;

			// if either fog in the transition clears the screen, clear the background this frame to avoid hall of mirrors
			glfogsettings[ FOG_CURRENT ].clearscreen = ( glfogsettings[ FOG_TARGET ].clearscreen || glfogsettings[ FOG_LAST ].clearscreen );
		}

		glfogsettings[ FOG_CURRENT ].dirty = 1;
	} else {
		// probably usually not necessary to copy the whole thing.
		// potential FIXME: since this is the most common occurance, diff first and only set changes
		Com_Memcpy( &glfogsettings[ FOG_CURRENT ], &glfogsettings[ FOG_TARGET ], sizeof ( glfog_t ) );
		glfogsettings[ FOG_CURRENT ].dirty = 0;
	}

	// shorten the far clip if the fog opaque distance is closer than the procedural farcip dist

	if ( glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR ) {
		if ( glfogsettings[ FOG_CURRENT ].end < tr.viewParms.zFar ) {
			tr.viewParms.zFar = glfogsettings[ FOG_CURRENT ].end;
		}
		if ( GGameType & GAME_WolfSP && backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
			tr.viewParms.zFar += 1000;	// zfar out slightly further for snooper.  this works fine with our maps, but could be 'funky' with later maps
		}
	}
//	else
//		glfogsettings[FOG_CURRENT].end = 5;

	if ( r_speeds->integer == 5 ) {
		if ( glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR ) {
			common->Printf( "farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n", glfogsettings[ FOG_CURRENT ].density, tr.viewParms.zFar, glfogsettings[ FOG_CURRENT ].end );
		} else {
			common->Printf( "density fog - den: %0.6f  calc zFar: %0.1f  fog zFar: %0.1f\n", glfogsettings[ FOG_CURRENT ].density, tr.viewParms.zFar, glfogsettings[ FOG_CURRENT ].end );
		}
	}
}

static void SetFarClip() {
	if ( !( GGameType & GAME_Tech3 ) ) {
		tr.viewParms.zFar = 4096;
		return;
	}

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
	}

	//	This lets you use r_zfar from the command line to experiment with different
	// distances, but setting it back to 0 uses the map (or procedurally generated) default
	if ( r_zfar->value ) {
		tr.viewParms.zFar = r_zfar->integer;
		R_SetFrameFog();
		if ( r_speeds->integer == 5 ) {
			common->Printf( "r_zfar value forcing farclip at: %f\n", tr.viewParms.zFar );
		}
		return;
	}

	//
	// set far clipping planes dynamically
	//
	float farthestCornerDistance = 0;
	for ( int i = 0; i < 8; i++ ) {
		vec3_t v;
		if ( i & 1 ) {
			v[ 0 ] = tr.viewParms.visBounds[ 0 ][ 0 ];
		} else {
			v[ 0 ] = tr.viewParms.visBounds[ 1 ][ 0 ];
		}

		if ( i & 2 ) {
			v[ 1 ] = tr.viewParms.visBounds[ 0 ][ 1 ];
		} else {
			v[ 1 ] = tr.viewParms.visBounds[ 1 ][ 1 ];
		}

		if ( i & 4 ) {
			v[ 2 ] = tr.viewParms.visBounds[ 0 ][ 2 ];
		} else {
			v[ 2 ] = tr.viewParms.visBounds[ 1 ][ 2 ];
		}

		vec3_t vecTo;
		VectorSubtract( v, tr.viewParms.orient.origin, vecTo );

		float distance = vecTo[ 0 ] * vecTo[ 0 ] + vecTo[ 1 ] * vecTo[ 1 ] + vecTo[ 2 ] * vecTo[ 2 ];

		if ( distance > farthestCornerDistance ) {
			farthestCornerDistance = distance;
		}
	}
	tr.viewParms.zFar = sqrt( farthestCornerDistance );

	// ydnar: add global q3 fog
	if ( tr.world != NULL && tr.world->globalFog >= 0 && tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque < tr.viewParms.zFar ) {
		tr.viewParms.zFar = tr.world->fogs[ tr.world->globalFog ].shader->fogParms.depthForOpaque;
	}

	R_SetFrameFog();
}

//	Setup that culling frustum planes for the current view
static void R_SetupFrustum() {
	float ang = DEG2RAD( tr.viewParms.fovX ) * 0.5f;
	float xs = sin( ang );
	float xc = cos( ang );

	VectorScale( tr.viewParms.orient.axis[ 0 ], xs, tr.viewParms.frustum[ 0 ].normal );
	VectorMA( tr.viewParms.frustum[ 0 ].normal, xc, tr.viewParms.orient.axis[ 1 ], tr.viewParms.frustum[ 0 ].normal );

	VectorScale( tr.viewParms.orient.axis[ 0 ], xs, tr.viewParms.frustum[ 1 ].normal );
	VectorMA( tr.viewParms.frustum[ 1 ].normal, -xc, tr.viewParms.orient.axis[ 1 ], tr.viewParms.frustum[ 1 ].normal );

	ang = DEG2RAD( tr.viewParms.fovY ) * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.orient.axis[ 0 ], xs, tr.viewParms.frustum[ 2 ].normal );
	VectorMA( tr.viewParms.frustum[ 2 ].normal, xc, tr.viewParms.orient.axis[ 2 ], tr.viewParms.frustum[ 2 ].normal );

	VectorScale( tr.viewParms.orient.axis[ 0 ], xs, tr.viewParms.frustum[ 3 ].normal );
	VectorMA( tr.viewParms.frustum[ 3 ].normal, -xc, tr.viewParms.orient.axis[ 2 ], tr.viewParms.frustum[ 3 ].normal );

	for ( int i = 0; i < 4; i++ ) {
		tr.viewParms.frustum[ i ].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[ i ].dist = DotProduct( tr.viewParms.orient.origin, tr.viewParms.frustum[ i ].normal );
		SetPlaneSignbits( &tr.viewParms.frustum[ i ] );
	}

	if ( GGameType & GAME_ET ) {
		// ydnar: farplane (testing! use farplane for real)
		VectorScale( tr.viewParms.orient.axis[ 0 ], -1, tr.viewParms.frustum[ 4 ].normal );
		tr.viewParms.frustum[ 4 ].dist = DotProduct( tr.viewParms.orient.origin, tr.viewParms.frustum[ 4 ].normal ) - tr.viewParms.zFar;
		tr.viewParms.frustum[ 4 ].type = PLANE_NON_AXIAL;
		SetPlaneSignbits( &tr.viewParms.frustum[ 4 ] );
	}
}

static void R_SetupProjection() {
	// dynamically compute far clip plane distance
	SetFarClip();

	if ( GGameType & GAME_ET ) {
		// ydnar: set frustum planes (this happens here because of zfar manipulation)
		R_SetupFrustum();
	}

	//
	// set up projection matrix
	//
	float zNear = r_znear->value;
	float zFar  = tr.viewParms.zFar;

	// ydnar: high fov values let players see through walls
	// solution is to move z near plane inward, which decreases zbuffer precision
	// but if a player wants to play with fov 160, then they can deal with z-fighting
	// assume fov 90 = scale 1, fov 180 = scale 1/16th
	if ( GGameType & GAME_ET && tr.refdef.fov_x > 90.0f ) {
		zNear /= ( ( tr.refdef.fov_x - 90.0f ) * 0.09f + 1.0f );
	}

	float ymax = zNear * tan( DEG2RAD( tr.viewParms.fovY * 0.5f ) );
	float ymin = -ymax;

	float xmax = zNear * tan( DEG2RAD( tr.viewParms.fovX * 0.5f ) );
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;
	float depth = zFar - zNear;

	tr.viewParms.projectionMatrix[ 0 ] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[ 4 ] = 0;
	tr.viewParms.projectionMatrix[ 8 ] = ( xmax + xmin ) / width;	// normally 0
	tr.viewParms.projectionMatrix[ 12 ] = 0;

	tr.viewParms.projectionMatrix[ 1 ] = 0;
	tr.viewParms.projectionMatrix[ 5 ] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[ 9 ] = ( ymax + ymin ) / height;	// normally 0
	tr.viewParms.projectionMatrix[ 13 ] = 0;

	tr.viewParms.projectionMatrix[ 2 ] = 0;
	tr.viewParms.projectionMatrix[ 6 ] = 0;
	tr.viewParms.projectionMatrix[ 10 ] = -( zFar + zNear ) / depth;
	tr.viewParms.projectionMatrix[ 14 ] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[ 3 ] = 0;
	tr.viewParms.projectionMatrix[ 7 ] = 0;
	tr.viewParms.projectionMatrix[ 11 ] = -1;
	tr.viewParms.projectionMatrix[ 15 ] = 0;
}

//	Sets up the modelview matrix for a given viewParm
static void R_RotateForViewer() {
	Com_Memset( &tr.orient, 0, sizeof ( tr.orient ) );
	tr.orient.axis[ 0 ][ 0 ] = 1;
	tr.orient.axis[ 1 ][ 1 ] = 1;
	tr.orient.axis[ 2 ][ 2 ] = 1;
	VectorCopy( tr.viewParms.orient.origin, tr.orient.viewOrigin );

	// transform by the camera placement
	vec3_t origin;
	VectorCopy( tr.viewParms.orient.origin, origin );

	float viewerMatrix[ 16 ];

	viewerMatrix[ 0 ] = tr.viewParms.orient.axis[ 0 ][ 0 ];
	viewerMatrix[ 4 ] = tr.viewParms.orient.axis[ 0 ][ 1 ];
	viewerMatrix[ 8 ] = tr.viewParms.orient.axis[ 0 ][ 2 ];
	viewerMatrix[ 12 ] = -origin[ 0 ] * viewerMatrix[ 0 ] + -origin[ 1 ] * viewerMatrix[ 4 ] + -origin[ 2 ] * viewerMatrix[ 8 ];

	viewerMatrix[ 1 ] = tr.viewParms.orient.axis[ 1 ][ 0 ];
	viewerMatrix[ 5 ] = tr.viewParms.orient.axis[ 1 ][ 1 ];
	viewerMatrix[ 9 ] = tr.viewParms.orient.axis[ 1 ][ 2 ];
	viewerMatrix[ 13 ] = -origin[ 0 ] * viewerMatrix[ 1 ] + -origin[ 1 ] * viewerMatrix[ 5 ] + -origin[ 2 ] * viewerMatrix[ 9 ];

	viewerMatrix[ 2 ] = tr.viewParms.orient.axis[ 2 ][ 0 ];
	viewerMatrix[ 6 ] = tr.viewParms.orient.axis[ 2 ][ 1 ];
	viewerMatrix[ 10 ] = tr.viewParms.orient.axis[ 2 ][ 2 ];
	viewerMatrix[ 14 ] = -origin[ 0 ] * viewerMatrix[ 2 ] + -origin[ 1 ] * viewerMatrix[ 6 ] + -origin[ 2 ] * viewerMatrix[ 10 ];

	viewerMatrix[ 3 ] = 0;
	viewerMatrix[ 7 ] = 0;
	viewerMatrix[ 11 ] = 0;
	viewerMatrix[ 15 ] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, tr.orient.modelMatrix );

	tr.viewParms.world = tr.orient;
}

void R_LocalNormalToWorld( vec3_t local, vec3_t world ) {
	world[ 0 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 0 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 0 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 0 ];
	world[ 1 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 1 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 1 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 1 ];
	world[ 2 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 2 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 2 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 2 ];
}

void R_LocalPointToWorld( vec3_t local, vec3_t world ) {
	world[ 0 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 0 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 0 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 0 ] + tr.orient.origin[ 0 ];
	world[ 1 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 1 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 1 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 1 ] + tr.orient.origin[ 1 ];
	world[ 2 ] = local[ 0 ] * tr.orient.axis[ 0 ][ 2 ] + local[ 1 ] * tr.orient.axis[ 1 ][ 2 ] + local[ 2 ] * tr.orient.axis[ 2 ][ 2 ] + tr.orient.origin[ 2 ];
}

void R_TransformModelToClip( const vec3_t src, const float* modelMatrix, const float* projectionMatrix,
	vec4_t eye, vec4_t dst ) {
	for ( int i = 0; i < 4; i++ ) {
		eye[ i ] =
			src[ 0 ] * modelMatrix[ i + 0 * 4 ] +
			src[ 1 ] * modelMatrix[ i + 1 * 4 ] +
			src[ 2 ] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( int i = 0; i < 4; i++ ) {
		dst[ i ] =
			eye[ 0 ] * projectionMatrix[ i + 0 * 4 ] +
			eye[ 1 ] * projectionMatrix[ i + 1 * 4 ] +
			eye[ 2 ] * projectionMatrix[ i + 2 * 4 ] +
			eye[ 3 ] * projectionMatrix[ i + 3 * 4 ];
	}
}

void R_TransformClipToWindow( const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window ) {
	normalized[ 0 ] = clip[ 0 ] / clip[ 3 ];
	normalized[ 1 ] = clip[ 1 ] / clip[ 3 ];
	normalized[ 2 ] = ( clip[ 2 ] + clip[ 3 ] ) / ( 2 * clip[ 3 ] );

	window[ 0 ] = 0.5f * ( 1.0f + normalized[ 0 ] ) * view->viewportWidth;
	window[ 1 ] = 0.5f * ( 1.0f + normalized[ 1 ] ) * view->viewportHeight;
	window[ 2 ] = normalized[ 2 ];

	window[ 0 ] = ( int )( window[ 0 ] + 0.5 );
	window[ 1 ] = ( int )( window[ 1 ] + 0.5 );
}

//	Generates an orientation for an entity and viewParms
//	Does NOT produce any GL calls
//	Called by both the front end and the back end
void R_RotateForEntity( const trRefEntity_t* ent, const viewParms_t* viewParms,
	orientationr_t* orient ) {
	if ( ent->e.reType != RT_MODEL ) {
		*orient = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, orient->origin );

	idRenderModel* model = R_GetModelByHandle( ent->e.hModel );
	if ( model->type == MOD_SPRITE || model->type == MOD_SPRITE2 ) {
		//	Sprites handle orientation in drawing code.
		AxisClear( orient->axis );
	} else if ( ( GGameType & GAME_Hexen2 ) && ( model->q1_flags & H2EF_FACE_VIEW ) ) {
		float fvaxis[ 3 ][ 3 ];

		VectorSubtract( viewParms->orient.origin, ent->e.origin, fvaxis[ 0 ] );
		VectorNormalize( fvaxis[ 0 ] );

		if ( fvaxis[ 0 ][ 1 ] == 0 && fvaxis[ 0 ][ 0 ] == 0 ) {
			fvaxis[ 1 ][ 0 ] = 0;
			fvaxis[ 1 ][ 1 ] = 1;
			fvaxis[ 1 ][ 2 ] = 0;
			fvaxis[ 2 ][ 1 ] = 0;
			fvaxis[ 2 ][ 2 ] = 0;
			if ( fvaxis[ 0 ][ 2 ] > 0 ) {
				fvaxis[ 2 ][ 0 ] = -1;
			} else {
				fvaxis[ 2 ][ 0 ] = 1;
			}
		} else {
			fvaxis[ 2 ][ 0 ] = 0;
			fvaxis[ 2 ][ 1 ] = 0;
			fvaxis[ 2 ][ 2 ] = 1;
			CrossProduct( fvaxis[ 2 ], fvaxis[ 0 ], fvaxis[ 1 ] );
			VectorNormalize( fvaxis[ 1 ] );
			CrossProduct( fvaxis[ 0 ], fvaxis[ 1 ], fvaxis[ 2 ] );
			VectorNormalize( fvaxis[ 2 ] );
		}

		MatrixMultiply( ent->e.axis, fvaxis, orient->axis );
	} else {
		AxisCopy( ent->e.axis, orient->axis );
	}

	float glMatrix[ 16 ];

	glMatrix[ 0 ] = orient->axis[ 0 ][ 0 ];
	glMatrix[ 4 ] = orient->axis[ 1 ][ 0 ];
	glMatrix[ 8 ] = orient->axis[ 2 ][ 0 ];
	glMatrix[ 12 ] = orient->origin[ 0 ];

	glMatrix[ 1 ] = orient->axis[ 0 ][ 1 ];
	glMatrix[ 5 ] = orient->axis[ 1 ][ 1 ];
	glMatrix[ 9 ] = orient->axis[ 2 ][ 1 ];
	glMatrix[ 13 ] = orient->origin[ 1 ];

	glMatrix[ 2 ] = orient->axis[ 0 ][ 2 ];
	glMatrix[ 6 ] = orient->axis[ 1 ][ 2 ];
	glMatrix[ 10 ] = orient->axis[ 2 ][ 2 ];
	glMatrix[ 14 ] = orient->origin[ 2 ];

	glMatrix[ 3 ] = 0;
	glMatrix[ 7 ] = 0;
	glMatrix[ 11 ] = 0;
	glMatrix[ 15 ] = 1;

	myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, orient->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	vec3_t delta;
	VectorSubtract( viewParms->orient.origin, orient->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		//	In Hexen 2 axis can have different scales.
		for ( int i = 0; i < 3; i++ ) {
			float axisLength = VectorLength( ent->e.axis[ i ] );
			if ( !axisLength ) {
				axisLength = 0;
			} else {
				axisLength = 1.0f / axisLength;
			}
			orient->viewOrigin[ i ] = DotProduct( delta, orient->axis[ i ] ) * axisLength;
		}
	} else {
		orient->viewOrigin[ 0 ] = DotProduct( delta, orient->axis[ 0 ] );
		orient->viewOrigin[ 1 ] = DotProduct( delta, orient->axis[ 1 ] );
		orient->viewOrigin[ 2 ] = DotProduct( delta, orient->axis[ 2 ] );
	}
}

//	Returns CULL_IN, CULL_CLIP, or CULL_OUT
int R_CullLocalBox( vec3_t bounds[ 2 ] ) {
	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// transform into world space
	vec3_t transformed[ 8 ];
	for ( int i = 0; i < 8; i++ ) {
		vec3_t v;
		v[ 0 ] = bounds[ i & 1 ][ 0 ];
		v[ 1 ] = bounds[ ( i >> 1 ) & 1 ][ 1 ];
		v[ 2 ] = bounds[ ( i >> 2 ) & 1 ][ 2 ];

		VectorCopy( tr.orient.origin, transformed[ i ] );
		VectorMA( transformed[ i ], v[ 0 ], tr.orient.axis[ 0 ], transformed[ i ] );
		VectorMA( transformed[ i ], v[ 1 ], tr.orient.axis[ 1 ], transformed[ i ] );
		VectorMA( transformed[ i ], v[ 2 ], tr.orient.axis[ 2 ], transformed[ i ] );
	}

	// check against frustum planes
	bool anyBack = false;
	for ( int i = 0; i < ( GGameType & GAME_ET ? 5 : 4 ); i++ ) {
		cplane_t* frust = &tr.viewParms.frustum[ i ];

		bool front = false;
		bool back = false;
		for ( int j = 0; j < 8; j++ ) {
			float dist = DotProduct( transformed[ j ], frust->normal );
			if ( dist > frust->dist ) {
				front = true;
				if ( back ) {
					break;		// a point is in front
				}
			} else {
				back = true;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
}

int R_CullPointAndRadius( vec3_t pt, float radius ) {
	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// check against frustum planes
	bool mightBeClipped = false;
	for ( int i = 0; i < ( GGameType & GAME_ET ? 5 : 4 ); i++ ) {
		cplane_t* frust = &tr.viewParms.frustum[ i ];

		float dist = DotProduct( pt, frust->normal ) - frust->dist;
		if ( dist < -radius ) {
			return CULL_OUT;
		} else if ( dist <= radius ) {
			mightBeClipped = true;
		}
	}

	if ( mightBeClipped ) {
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}

int R_CullLocalPointAndRadius( vec3_t pt, float radius ) {
	vec3_t transformed;

	R_LocalPointToWorld( pt, transformed );

	return R_CullPointAndRadius( transformed, radius );
}

void R_AddDrawSurfOld( surfaceType_t* surface, shader_t* shader, int fogIndex,
	int dlightMap, int frontFace, int atiTess, int forcedSortIndex ) {
	// instead of checking for overflow, we just mask the index
	// so it wraps around
	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[ index ].sort =
		((unsigned long long)forcedSortIndex << QSORT_FORCED_SORT_SHIFT) |
		( shader->sortedIndex << QSORT_SHADERNUM_SHIFT ) |
		tr.shiftedEntityNum | ( atiTess << QSORT_ATI_TESS_SHIFT ) |
		( fogIndex << QSORT_FOGNUM_SHIFT ) | dlightMap |
		( GGameType & GAME_ET ? ( frontFace << QSORT_FRONTFACE_SHIFT ) : 0 );
	tr.refdef.drawSurfs[ index ].surface = surface;
	tr.refdef.numDrawSurfs++;
}

void R_AddDrawSurf( idSurface* surface, shader_t* shader, int fogIndex,
	int dlightMap, int frontFace, int atiTess, int forcedSortIndex ) {
	R_AddDrawSurfOld( &surface->data->surfaceType, shader, fogIndex, dlightMap, frontFace, atiTess, forcedSortIndex );
}

//	See if a sprite is inside a fog volume
static int R_SpriteFogNum( trRefEntity_t* ent ) {
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( int i = 1; i < tr.world->numfogs; i++ ) {
		mbrush46_fog_t* fog = &tr.world->fogs[ i ];
		int j;
		for ( j = 0; j < 3; j++ ) {
			if ( ent->e.origin[ j ] - ent->e.radius >= fog->bounds[ 1 ][ j ] ) {
				break;
			}
			if ( ent->e.origin[ j ] + ent->e.radius <= fog->bounds[ 0 ][ j ] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

static void R_AddBadModelSurface( trRefEntity_t* ent ) {
	if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}
	R_AddDrawSurfOld( &entitySurface, tr.defaultShader, 0, 0, 0, ATI_TESS_NONE, 0 );
}

static void R_AddModelSurfaces( idRenderModel* model, trRefEntity_t* ent, int forcedSortIndex ) {
	switch ( model->type ) {
	case MOD_BAD:
		R_AddBadModelSurface( ent );
		break;

	case MOD_MESH1:
		R_AddMdlSurfaces( ent, forcedSortIndex );
		break;

	case MOD_BRUSH29:
		R_DrawBrushModelQ1( ent, forcedSortIndex );
		break;

	case MOD_SPRITE:
		R_AddSprSurfaces( ent, forcedSortIndex );
		break;

	case MOD_MESH2:
		R_AddMd2Surfaces( ent, forcedSortIndex );
		break;

	case MOD_BRUSH38:
		R_DrawBrushModelQ2( ent, forcedSortIndex );
		break;

	case MOD_SPRITE2:
		R_AddSp2Surfaces( ent, forcedSortIndex );
		break;

	case MOD_MESH3:
		R_AddMD3Surfaces( ent );
		break;

	case MOD_MD4:
		R_AddAnimSurfaces( ent );
		break;

	case MOD_MDC:
		R_AddMDCSurfaces( ent );
		break;

	case MOD_MDS:
		R_AddMdsAnimSurfaces( ent );
		break;

	case MOD_MDM:
		R_MDM_AddAnimSurfaces( ent );
		break;

	case MOD_BRUSH46:
		R_AddBrushModelSurfaces( ent );
		break;

	default:
		common->Error( "R_AddEntitySurfaces: Bad modeltype" );
	}
}

static void R_AddEntitySurfaces() {
	cl_numtransvisedicts = 0;
	cl_numtranswateredicts = 0;

	if ( !r_drawentities->integer ) {
		return;
	}

	for ( tr.currentEntityNum = 0; tr.currentEntityNum < tr.refdef.num_entities; tr.currentEntityNum++ ) {
		tr.currentEntity = &tr.refdef.entities[ tr.currentEntityNum ];

		trRefEntity_t* ent = tr.currentEntity;
		ent->dlightBits = 0;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		int forcedSortIndex = tr.currentEntity->e.renderfx & RF_TRANSLUCENT ? 1 : 0;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		//
		if ( ( ent->e.renderfx & RF_FIRST_PERSON ) && tr.viewParms.isPortal ) {
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType ) {
		case RT_PORTALSURFACE:
			break;		// don't draw anything

		case RT_SPRITE:
		case RT_SPLASH:
		case RT_BEAM:
		case RT_BEAM_Q2:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_CORE_TAPER:
		case RT_RAIL_RINGS:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
				continue;
			}
			R_AddDrawSurfOld( &entitySurface, R_GetShaderByHandle( ent->e.customShader ), R_SpriteFogNum( ent ), 0, 0, ATI_TESS_NONE, 0 );
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity( ent, &tr.viewParms, &tr.orient );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if ( !tr.currentModel ) {
				R_AddDrawSurfOld( &entitySurface, tr.defaultShader, 0, 0, 0, ATI_TESS_NONE, 0 );
			} else {
				if ( GGameType & GAME_Hexen2 ) {
					if ( ( ent->e.renderfx & RF_TRANSLUCENT ) || R_MdlHasHexen2Transparency( tr.currentModel ) ) {
						mbrush29_leaf_t* pLeaf = Mod_PointInLeafQ1( tr.currentEntity->e.origin, tr.worldModel );
						if ( pLeaf->contents != BSP29CONTENTS_WATER ) {
							cl_transvisedicts[ cl_numtransvisedicts++ ].ent = tr.currentEntity;
						} else {
							cl_transwateredicts[ cl_numtranswateredicts++ ].ent = tr.currentEntity;
						}
						break;
					}
				}
				R_AddModelSurfaces( tr.currentModel, ent, forcedSortIndex );
			}
			break;

		default:
			common->Error( "R_AddEntitySurfaces: Bad reType" );
		}
	}
}

static int transCompare( const void* arg1, const void* arg2 ) {
	const sortedent_t* a1 = ( const sortedent_t* )arg1;
	const sortedent_t* a2 = ( const sortedent_t* )arg2;
	return ( a2->len - a1->len );	// Sorted in reverse order.  Neat, huh?
}

static void R_DrawTransEntitiesOnList( bool inwater, int& forcedSortIndex ) {
	sortedent_t* theents = inwater ? cl_transwateredicts : cl_transvisedicts;
	int numents = inwater ? cl_numtranswateredicts : cl_numtransvisedicts;

	for ( int i = 0; i < numents; i++ ) {
		vec3_t result;
		VectorSubtract( theents[ i ].ent->e.origin, tr.viewParms.orient.origin, result );
		theents[ i ].len = result[ 0 ] * result[ 0 ] + result[ 1 ] * result[ 1 ] + result[ 2 ] * result[ 2 ];
	}

	qsort( ( void* )theents, numents, sizeof ( sortedent_t ), transCompare );
	// Add in BETTER sorting here

	for ( int i = 0; i < numents; i++ ) {
		tr.currentEntity = theents[ i ].ent;
		tr.currentModel = R_GetModelByHandle( tr.currentEntity->e.hModel );
		tr.currentEntityNum = tr.currentEntity - tr.refdef.entities;
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orient );
		R_AddModelSurfaces( tr.currentModel, tr.currentEntity, forcedSortIndex++ );
	}
}

//	Adds all the scene's polys into this view's drawsurf list
static void R_AddPolygonSurfaces() {
	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	srfPoly_t* poly = tr.refdef.polys;
	for ( int i = 0; i < tr.refdef.numPolys; i++, poly++ ) {
		shader_t* sh = R_GetShaderByHandle( poly->hShader );
		R_AddDrawSurfOld( ( surfaceType_t* )poly, sh, poly->fogIndex, false, 0, ATI_TESS_NONE, 0 );
	}
}

static void R_AddPolygonBufferSurfaces() {
	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	srfPolyBuffer_t* polybuffer = tr.refdef.polybuffers;
	for ( int i = 0; i < tr.refdef.numPolyBuffers; i++, polybuffer++ ) {
		shader_t* sh = R_GetShaderByHandle( polybuffer->pPolyBuffer->shader );
		R_AddDrawSurfOld( ( surfaceType_t* )polybuffer, sh, polybuffer->fogIndex, 0, 0, ATI_TESS_NONE, 0 );
	}
}

static void R_GenerateDrawSurfs() {
	if ( !( GGameType & GAME_Tech3 ) ) {
		R_SetupProjection();
	}

	if ( GGameType & GAME_ET ) {
		// set the projection matrix (and view frustum) here
		// first with max or fog distance so we can have proper
		// arbitrary frustum farplane culling optimization
		if ( tr.refdef.rdflags & RDF_NOWORLDMODEL || tr.world == NULL ) {
			VectorSet( tr.viewParms.visBounds[ 0 ], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
			VectorSet( tr.viewParms.visBounds[ 1 ], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
		} else {
			VectorCopy( tr.world->nodes->mins, tr.viewParms.visBounds[ 0 ] );
			VectorCopy( tr.world->nodes->maxs, tr.viewParms.visBounds[ 1 ] );
		}

		R_SetupProjection();

		R_CullDecalProjectors();
		R_CullDlights();
	}

	if ( GGameType & GAME_QuakeHexen ) {
		R_DrawWorldQ1();
	} else if ( GGameType & GAME_Quake2 ) {
		R_DrawWorldQ2();
	} else if ( GGameType & GAME_Tech3 ) {
		R_AddWorldSurfaces();
	}

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	if ( GGameType & GAME_Tech3 ) {
		R_SetupProjection();
	}

	R_AddEntitySurfaces();

	R_AddPolygonSurfaces();

	R_AddPolygonBufferSurfaces();

	if ( !( GGameType & GAME_Tech3 ) ) {
		R_AddDrawSurfOld( &particlesSurface, tr.particleShader, 0, false, false, ATI_TESS_NONE, 2 );
	}

	int forcedSortIndex = 3;
	if ( GGameType & GAME_Quake ) {
		R_DrawWaterSurfaces(forcedSortIndex);
	} else if ( GGameType & GAME_Hexen2 ) {
		R_DrawTransEntitiesOnList( r_viewleaf->contents == BSP29CONTENTS_EMPTY, forcedSortIndex );	// This restores the depth mask

		R_DrawWaterSurfaces(forcedSortIndex);

		R_DrawTransEntitiesOnList( r_viewleaf->contents != BSP29CONTENTS_EMPTY, forcedSortIndex );
	} else if ( GGameType & GAME_Quake2 ) {
		R_DrawAlphaSurfaces(forcedSortIndex);
	}
}

static void R_PlaneForSurface( surfaceType_t* surfType, cplane_t* plane ) {
	if ( !surfType ) {
		Com_Memset( plane, 0, sizeof ( *plane ) );
		plane->normal[ 0 ] = 1;
		return;
	}
	switch ( *surfType ) {
	case SF_FACE:
		*plane = ( ( srfSurfaceFace_t* )surfType )->plane;
		return;

	case SF_TRIANGLES:
	{
		srfTriangles_t* tri = ( srfTriangles_t* )surfType;
		bsp46_drawVert_t* v1 = tri->verts + tri->indexes[ 0 ];
		bsp46_drawVert_t* v2 = tri->verts + tri->indexes[ 1 ];
		bsp46_drawVert_t* v3 = tri->verts + tri->indexes[ 2 ];
		vec4_t plane4;
		PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[ 3 ];
	}
		return;

	case SF_POLY:
	{
		srfPoly_t* poly = ( srfPoly_t* )surfType;
		vec4_t plane4;
		PlaneFromPoints( plane4, poly->verts[ 0 ].xyz, poly->verts[ 1 ].xyz, poly->verts[ 2 ].xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[ 3 ];
	}
		return;

	default:
		Com_Memset( plane, 0, sizeof ( *plane ) );
		plane->normal[ 0 ] = 1;
		return;
	}
}

static bool IsMirror( const drawSurf_t* drawSurf, int entityNum ) {
	// create plane axis for the portal we are seeing
	cplane_t originalPlane;
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	cplane_t plane;
	if ( entityNum != REF_ENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[ entityNum ];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orient );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orient.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orient.origin );
	} else {
		plane = originalPlane;
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( int i = 0; i < tr.refdef.num_entities; i++ ) {
		trRefEntity_t* e = &tr.refdef.entities[ i ];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		float d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64 ) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[ 0 ] == e->e.origin[ 0 ] &&
			 e->e.oldorigin[ 1 ] == e->e.origin[ 1 ] &&
			 e->e.oldorigin[ 2 ] == e->e.origin[ 2 ] ) {
			return true;
		}

		return false;
	}
	return false;
}

//	Determines if a surface is completely offscreen.
static bool SurfIsOffscreen( const drawSurf_t* drawSurf, vec4_t clipDest[ 128 ] ) {
	if ( glConfig.smpActive ) {
		// FIXME!  we can't do RB_BeginSurface/RB_EndSurface stuff with smp!
		return false;
	}

	R_RotateForViewer();

	int entityNum;
	shader_t* shader;
	int fogNum;
	int dlighted;
	int frontFace;
	int atiTess;
	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &frontFace, &atiTess );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );

	unsigned int pointOr = 0;
	unsigned int pointAnd = ( unsigned int )~0;
	for ( int i = 0; i < tess.numVertexes; i++ ) {
		vec4_t clip, eye;
		R_TransformModelToClip( tess.xyz[ i ], tr.orient.modelMatrix, tr.viewParms.projectionMatrix, eye, clip );

		unsigned int pointFlags = 0;
		for ( int j = 0; j < 3; j++ ) {
			if ( clip[ j ] >= clip[ 3 ] ) {
				pointFlags |= ( 1 << ( j * 2 ) );
			} else if ( clip[ j ] <= -clip[ 3 ] ) {
				pointFlags |= ( 1 << ( j * 2 + 1 ) );
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd ) {
		return true;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	int numTriangles = tess.numIndexes / 3;

	float shortest = 100000000;
	for ( int i = 0; i < tess.numIndexes; i += 3 ) {
		vec3_t normal;
		VectorSubtract( tess.xyz[ tess.indexes[ i ] ], tr.viewParms.orient.origin, normal );

		float len = VectorLengthSquared( normal );				// lose the sqrt
		if ( len < shortest ) {
			shortest = len;
		}

		if ( DotProduct( normal, tess.normal[ tess.indexes[ i ] ] ) >= 0 ) {
			numTriangles--;
		}
	}
	if ( !numTriangles ) {
		return true;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf, entityNum ) ) {
		return false;
	}

	if ( shortest > ( tess.shader->portalRange * tess.shader->portalRange ) ) {
		return true;
	}

	return false;
}

//	entityNum is the entity that the portal surface is a part of, which may
// be moving and rotating.
//
//	Returns true if it should be mirrored
static bool R_GetPortalOrientations( drawSurf_t* drawSurf, int entityNum,
	orientation_t* surface, orientation_t* camera,
	vec3_t pvsOrigin, bool* mirror ) {
	// create plane axis for the portal we are seeing
	cplane_t originalPlane;
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	cplane_t plane;
	if ( entityNum != REF_ENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[ entityNum ];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orient );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orient.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orient.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[ 0 ] );
	PerpendicularVector( surface->axis[ 1 ], surface->axis[ 0 ] );
	CrossProduct( surface->axis[ 0 ], surface->axis[ 1 ], surface->axis[ 2 ] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( int i = 0; i < tr.refdef.num_entities; i++ ) {
		trRefEntity_t* e = &tr.refdef.entities[ i ];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		float d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64 ) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[ 0 ] == e->e.origin[ 0 ] &&
			 e->e.oldorigin[ 1 ] == e->e.origin[ 1 ] &&
			 e->e.oldorigin[ 2 ] == e->e.origin[ 2 ] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[ 0 ], camera->axis[ 0 ] );
			VectorCopy( surface->axis[ 1 ], camera->axis[ 1 ] );
			VectorCopy( surface->axis[ 2 ], camera->axis[ 2 ] );

			*mirror = true;
			return true;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[ 0 ], surface->origin );

		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[ 0 ], camera->axis[ 0 ] );
		VectorSubtract( vec3_origin, camera->axis[ 1 ], camera->axis[ 1 ] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = ( tr.refdef.time / 1000.0f ) * e->e.frame;
				vec3_t transformed;
				VectorCopy( camera->axis[ 1 ], transformed );
				RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
				CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				vec3_t transformed;
				VectorCopy( camera->axis[ 1 ], transformed );
				RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
				CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
			}
		} else if ( e->e.skinNum ) {
			d = e->e.skinNum;
			vec3_t transformed;
			VectorCopy( camera->axis[ 1 ], transformed );
			RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
			CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
		}
		*mirror = false;
		return true;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return false;
}

static void R_MirrorPoint( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out ) {
	vec3_t local;
	VectorSubtract( in, surface->origin, local );

	vec3_t transformed;
	VectorClear( transformed );
	for ( int i = 0; i < 3; i++ ) {
		float d = DotProduct( local, surface->axis[ i ] );
		VectorMA( transformed, d, camera->axis[ i ], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

static void R_MirrorVector( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out ) {
	VectorClear( out );
	for ( int i = 0; i < 3; i++ ) {
		float d = DotProduct( in, surface->axis[ i ] );
		VectorMA( out, d, camera->axis[ i ], out );
	}
}

//	Returns true if another view has been rendered
static bool R_MirrorViewBySurface( drawSurf_t* drawSurf, int entityNum ) {
	// don't recursively mirror
	if ( tr.viewParms.isPortal ) {
		common->DPrintf( S_COLOR_RED "WARNING: recursive mirror/portal found\n" );
		return false;
	}

	if ( r_noportals->integer || r_fastsky->integer == 1 ) {
		return false;
	}

	// trivially reject portal/mirror
	vec4_t clipDest[ 128 ];
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
		return false;
	}

	// save old viewParms so we can return to it after the mirror view
	viewParms_t oldParms = tr.viewParms;

	viewParms_t newParms = tr.viewParms;
	newParms.isPortal = true;
	orientation_t surface, camera;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera,
			 newParms.pvsOrigin, &newParms.isMirror ) ) {
		return false;		// bad portal, no portalentity
	}

	R_MirrorPoint( oldParms.orient.origin, &surface, &camera, newParms.orient.origin );

	VectorSubtract( vec3_origin, camera.axis[ 0 ], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );

	R_MirrorVector( oldParms.orient.axis[ 0 ], &surface, &camera, newParms.orient.axis[ 0 ] );
	R_MirrorVector( oldParms.orient.axis[ 1 ], &surface, &camera, newParms.orient.axis[ 1 ] );
	R_MirrorVector( oldParms.orient.axis[ 2 ], &surface, &camera, newParms.orient.axis[ 2 ] );

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView( &newParms );

	tr.viewParms = oldParms;

	return true;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
=================
qsort replacement

=================
*/
#define SWAP_DRAW_SURF( a,b ) temp = *( drawSurf_t* )a; *( drawSurf_t* )a = *( drawSurf_t* )b; *( drawSurf_t* )b = temp;

/* this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort */

#define CUTOFF 8			/* testing shows that this is good value */

static void shortsort( drawSurf_t* lo, drawSurf_t* hi ) {
	while ( hi > lo ) {
		drawSurf_t* max = lo;
		for ( drawSurf_t* p = lo + 1; p <= hi; p++ ) {
			if ( p->sort > max->sort ) {
				max = p;
			}
		}
		drawSurf_t temp;
		SWAP_DRAW_SURF( max, hi );
		hi--;
	}
}

//	sort the array between lo and hi (inclusive)
//FIXME: this was lifted and modified from the microsoft lib source...
static void qsortFast( void* base, unsigned num, unsigned width ) {
	char* lo, * hi;				/* ends of sub-array currently sorting */
	char* mid;					/* points to middle of subarray */
	char* loguy, * higuy;		/* traveling pointers for partition step */
	unsigned size;				/* size of the sub-array */
	char* lostk[ 30 ], * histk[ 30 ];
	int stkptr;					/* stack for saving sub-array to be processed */
	drawSurf_t temp;

	/* Note: the number of stack entries required is no more than
	    1 + log2(size), so 30 is sufficient for any array */

	if ( num < 2 || width == 0 ) {
		return;					/* nothing to do */
	}

	stkptr = 0;					/* initialize stack */

	lo = ( char* )base;
	hi = ( char* )base + width * ( num - 1 );		/* initialize limits */

	/* this entry point is for pseudo-recursion calling: setting
	    lo and hi and jumping to here is like recursion, but stkptr is
	    prserved, locals aren't, so we preserve stuff on the stack */
recurse:

	size = ( hi - lo ) / width + 1;			/* number of el's to sort */

	/* below a certain size, it is faster to use a O(n^2) sorting method */
	if ( size <= CUTOFF ) {
		shortsort( ( drawSurf_t* )lo, ( drawSurf_t* )hi );
	} else {
		/* First we pick a partititioning element.  The efficiency of the
		    algorithm demands that we find one that is approximately the
		    median of the values, but also that we select one fast.  Using
		    the first one produces bad performace if the array is already
		    sorted, so we use the middle one, which would require a very
		    wierdly arranged array for worst case performance.  Testing shows
		    that a median-of-three algorithm does not, in general, increase
		    performance. */

		mid = lo + ( size / 2 ) * width;		/* find middle element */
		SWAP_DRAW_SURF( mid, lo );					/* swap it to beginning of array */

		/* We now wish to partition the array into three pieces, one
		    consisiting of elements <= partition element, one of elements
		    equal to the parition element, and one of element >= to it.  This
		    is done below; comments indicate conditions established at every
		    step. */

		loguy = lo;
		higuy = hi + width;

		/* Note that higuy decreases and loguy increases on every iteration,
		    so loop must terminate. */
		for (;; ) {
			/* lo <= loguy < hi, lo < higuy <= hi + 1,
			    A[i] <= A[lo] for lo <= i <= loguy,
			    A[i] >= A[lo] for higuy <= i <= hi */

			do {
				loguy += width;
			} while ( loguy <= hi && ( ( ( drawSurf_t* )loguy )->sort <= ( ( drawSurf_t* )lo )->sort ) );

			/* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
			    either loguy > hi or A[loguy] > A[lo] */

			do {
				higuy -= width;
			} while ( higuy > lo && ( ( ( drawSurf_t* )higuy )->sort >= ( ( drawSurf_t* )lo )->sort ) );

			/* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
			    either higuy <= lo or A[higuy] < A[lo] */

			if ( higuy < loguy ) {
				break;
			}

			/* if loguy > hi or higuy <= lo, then we would have exited, so
			    A[loguy] > A[lo], A[higuy] < A[lo],
			    loguy < hi, highy > lo */

			SWAP_DRAW_SURF( loguy, higuy );

			/* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
			    of loop is re-established */
		}

		/*     A[i] >= A[lo] for higuy < i <= hi,
		        A[i] <= A[lo] for lo <= i < loguy,
		        higuy < loguy, lo <= higuy <= hi
		    implying:
		        A[i] >= A[lo] for loguy <= i <= hi,
		        A[i] <= A[lo] for lo <= i <= higuy,
		        A[i] = A[lo] for higuy < i < loguy */

		SWAP_DRAW_SURF( lo, higuy );		/* put partition element in place */

		/* OK, now we have the following:
		        A[i] >= A[higuy] for loguy <= i <= hi,
		        A[i] <= A[higuy] for lo <= i < higuy
		        A[i] = A[lo] for higuy <= i < loguy    */

		/* We've finished the partition, now we want to sort the subarrays
		    [lo, higuy-1] and [loguy, hi].
		    We do the smaller one first to minimize stack usage.
		    We only sort arrays of length 2 or more.*/

		if ( higuy - 1 - lo >= hi - loguy ) {
			if ( lo + width < higuy ) {
				lostk[ stkptr ] = lo;
				histk[ stkptr ] = higuy - width;
				++stkptr;
			}							/* save big recursion for later */

			if ( loguy < hi ) {
				lo = loguy;
				goto recurse;			/* do small recursion */
			}
		} else {
			if ( loguy < hi ) {
				lostk[ stkptr ] = loguy;
				histk[ stkptr ] = hi;
				++stkptr;				/* save big recursion for later */
			}

			if ( lo + width < higuy ) {
				hi = higuy - width;
				goto recurse;			/* do small recursion */
			}
		}
	}

	/* We have sorted the array, except for any pending sorts on the stack.
	    Check if there are any, and do them. */

	--stkptr;
	if ( stkptr >= 0 ) {
		lo = lostk[ stkptr ];
		hi = histk[ stkptr ];
		goto recurse;			/* pop subarray from stack */
	}
}

static void R_SortDrawSurfs( drawSurf_t* drawSurfs, int numDrawSurfs ) {
	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	qsortFast( drawSurfs, numDrawSurfs, sizeof ( drawSurf_t ) );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( int i = 0; i < numDrawSurfs; i++ ) {
		int entityNum;
		shader_t* shader;
		int fogNum;
		int dlighted;
		int frontFace;
		int atiTess;
		R_DecomposeSort( ( drawSurfs + i )->sort, &entityNum, &shader, &fogNum, &dlighted, &frontFace, &atiTess );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			common->Error( "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( drawSurfs + i, entityNum ) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

static void R_DebugPolygon( int color, int numPoints, float* points ) {
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color & 1, ( color >> 1 ) & 1, ( color >> 2 ) & 1 );
	qglBegin( GL_POLYGON );
	for ( int i = 0; i < numPoints; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( int i = 0; i < numPoints; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

//	Visualization aid for movement clipping debugging
static void R_DebugGraphics() {
	if ( !r_debugSurface->integer ) {
		return;
	}

	// moved this in here to keep from /always/ doing the fog state change
	R_FogOff();

	// the render thread can't make callbacks to the main thread
	R_SyncRenderThread();

	GL_Bind( tr.whiteImage );
	GL_Cull( CT_FRONT_SIDED );

	if ( r_debugSurface->integer == 1 ) {
		CM_DrawDebugSurface( R_DebugPolygon );
	} else if ( BotDrawDebugPolygonsFunc ) {
		BotDrawDebugPolygonsFunc( R_DebugPolygon, r_debugSurface->integer );
	}

	R_FogOn();
}

//	A view may be either the actual camera view, or a mirror / remote location
void R_RenderView( viewParms_t* parms ) {
	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	// Ridah, purge media that were left over from the last level
	if ( r_cache->integer ) {
		static int lastTime;
		if ( ( lastTime > tr.refdef.time ) || ( lastTime < ( tr.refdef.time - 200 ) ) ) {
			R_PurgeShaders();
			R_PurgeBackupImages( 1 );
			lastTime = tr.refdef.time;
		}
	}

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	tr.viewCount++;

	int firstDrawSurf = tr.refdef.numDrawSurfs;

	// set viewParms.world
	R_RotateForViewer();

	// ydnar: removed this because we do a full frustum/projection setup
	// based on world bounds zfar or fog bounds
	if ( !( GGameType & GAME_ET ) ) {
		R_SetupFrustum();
	}

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}

bool R_GetScreenPosFromWorldPos( vec3_t origin, int& u, int& v ) {
	vec4_t eye;
	vec4_t Out;
	R_TransformModelToClip( origin, tr.viewParms.world.modelMatrix, tr.viewParms.projectionMatrix, eye, Out );

	if ( eye[ 2 ] > -r_znear->value ) {
		return false;
	}

	vec4_t normalized;
	vec4_t window;
	R_TransformClipToWindow( Out, &tr.viewParms, normalized, window );
	u = tr.refdef.x + ( int )window[ 0 ];
	v = tr.refdef.y + tr.refdef.height - ( int )window[ 1 ];
	return true;
}

void R_FogOn() {
	if ( fogIsOn ) {
		return;
	}

	if ( GGameType & GAME_WolfSP && backEnd.projection2D ) {
		// no fog in 2d
		return;
	}

	if ( GGameType & GAME_WolfMP && r_uiFullScreen->integer ) {
		// don't fog in the menu
		return;
	}

	if ( !r_wolffog->integer ) {
		return;
	}

	if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) {
		// don't force world fog on portal sky
		if ( !( glfogsettings[ FOG_PORTALVIEW ].registered ) ) {
			return;
		}
	} else if ( !glfogNum ) {
		return;
	}

	qglEnable( GL_FOG );
	fogIsOn = true;
}

//	Allow disabling fog temporarily
void R_FogOff() {
	if ( !fogIsOn ) {
		return;
	}
	qglDisable( GL_FOG );
	fogIsOn = false;
}

void R_Fog( glfog_t* curfog ) {
	if ( !r_wolffog->integer ) {
		R_FogOff();
		return;
	}

	if ( !curfog->registered ) {
		R_FogOff();
		return;
	}

	//	Assme values of '0' for these parameters means 'use default'
	if ( !curfog->density ) {
		curfog->density = 1;
	}
	if ( !curfog->hint ) {
		curfog->hint = GL_DONT_CARE;
	}
	if ( !curfog->mode ) {
		curfog->mode = GL_LINEAR;
	}

	R_FogOn();

	qglFogi( GL_FOG_MODE, curfog->mode );
	qglFogfv( GL_FOG_COLOR, curfog->color );
	qglFogf( GL_FOG_DENSITY, curfog->density );
	qglHint( GL_FOG_HINT, curfog->hint );
	if ( GGameType & GAME_WolfSP && backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
		qglFogf( GL_FOG_START, curfog->end );		// snooper starts GL fog out further
	} else {
		qglFogf( GL_FOG_START, curfog->start );
	}

	if ( r_zfar->value ) {
		//	Allow override for helping level designers test fog distances
		qglFogf( GL_FOG_END, r_zfar->value );
	} else {
		if ( GGameType & GAME_WolfSP && backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
			// snooper ends GL fog out further.  this works fine with our maps, but could be 'funky' with later maps
			qglFogf( GL_FOG_END, curfog->end + 1000 );
		} else {
			qglFogf( GL_FOG_END, curfog->end );
		}
	}

	// NV fog mode
	if ( glConfig.NVFogAvailable ) {
		qglFogi( GL_FOG_DISTANCE_MODE_NV, glConfig.NVFogMode );
	}

	qglClearColor( curfog->color[ 0 ], curfog->color[ 1 ], curfog->color[ 2 ], curfog->color[ 3 ] );
}

//  if fogvar == FOG_CMD_SWITCHFOG
//	{
//		fogvar is the command
//		var1 is the fog to switch to
//		var2 is the time to transition
//  }
//  else
//	{
//		fogvar is the fog that's being set
//		var1 is the near fog z value
//		var2 is the far fog z value
//		rgb = color
//		density is density, and is used to derive the values of 'mode', 'drawsky', and 'clearscreen'
//  }
void R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) {
	if ( fogvar != FOG_CMD_SWITCHFOG ) {
		// just set the parameters and return
		if ( var1 == 0 && var2 == 0 ) {
			// clear this fog
			glfogsettings[ fogvar ].registered = false;
			return;
		}

		glfogsettings[ fogvar ].color[ 0 ] = r * tr.identityLight;
		glfogsettings[ fogvar ].color[ 1 ] = g * tr.identityLight;
		glfogsettings[ fogvar ].color[ 2 ] = b * tr.identityLight;
		glfogsettings[ fogvar ].color[ 3 ] = 1;
		glfogsettings[ fogvar ].start = var1;
		glfogsettings[ fogvar ].end = var2;
		if ( GGameType & GAME_WolfSP ? density >= 1 : density > 1 ) {
			glfogsettings[ fogvar ].mode = GL_LINEAR;
			glfogsettings[ fogvar ].drawsky = false;
			glfogsettings[ fogvar ].clearscreen = true;
			glfogsettings[ fogvar ].density = 1.0;
		} else {
			glfogsettings[ fogvar ].mode = GL_EXP;
			glfogsettings[ fogvar ].drawsky = true;
			glfogsettings[ fogvar ].clearscreen = false;
			glfogsettings[ fogvar ].density = density;
		}
		glfogsettings[ fogvar ].hint = GL_DONT_CARE;
		glfogsettings[ fogvar ].registered = true;
		return;
	}

	// FOG_MAP now used to mean 'no fog'
	if ( GGameType & GAME_WolfSP && var1 == FOG_MAP ) {
		// transitioning from...
		if ( glfogsettings[ FOG_CURRENT ].registered ) {
			memcpy( &glfogsettings[ FOG_LAST ], &glfogsettings[ FOG_CURRENT ], sizeof ( glfog_t ) );
		}

		memcpy( &glfogsettings[ FOG_TARGET ], &glfogsettings[ glfogNum ], sizeof ( glfog_t ) );

		// clear, clear, clear
		memset( &glfogsettings[ FOG_MAP ], 0, sizeof ( glfog_t ) );
		memset( &glfogsettings[ FOG_TARGET ], 0, sizeof ( glfog_t ) );
		glfogNum = FOG_NONE;
		return;
	}

	// don't switch to invalid fogs
	if ( glfogsettings[ var1 ].registered != true ) {
		return;
	}

	glfogNum = ( glfogType_t )var1;

	// transitioning to new fog, store the current values as the 'from'

	if ( glfogsettings[ FOG_CURRENT ].registered ) {
		memcpy( &glfogsettings[ FOG_LAST ], &glfogsettings[ FOG_CURRENT ], sizeof ( glfog_t ) );
	} else {
		// if no current fog fall back to world fog
		// FIXME: handle transition if there is no FOG_MAP fog
		memcpy( &glfogsettings[ FOG_LAST ], &glfogsettings[ GGameType & GAME_WolfSP ? glfogNum : FOG_MAP ], sizeof ( glfog_t ) );
	}

	memcpy( &glfogsettings[ FOG_TARGET ], &glfogsettings[ glfogNum ], sizeof ( glfog_t ) );

	if ( GGameType & GAME_WolfSP && !var2 ) {
		// instant
		glfogsettings[ FOG_TARGET ].startTime = 0;
		glfogsettings[ FOG_TARGET ].finishTime = 0;
		glfogsettings[ FOG_TARGET ].dirty = 1;
		glfogsettings[ FOG_CURRENT ].dirty = 1;
	} else {
		// setup transition times
		glfogsettings[ FOG_TARGET ].startTime = tr.refdef.time;
		glfogsettings[ FOG_TARGET ].finishTime = tr.refdef.time + var2;
	}
}

void R_DebugText( const vec3_t org, float r, float g, float b, const char* text, bool neverOcclude ) {
	if ( neverOcclude ) {
		qglDepthRange( 0, 0 );		// never occluded

	}
	qglColor3f( r, g, b );
	qglRasterPos3fv( org );
	qglPushAttrib( GL_LIST_BIT );
	qglListBase( gl_NormalFontBase );
	qglCallLists( String::Length( text ), GL_UNSIGNED_BYTE, text );
	qglListBase( 0 );
	qglPopAttrib();

	if ( neverOcclude ) {
		qglDepthRange( 0, 1 );
	}
}
