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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../../common/content_types.h"
#include "local.h"

aas_settings_t aassettings;

static void AAS_InitSettingsQ3() {
	aassettings.phys_friction               = LibVarValue( "phys_friction", "6" );
	aassettings.phys_stopspeed              = LibVarValue( "phys_stopspeed", "100" );
	aassettings.phys_gravity                = LibVarValue( "phys_gravity", "800" );
	aassettings.phys_waterfriction          = LibVarValue( "phys_waterfriction", "1" );
	aassettings.phys_watergravity           = LibVarValue( "phys_watergravity", "400" );
	aassettings.phys_maxvelocity            = LibVarValue( "phys_maxvelocity", "320" );
	aassettings.phys_maxwalkvelocity        = LibVarValue( "phys_maxwalkvelocity", "320" );
	aassettings.phys_maxcrouchvelocity      = LibVarValue( "phys_maxcrouchvelocity", "100" );
	aassettings.phys_maxswimvelocity        = LibVarValue( "phys_maxswimvelocity", "150" );
	aassettings.phys_walkaccelerate         = LibVarValue( "phys_walkaccelerate", "10" );
	aassettings.phys_airaccelerate          = LibVarValue( "phys_airaccelerate", "1" );
	aassettings.phys_swimaccelerate         = LibVarValue( "phys_swimaccelerate", "4" );
	aassettings.phys_maxstep                = LibVarValue( "phys_maxstep", "19" );
	aassettings.phys_maxsteepness           = LibVarValue( "phys_maxsteepness", "0.7" );
	aassettings.phys_maxwaterjump           = LibVarValue( "phys_maxwaterjump", "18" );
	aassettings.phys_maxbarrier             = LibVarValue( "phys_maxbarrier", "33" );
	aassettings.phys_jumpvel                = LibVarValue( "phys_jumpvel", "270" );
	aassettings.phys_falldelta5             = LibVarValue( "phys_falldelta5", "40" );
	aassettings.phys_falldelta10            = LibVarValue( "phys_falldelta10", "60" );
	aassettings.rs_waterjump                = LibVarValue( "rs_waterjump", "400" );
	aassettings.rs_teleport                 = LibVarValue( "rs_teleport", "50" );
	aassettings.rs_barrierjump              = LibVarValue( "rs_barrierjump", "100" );
	aassettings.rs_startcrouch              = LibVarValue( "rs_startcrouch", "300" );
	aassettings.rs_startgrapple             = LibVarValue( "rs_startgrapple", "500" );
	aassettings.rs_startwalkoffledge        = LibVarValue( "rs_startwalkoffledge", "70" );
	aassettings.rs_startjump                = LibVarValue( "rs_startjump", "300" );
	aassettings.rs_rocketjump               = LibVarValue( "rs_rocketjump", "500" );
	aassettings.rs_bfgjump                  = LibVarValue( "rs_bfgjump", "500" );
	aassettings.rs_jumppad                  = LibVarValue( "rs_jumppad", "250" );
	aassettings.rs_aircontrolledjumppad     = LibVarValue( "rs_aircontrolledjumppad", "300" );
	aassettings.rs_funcbob                  = LibVarValue( "rs_funcbob", "300" );
	aassettings.rs_startelevator            = LibVarValue( "rs_startelevator", "50" );
	aassettings.rs_falldamage5              = LibVarValue( "rs_falldamage5", "300" );
	aassettings.rs_falldamage10             = LibVarValue( "rs_falldamage10", "500" );
	aassettings.rs_maxfallheight            = LibVarValue( "rs_maxfallheight", "0" );
	aassettings.rs_maxjumpfallheight        = LibVarValue( "rs_maxjumpfallheight", "450" );
}

static void AAS_InitSettingsWolf() {
	aassettings.phys_friction = 6;
	aassettings.phys_stopspeed = 100;
	aassettings.phys_gravity = 800;
	aassettings.phys_waterfriction = 1;
	aassettings.phys_watergravity = 400;
	aassettings.phys_maxvelocity = 320;
	aassettings.phys_maxwalkvelocity = 300;
	aassettings.phys_maxcrouchvelocity = 100;
	aassettings.phys_maxswimvelocity = 150;
	aassettings.phys_walkaccelerate = 10;
	aassettings.phys_airaccelerate = 1;
	aassettings.phys_swimaccelerate = 4;
	aassettings.phys_maxstep = 18;
	aassettings.phys_maxsteepness = 0.7;
	aassettings.phys_maxwaterjump = 17;
	aassettings.phys_jumpvel = 270;
	aassettings.phys_falldelta5 = 25;
	aassettings.phys_falldelta10 = 40;
	if ( GGameType & GAME_WolfMP ) {
		// Ridah, calculate maxbarrier according to jumpvel and gravity
		aassettings.phys_maxbarrier = -0.8 + ( 0.5 * aassettings.phys_jumpvel * aassettings.phys_jumpvel / aassettings.phys_gravity );
	} else   {
		aassettings.phys_maxbarrier = 49;
	}
	aassettings.rs_waterjump = 700;
	aassettings.rs_teleport = 50;
	aassettings.rs_barrierjump = 900;
	aassettings.rs_startcrouch = 300;
	aassettings.rs_startgrapple = 500;
	aassettings.rs_startwalkoffledge = 300;
	aassettings.rs_startjump = 500;
	aassettings.rs_rocketjump = 300;
	aassettings.rs_bfgjump = 300;
	aassettings.rs_jumppad = 200;
	aassettings.rs_aircontrolledjumppad = 250;
	aassettings.rs_funcbob = 300;
	aassettings.rs_startelevator = 0;
	aassettings.rs_falldamage5 = 400;
	aassettings.rs_falldamage10 = 900;
	aassettings.rs_maxfallheight = 0;
	aassettings.rs_maxjumpfallheight = 450;
}

void AAS_InitSettings() {
	if ( GGameType & GAME_Quake3 ) {
		AAS_InitSettingsQ3();
	} else   {
		AAS_InitSettingsWolf();
	}
}

