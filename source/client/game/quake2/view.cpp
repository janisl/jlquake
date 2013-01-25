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
#include "../../client_main.h"
#include "../../ui/ui.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../light_styles.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/system.h"

static Cvar* clq2_showclamp;
static Cvar* clq2_gun;
static Cvar* clq2_add_entities;
static Cvar* clq2_add_particles;
static Cvar* clq2_add_blend;

static Cvar* clq2_testparticles;
static Cvar* clq2_testentities;
static Cvar* clq2_testlights;
static Cvar* clq2_testblend;

//
// development tools for weapons
//
static int gun_frame;
static qhandle_t gun_model;

static char crosshair_pic[ MAX_QPATH ];
static int crosshair_width, crosshair_height;

//	Allows rendering code to cache all needed sbar graphics
void SCR_TouchPics() {
	int i, j;

	for ( i = 0; i < 2; i++ )
		for ( j = 0; j < 11; j++ )
			R_RegisterPic( sb_nums[ i ][ j ] );

	if ( crosshair->value ) {
		if ( crosshair->value > 3 || crosshair->value < 0 ) {
			crosshair->value = 3;
		}

		String::Sprintf( crosshair_pic, sizeof ( crosshair_pic ), "ch%i", ( int )( crosshair->value ) );
		R_GetPicSize( &crosshair_width, &crosshair_height, crosshair_pic );
		if ( !crosshair_width ) {
			crosshair_pic[ 0 ] = 0;
		}
	}
}

static void CLQ2_CalcLerpFrac() {
	if ( cl.serverTime > cl.q2_frame.servertime ) {
		if ( clq2_showclamp->value ) {
			common->Printf( "high clamp %i\n", cl.serverTime - cl.q2_frame.servertime );
		}
		cl.serverTime = cl.q2_frame.servertime;
		cl.q2_lerpfrac = 1.0;
	} else if ( cl.serverTime < cl.q2_frame.servertime - 100 )     {
		if ( clq2_showclamp->value ) {
			common->Printf( "low clamp %i\n", cl.q2_frame.servertime - 100 - cl.serverTime );
		}
		cl.serverTime = cl.q2_frame.servertime - 100;
		cl.q2_lerpfrac = 0;
	} else   {
		cl.q2_lerpfrac = 1.0 - ( cl.q2_frame.servertime - cl.serverTime ) * 0.01;
	}

	if ( cl_timedemo->value ) {
		cl.q2_lerpfrac = 1.0;
	}
}

static void CLQ2_AddViewWeapon( q2player_state_t* ps, q2player_state_t* ops, vec3_t viewangles ) {
	// allow the gun to be completely removed
	if ( !clq2_gun->value ) {
		return;
	}

	// don't draw gun if in wide angle view
	if ( ps->fov > 90 ) {
		return;
	}

	refEntity_t gun = {};		// view model

	if ( gun_model ) {
		gun.hModel = gun_model;	// development tool
	} else   {
		gun.hModel = cl.model_draw[ ps->gunindex ];
	}
	if ( !gun.hModel ) {
		return;
	}

	// set up gun position
	vec3_t angles;
	for ( int i = 0; i < 3; i++ ) {
		gun.origin[ i ] = cl.refdef.vieworg[ i ] + ops->gunoffset[ i ]
						  + cl.q2_lerpfrac * ( ps->gunoffset[ i ] - ops->gunoffset[ i ] );
		angles[ i ] = viewangles[ i ] + LerpAngle( ops->gunangles[ i ],
			ps->gunangles[ i ], cl.q2_lerpfrac );
	}
	AnglesToAxis( angles, gun.axis );

	if ( gun_frame ) {
		gun.frame = gun_frame;	// development tool
		gun.oldframe = gun_frame;	// development tool
	} else   {
		gun.frame = ps->gunframe;
		if ( gun.frame == 0 ) {
			gun.oldframe = 0;	// just changed weapons, don't lerp from old
		} else   {
			gun.oldframe = ops->gunframe;
		}
	}

	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	if ( q2_hand->integer == 1 ) {
		gun.renderfx |= RF_LEFTHAND;
	}
	gun.backlerp = 1.0 - cl.q2_lerpfrac;
	VectorCopy( gun.origin, gun.oldorigin );	// don't lerp at all
	R_AddRefEntityToScene( &gun );
}

