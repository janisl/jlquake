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

#include "ui.h"
#include "console.h"
#include "../client_main.h"
#include "../public.h"
#include "../game/particles.h"
#include "../game/quake_hexen2/main.h"
#include "../game/quake_hexen2/menu.h"
#include "../game/quake_hexen2/view.h"
#include "../game/quake/local.h"
#include "../game/hexen2/local.h"
#include "../game/quake2/local.h"
#include "../game/tech3/local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/system.h"

struct graphsamp_t {
	float value;
	int color;
};

bool scr_initialized;			// ready to draw

static int current;
static graphsamp_t values[ 1024 ];

static Cvar* cl_graphheight;
static Cvar* cl_graphscale;
static Cvar* cl_graphshift;

Cvar* scr_viewsize;
Cvar* crosshair;

static Cvar* show_fps;

Cvar* scr_showpause;

float h2_introTime = 0.0;

vrect_t scr_vrect;

static Cvar* scr_centertime;
static Cvar* scr_printspeed;

static char scr_centerstring[ 1024 ];
static float scr_centertime_start;				// for slow victory printing
static float scr_centertime_off;
static int scr_center_lines;

static qhandle_t scr_net;
static qhandle_t draw_backtile;

int scr_draw_loading;

Cvar* scr_netgraph;
Cvar* cl_debuggraph;
Cvar* cl_timegraph;

Cvar* clq2_stereo_separation;

void SCRQH_BeginLoadingPlaque() {
	S_StopAllSounds();

	if ( cls.state != CA_ACTIVE ) {
		return;
	}
	if ( clc.qh_signon != SIGNONS ) {
		return;
	}

	// redraw with no console and the loading plaque
	Con_ClearNotify();
	SCR_ClearCenterString();
	con.displayFrac = 0;

	scr_draw_loading = true;
	SCR_UpdateScreen();
	scr_draw_loading = false;

	cls.disable_screen = cls.realtime;
}

void SCRQ2_BeginLoadingPlaque( bool Clear ) {
	S_StopAllSounds();
	cl.q2_sound_prepped = false;		// don't play ambients
	CDAudio_Stop();
	if ( cls.disable_screen ) {
		return;
	}
	if ( com_developer->value ) {
		return;
	}
	if ( cls.state == CA_DISCONNECTED ) {
		return;	// if at console, don't bring up the plaque
	}
	if ( in_keyCatchers & KEYCATCH_CONSOLE ) {
		return;
	}
	scr_draw_loading = Clear ? 2 : 1;
	SCR_UpdateScreen();
	cls.disable_screen = Sys_Milliseconds();
	cls.q2_disable_servercount = cl.servercount;
}

void SCR_EndLoadingPlaque() {
	cls.disable_screen = 0;
	Con_ClearNotify();
}

void SCR_DebugGraph( float value, int color ) {
	values[ current & 1023 ].value = value;
	values[ current & 1023 ].color = color;
	current++;
}