// returns true if the bot is against a ladder
bool AAS_AgainstLadder( const vec3_t origin, int ms_areanum ) {
	vec3_t org;
	VectorCopy( origin, org );
	int areanum = AAS_PointAreaNum( org );
	if ( !areanum ) {
		org[ 0 ] += 1;
		areanum = AAS_PointAreaNum( org );
		if ( !areanum ) {
			org[ 1 ] += 1;
			areanum = AAS_PointAreaNum( org );
			if ( !areanum ) {
				org[ 0 ] -= 2;
				areanum = AAS_PointAreaNum( org );
				if ( !areanum ) {
					org[ 1 ] -= 2;
					areanum = AAS_PointAreaNum( org );
				}
			}
		}
	}
	//if in solid... wrrr shouldn't happen
	if ( !areanum ) {
		if ( GGameType & GAME_Quake3 ) {
			return false;
		}
		// RF, it does if they're in a monsterclip brush
		areanum = ms_areanum;
	}
	//if not in a ladder area
	if ( !( aasworld->areasettings[ areanum ].areaflags & AREA_LADDER ) ) {
		return false;
	}
	//if a crouch only area
	if ( !( aasworld->areasettings[ areanum ].presencetype & PRESENCE_NORMAL ) ) {
		return false;
	}

	aas_area_t* area = &aasworld->areas[ areanum ];
	for ( int i = 0; i < area->numfaces; i++ ) {
		int facenum = aasworld->faceindex[ area->firstface + i ];
		int side = facenum < 0;
		aas_face_t* face = &aasworld->faces[ abs( facenum ) ];
		//if the face isn't a ladder face
		if ( !( face->faceflags & FACE_LADDER ) ) {
			continue;
		}
		//get the plane the face is in
		aas_plane_t* plane = &aasworld->planes[ face->planenum ^ side ];
		//if the origin is pretty close to the plane
		if ( abs( DotProduct( plane->normal, origin ) - plane->dist ) < ( GGameType & GAME_ET ? 7 : 3 ) ) {
			// RF, if hanging on to the edge of a ladder, we have to account for bounding box touching
			if ( AAS_PointInsideFace( abs( facenum ), origin, GGameType & GAME_ET ? 2.0 : 0.1f ) ) {
				return true;
			}
		}
	}
	return false;
}