//	Sets cl.refdef view values
static void CLQ2_CalcViewValues( float* blendColour ) {
	CLQ2_CalcLerpFrac();

	// find the previous frame to interpolate from
	q2player_state_t* ps = &cl.q2_frame.playerstate;
	int i = ( cl.q2_frame.serverframe - 1 ) & UPDATE_MASK_Q2;
	q2frame_t* oldframe = &cl.q2_frames[ i ];
	if ( oldframe->serverframe != cl.q2_frame.serverframe - 1 || !oldframe->valid ) {
		oldframe = &cl.q2_frame;		// previous frame was dropped or involid
	}
	q2player_state_t* ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( abs( ops->pmove.origin[ 0 ] - ps->pmove.origin[ 0 ] ) > 256 * 8 ||
		 abs( ops->pmove.origin[ 1 ] - ps->pmove.origin[ 1 ] ) > 256 * 8 ||
		 abs( ops->pmove.origin[ 2 ] - ps->pmove.origin[ 2 ] ) > 256 * 8 ) {
		ops = ps;		// don't interpolate

	}
	float lerp = cl.q2_lerpfrac;

	// calculate the origin
	if ( ( clq2_predict->value ) && !( cl.q2_frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION ) ) {
		// use predicted values
		float backlerp = 1.0 - lerp;
		for ( i = 0; i < 3; i++ ) {
			cl.refdef.vieworg[ i ] = cl.q2_predicted_origin[ i ] + ops->viewoffset[ i ]
									 + cl.q2_lerpfrac * ( ps->viewoffset[ i ] - ops->viewoffset[ i ] )
									 - backlerp * cl.q2_prediction_error[ i ];
		}

		// smooth out stair climbing
		unsigned delta = cls.realtime - cl.q2_predicted_step_time;
		if ( delta < 100 ) {
			cl.refdef.vieworg[ 2 ] -= cl.q2_predicted_step * ( 100 - delta ) * 0.01;
		}
	} else   {
		// just use interpolated values
		for ( i = 0; i < 3; i++ ) {
			cl.refdef.vieworg[ i ] = ops->pmove.origin[ i ] * 0.125 + ops->viewoffset[ i ]
									 + lerp * ( ps->pmove.origin[ i ] * 0.125 + ps->viewoffset[ i ]
												- ( ops->pmove.origin[ i ] * 0.125 + ops->viewoffset[ i ] ) );
		}
	}

	// if not running a demo or on a locked frame, add the local angle movement
	vec3_t viewangles;
	if ( cl.q2_frame.playerstate.pmove.pm_type < Q2PM_DEAD ) {
		// use predicted values
		for ( i = 0; i < 3; i++ ) {
			viewangles[ i ] = cl.q2_predicted_angles[ i ];
		}
	} else   {
		// just use interpolated values
		for ( i = 0; i < 3; i++ ) {
			viewangles[ i ] = LerpAngle( ops->viewangles[ i ], ps->viewangles[ i ], lerp );
		}
	}

	for ( i = 0; i < 3; i++ ) {
		viewangles[ i ] += LerpAngle( ops->kick_angles[ i ], ps->kick_angles[ i ], lerp );
	}

	AnglesToAxis( viewangles, cl.refdef.viewaxis );

	// interpolate field of view
	cl.refdef.fov_x = ops->fov + lerp * ( ps->fov - ops->fov );

	// don't interpolate blend color
	for ( i = 0; i < 4; i++ ) {
		blendColour[ i ] = ps->blend[ i ];
	}

	if ( clq2_add_entities->integer ) {
		// add the weapon
		CLQ2_AddViewWeapon( ps, ops, viewangles );
	}
}

//	If clq2_testparticles is set, create 4096 particles in the view
static void VQ2_TestParticles() {
	for ( int i = 0; i < MAX_PARTICLES; i++ ) {
		float d = i * 0.25;
		float r = 4 * ( ( i & 7 ) - 3.5 );
		float u = 4 * ( ( ( i >> 3 ) & 7 ) - 3.5 );

		vec3_t origin;
		for ( int j = 0; j < 3; j++ ) {
			origin[ j ] = cl.refdef.vieworg[ j ] + cl.refdef.viewaxis[ 0 ][ j ] * d -
						  cl.refdef.viewaxis[ 1 ][ j ] * r + cl.refdef.viewaxis[ 2 ][ j ] * u;
		}

		R_AddParticleToScene( origin, r_palette[ 8 ][ 0 ], r_palette[ 8 ][ 1 ], r_palette[ 8 ][ 2 ], ( int )( clq2_testparticles->value * 255 ), 1, PARTTEX_Default );
	}
}