void SCR_DrawDebugGraph() {
	//
	// draw the graph
	//
	int x = 0;
	int y, w;
	if ( GGameType & GAME_Tech3 ) {
		w = cls.glconfig.vidWidth;
		y = cls.glconfig.vidHeight;
	} else   {
		w = viddef.width;
		y = viddef.height;
	}
	if ( GGameType & GAME_Tech3 ) {
		R_SetColor( g_color_table[ 0 ] );
		R_StretchPic( x, y - cl_graphheight->integer,
			w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
		R_SetColor( NULL );
	} else   {
		UI_FillPal( x, y - cl_graphheight->integer,
			w, cl_graphheight->integer, 8 );
	}

	for ( int a = 0; a < w; a++ ) {
		int i = ( current - 1 - a + 1024 ) & 1023;
		float v = values[ i ].value;
		int color = values[ i ].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if ( v < 0 ) {
			v += cl_graphheight->integer * ( 1 + ( int )( -v / cl_graphheight->integer ) );
		}
		int h = ( int )v % cl_graphheight->integer;
		if ( GGameType & GAME_Tech3 ) {
			R_StretchPic( x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
		} else if ( GGameType & GAME_Quake2 )     {
			UI_FillPal( x + w - 1 - a, y - h, 1,  h, color );
		} else   {
			float r = ( color & 0xff ) / 255.0;
			float g = ( ( color >> 8 ) & 0xff ) / 255.0;
			float b = ( ( color >> 16 ) & 0xff ) / 255.0;
			UI_Fill( x + w - 1 - a, y - h, 1, h, r, g, b, 1 );
		}
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

//	Called for important messages that should stay in the center of the screen
// for a few moments
void SCR_CenterPrint( const char* str ) {
	String::NCpy( scr_centerstring, str, sizeof ( scr_centerstring ) - 1 );
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.qh_serverTimeFloat;

	// count the number of lines for centering
	scr_center_lines = 1;
	const char* s = str;
	while ( *s ) {
		if ( *s == '\n' ) {
			scr_center_lines++;
		}
		s++;
	}

	if ( GGameType & GAME_Quake2 ) {
		// echo it to the console
		common->Printf( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );

		const char* s = str;
		do {
			// scan the width of the line
			int l;
			for ( l = 0; l < 40; l++ ) {
				if ( s[ l ] == '\n' || !s[ l ] ) {
					break;
				}
			}
			int i;
			char line[ 64 ];
			for ( i = 0; i < ( 40 - l ) / 2; i++ ) {
				line[ i ] = ' ';
			}

			for ( int j = 0; j < l; j++ ) {
				line[ i++ ] = s[ j ];
			}

			line[ i ] = '\n';
			line[ i + 1 ] = 0;

			common->Printf( "%s", line );

			while ( *s && *s != '\n' ) {
				s++;
			}

			if ( !*s ) {
				break;
			}
			s++;		// skip the \n
		} while ( 1 );
		common->Printf( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );
		Con_ClearNotify();
	}
}

static void SCR_DrawCenterString() {
	if ( GGameType & GAME_Hexen2 ) {
		SCRH2_DrawCenterString( scr_centerstring );
		return;
	}

	// the finale prints the characters one at a time
	int remaining;
	if ( GGameType & GAME_Quake && cl.qh_intermission ) {
		remaining = scr_printspeed->value * ( cl.qh_serverTimeFloat - scr_centertime_start );
	} else   {
		remaining = 9999;
	}

	const char* start = scr_centerstring;

	int y;
	if ( scr_center_lines <= 4 ) {
		y = viddef.height * 0.35;
	} else   {
		y = 48;
	}

	do {
		// scan the width of the line
		char buf[ 41 ];
		int l;
		for ( l = 0; l < 40; l++ ) {
			if ( start[ l ] == '\n' || !start[ l ] ) {
				break;
			}
			buf[ l ] = start[ l ];
			if ( !remaining ) {
				break;
			}
			remaining--;
		}
		buf[ l ] = 0;
		int x = ( viddef.width - l * 8 ) / 2;
		UI_DrawString( x, y, buf );
		if ( !remaining ) {
			return;
		}

		y += 8;

		while ( *start && *start != '\n' ) {
			start++;
		}

		if ( !*start ) {
			break;
		}
		start++;		// skip the \n
	} while ( 1 );
}

void SCR_CheckDrawCenterString() {
	scr_centertime_off -= cls.frametime * 0.001;

	if ( scr_centertime_off <= 0 && ( GGameType & GAME_Quake2 || !cl.qh_intermission ) ) {
		return;
	}
	if ( !( GGameType & GAME_Quake2 ) && in_keyCatchers != 0 ) {
		return;
	}

	SCR_DrawCenterString();
}

void SCR_ClearCenterString() {
	scr_centertime_off = 0;
}

//	Sets scr_vrect, the coordinates of the rendered window
void SCR_CalcVrect() {
	// bound viewsize
	if ( GGameType & GAME_QuakeHexen ) {
		if ( scr_viewsize->value < 30 ) {
			Cvar_Set( "viewsize","30" );
		}
		if ( scr_viewsize->value > 120 ) {
			Cvar_Set( "viewsize","120" );
		}

		if ( GGameType & GAME_Hexen2 ) {
			// intermission is always full screen
			if ( cl.qh_intermission || scr_viewsize->value >= 110 ) {
				// No status bar
				sbqh_lines = 0;
			} else   {
				sbqh_lines = 36;
			}
		} else   {
			// intermission is always full screen
			if ( cl.qh_intermission || scr_viewsize->value >= 120 ) {
				sbqh_lines = 0;		// no status bar at all
			} else if ( scr_viewsize->value >= 110 )     {
				sbqh_lines = 24;	// no inventory
			} else   {
				sbqh_lines = 24 + 16 + 8;
			}
		}
	} else   {
		if ( scr_viewsize->value < 40 ) {
			Cvar_Set( "viewsize","40" );
		}
		if ( scr_viewsize->value > 100 ) {
			Cvar_Set( "viewsize","100" );
		}
	}

	int size = !( GGameType & GAME_Quake2 ) && ( scr_viewsize->value >= 100 || cl.qh_intermission ) ? 100 : scr_viewsize->value;

	int h = ( GGameType & GAME_QuakeWorld && !clqh_sbar->value && size == 100 ) || GGameType & GAME_Quake2 ? viddef.height : viddef.height - sbqh_lines;

	scr_vrect.width = viddef.width * size / 100;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height * size / 100;
	if ( scr_vrect.height > h ) {
		scr_vrect.height = h;
	}
	scr_vrect.height &= ~1;

	scr_vrect.x = ( viddef.width - scr_vrect.width ) / 2;
	scr_vrect.y = ( h - scr_vrect.height ) / 2;
}

//	Keybinding command
static void SCR_SizeUp_f() {
	Cvar_SetValue( "viewsize",scr_viewsize->value + 10 );
}

//	Keybinding command
static void SCR_SizeDown_f() {
	Cvar_SetValue( "viewsize",scr_viewsize->value - 10 );
}

void SCR_DrawNet() {
	if ( GGameType & GAME_Quake2 ) {
		if ( clc.netchan.outgoingSequence - clc.netchan.incomingAcknowledged < CMD_BACKUP_Q2 - 1 ) {
			return;
		}
	} else if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) )       {
		if ( clc.netchan.outgoingSequence - clc.netchan.incomingAcknowledged < UPDATE_BACKUP_QW - 1 ) {
			return;
		}
	} else   {
		if ( cls.realtime - cl.qh_last_received_message * 1000 < 300 ) {
			return;
		}
	}
	if ( clc.demoplaying ) {
		return;
	}

	if ( GGameType & GAME_Quake2 ) {
		UI_DrawNamedPic( scr_vrect.x + 64, scr_vrect.y, "net" );
	} else   {
		UI_DrawPicShader( scr_vrect.x + 64, scr_vrect.y, scr_net );
	}
}

void SCR_DrawFPS() {
	static int lastframetime;
	static int lastframecount;
	static int lastfps;

	if ( !show_fps->value ) {
		return;
	}

	int t = Sys_Milliseconds();
	if ( ( t - lastframetime ) >= 1000 ) {
		lastfps = cls.framecount - lastframecount;
		lastframecount = cls.framecount;
		lastframetime = t;
	}

	char st[ 80 ];
	sprintf( st, "%3d FPS", lastfps );
	int x = viddef.width - String::Length( st ) * 8 - 8;
	int y = viddef.height - sbqh_lines - 8;
	UI_DrawString( x, y, st );
}

//	Clear any parts of the tiled background that were drawn on last frame
void SCR_TileClear() {
	if ( con.displayFrac == 1.0 ) {
		return;		// full screen console
	}
	if ( scr_vrect.height == viddef.height ) {
		return;		// full screen rendering
	}

	int top = scr_vrect.y;
	int bottom = top + scr_vrect.height;
	int left = scr_vrect.x;
	int right = left + scr_vrect.width;

	if ( top > 0 ) {
		// clear above view screen
		if ( GGameType & GAME_Quake2 ) {
			UI_NamedTileClear( 0, 0, viddef.width, top, "backtile" );
		} else   {
			UI_TileClear( 0, 0, viddef.width, top, draw_backtile );
		}
	}
	if ( viddef.height > bottom ) {
		// clear below view screen
		if ( GGameType & GAME_Quake2 ) {
			UI_NamedTileClear( 0, bottom, viddef.width, viddef.height - bottom, "backtile" );
		} else   {
			UI_TileClear( 0, bottom, viddef.width, viddef.height - bottom, draw_backtile );
		}
	}
	if ( left > 0 ) {
		// clear left of view screen
		if ( GGameType & GAME_Quake2 ) {
			UI_NamedTileClear( 0, top, left, scr_vrect.height, "backtile" );
		} else   {
			UI_TileClear( 0, top, left, scr_vrect.height, draw_backtile );
		}
	}
	if ( viddef.width > right ) {
		// clear left of view screen
		if ( GGameType & GAME_Quake2 ) {
			UI_NamedTileClear( right, top, viddef.width - right, scr_vrect.height, "backtile" );
		} else   {
			UI_TileClear( right, top, viddef.width - right, scr_vrect.height, draw_backtile );
		}
	}
}

//	This is called every frame, and can also be called explicitly to flush
// text to the screen.
void SCR_UpdateScreen() {
	if ( GGameType & GAME_Quake ) {
		SCRQ1_DrawScreen( STEREO_CENTER );
	} else if ( GGameType & GAME_Hexen2 )     {
		SCRH2_DrawScreen( STEREO_CENTER );
	} else if ( GGameType & GAME_Quake2 )     {
		// if the screen is disabled (loading plaque is up, or vid mode changing)
		// do nothing at all
		if ( cls.disable_screen ) {
			if ( Sys_Milliseconds() - cls.disable_screen > 120000 ) {
				cls.disable_screen = 0;
				common->Printf( "Loading plaque timed out.\n" );
			}
			return;
		}

		if ( !scr_initialized ) {
			return;				// not initialized yet

		}
		/*
		** range check cl_camera_separation so we don't inadvertently fry someone's
		** brain
		*/
		if ( clq2_stereo_separation->value > 1.0 ) {
			Cvar_SetValueLatched( "cl_stereo_separation", 1.0 );
		} else if ( clq2_stereo_separation->value < 0 )     {
			Cvar_SetValueLatched( "cl_stereo_separation", 0.0 );
		}

		if ( cls.glconfig.stereoEnabled ) {
			SCRQ2_DrawScreen( STEREO_LEFT, -clq2_stereo_separation->value / 2 );
			SCRQ2_DrawScreen( STEREO_RIGHT, clq2_stereo_separation->value / 2 );
		} else   {
			SCRQ2_DrawScreen( STEREO_CENTER, 0 );
		}

		R_EndFrame( NULL, NULL );

		if ( cls.state == CA_ACTIVE && cl.q2_refresh_prepped && cl.q2_frame.valid ) {
			CL_UpdateParticles( 800 );
		}
	} else   {
		static int recursive;

		if ( !scr_initialized ) {
			return;				// not initialized yet
		}

		if ( GGameType & GAME_ET ) {
			if ( ++recursive >= 2 ) {
				recursive = 0;
				// Gordon: i'm breaking this again, because we've removed most of our cases but still have one which will not fix easily
				return;
				//Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
			}
		} else   {
			if ( ++recursive > 2 ) {
				common->FatalError( "SCR_UpdateScreen: recursively called" );
			}
		}
		recursive = 1;

		// if running in stereo, we need to draw the frame twice
		if ( cls.glconfig.stereoEnabled ) {
			SCRT3_DrawScreenField( STEREO_LEFT );
			SCRT3_DrawScreenField( STEREO_RIGHT );
		} else   {
			SCRT3_DrawScreenField( STEREO_CENTER );
		}

		if ( com_speeds->integer ) {
			R_EndFrame( &time_frontend, &time_backend );
		} else   {
			R_EndFrame( NULL, NULL );
		}

		recursive = 0;
	}
}

void SCRQH_InitImages() {
	scr_net = R_ShaderFromWad( "net" );
	if ( GGameType & GAME_Quake ) {
		draw_backtile = R_ShaderFromWadRepeat( "backtile" );
		char_texture = R_LoadRawFontImageFromWad( "conchars", 128, 128 );
	} else   {
		draw_backtile = R_CacheShaderRepeat( "gfx/menu/backtile.lmp" );
		char_texture = R_LoadRawFontImageFromFile( "gfx/menu/conchars.lmp", 256, 128 );
		char_smalltexture = R_LoadRawFontImageFromWad( "tinyfont", 128, 32 );
	}
	Con_InitBackgroundImage();
	MQH_InitImages();
	VQH_InitCrosshairTexture();
}

void SCR_Init() {
	cl_graphheight = Cvar_Get( "graphheight", "32", CVAR_CHEAT );
	cl_graphscale = Cvar_Get( "graphscale", "1", CVAR_CHEAT );
	cl_graphshift = Cvar_Get( "graphshift", "0", CVAR_CHEAT );
	if ( !( GGameType & GAME_Tech3 ) ) {
		scr_printspeed = Cvar_Get( "scr_printspeed", "8", 0 );
		scr_viewsize = Cvar_Get( "viewsize", "100", CVAR_ARCHIVE );
		show_fps = Cvar_Get( "show_fps", "0", CVAR_ARCHIVE );			// set for running times
		scr_showpause = Cvar_Get( "scr_showpause", "1", 0 );
	}
	if ( !( GGameType & GAME_QuakeHexen ) ) {
		cl_timegraph = Cvar_Get( "timegraph", "0", CVAR_CHEAT );
		cl_debuggraph = Cvar_Get( "debuggraph", "0", CVAR_CHEAT );
	}
	if ( GGameType & GAME_Quake ) {
		scr_centertime = Cvar_Get( "scr_centertime", "2", 0 );
	} else if ( GGameType & GAME_Hexen2 )     {
		scr_centertime = Cvar_Get( "scr_centertime", "4", 0 );
	} else if ( GGameType & GAME_Quake2 )     {
		scr_centertime = Cvar_Get( "scr_centertime", "2.5", 0 );
		clq2_stereo_separation = Cvar_Get( "cl_stereo_separation", "0.4", CVAR_ARCHIVE );
	}
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2 ) ) {
		scr_netgraph = Cvar_Get( "netgraph", "0", 0 );
	}

	//
	// register our commands
	//
	if ( !( GGameType & GAME_Tech3 ) ) {
		Cmd_AddCommand( "sizeup", SCR_SizeUp_f );
		Cmd_AddCommand( "sizedown", SCR_SizeDown_f );
	}

	if ( GGameType & GAME_Quake ) {
		SCRQ1_Init();
	}
	if ( GGameType & GAME_Hexen2 ) {
		SCRH2_Init();
	}
	if ( GGameType & GAME_Quake2 ) {
		SCRQ2_Init();
	}

	scr_initialized = true;
}