// applies ground friction to the given velocity
static void AAS_Accelerate( vec3_t velocity, float frametime, const vec3_t wishdir, float wishspeed, float accel ) {
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct( velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if ( addspeed <= 0 ) {
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	for ( i = 0; i < 3; i++ ) {
		velocity[ i ] += accelspeed * wishdir[ i ];
	}
}

// applies ground friction to the given velocity
static void AAS_ApplyFriction( vec3_t vel, float friction, float stopspeed, float frametime ) {
	//horizontal speed
	float speed = sqrt( vel[ 0 ] * vel[ 0 ] + vel[ 1 ] * vel[ 1 ] );
	if ( !speed ) {
		return;
	}
	float control = speed < stopspeed ? stopspeed : speed;
	float newspeed = speed - frametime * control * friction;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;
	vel[ 0 ] *= newspeed;
	vel[ 1 ] *= newspeed;
}

static bool AAS_ClipToBBox( aas_trace_t* trace, const vec3_t start, const vec3_t end,
	int presencetype, const vec3_t mins, const vec3_t maxs ) {
	vec3_t bboxmins, bboxmaxs, absmins, absmaxs;
	AAS_PresenceTypeBoundingBox( presencetype, bboxmins, bboxmaxs );
	VectorSubtract( mins, bboxmaxs, absmins );
	VectorSubtract( maxs, bboxmins, absmaxs );

	VectorCopy( end, trace->endpos );
	trace->fraction = 1;
	int i;
	for ( i = 0; i < 3; i++ ) {
		if ( start[ i ] < absmins[ i ] && end[ i ] < absmins[ i ] ) {
			return false;
		}
		if ( start[ i ] > absmaxs[ i ] && end[ i ] > absmaxs[ i ] ) {
			return false;
		}
	}
	//check bounding box collision
	vec3_t dir;
	VectorSubtract( end, start, dir );
	float frac = 1;
	vec3_t mid;
	for ( i = 0; i < 3; i++ ) {
		//get plane to test collision with for the current axis direction
		float planedist;
		if ( dir[ i ] > 0 ) {
			planedist = absmins[ i ];
		} else   {
			planedist = absmaxs[ i ];
		}
		//calculate collision fraction
		float front = start[ i ] - planedist;
		float back = end[ i ] - planedist;
		frac = front / ( front - back );
		//check if between bounding planes of next axis
		int side = i + 1;
		if ( side > 2 ) {
			side = 0;
		}
		mid[ side ] = start[ side ] + dir[ side ] * frac;
		if ( mid[ side ] > absmins[ side ] && mid[ side ] < absmaxs[ side ] ) {
			//check if between bounding planes of next axis
			side++;
			if ( side > 2 ) {
				side = 0;
			}
			mid[ side ] = start[ side ] + dir[ side ] * frac;
			if ( mid[ side ] > absmins[ side ] && mid[ side ] < absmaxs[ side ] ) {
				mid[ i ] = planedist;
				break;
			}
		}
	}
	//if there was a collision
	if ( i != 3 ) {
		trace->startsolid = false;
		trace->fraction = frac;
		trace->ent = 0;
		trace->planenum = 0;
		trace->area = 0;
		trace->lastarea = 0;
		//trace endpos
		for ( int j = 0; j < 3; j++ ) {
			trace->endpos[ j ] = start[ j ] + dir[ j ] * frac;
		}
		return true;
	}
	return false;
}

// calculates the horizontal velocity needed to perform a jump from start
// to end
// Parameter:			zvel	: z velocity for jump
//						start	: start position of jump
//						end		: end position of jump
//						*speed	: returned speed for jump
// Returns:				false if too high or too far from start to end
bool AAS_HorizontalVelocityForJump( float zvel, const vec3_t start, const vec3_t end, float* velocity ) {
	float phys_gravity = aassettings.phys_gravity;
	float phys_maxvelocity = aassettings.phys_maxvelocity;

	//maximum height a player can jump with the given initial z velocity
	float maxjump = 0.5 * phys_gravity * ( zvel / phys_gravity ) * ( zvel / phys_gravity );
	//top of the parabolic jump
	float top = start[ 2 ] + maxjump;
	//height the bot will fall from the top
	float height2fall = top - end[ 2 ];
	//if the goal is to high to jump to
	if ( height2fall < 0 ) {
		*velocity = phys_maxvelocity;
		return 0;
	}
	//time a player takes to fall the height
	float t = sqrt( height2fall / ( 0.5 * phys_gravity ) );
	//direction from start to end
	vec3_t dir;
	VectorSubtract( end, start, dir );

	if ( ( t + zvel / phys_gravity ) == 0.0f ) {
		*velocity = phys_maxvelocity;
		return 0;
	}
	//calculate horizontal speed
	*velocity = sqrt( dir[ 0 ] * dir[ 0 ] + dir[ 1 ] * dir[ 1 ] ) / ( t + zvel / phys_gravity );
	//the horizontal speed must be lower than the max speed
	if ( *velocity > phys_maxvelocity ) {
		*velocity = phys_maxvelocity;
		return false;
	}
	return true;
}

bool AAS_DropToFloor( vec3_t origin, const vec3_t mins, const vec3_t maxs ) {
	vec3_t end;
	bsp_trace_t trace;

	VectorCopy( origin, end );
	end[ 2 ] -= 100;
	trace = AAS_Trace( origin, mins, maxs, end, 0,
		GGameType & GAME_ET ? BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP : BSP46CONTENTS_SOLID );
	if ( trace.startsolid ) {
		return false;
	}
	VectorCopy( trace.endpos, origin );
	return true;
}

// returns true if the bot is on the ground
static bool AAS_OnGroundET( const vec3_t origin, int presencetype, int passent ) {
	bsp_trace_t trace;
	vec3_t end, up = {0, 0, 1};
	vec3_t mins, maxs;

	VectorCopy( origin, end );
	end[ 2 ] -= 10;

	//trace = AAS_TraceClientBBox(origin, end, presencetype, passent);
	AAS_PresenceTypeBoundingBox( presencetype, mins, maxs );
	trace = AAS_Trace( origin, mins, maxs, end, passent, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );

	//if in solid
	if ( trace.startsolid ) {
		return true;
	}
	//if nothing hit at all
	if ( trace.fraction >= 1.0 ) {
		return false;
	}
	//if too far from the hit plane
	if ( origin[ 2 ] - trace.endpos[ 2 ] > 10 ) {
		return false;
	}
	//check if the plane isn't too steep
	//plane = AAS_PlaneFromNum(trace.planenum);
	if ( DotProduct( trace.plane.normal, up ) < aassettings.phys_maxsteepness ) {
		return false;
	}
	//the bot is on the ground
	return true;
}

// returns true if the bot is on the ground
bool AAS_OnGround( const vec3_t origin, int presencetype, int passent ) {
	if ( GGameType & GAME_ET ) {
		return AAS_OnGroundET( origin, presencetype, passent );
	}

	aas_trace_t trace;
	vec3_t end, up = {0, 0, 1};
	aas_plane_t* plane;

	VectorCopy( origin, end );
	end[ 2 ] -= 10;

	trace = AAS_TraceClientBBox( origin, end, presencetype, passent );

	//if in solid
	if ( trace.startsolid ) {
		return false;
	}
	//if nothing hit at all
	if ( trace.fraction >= 1.0 ) {
		return false;
	}
	//if too far from the hit plane
	if ( origin[ 2 ] - trace.endpos[ 2 ] > 10 ) {
		return false;
	}
	//check if the plane isn't too steep
	plane = AAS_PlaneFromNum( trace.planenum );
	if ( DotProduct( plane->normal, up ) < aassettings.phys_maxsteepness ) {
		return false;
	}
	//the bot is on the ground
	return true;
}

// returns true if a bot at the given position is swimming
bool AAS_Swimming( const vec3_t origin ) {
	vec3_t testorg;
	VectorCopy( origin, testorg );
	testorg[ 2 ] -= 2;
	if ( AAS_PointContents( testorg ) & ( BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER ) ) {
		return true;
	}
	return false;
}

// returns the Z velocity when rocket jumping at the origin
static float AAS_WeaponJumpZVelocity( const vec3_t origin, float radiusdamage ) {
	//look down (90 degrees)
	vec3_t viewangles;
	viewangles[ PITCH ] = 90;
	viewangles[ YAW ] = 0;
	viewangles[ ROLL ] = 0;
	//get the start point shooting from
	vec3_t start;
	VectorCopy( origin, start );
	start[ 2 ] += 8;	//view offset Z
	vec3_t forward, right;
	AngleVectors( viewangles, forward, right, NULL );
	vec3_t rocketoffset = {8, 8, -8};
	start[ 0 ] += forward[ 0 ] * rocketoffset[ 0 ] + right[ 0 ] * rocketoffset[ 1 ];
	start[ 1 ] += forward[ 1 ] * rocketoffset[ 0 ] + right[ 1 ] * rocketoffset[ 1 ];
	start[ 2 ] += forward[ 2 ] * rocketoffset[ 0 ] + right[ 2 ] * rocketoffset[ 1 ] + rocketoffset[ 2 ];
	//end point of the trace
	vec3_t end;
	VectorMA( start, 500, forward, end );
	//trace a line to get the impact point
	bsp_trace_t bsptrace = AAS_Trace( start, NULL, NULL, end, 1, BSP46CONTENTS_SOLID );
	//calculate the damage the bot will get from the rocket impact
	vec3_t v;
	vec3_t botmins = {-16, -16, -24};
	vec3_t botmaxs = {16, 16, 32};
	VectorAdd( botmins, botmaxs, v );
	VectorMA( origin, 0.5, v, v );
	VectorSubtract( bsptrace.endpos, v, v );

	float points = radiusdamage - 0.5 * VectorLength( v );
	if ( points < 0 ) {
		points = 0;
	}
	//the owner of the rocket gets half the damage
	points *= 0.5;
	//mass of the bot (p_client.c: PutClientInServer)
	float mass = 200;
	//knockback is the same as the damage points
	float knockback = points;
	//direction of the damage (from trace.endpos to bot origin)
	vec3_t dir;
	VectorSubtract( origin, bsptrace.endpos, dir );
	VectorNormalize( dir );
	//damage velocity
	vec3_t kvel;
	VectorScale( dir, 1600.0 * ( float )knockback / mass, kvel );	//the rocket jump hack...
	//rocket impact velocity + jump velocity
	return kvel[ 2 ] + aassettings.phys_jumpvel;
}

float AAS_RocketJumpZVelocity( const vec3_t origin ) {
	//rocket radius damage is 120 (p_weapon.c: Weapon_RocketLauncher_Fire)
	return AAS_WeaponJumpZVelocity( origin, 120 );
}

float AAS_BFGJumpZVelocity( const vec3_t origin ) {
	//bfg radius damage is 1000 (p_weapon.c: weapon_bfg_fire)
	return AAS_WeaponJumpZVelocity( origin, 120 );
}

// predicts the movement
// assumes regular bounding box sizes
// NOTE: out of water jumping is not included
// NOTE: grappling hook is not included
//
// Parameter:			origin			: origin to start with
//						presencetype	: presence type to start with
//						velocity		: velocity to start with
//						cmdmove			: client command movement
//						cmdframes		: number of frame cmdmove is valid
//						maxframes		: maximum number of predicted frames
//						frametime		: duration of one predicted frame
//						stopevent		: events that stop the prediction
//						stopareanum		: stop as soon as entered this area
static bool AAS_ClientMovementPrediction( aas_clientmove_t* move,
	int entnum, const vec3_t origin,
	int presencetype, int hitent, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum,
	const vec3_t mins, const vec3_t maxs, bool visualize ) {
	if ( GGameType & GAME_ET && visualize ) {
		AAS_ClearShownPolygons();
		AAS_ClearShownDebugLines();
	}

	// don't let us succeed on interaction with area 0
	if ( GGameType & GAME_ET && stopareanum == 0 ) {
		stopevent &= ~( SE_ENTERAREA | SE_HITGROUNDAREA );
	}

	if ( frametime <= 0 ) {
		frametime = 0.1f;
	}

	float phys_friction = aassettings.phys_friction;
	float phys_stopspeed = aassettings.phys_stopspeed;
	float phys_gravity = aassettings.phys_gravity;
	float phys_waterfriction = aassettings.phys_waterfriction;
	float phys_watergravity = aassettings.phys_watergravity;
	float phys_maxwalkvelocity = aassettings.phys_maxwalkvelocity;	// * frametime;
	float phys_maxcrouchvelocity = aassettings.phys_maxcrouchvelocity;	// * frametime;
	float phys_maxswimvelocity = aassettings.phys_maxswimvelocity;	// * frametime;
	float phys_walkaccelerate = aassettings.phys_walkaccelerate;
	float phys_airaccelerate = aassettings.phys_airaccelerate;
	float phys_swimaccelerate = aassettings.phys_swimaccelerate;
	float phys_maxstep = aassettings.phys_maxstep;
	float phys_maxsteepness = aassettings.phys_maxsteepness;
	float phys_jumpvel = aassettings.phys_jumpvel * frametime;

	Com_Memset( move, 0, sizeof ( aas_clientmove_t ) );
	aas_trace_t trace;
	bsp_trace_t bsptrace;
	Com_Memset( &trace, 0, sizeof ( aas_trace_t ) );
	Com_Memset( &bsptrace, 0, sizeof ( bsp_trace_t ) );
	vec3_t _mins, _maxs;
	AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, _mins, _maxs );
	//start at the current origin
	vec3_t org;
	VectorCopy( origin, org );
	org[ 2 ] += 0.25;

	if ( GGameType & GAME_ET ) {
		// test this position, if it's in solid, move it up to adjust for capsules
		bsptrace = AAS_Trace( org, _mins, _maxs, org, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );
		while ( bsptrace.startsolid ) {
			org[ 2 ] += 8;
			bsptrace = AAS_Trace( org, _mins, _maxs, org, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );
			if ( bsptrace.startsolid && ( org[ 2 ] - origin[ 2 ] > 16 ) ) {
				move->stopevent = SE_NONE;
				return false;
			}
		}
	}

	//velocity to test for the first frame
	vec3_t frame_test_vel;
	VectorScale( velocity, frametime, frame_test_vel );
	//
	int jump_frame = -1;
	aas_plane_t* lplane = NULL;
	//predict a maximum of 'maxframes' ahead
	int n;
	for ( n = 0; n < maxframes; n++ ) {
		bool swimming = AAS_Swimming( org );
		//get gravity depending on swimming or not
		float gravity = swimming ? phys_watergravity : phys_gravity;
		//apply gravity at the START of the frame
		frame_test_vel[ 2 ] = frame_test_vel[ 2 ] - ( gravity * 0.1 * frametime );
		//if on the ground or swimming
		if ( onground || swimming ) {
			float friction = swimming ? phys_friction : phys_waterfriction;
			//apply friction
			VectorScale( frame_test_vel, 1 / frametime, frame_test_vel );
			AAS_ApplyFriction( frame_test_vel, friction, phys_stopspeed, frametime );
			VectorScale( frame_test_vel, frametime, frame_test_vel );
		}
		bool crouch = false;
		//apply command movement
		if ( GGameType & GAME_ET && cmdframes < 0 ) {
			// cmdmove is the destination, we should keep moving towards it
			vec3_t wishdir;
			VectorSubtract( cmdmove, org, wishdir );
			VectorNormalize( wishdir );
			VectorScale( wishdir, phys_maxwalkvelocity, wishdir );
			vec3_t savevel;
			VectorCopy( frame_test_vel, savevel );
			VectorScale( wishdir, frametime, frame_test_vel );
			if ( !swimming ) {
				frame_test_vel[ 2 ] = savevel[ 2 ];
			}
		} else if ( n < cmdframes )     {
			int ax = 0;
			float maxvel = phys_maxwalkvelocity;
			float accelerate = phys_airaccelerate;
			vec3_t wishdir;
			VectorCopy( cmdmove, wishdir );
			if ( onground ) {
				if ( cmdmove[ 2 ] < -300 ) {
					crouch = true;
					maxvel = phys_maxcrouchvelocity;
				}
				//if not swimming and upmove is positive then jump
				if ( !swimming && cmdmove[ 2 ] > 1 ) {
					//jump velocity minus the gravity for one frame + 5 for safety
					frame_test_vel[ 2 ] = phys_jumpvel - ( gravity * 0.1 * frametime ) + 5;
					jump_frame = n;
					//jumping so air accelerate
					accelerate = phys_airaccelerate;
				} else   {
					accelerate = phys_walkaccelerate;
				}
				ax = 2;
			}
			if ( swimming ) {
				maxvel = phys_maxswimvelocity;
				accelerate = phys_swimaccelerate;
				ax = 3;
			} else   {
				wishdir[ 2 ] = 0;
			}

			float wishspeed = VectorNormalize( wishdir );
			if ( wishspeed > maxvel ) {
				wishspeed = maxvel;
			}
			VectorScale( frame_test_vel, 1 / frametime, frame_test_vel );
			AAS_Accelerate( frame_test_vel, frametime, wishdir, wishspeed, accelerate );
			VectorScale( frame_test_vel, frametime, frame_test_vel );
		}
		if ( !( GGameType & GAME_ET ) ) {
			if ( crouch ) {
				presencetype = PRESENCE_CROUCH;
			} else if ( presencetype == PRESENCE_CROUCH )     {
				if ( AAS_PointPresenceType( org ) & PRESENCE_NORMAL ) {
					presencetype = PRESENCE_NORMAL;
				}
			}
		}
		//save the current origin
		vec3_t lastorg;
		VectorCopy( org, lastorg );
		//move linear during one frame
		vec3_t left_test_vel;
		VectorCopy( frame_test_vel, left_test_vel );
		int j = 0;
		do {
			vec3_t end;
			VectorAdd( org, left_test_vel, end );
			//trace a bounding box
			if ( GGameType & GAME_ET ) {
				bsptrace = AAS_Trace( org, _mins, _maxs, end, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );
				trace.startsolid = bsptrace.startsolid;
				trace.fraction = bsptrace.fraction;
				VectorCopy( bsptrace.endpos, trace.endpos );
				trace.ent = bsptrace.ent;
			} else   {
				trace = AAS_TraceClientBBox( org, end, presencetype, entnum );
			}

			if ( visualize ) {
				if ( trace.startsolid ) {
					BotImport_Print( PRT_MESSAGE, "PredictMovement: start solid\n" );
				}
				AAS_DebugLine( org, trace.endpos, LINECOLOR_RED );
			}

			if ( GGameType & GAME_ET && stopevent & ETSE_HITENT ) {
				if ( bsptrace.fraction < 1.0 && bsptrace.ent == hitent ) {
					int areanum = AAS_PointAreaNum( org );
					VectorCopy( org, move->endpos );
					move->endarea = areanum;
					VectorScale( frame_test_vel, 1 / frametime, move->velocity );
					move->aasTrace = trace;
					move->bspTrace = bsptrace;
					move->stopevent = ETSE_HITENT;
					move->presencetype = aasworld->areasettings[ areanum ].presencetype;
					move->endcontents = 0;
					move->time = n * frametime;
					move->frames = n;
					return true;
				}
			}

			if ( stopevent & ( GGameType & GAME_Quake3 ? SE_ENTERAREA | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER | Q3SE_TOUCHCLUSTERPORTAL : SE_ENTERAREA ) ) {
				int areas[ 20 ];
				vec3_t points[ 20 ];
				int numareas = AAS_TraceAreas( org, trace.endpos, areas, points, 20 );
				for ( int i = 0; i < numareas; i++ ) {
					if ( stopevent & SE_ENTERAREA ) {
						if ( areas[ i ] == stopareanum ) {
							VectorCopy( points[ i ], move->endpos );
							VectorScale( frame_test_vel, 1 / frametime, move->velocity );
							move->endarea = areas[ i ];
							move->aasTrace = trace;
							move->bspTrace = bsptrace;
							move->stopevent = SE_ENTERAREA;
							move->presencetype = GGameType & GAME_ET ? aasworld->areasettings[ areas[ i ] ].presencetype : presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return true;
						}
					}
					//NOTE: if not the first frame
					if ( ( stopevent & SE_TOUCHJUMPPAD ) && n ) {
						if ( aasworld->areasettings[ areas[ i ] ].contents & AREACONTENTS_JUMPPAD ) {
							VectorCopy( points[ i ], move->endpos );
							VectorScale( frame_test_vel, 1 / frametime, move->velocity );
							move->endarea = areas[ i ];
							move->aasTrace = trace;
							move->bspTrace = bsptrace;
							move->stopevent = SE_TOUCHJUMPPAD;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return true;
						}
					}
					if ( stopevent & SE_TOUCHTELEPORTER ) {
						if ( aasworld->areasettings[ areas[ i ] ].contents & AREACONTENTS_TELEPORTER ) {
							VectorCopy( points[ i ], move->endpos );
							move->endarea = areas[ i ];
							VectorScale( frame_test_vel, 1 / frametime, move->velocity );
							move->aasTrace = trace;
							move->bspTrace = bsptrace;
							move->stopevent = SE_TOUCHTELEPORTER;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return true;
						}
					}
					if ( stopevent & Q3SE_TOUCHCLUSTERPORTAL ) {
						if ( aasworld->areasettings[ areas[ i ] ].contents & AREACONTENTS_CLUSTERPORTAL ) {
							VectorCopy( points[ i ], move->endpos );
							move->endarea = areas[ i ];
							VectorScale( frame_test_vel, 1 / frametime, move->velocity );
							move->aasTrace = trace;
							move->bspTrace = bsptrace;
							move->stopevent = Q3SE_TOUCHCLUSTERPORTAL;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return true;
						}
					}
				}
			}

			if ( GGameType & GAME_Quake3 && stopevent & Q3SE_HITBOUNDINGBOX ) {
				if ( AAS_ClipToBBox( &trace, org, trace.endpos, presencetype, mins, maxs ) ) {
					VectorCopy( trace.endpos, move->endpos );
					move->endarea = AAS_PointAreaNum( move->endpos );
					VectorScale( frame_test_vel, 1 / frametime, move->velocity );
					move->aasTrace = trace;
					move->stopevent = Q3SE_HITBOUNDINGBOX;
					move->presencetype = presencetype;
					move->endcontents = 0;
					move->time = n * frametime;
					move->frames = n;
					return true;
				}
			}

			if ( GGameType & GAME_ET && stopevent & ETSE_STUCK ) {
				if ( bsptrace.fraction < 1.0 ) {
					cplane_t* cplane = &bsptrace.plane;
					vec3_t wishdir;
					VectorNormalize2( frame_test_vel, wishdir );
					if ( DotProduct( cplane->normal, wishdir ) < -0.8 ) {
						int areanum = AAS_PointAreaNum( org );
						VectorCopy( org, move->endpos );
						move->endarea = areanum;
						VectorScale( frame_test_vel, 1 / frametime, move->velocity );
						move->aasTrace = trace;
						move->bspTrace = bsptrace;
						move->stopevent = ETSE_STUCK;
						move->presencetype = aasworld->areasettings[ areanum ].presencetype;
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return true;
					}
				}
			}

			//move the entity to the trace end point
			VectorCopy( trace.endpos, org );
			//if there was a collision
			if ( trace.fraction < 1.0 ) {
				//get the plane the bounding box collided with
				aas_plane_t* plane;
				if ( GGameType & GAME_ET ) {
					//JL HACK ALERT! Should use cplane_t internally.
					plane = reinterpret_cast<aas_plane_t*>( &bsptrace.plane );
				} else   {
					plane = AAS_PlaneFromNum( trace.planenum );
				}

				if ( stopevent & SE_HITGROUNDAREA ) {
					vec3_t up = {0, 0, 1};
					if ( DotProduct( plane->normal, up ) > phys_maxsteepness ) {
						vec3_t start;
						VectorCopy( org, start );
						start[ 2 ] += 0.5;
						if ( ( GGameType & GAME_ET && stopareanum < 0 && AAS_PointAreaNum( start ) ) ||
							 AAS_PointAreaNum( start ) == stopareanum ) {
							VectorCopy( start, move->endpos );
							move->endarea = stopareanum;
							VectorScale( frame_test_vel, 1 / frametime, move->velocity );
							move->aasTrace = trace;
							move->bspTrace = bsptrace;
							move->stopevent = SE_HITGROUNDAREA;
							move->presencetype = GGameType & GAME_ET ?
												 aasworld->areasettings[ stopareanum ].presencetype : presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return true;
						}
					}
				}
				//assume there's no step
				bool step = false;
				//if it is a vertical plane and the bot didn't jump recently
				if ( plane->normal[ 2 ] == 0 && ( jump_frame < 0 || n - jump_frame > 2 ) ) {
					//check for a step
					vec3_t start;
					VectorMA( org, -0.25, plane->normal, start );
					vec3_t stepend;
					VectorCopy( start, stepend );
					start[ 2 ] += phys_maxstep;
					aas_trace_t steptrace;
					bsp_trace_t stepbsptrace;
					if ( GGameType & GAME_ET ) {
						stepbsptrace = AAS_Trace( start, _mins, _maxs, stepend, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );
						steptrace.startsolid = stepbsptrace.startsolid;
						steptrace.fraction = stepbsptrace.fraction;
						VectorCopy( stepbsptrace.endpos, steptrace.endpos );
						steptrace.ent = stepbsptrace.ent;
					} else   {
						steptrace = AAS_TraceClientBBox( start, stepend, presencetype, entnum );
					}

					if ( !steptrace.startsolid ) {
						aas_plane_t* plane2;
						if ( GGameType & GAME_ET ) {
							//JL HACK again
							plane2 = reinterpret_cast<aas_plane_t*>( &stepbsptrace.plane );
						} else   {
							plane2 = AAS_PlaneFromNum( steptrace.planenum );
						}
						vec3_t up = {0, 0, 1};
						if ( DotProduct( plane2->normal, up ) > phys_maxsteepness ) {
							VectorSubtract( end, steptrace.endpos, left_test_vel );
							left_test_vel[ 2 ] = 0;
							frame_test_vel[ 2 ] = 0;
							if ( visualize ) {
								if ( steptrace.endpos[ 2 ] - org[ 2 ] > 0.125 ) {
									VectorCopy( org, start );
									start[ 2 ] = steptrace.endpos[ 2 ];
									AAS_DebugLine( org, start, LINECOLOR_BLUE );
								}
							}
							org[ 2 ] = steptrace.endpos[ 2 ];
							step = true;
						}
					}
				}
				//
				if ( !step ) {
					//velocity left to test for this frame is the projection
					//of the current test velocity into the hit plane
					VectorMA( left_test_vel, -DotProduct( left_test_vel, plane->normal ),
						plane->normal, left_test_vel );
					if ( GGameType & GAME_ET ) {
						// RF: from PM_SlideMove()
						// if this is the same plane we hit before, nudge velocity
						// out along it, which fixes some epsilon issues with
						// non-axial planes
						if ( lplane && DotProduct( lplane->normal, plane->normal ) > 0.99 ) {
							VectorAdd( plane->normal, left_test_vel, left_test_vel );
						}
						lplane = plane;
					}
					//store the old velocity for landing check
					vec3_t old_frame_test_vel;
					VectorCopy( frame_test_vel, old_frame_test_vel );
					//test velocity for the next frame is the projection
					//of the velocity of the current frame into the hit plane
					VectorMA( frame_test_vel, -DotProduct( frame_test_vel, plane->normal ),
						plane->normal, frame_test_vel );
					//check for a landing on an almost horizontal floor
					vec3_t up = {0, 0, 1};
					if ( DotProduct( plane->normal, up ) > phys_maxsteepness ) {
						onground = true;
					}
					if ( stopevent & SE_HITGROUNDDAMAGE ) {
						float delta = 0;
						if ( old_frame_test_vel[ 2 ] < 0 &&
							 frame_test_vel[ 2 ] > old_frame_test_vel[ 2 ] &&
							 !onground ) {
							delta = old_frame_test_vel[ 2 ];
						} else if ( onground )     {
							delta = frame_test_vel[ 2 ] - old_frame_test_vel[ 2 ];
						}
						if ( delta ) {
							delta = delta * 10;
							delta = delta * delta * 0.0001;
							if ( swimming ) {
								delta = 0;
							}
							// never take falling damage if completely underwater
							if ( delta > 40 ) {
								VectorCopy( org, move->endpos );
								move->endarea = AAS_PointAreaNum( org );
								VectorCopy( frame_test_vel, move->velocity );
								move->aasTrace = trace;
								move->bspTrace = bsptrace;
								move->stopevent = SE_HITGROUNDDAMAGE;
								if ( GGameType & GAME_ET ) {
									if ( move->endarea ) {
										move->presencetype = aasworld->areasettings[ move->endarea ].presencetype;
									}
								} else   {
									move->presencetype = presencetype;
								}
								move->endcontents = 0;
								move->time = n * frametime;
								move->frames = n;
								return true;
							}
						}
					}
				}
			}
			//extra check to prevent endless loop
			if ( ++j > 20 ) {
				return false;
			}
			//while there is a plane hit
		} while ( trace.fraction < 1.0 );
		//if going down
		if ( frame_test_vel[ 2 ] <= 10 ) {
			//check for a liquid at the feet of the bot
			vec3_t feet;
			VectorCopy( org, feet );
			feet[ 2 ] -= 22;
			int pc = AAS_PointContents( feet );
			//get event from pc
			int event = SE_NONE;
			if ( pc & BSP46CONTENTS_LAVA ) {
				event |= SE_ENTERLAVA;
			}
			if ( pc & BSP46CONTENTS_SLIME ) {
				event |= SE_ENTERSLIME;
			}
			if ( pc & BSP46CONTENTS_WATER ) {
				event |= SE_ENTERWATER;
			}
			//
			int areanum = AAS_PointAreaNum( org );
			if ( aasworld->areasettings[ areanum ].contents & AREACONTENTS_LAVA ) {
				event |= SE_ENTERLAVA;
			}
			if ( aasworld->areasettings[ areanum ].contents & AREACONTENTS_SLIME ) {
				event |= SE_ENTERSLIME;
			}
			if ( aasworld->areasettings[ areanum ].contents & AREACONTENTS_WATER ) {
				event |= SE_ENTERWATER;
			}
			//if in lava or slime
			if ( event & stopevent ) {
				VectorCopy( org, move->endpos );
				move->endarea = areanum;
				VectorScale( frame_test_vel, 1 / frametime, move->velocity );
				move->stopevent = event & stopevent;
				move->presencetype = GGameType & GAME_ET ? aasworld->areasettings[ areanum ].presencetype : presencetype;
				move->endcontents = pc;
				move->time = n * frametime;
				move->frames = n;
				return true;
			}
		}

		onground = AAS_OnGround( org, presencetype, entnum );
		//if onground and on the ground for at least one whole frame
		if ( onground ) {
			if ( stopevent & SE_HITGROUND ) {
				VectorCopy( org, move->endpos );
				move->endarea = AAS_PointAreaNum( org );
				VectorScale( frame_test_vel, 1 / frametime, move->velocity );
				move->aasTrace = trace;
				move->bspTrace = bsptrace;
				move->stopevent = SE_HITGROUND;
				if ( GGameType & GAME_ET ) {
					if ( move->endarea ) {
						move->presencetype = aasworld->areasettings[ move->endarea ].presencetype;
					}
				} else   {
					move->presencetype = presencetype;
				}
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return true;
			}
		} else if ( stopevent & SE_LEAVEGROUND )     {
			VectorCopy( org, move->endpos );
			move->endarea = AAS_PointAreaNum( org );
			VectorScale( frame_test_vel, 1 / frametime, move->velocity );
			move->aasTrace = trace;
			move->bspTrace = bsptrace;
			move->stopevent = SE_LEAVEGROUND;
			if ( GGameType & GAME_ET ) {
				if ( move->endarea ) {
					move->presencetype = aasworld->areasettings[ move->endarea ].presencetype;
				}
			} else   {
				move->presencetype = presencetype;
			}
			move->endcontents = 0;
			move->time = n * frametime;
			move->frames = n;
			return true;
		} else if ( stopevent & SE_GAP )     {
			aas_trace_t gaptrace;
			bsp_trace_t gapbsptrace;

			vec3_t start;
			VectorCopy( org, start );
			vec3_t end;
			VectorCopy( start, end );
			end[ 2 ] -= 48 + aassettings.phys_maxbarrier;
			if ( GGameType & GAME_ET ) {
				gapbsptrace = AAS_Trace( start, _mins, _maxs, end, -1, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP );
				gaptrace.startsolid = gapbsptrace.startsolid;
				gaptrace.fraction = gapbsptrace.fraction;
				VectorCopy( gapbsptrace.endpos, gaptrace.endpos );
				gaptrace.ent = gapbsptrace.ent;
			} else   {
				gaptrace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );
			}
			//if solid is found the bot cannot walk any further and will not fall into a gap
			if ( !gaptrace.startsolid ) {
				//if it is a gap (lower than one step height)
				if ( gaptrace.endpos[ 2 ] < org[ 2 ] - aassettings.phys_maxstep - 1 ) {
					//----(SA)	modified since slime is no longer deadly
					if ( !( AAS_PointContents( end ) & ( GGameType & GAME_Quake3 ? BSP46CONTENTS_WATER :
														 BSP46CONTENTS_WATER | BSP46CONTENTS_SLIME ) ) ) {
						VectorCopy( lastorg, move->endpos );
						move->endarea = AAS_PointAreaNum( lastorg );
						VectorScale( frame_test_vel, 1 / frametime, move->velocity );
						move->aasTrace = trace;
						move->bspTrace = bsptrace;
						move->stopevent = SE_GAP;
						if ( GGameType & GAME_ET ) {
							int areanum = AAS_PointAreaNum( org );
							if ( areanum ) {
								move->presencetype = aasworld->areasettings[ areanum ].presencetype;
							}
						} else   {
							move->presencetype = presencetype;
						}
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return true;
					}
				}
			}
		}
		if ( !( GGameType & GAME_Quake3 ) && stopevent & SE_TOUCHJUMPPAD ) {
			if ( aasworld->areasettings[ AAS_PointAreaNum( org ) ].contents & AREACONTENTS_JUMPPAD ) {
				VectorCopy( org, move->endpos );
				move->endarea = AAS_PointAreaNum( org );
				VectorScale( frame_test_vel, 1 / frametime, move->velocity );
				move->aasTrace = trace;
				move->bspTrace = bsptrace;
				move->stopevent = SE_TOUCHJUMPPAD;
				if ( GGameType & GAME_ET ) {
					if ( move->endarea ) {
						move->presencetype = aasworld->areasettings[ move->endarea ].presencetype;
					}
				} else   {
					move->presencetype = presencetype;
				}
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return true;
			}
		}
		if ( !( GGameType & GAME_Quake3 ) && stopevent & SE_TOUCHTELEPORTER ) {
			if ( aasworld->areasettings[ AAS_PointAreaNum( org ) ].contents & AREACONTENTS_TELEPORTER ) {
				VectorCopy( org, move->endpos );
				move->endarea = AAS_PointAreaNum( org );
				VectorScale( frame_test_vel, 1 / frametime, move->velocity );
				move->aasTrace = trace;
				move->bspTrace = bsptrace;
				move->stopevent = SE_TOUCHTELEPORTER;
				if ( GGameType & GAME_ET ) {
					if ( move->endarea ) {
						move->presencetype = aasworld->areasettings[ move->endarea ].presencetype;
					}
				} else   {
					move->presencetype = presencetype;
				}
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return true;
			}
		}
	}

	VectorCopy( org, move->endpos );
	move->endarea = AAS_PointAreaNum( org );
	VectorScale( frame_test_vel, 1 / frametime, move->velocity );
	move->stopevent = SE_NONE;
	if ( GGameType & GAME_ET ) {
		int areanum = AAS_PointAreaNum( org );
		move->presencetype = aasworld->areasettings ? aasworld->areasettings[ areanum ].presencetype : 0;
	} else   {
		move->presencetype = presencetype;
	}
	move->endcontents = 0;
	move->time = n * frametime;
	move->frames = n;

	return true;
}