//	If clq2_testentities is set, create 32 player models
static void VQ2_TestEntities() {
	for ( int i = 0; i < 32; i++ ) {
		refEntity_t ent;
		Com_Memset( &ent, 0, sizeof ( ent ) );

		float r = 64 * ( ( i % 4 ) - 1.5 );
		float f = 64 * ( i / 4 ) + 128;

		for ( int j = 0; j < 3; j++ ) {
			ent.origin[ j ] = cl.refdef.vieworg[ j ] + cl.refdef.viewaxis[ 0 ][ j ] * f -
							  cl.refdef.viewaxis[ 1 ][ j ] * r;
		}
		AxisClear( ent.axis );

		ent.hModel = cl.q2_baseclientinfo.model;
		ent.customSkin = R_GetImageHandle( cl.q2_baseclientinfo.skin );
		R_AddRefEntityToScene( &ent );
	}
}

//	If clq2_testlights is set, create 32 lights models
static void VQ2_TestLights() {
	for ( int i = 0; i < 32; i++ ) {
		float r = 64 * ( ( i % 4 ) - 1.5 );
		float f = 64 * ( i / 4 ) + 128;

		vec3_t origin;
		for ( int j = 0; j < 3; j++ ) {
			origin[ j ] = cl.refdef.vieworg[ j ] + cl.refdef.viewaxis[ 0 ][ j ] * f -
						  cl.refdef.viewaxis[ 1 ][ j ] * r;
		}
		R_AddLightToScene( origin, 200, ( ( i % 6 ) + 1 ) & 1, ( ( ( i % 6 ) + 1 ) & 2 ) >> 1, ( ( ( i % 6 ) + 1 ) & 4 ) >> 2 );
	}
}

static void VQ2_DrawCrosshair() {
	if ( !crosshair->value ) {
		return;
	}

	if ( crosshair->modified ) {
		crosshair->modified = false;
		SCR_TouchPics();
	}

	if ( !crosshair_pic[ 0 ] ) {
		return;
	}

	UI_DrawNamedPic( scr_vrect.x + ( ( scr_vrect.width - crosshair_width ) >> 1 ),
		scr_vrect.y + ( ( scr_vrect.height - crosshair_height ) >> 1 ), crosshair_pic );
}

void VQ2_RenderView( float stereo_separation ) {
	if ( cls.state != CA_ACTIVE ) {
		return;
	}

	if ( !cl.q2_refresh_prepped ) {
		return;			// still loading

	}
	if ( cl_timedemo->value ) {
		if ( !cl.q2_timedemo_start ) {
			cl.q2_timedemo_start = Sys_Milliseconds();
		}
		cl.q2_timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( !cl.q2_frame.valid ) {
		return;
	}

	R_ClearScene();

	float v_blend[ 4 ];
	CLQ2_CalcViewValues( v_blend );

	if ( clq2_add_entities->integer ) {
		if ( clq2_testentities->integer ) {
			VQ2_TestEntities();
		} else   {
			CLQ2_AddPacketEntities( &cl.q2_frame );
			CLQ2_AddTEnts();
		}
	}

	if ( clq2_add_particles->integer ) {
		if ( clq2_testparticles->integer ) {
			VQ2_TestParticles();
		} else   {
			CL_AddParticles();
		}
	}

	if ( clq2_testlights->integer ) {
		VQ2_TestLights();
	} else   {
		CL_AddDLights();
	}

	CL_AddLightStyles();

	if ( clq2_testblend->value ) {
		v_blend[ 0 ] = 1;
		v_blend[ 1 ] = 0.5;
		v_blend[ 2 ] = 0.25;
		v_blend[ 3 ] = 0.5;
	}

	// offset vieworg appropriately if we're doing stereo separation
	if ( stereo_separation != 0 ) {
		vec3_t tmp;

		VectorScale( cl.refdef.viewaxis[ 1 ], -stereo_separation, tmp );
		VectorAdd( cl.refdef.vieworg, tmp, cl.refdef.vieworg );
	}

	// never let it sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	cl.refdef.vieworg[ 0 ] += 1.0 / 16;
	cl.refdef.vieworg[ 1 ] += 1.0 / 16;
	cl.refdef.vieworg[ 2 ] += 1.0 / 16;

	cl.refdef.x = scr_vrect.x;
	cl.refdef.y = scr_vrect.y;
	cl.refdef.width = scr_vrect.width;
	cl.refdef.height = scr_vrect.height;
	cl.refdef.fov_y = CalcFov( cl.refdef.fov_x, cl.refdef.width, cl.refdef.height );
	cl.refdef.time = cl.serverTime;

	for ( int i = 0; i < MAX_MAP_AREA_BYTES; i++ ) {
		cl.refdef.areamask[ i ] = ~cl.q2_frame.areabits[ i ];
	}

	if ( !clq2_add_blend->value ) {
		VectorClear( v_blend );
	}

	cl.refdef.rdflags = 0;
	if ( cl.q2_frame.playerstate.rdflags & Q2RDF_NOWORLDMODEL ) {
		cl.refdef.rdflags |= RDF_NOWORLDMODEL;
	}
	if ( cl.q2_frame.playerstate.rdflags & Q2RDF_IRGOGGLES ) {
		cl.refdef.rdflags |= RDF_IRGOGGLES;
	}

	R_RenderScene( &cl.refdef );

	R_VerifyNoRenderCommands();
	R_PolyBlend( &cl.refdef, v_blend );
	R_SyncRenderThread();

	VQ2_DrawCrosshair();
}

// gun frame debugging functions
static void VQ2_Gun_Next_f() {
	gun_frame++;
	common->Printf( "frame %i\n", gun_frame );
}

static void VQ2_Gun_Prev_f() {
	gun_frame--;
	if ( gun_frame < 0 ) {
		gun_frame = 0;
	}
	common->Printf( "frame %i\n", gun_frame );
}

static void VQ2_Gun_Model_f() {
	if ( Cmd_Argc() != 2 ) {
		gun_model = 0;
		return;
	}
	char name[ MAX_QPATH ];
	String::Sprintf( name, sizeof ( name ), "models/%s/tris.md2", Cmd_Argv( 1 ) );
	gun_model = R_RegisterModel( name );
}

static void VQ2_Viewpos_f() {
	common->Printf( "(%i %i %i) : %i\n", ( int )cl.refdef.vieworg[ 0 ],
		( int )cl.refdef.vieworg[ 1 ], ( int )cl.refdef.vieworg[ 2 ],
		( int )VecToYaw( cl.refdef.viewaxis[ 0 ] ) );
}

static void VQ2_TimeRefreshScene() {
	R_ClearScene();
	if ( clq2_add_entities->integer ) {
		CLQ2_AddPacketEntities( &cl.q2_frame );
		CLQ2_AddTEnts();
	}
	if ( clq2_add_particles->integer ) {
		CL_AddParticles();
	}
	CL_AddDLights();
	CL_AddLightStyles();
	R_RenderScene( &cl.refdef );
}

static void SCRQ2_TimeRefresh_f() {
	if ( cls.state != CA_ACTIVE ) {
		return;
	}

	int start = Sys_Milliseconds();

	vec3_t viewangles;
	viewangles[ 0 ] = 0;
	viewangles[ 1 ] = 0;
	viewangles[ 2 ] = 0;
	if ( Cmd_Argc() == 2 ) {
		// run without page flipping
		R_VerifyNoRenderCommands();
		R_BeginFrame( STEREO_CENTER );
		R_SyncRenderThread();
		for ( int i = 0; i < 128; i++ ) {
			viewangles[ 1 ] = i / 128.0 * 360.0;
			AnglesToAxis( viewangles, cl.refdef.viewaxis );
			VQ2_TimeRefreshScene();
		}
		R_VerifyNoRenderCommands();
		R_EndFrame( NULL, NULL );
	} else   {
		for ( int i = 0; i < 128; i++ ) {
			viewangles[ 1 ] = i / 128.0 * 360.0;
			AnglesToAxis( viewangles, cl.refdef.viewaxis );

			R_VerifyNoRenderCommands();
			R_BeginFrame( STEREO_CENTER );
			R_SyncRenderThread();
			VQ2_TimeRefreshScene();
			R_VerifyNoRenderCommands();
			R_EndFrame( NULL, NULL );
		}
	}

	int stop = Sys_Milliseconds();
	float time = ( stop - start ) / 1000.0;
	common->Printf( "%f seconds (%f fps)\n", time, 128 / time );
}

void VQ2_Init() {
	Cmd_AddCommand( "gun_next", VQ2_Gun_Next_f );
	Cmd_AddCommand( "gun_prev", VQ2_Gun_Prev_f );
	Cmd_AddCommand( "gun_model", VQ2_Gun_Model_f );

	Cmd_AddCommand( "viewpos", VQ2_Viewpos_f );
	Cmd_AddCommand( "timerefresh", SCRQ2_TimeRefresh_f );

	clq2_showclamp = Cvar_Get( "showclamp", "0", 0 );
	clq2_gun = Cvar_Get( "cl_gun", "1", 0 );
	clq2_add_entities = Cvar_Get( "cl_entities", "1", 0 );
	clq2_add_particles = Cvar_Get( "cl_particles", "1", 0 );
	clq2_add_blend = Cvar_Get( "cl_blend", "1", 0 );

	clq2_testblend = Cvar_Get( "cl_testblend", "0", 0 );
	clq2_testparticles = Cvar_Get( "cl_testparticles", "0", 0 );
	clq2_testentities = Cvar_Get( "cl_testentities", "0", 0 );
	clq2_testlights = Cvar_Get( "cl_testlights", "0", CVAR_CHEAT );
}