bool AAS_PredictClientMovement( aas_clientmove_t* move,
	int entnum, const vec3_t origin,
	int presencetype, int hitent, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize ) {
	vec3_t mins, maxs;
	return AAS_ClientMovementPrediction( move, entnum, origin, presencetype,
		hitent, onground, velocity, cmdmove, cmdframes, maxframes, frametime,
		stopevent, stopareanum, mins, maxs, visualize );
}

bool AAS_PredictClientMovementQ3( aas_clientmove_q3_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize ) {
	aas_clientmove_t intMove;
	bool ret = AAS_PredictClientMovement( &intMove, entnum, origin, presencetype, 0, onground,
		velocity, cmdmove, cmdframes, maxframes,
		frametime, stopevent, stopareanum, visualize );
	VectorCopy( intMove.endpos, move->endpos );
	move->endarea = intMove.endarea;
	VectorCopy( intMove.velocity, move->velocity );
	move->trace = intMove.aasTrace;
	move->presencetype = intMove.presencetype;
	move->stopevent = intMove.stopevent;
	move->endcontents = intMove.endcontents;
	move->time = intMove.time;
	move->frames = intMove.frames;
	return ret;
}

bool AAS_PredictClientMovementWolf( aas_clientmove_rtcw_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize ) {
	aas_clientmove_t intMove;
	bool ret = AAS_PredictClientMovement( &intMove, entnum, origin, presencetype, 0, onground,
		velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
	VectorCopy( intMove.endpos, move->endpos );
	VectorCopy( intMove.velocity, move->velocity );
	move->trace = intMove.aasTrace;
	move->presencetype = intMove.presencetype;
	move->stopevent = intMove.stopevent;
	move->endcontents = intMove.endcontents;
	move->time = intMove.time;
	move->frames = intMove.frames;
	return ret;
}

bool AAS_PredictClientMovementET( aas_clientmove_et_t* move,
	int entnum, const vec3_t origin,
	int hitent, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize ) {
	aas_clientmove_t intMove;
	bool ret = AAS_PredictClientMovement( &intMove, entnum, origin, PRESENCE_NORMAL, hitent, onground,
		velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
	VectorCopy( intMove.endpos, move->endpos );
	VectorCopy( intMove.velocity, move->velocity );
	move->trace = intMove.bspTrace;
	move->presencetype = intMove.presencetype;
	move->stopevent = intMove.stopevent;
	move->endcontents = intMove.endcontents;
	move->time = intMove.time;
	move->frames = intMove.frames;
	return ret;
}

bool AAS_ClientMovementHitBBox( aas_clientmove_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	const vec3_t mins, const vec3_t maxs, bool visualize ) {
	return AAS_ClientMovementPrediction( move, entnum, origin, presencetype, 0, onground,
		velocity, cmdmove, cmdframes, maxframes,
		frametime, Q3SE_HITBOUNDINGBOX, 0,
		mins, maxs, visualize );
}

void AAS_JumpReachRunStart( const aas_reachability_t* reach, vec3_t runstart ) {
	vec3_t hordir;
	hordir[ 0 ] = reach->start[ 0 ] - reach->end[ 0 ];
	hordir[ 1 ] = reach->start[ 1 ] - reach->end[ 1 ];
	hordir[ 2 ] = 0;
	VectorNormalize( hordir );
	//start point
	vec3_t start;
	VectorCopy( reach->start, start );
	start[ 2 ] += 1;
	//get command movement
	vec3_t cmdmove;
	VectorScale( hordir, 400, cmdmove );

	aas_clientmove_t move;
	AAS_PredictClientMovement( &move, -1, start, PRESENCE_NORMAL, 0, true,
		vec3_origin, cmdmove, 1, 2, 0.1f,
		SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA |
		SE_HITGROUNDDAMAGE | SE_GAP, 0, false );
	VectorCopy( move.endpos, runstart );
	//don't enter slime or lava and don't fall from too high
	//----(SA)	modified since slime is no longer deadly
	if ( move.stopevent & ( ( GGameType & GAME_Quake3 ? SE_ENTERSLIME : 0 ) | SE_ENTERLAVA | SE_HITGROUNDDAMAGE ) ) {
		VectorCopy( start, runstart );
	}
}
