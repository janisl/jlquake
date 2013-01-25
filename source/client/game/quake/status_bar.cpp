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
#include "../camera.h"
#include "../../ui/console.h"
#include "../../../server/public.h"
#include "../quake_hexen2/main.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"

#define Q1SBAR_HEIGHT     24

#define STAT_MINUS      10	// num frame for '-' stats digit

struct team_t {
	char team[ 16 + 1 ];
	int frags;
	int players;
	int plow, phigh, ptotal;
};

int sbqh_lines;					// scan lines to draw

Cvar* clqw_hudswap;

static image_t* sbq1_nums[ 2 ][ 11 ];
static image_t* sbq1_colon;
static image_t* sbq1_slash;
static image_t* sbq1_sbar;
static image_t* sbq1_ibar;
static image_t* sbq1_scorebar;

static image_t* sbq1_weapons[ 7 ][ 8 ];		// 0 is active, 1 is owned, 2-5 are flashes
static image_t* sbq1_ammo[ 4 ];
static image_t* sbq1_sigil[ 4 ];
static image_t* sbq1_armor[ 3 ];
static image_t* sbq1_items[ 32 ];
static image_t* sbq1_disc;

static image_t* sbq1_faces[ 7 ][ 2 ];		// 0 is gibbed, 1 is dead, 2-6 are alive
// 0 is static, 1 is temporary animation
static image_t* sbq1_face_invis;
static image_t* sbq1_face_quad;
static image_t* sbq1_face_invuln;
static image_t* sbq1_face_invis_invuln;

static image_t* rsbq1_invbar[ 2 ];
static image_t* rsbq1_weapons[ 5 ];
static image_t* rsbq1_items[ 2 ];
static image_t* rsbq1_ammo[ 3 ];
static image_t* rsbq1_teambord;			// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
static image_t* hsbq1_weapons[ 7 ][ 5 ];			// 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
static int hipweapons[ 4 ] = {Q1HIT_LASER_CANNON_BIT,Q1HIT_MJOLNIR_BIT,4,Q1HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
static image_t* hsbq1_items[ 2 ];

static bool sbq1_showscores;
static bool sbq1_showteamscores;

static bool sbq1_largegame = false;

static int sbq1_fragsort[ BIGGEST_MAX_CLIENTS_QH ];
static int sbq1_scoreboardlines;
static team_t sbq1_teams[ MAX_CLIENTS_QHW ];
static int sbq1_teamsort[ MAX_CLIENTS_QHW ];
static int sbq1_scoreboardteams;

static void SbarQ1_ShowScores() {
	if ( sbq1_showscores ) {
		return;
	}
	sbq1_showscores = true;
}

static void SbarQ1_DontShowScores() {
	sbq1_showscores = false;
}

static void SbarQW_ShowTeamScores() {
	if ( sbq1_showteamscores ) {
		return;
	}
	sbq1_showteamscores = true;
}

static void SbarQW_DontShowTeamScores() {
	sbq1_showteamscores = false;
}

void SbarQ1_Init() {
	Cmd_AddCommand( "+showscores", SbarQ1_ShowScores );
	Cmd_AddCommand( "-showscores", SbarQ1_DontShowScores );

	if ( GGameType & GAME_QuakeWorld ) {
		Cmd_AddCommand( "+showteamscores", SbarQW_ShowTeamScores );
		Cmd_AddCommand( "-showteamscores", SbarQW_DontShowTeamScores );
	}
}

void SbarQ1_InitImages() {
	for ( int i = 0; i < 10; i++ ) {
		sbq1_nums[ 0 ][ i ] = R_PicFromWad( va( "num_%i",i ) );
		sbq1_nums[ 1 ][ i ] = R_PicFromWad( va( "anum_%i",i ) );
	}

	sbq1_nums[ 0 ][ 10 ] = R_PicFromWad( "num_minus" );
	sbq1_nums[ 1 ][ 10 ] = R_PicFromWad( "anum_minus" );

	sbq1_colon = R_PicFromWad( "num_colon" );
	sbq1_slash = R_PicFromWad( "num_slash" );

	sbq1_weapons[ 0 ][ 0 ] = R_PicFromWad( "inv_shotgun" );
	sbq1_weapons[ 0 ][ 1 ] = R_PicFromWad( "inv_sshotgun" );
	sbq1_weapons[ 0 ][ 2 ] = R_PicFromWad( "inv_nailgun" );
	sbq1_weapons[ 0 ][ 3 ] = R_PicFromWad( "inv_snailgun" );
	sbq1_weapons[ 0 ][ 4 ] = R_PicFromWad( "inv_rlaunch" );
	sbq1_weapons[ 0 ][ 5 ] = R_PicFromWad( "inv_srlaunch" );
	sbq1_weapons[ 0 ][ 6 ] = R_PicFromWad( "inv_lightng" );

	sbq1_weapons[ 1 ][ 0 ] = R_PicFromWad( "inv2_shotgun" );
	sbq1_weapons[ 1 ][ 1 ] = R_PicFromWad( "inv2_sshotgun" );
	sbq1_weapons[ 1 ][ 2 ] = R_PicFromWad( "inv2_nailgun" );
	sbq1_weapons[ 1 ][ 3 ] = R_PicFromWad( "inv2_snailgun" );
	sbq1_weapons[ 1 ][ 4 ] = R_PicFromWad( "inv2_rlaunch" );
	sbq1_weapons[ 1 ][ 5 ] = R_PicFromWad( "inv2_srlaunch" );
	sbq1_weapons[ 1 ][ 6 ] = R_PicFromWad( "inv2_lightng" );

	for ( int i = 0; i < 5; i++ ) {
		sbq1_weapons[ 2 + i ][ 0 ] = R_PicFromWad( va( "inva%i_shotgun",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 1 ] = R_PicFromWad( va( "inva%i_sshotgun",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 2 ] = R_PicFromWad( va( "inva%i_nailgun",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 3 ] = R_PicFromWad( va( "inva%i_snailgun",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 4 ] = R_PicFromWad( va( "inva%i_rlaunch",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 5 ] = R_PicFromWad( va( "inva%i_srlaunch",i + 1 ) );
		sbq1_weapons[ 2 + i ][ 6 ] = R_PicFromWad( va( "inva%i_lightng",i + 1 ) );
	}

	sbq1_ammo[ 0 ] = R_PicFromWad( "sb_shells" );
	sbq1_ammo[ 1 ] = R_PicFromWad( "sb_nails" );
	sbq1_ammo[ 2 ] = R_PicFromWad( "sb_rocket" );
	sbq1_ammo[ 3 ] = R_PicFromWad( "sb_cells" );

	sbq1_armor[ 0 ] = R_PicFromWad( "sb_armor1" );
	sbq1_armor[ 1 ] = R_PicFromWad( "sb_armor2" );
	sbq1_armor[ 2 ] = R_PicFromWad( "sb_armor3" );

	sbq1_items[ 0 ] = R_PicFromWad( "sb_key1" );
	sbq1_items[ 1 ] = R_PicFromWad( "sb_key2" );
	sbq1_items[ 2 ] = R_PicFromWad( "sb_invis" );
	sbq1_items[ 3 ] = R_PicFromWad( "sb_invuln" );
	sbq1_items[ 4 ] = R_PicFromWad( "sb_suit" );
	sbq1_items[ 5 ] = R_PicFromWad( "sb_quad" );

	sbq1_disc = R_PicFromWad( "disc" );

	sbq1_sigil[ 0 ] = R_PicFromWad( "sb_sigil1" );
	sbq1_sigil[ 1 ] = R_PicFromWad( "sb_sigil2" );
	sbq1_sigil[ 2 ] = R_PicFromWad( "sb_sigil3" );
	sbq1_sigil[ 3 ] = R_PicFromWad( "sb_sigil4" );

	sbq1_faces[ 4 ][ 0 ] = R_PicFromWad( "face1" );
	sbq1_faces[ 4 ][ 1 ] = R_PicFromWad( "face_p1" );
	sbq1_faces[ 3 ][ 0 ] = R_PicFromWad( "face2" );
	sbq1_faces[ 3 ][ 1 ] = R_PicFromWad( "face_p2" );
	sbq1_faces[ 2 ][ 0 ] = R_PicFromWad( "face3" );
	sbq1_faces[ 2 ][ 1 ] = R_PicFromWad( "face_p3" );
	sbq1_faces[ 1 ][ 0 ] = R_PicFromWad( "face4" );
	sbq1_faces[ 1 ][ 1 ] = R_PicFromWad( "face_p4" );
	sbq1_faces[ 0 ][ 0 ] = R_PicFromWad( "face5" );
	sbq1_faces[ 0 ][ 1 ] = R_PicFromWad( "face_p5" );

	sbq1_face_invis = R_PicFromWad( "face_invis" );
	sbq1_face_invuln = R_PicFromWad( "face_invul2" );
	sbq1_face_invis_invuln = R_PicFromWad( "face_inv2" );
	sbq1_face_quad = R_PicFromWad( "face_quad" );

	sbq1_sbar = R_PicFromWad( "sbar" );
	sbq1_ibar = R_PicFromWad( "ibar" );
	sbq1_scorebar = R_PicFromWad( "scorebar" );

	if ( q1_hipnotic ) {
		hsbq1_weapons[ 0 ][ 0 ] = R_PicFromWad( "inv_laser" );
		hsbq1_weapons[ 0 ][ 1 ] = R_PicFromWad( "inv_mjolnir" );
		hsbq1_weapons[ 0 ][ 2 ] = R_PicFromWad( "inv_gren_prox" );
		hsbq1_weapons[ 0 ][ 3 ] = R_PicFromWad( "inv_prox_gren" );
		hsbq1_weapons[ 0 ][ 4 ] = R_PicFromWad( "inv_prox" );

		hsbq1_weapons[ 1 ][ 0 ] = R_PicFromWad( "inv2_laser" );
		hsbq1_weapons[ 1 ][ 1 ] = R_PicFromWad( "inv2_mjolnir" );
		hsbq1_weapons[ 1 ][ 2 ] = R_PicFromWad( "inv2_gren_prox" );
		hsbq1_weapons[ 1 ][ 3 ] = R_PicFromWad( "inv2_prox_gren" );
		hsbq1_weapons[ 1 ][ 4 ] = R_PicFromWad( "inv2_prox" );

		for ( int i = 0; i < 5; i++ ) {
			hsbq1_weapons[ 2 + i ][ 0 ] = R_PicFromWad( va( "inva%i_laser",i + 1 ) );
			hsbq1_weapons[ 2 + i ][ 1 ] = R_PicFromWad( va( "inva%i_mjolnir",i + 1 ) );
			hsbq1_weapons[ 2 + i ][ 2 ] = R_PicFromWad( va( "inva%i_gren_prox",i + 1 ) );
			hsbq1_weapons[ 2 + i ][ 3 ] = R_PicFromWad( va( "inva%i_prox_gren",i + 1 ) );
			hsbq1_weapons[ 2 + i ][ 4 ] = R_PicFromWad( va( "inva%i_prox",i + 1 ) );
		}

		hsbq1_items[ 0 ] = R_PicFromWad( "sb_wsuit" );
		hsbq1_items[ 1 ] = R_PicFromWad( "sb_eshld" );
	}

	if ( q1_rogue ) {
		rsbq1_invbar[ 0 ] = R_PicFromWad( "r_invbar1" );
		rsbq1_invbar[ 1 ] = R_PicFromWad( "r_invbar2" );

		rsbq1_weapons[ 0 ] = R_PicFromWad( "r_lava" );
		rsbq1_weapons[ 1 ] = R_PicFromWad( "r_superlava" );
		rsbq1_weapons[ 2 ] = R_PicFromWad( "r_gren" );
		rsbq1_weapons[ 3 ] = R_PicFromWad( "r_multirock" );
		rsbq1_weapons[ 4 ] = R_PicFromWad( "r_plasma" );

		rsbq1_items[ 0 ] = R_PicFromWad( "r_shield1" );
		rsbq1_items[ 1 ] = R_PicFromWad( "r_agrav1" );

		rsbq1_teambord = R_PicFromWad( "r_teambord" );

		rsbq1_ammo[ 0 ] = R_PicFromWad( "r_ammolava" );
		rsbq1_ammo[ 1 ] = R_PicFromWad( "r_ammomulti" );
		rsbq1_ammo[ 2 ] = R_PicFromWad( "r_ammoplasma" );
	}
}

//=============================================================================
// drawing routines are relative to the status bar location

static void SbarQ1_DrawPic( int x, int y, image_t* pic ) {
	R_VerifyNoRenderCommands();
	if ( cl.qh_gametype == QHGAME_DEATHMATCH || GGameType & GAME_QuakeWorld ) {
		UI_DrawPic( x, y + ( viddef.height - Q1SBAR_HEIGHT ), pic );
	} else   {
		UI_DrawPic( x + ( ( viddef.width - 320 ) >> 1 ), y + ( viddef.height - Q1SBAR_HEIGHT ), pic );
	}
	R_SyncRenderThread();
}

static void SbarQ1_DrawSubPic( int x, int y, image_t* pic, int srcx, int srcy, int width, int height ) {
	R_VerifyNoRenderCommands();
	UI_DrawSubPic( x, y + ( viddef.height - Q1SBAR_HEIGHT ), pic, srcx, srcy, width, height );
	R_SyncRenderThread();
}

//	Draws one solid graphics character
static void SbarQ1_DrawCharacter( int x, int y, int num ) {
	R_VerifyNoRenderCommands();
	if ( cl.qh_gametype == QHGAME_DEATHMATCH || GGameType & GAME_QuakeWorld ) {
		UI_DrawChar( x + 4, y + viddef.height - Q1SBAR_HEIGHT, num );
	} else   {
		UI_DrawChar( x + ( ( viddef.width - 320 ) >> 1 ) + 4, y + viddef.height - Q1SBAR_HEIGHT, num );
	}
	R_SyncRenderThread();
}

static void SbarQ1_DrawString( int x, int y, const char* str ) {
	if ( cl.qh_gametype == QHGAME_DEATHMATCH || GGameType & GAME_QuakeWorld ) {
		UI_DrawString( x, y + viddef.height - Q1SBAR_HEIGHT, str );
	} else   {
		UI_DrawString( x + ( ( viddef.width - 320 ) >> 1 ), y + viddef.height - Q1SBAR_HEIGHT, str );
	}
}

int SbarQH_itoa( int num, char* buf ) {
	char* str = buf;

	if ( num < 0 ) {
		*str++ = '-';
		num = -num;
	}

	int pow10;
	for ( pow10 = 10; num >= pow10; pow10 *= 10 )
		;

	do {
		pow10 /= 10;
		int dig = num / pow10;
		*str++ = '0' + dig;
		num -= dig * pow10;
	} while ( pow10 != 1 );

	*str = 0;

	return str - buf;
}

static void SbarQ1_DrawNum( int x, int y, int num, int digits, int color ) {
	char str[ 12 ];
	int l = SbarQH_itoa( num, str );
	char* ptr = str;
	if ( l > digits ) {
		ptr += ( l - digits );
	}
	if ( l < digits ) {
		x += ( digits - l ) * 24;
	}

	while ( *ptr ) {
		int frame;
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else   {
			frame = *ptr - '0';
		}

		SbarQ1_DrawPic( x,y,sbq1_nums[ color ][ frame ] );
		x += 24;
		ptr++;
	}
}

static void SbarQ1_SortFrags( bool includespec ) {
	// sort by frags
	sbq1_scoreboardlines = 0;
	for ( int i = 0; i < ( GGameType & GAME_QuakeWorld ? MAX_CLIENTS_QHW : cl.qh_maxclients ); i++ ) {
		if ( cl.q1_players[ i ].name[ 0 ] &&
			 ( !( GGameType & GAME_QuakeWorld ) || !cl.q1_players[ i ].spectator || includespec ) ) {
			sbq1_fragsort[ sbq1_scoreboardlines ] = i;
			sbq1_scoreboardlines++;
			if ( GGameType & GAME_QuakeWorld && cl.q1_players[ i ].spectator ) {
				cl.q1_players[ i ].frags = -999;
			}
		}
	}

	for ( int i = 0; i < sbq1_scoreboardlines; i++ ) {
		for ( int j = 0; j < sbq1_scoreboardlines - 1 - i; j++ ) {
			if ( cl.q1_players[ sbq1_fragsort[ j ] ].frags < cl.q1_players[ sbq1_fragsort[ j + 1 ] ].frags ) {
				int k = sbq1_fragsort[ j ];
				sbq1_fragsort[ j ] = sbq1_fragsort[ j + 1 ];
				sbq1_fragsort[ j + 1 ] = k;
			}
		}
	}
}

static void SbarQW_SortTeams() {
	// request new ping times every two second
	sbq1_scoreboardteams = 0;

	int teamplay = String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) );
	if ( !teamplay ) {
		return;
	}

	// sort the teams
	Com_Memset( sbq1_teams, 0, sizeof ( sbq1_teams ) );
	for ( int i = 0; i < MAX_CLIENTS_QHW; i++ ) {
		sbq1_teams[ i ].plow = 999;
	}

	for ( int i = 0; i < MAX_CLIENTS_QHW; i++ ) {
		q1player_info_t* s = &cl.q1_players[ i ];
		if ( !s->name[ 0 ] ) {
			continue;
		}
		if ( s->spectator ) {
			continue;
		}

		// find his team in the list
		char t[ 16 + 1 ];
		t[ 16 ] = 0;
		String::NCpy( t, Info_ValueForKey( s->userinfo, "team" ), 16 );
		if ( !t[ 0 ] ) {
			continue;	// not on team
		}
		int j;
		for ( j = 0; j < sbq1_scoreboardteams; j++ ) {
			if ( !String::Cmp( sbq1_teams[ j ].team, t ) ) {
				sbq1_teams[ j ].frags += s->frags;
				sbq1_teams[ j ].players++;
				break;
			}
		}
		if ( j == sbq1_scoreboardteams ) {	// must add him
			j = sbq1_scoreboardteams++;
			String::Cpy( sbq1_teams[ j ].team, t );
			sbq1_teams[ j ].frags = s->frags;
			sbq1_teams[ j ].players = 1;
		}
		if ( sbq1_teams[ j ].plow > s->ping ) {
			sbq1_teams[ j ].plow = s->ping;
		}
		if ( sbq1_teams[ j ].phigh < s->ping ) {
			sbq1_teams[ j ].phigh = s->ping;
		}
		sbq1_teams[ j ].ptotal += s->ping;
	}

	// sort
	for ( int i = 0; i < sbq1_scoreboardteams; i++ ) {
		sbq1_teamsort[ i ] = i;
	}

	// good 'ol bubble sort
	for ( int i = 0; i < sbq1_scoreboardteams - 1; i++ ) {
		for ( int j = i + 1; j < sbq1_scoreboardteams; j++ ) {
			if ( sbq1_teams[ sbq1_teamsort[ i ] ].frags < sbq1_teams[ sbq1_teamsort[ j ] ].frags ) {
				int k = sbq1_teamsort[ i ];
				sbq1_teamsort[ i ] = sbq1_teamsort[ j ];
				sbq1_teamsort[ j ] = k;
			}
		}
	}
}

static int SbarQ1_ColorForMap( int m ) {
	m = ( m < 0 ) ? 0 : ( ( m > 13 ) ? 13 : m );
	m *= 16;
	return m + 8;
}

static void SbarQ1_DrawInventory() {
	bool headsup = GGameType & GAME_QuakeWorld && !( clqh_sbar->value || scr_viewsize->value < 100 );
	bool hudswap = GGameType & GAME_QuakeWorld && !!clqw_hudswap->value;// Get that nasty float out :)

	if ( !headsup ) {
		if ( q1_rogue ) {
			if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] >= Q1RIT_LAVA_NAILGUN ) {
				SbarQ1_DrawPic( 0, -24, rsbq1_invbar[ 0 ] );
			} else   {
				SbarQ1_DrawPic( 0, -24, rsbq1_invbar[ 1 ] );
			}
		} else   {
			SbarQ1_DrawPic( 0, -24, sbq1_ibar );
		}
	}

	// weapons
	for ( int i = 0; i < 7; i++ ) {
		if ( ( GGameType & GAME_QuakeWorld ? cl.qh_stats[ QWSTAT_ITEMS ] : cl.q1_items ) & ( Q1IT_SHOTGUN << i ) ) {
			float time = cl.q1_item_gettime[ i ];
			int flashon = ( int )( ( cl.qh_serverTimeFloat - time ) * 10 );
			if ( flashon < 0 ) {
				flashon = 0;
			}
			if ( flashon >= 10 ) {
				if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] == ( Q1IT_SHOTGUN << i ) ) {
					flashon = 1;
				} else   {
					flashon = 0;
				}
			} else   {
				flashon = ( flashon % 5 ) + 2;
			}

			if ( headsup ) {
				if ( i || viddef.height > 200 ) {
					SbarQ1_DrawSubPic( ( hudswap ) ? 0 : ( viddef.width - 24 ),-68 - ( 7 - i ) * 16, sbq1_weapons[ flashon ][ i ],0,0,24,16 );
				}
			} else   {
				SbarQ1_DrawPic( i * 24, -16, sbq1_weapons[ flashon ][ i ] );
			}
		}
	}

	// MED 01/04/97
	// hipnotic weapons
	if ( q1_hipnotic ) {
		int grenadeflashing = 0;
		for ( int i = 0; i < 4; i++ ) {
			if ( cl.q1_items & ( 1 << hipweapons[ i ] ) ) {
				float time = cl.q1_item_gettime[ hipweapons[ i ] ];
				int flashon = ( int )( ( cl.qh_serverTimeFloat - time ) * 10 );
				if ( flashon >= 10 ) {
					if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] == ( 1 << hipweapons[ i ] ) ) {
						flashon = 1;
					} else   {
						flashon = 0;
					}
				} else   {
					flashon = ( flashon % 5 ) + 2;
				}

				// check grenade launcher
				if ( i == 2 ) {
					if ( cl.q1_items & Q1HIT_PROXIMITY_GUN ) {
						if ( flashon ) {
							grenadeflashing = 1;
							SbarQ1_DrawPic( 96, -16, hsbq1_weapons[ flashon ][ 2 ] );
						}
					}
				} else if ( i == 3 )     {
					if ( cl.q1_items & ( Q1IT_SHOTGUN << 4 ) ) {
						if ( flashon && !grenadeflashing ) {
							SbarQ1_DrawPic( 96, -16, hsbq1_weapons[ flashon ][ 3 ] );
						} else if ( !grenadeflashing )     {
							SbarQ1_DrawPic( 96, -16, hsbq1_weapons[ 0 ][ 3 ] );
						}
					} else   {
						SbarQ1_DrawPic( 96, -16, hsbq1_weapons[ flashon ][ 4 ] );
					}
				} else   {
					SbarQ1_DrawPic( 176 + ( i * 24 ), -16, hsbq1_weapons[ flashon ][ i ] );
				}
			}
		}
	}

	if ( q1_rogue ) {
		// check for powered up weapon.
		if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] >= Q1RIT_LAVA_NAILGUN ) {
			for ( int i = 0; i < 5; i++ ) {
				if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] == ( Q1RIT_LAVA_NAILGUN << i ) ) {
					SbarQ1_DrawPic( ( i + 2 ) * 24, -16, rsbq1_weapons[ i ] );
				}
			}
		}
	}

	// ammo counts
	for ( int i = 0; i < 4; i++ ) {
		char num[ 6 ];
		sprintf( num, "%3i",cl.qh_stats[ Q1STAT_SHELLS + i ] );
		if ( headsup ) {
			SbarQ1_DrawSubPic( ( hudswap ) ? 0 : ( viddef.width - 42 ), -24 - ( 4 - i ) * 11, sbq1_ibar, 3 + ( i * 48 ), 0, 42, 11 );
			if ( num[ 0 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( hudswap ) ? 3 : ( viddef.width - 39 ), -24 - ( 4 - i ) * 11, 18 + num[ 0 ] - '0' );
			}
			if ( num[ 1 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( hudswap ) ? 11 : ( viddef.width - 31 ), -24 - ( 4 - i ) * 11, 18 + num[ 1 ] - '0' );
			}
			if ( num[ 2 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( hudswap ) ? 19 : ( viddef.width - 23 ), -24 - ( 4 - i ) * 11, 18 + num[ 2 ] - '0' );
			}
		} else   {
			if ( num[ 0 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( 6 * i + 1 ) * 8 - 2, -24, 18 + num[ 0 ] - '0' );
			}
			if ( num[ 1 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( 6 * i + 2 ) * 8 - 2, -24, 18 + num[ 1 ] - '0' );
			}
			if ( num[ 2 ] != ' ' ) {
				SbarQ1_DrawCharacter( ( 6 * i + 3 ) * 8 - 2, -24, 18 + num[ 2 ] - '0' );
			}
		}
	}

	// items
	for ( int i = 0; i < 6; i++ ) {
		if ( ( GGameType & GAME_QuakeWorld ? cl.qh_stats[ QWSTAT_ITEMS ] : cl.q1_items ) & ( 1 << ( 17 + i ) ) ) {
			//MED 01/04/97 changed keys
			if ( !q1_hipnotic || ( i > 1 ) ) {
				SbarQ1_DrawPic( 192 + i * 16, -16, sbq1_items[ i ] );
			}
		}
	}

	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if ( q1_hipnotic ) {
		for ( int i = 0; i < 2; i++ ) {
			if ( cl.q1_items & ( 1 << ( 24 + i ) ) ) {
				SbarQ1_DrawPic( 288 + i * 16, -16, hsbq1_items[ i ] );
			}
		}
	}

	if ( q1_rogue ) {
		// new rogue items
		for ( int i = 0; i < 2; i++ ) {
			if ( cl.q1_items & ( 1 << ( 29 + i ) ) ) {
				SbarQ1_DrawPic( 288 + i * 16, -16, rsbq1_items[ i ] );
			}
		}
	} else   {
		// sigils
		for ( int i = 0; i < 4; i++ ) {
			if ( ( GGameType & GAME_QuakeWorld ? cl.qh_stats[ QWSTAT_ITEMS ] : cl.q1_items ) & ( 1 << ( 28 + i ) ) ) {
				SbarQ1_DrawPic( 320 - 32 + i * 8, -16, sbq1_sigil[ i ] );
			}
		}
	}
}

static void SbarQ1_SoloScoreboard() {
	SbarQ1_DrawPic( 0, 0, sbq1_scorebar );
	char str[ 80 ];
	if ( !( GGameType & GAME_QuakeWorld ) ) {
		sprintf( str, "Monsters:%3i /%3i", cl.qh_stats[ Q1STAT_MONSTERS ], cl.qh_stats[ Q1STAT_TOTALMONSTERS ] );
		SbarQ1_DrawString( 8, 4, str );

		sprintf( str, "Secrets :%3i /%3i", cl.qh_stats[ Q1STAT_SECRETS ], cl.qh_stats[ Q1STAT_TOTALSECRETS ] );
		SbarQ1_DrawString( 8, 12, str );
	}

	// time
	int minutes = cl.qh_serverTimeFloat / 60;
	int seconds = cl.qh_serverTimeFloat - 60 * minutes;
	int tens = seconds / 10;
	int units = seconds - 10 * tens;
	sprintf( str, "Time :%3i:%i%i", minutes, tens, units );
	SbarQ1_DrawString( 184, 4, str );

	if ( !( GGameType & GAME_QuakeWorld ) ) {
		// draw level name
		int l = String::Length( cl.qh_levelname );
		SbarQ1_DrawString( 232 - l * 4, 12, cl.qh_levelname );
	}
}

static void SbarQ1_DrawFrags() {
	SbarQ1_SortFrags( false );

	// draw the text
	int l = sbq1_scoreboardlines <= 4 ? sbq1_scoreboardlines : 4;

	int x = 23;
	int xofs;
	if ( GGameType & GAME_QuakeWorld || cl.qh_gametype == QHGAME_DEATHMATCH ) {
		xofs = 0;
	} else   {
		xofs = ( viddef.width - 320 ) >> 1;
	}
	int y = viddef.height - Q1SBAR_HEIGHT - 23;

	for ( int i = 0; i < l; i++ ) {
		int k = sbq1_fragsort[ i ];
		q1player_info_t* s = &cl.q1_players[ k ];
		if ( !s->name[ 0 ] ) {
			continue;
		}
		if ( GGameType & GAME_QuakeWorld && s->spectator ) {
			continue;
		}
		R_VerifyNoRenderCommands();

		// draw background
		int top = SbarQ1_ColorForMap( s->topcolor );
		int bottom = SbarQ1_ColorForMap( s->bottomcolor );

		UI_FillPal( xofs + x * 8 + 10, y, 28, 4, top );
		UI_FillPal( xofs + x * 8 + 10, y + 4, 28, 3, bottom );
		R_SyncRenderThread();

		// draw number
		char num[ 12 ];
		sprintf( num, "%3i", s->frags );

		SbarQ1_DrawCharacter( ( x + 1 ) * 8, -24, num[ 0 ] );
		SbarQ1_DrawCharacter( ( x + 2 ) * 8, -24, num[ 1 ] );
		SbarQ1_DrawCharacter( ( x + 3 ) * 8, -24, num[ 2 ] );

		if ( k == cl.viewentity - 1 ) {
			SbarQ1_DrawCharacter( x * 8 + 2, -24, 16 );
			SbarQ1_DrawCharacter( ( x + 4 ) * 8 - 4, -24, 17 );
		}
		x += 4;
	}
}

static void SbarQ1_DrawFace() {
	// PGM 01/19/97 - team color drawing
	// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if ( q1_rogue &&
		 ( cl.qh_maxclients != 1 ) &&
		 ( svqh_teamplay->value > 3 ) &&
		 ( svqh_teamplay->value < 7 ) ) {
		q1player_info_t* s = &cl.q1_players[ cl.viewentity - 1 ];
		// draw background
		int top = SbarQ1_ColorForMap( s->topcolor );
		int bottom = SbarQ1_ColorForMap( s->bottomcolor );

		int xofs;
		if ( cl.qh_gametype == QHGAME_DEATHMATCH ) {
			xofs = 113;
		} else   {
			xofs = ( ( viddef.width - 320 ) >> 1 ) + 113;
		}

		SbarQ1_DrawPic( 112, 0, rsbq1_teambord );
		R_VerifyNoRenderCommands();
		UI_FillPal( xofs, viddef.height - Q1SBAR_HEIGHT + 3, 22, 9, top );
		UI_FillPal( xofs, viddef.height - Q1SBAR_HEIGHT + 12, 22, 9, bottom );
		R_SyncRenderThread();

		// draw number
		char num[ 12 ];
		sprintf( num, "%3i",s->frags );

		if ( top == 8 ) {
			if ( num[ 0 ] != ' ' ) {
				SbarQ1_DrawCharacter( 109, 3, 18 + num[ 0 ] - '0' );
			}
			if ( num[ 1 ] != ' ' ) {
				SbarQ1_DrawCharacter( 116, 3, 18 + num[ 1 ] - '0' );
			}
			if ( num[ 2 ] != ' ' ) {
				SbarQ1_DrawCharacter( 123, 3, 18 + num[ 2 ] - '0' );
			}
		} else   {
			SbarQ1_DrawCharacter( 109, 3, num[ 0 ] );
			SbarQ1_DrawCharacter( 116, 3, num[ 1 ] );
			SbarQ1_DrawCharacter( 123, 3, num[ 2 ] );
		}

		return;
	}

	int items = GGameType & GAME_QuakeWorld ? cl.qh_stats[ QWSTAT_ITEMS ] : cl.q1_items;
	if ( ( items & ( Q1IT_INVISIBILITY | Q1IT_INVULNERABILITY ) )
		 == ( Q1IT_INVISIBILITY | Q1IT_INVULNERABILITY ) ) {
		SbarQ1_DrawPic( 112, 0, sbq1_face_invis_invuln );
		return;
	}
	if ( items & Q1IT_QUAD ) {
		SbarQ1_DrawPic( 112, 0, sbq1_face_quad );
		return;
	}
	if ( items & Q1IT_INVISIBILITY ) {
		SbarQ1_DrawPic( 112, 0, sbq1_face_invis );
		return;
	}
	if ( items & Q1IT_INVULNERABILITY ) {
		SbarQ1_DrawPic( 112, 0, sbq1_face_invuln );
		return;
	}

	int f;
	if ( cl.qh_stats[ Q1STAT_HEALTH ] >= 100 ) {
		f = 4;
	} else   {
		f = cl.qh_stats[ Q1STAT_HEALTH ] / 20;
	}

	int anim;
	if ( cl.qh_serverTimeFloat <= cl.q1_faceanimtime ) {
		anim = 1;
	} else   {
		anim = 0;
	}
	SbarQ1_DrawPic( 112, 0, sbq1_faces[ f ][ anim ] );
}

static void SbarQ1_DrawNormal() {
	if ( !( GGameType & GAME_QuakeWorld ) || clqh_sbar->value || scr_viewsize->value < 100 ) {
		SbarQ1_DrawPic( 0, 0, sbq1_sbar );
	}

	int items = GGameType & GAME_QuakeWorld ? cl.qh_stats[ QWSTAT_ITEMS ] : cl.q1_items;

	// keys (hipnotic only)
	//MED 01/04/97 moved keys here so they would not be overwritten
	if ( q1_hipnotic ) {
		if ( items & Q1IT_KEY1 ) {
			SbarQ1_DrawPic( 209, 3, sbq1_items[ 0 ] );
		}
		if ( items & Q1IT_KEY2 ) {
			SbarQ1_DrawPic( 209, 12, sbq1_items[ 1 ] );
		}
	}

	// armor
	if ( items & Q1IT_INVULNERABILITY ) {
		SbarQ1_DrawNum( 24, 0, 666, 3, 1 );
		SbarQ1_DrawPic( 0, 0, sbq1_disc );
	} else   {
		if ( q1_rogue ) {
			SbarQ1_DrawNum( 24, 0, cl.qh_stats[ Q1STAT_ARMOR ], 3,
				cl.qh_stats[ Q1STAT_ARMOR ] <= 25 );
			if ( items & Q1RIT_ARMOR3 ) {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 2 ] );
			} else if ( items & Q1RIT_ARMOR2 )     {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 1 ] );
			} else if ( items & Q1RIT_ARMOR1 )     {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 0 ] );
			}
		} else   {
			SbarQ1_DrawNum( 24, 0, cl.qh_stats[ Q1STAT_ARMOR ], 3,
				cl.qh_stats[ Q1STAT_ARMOR ] <= 25 );
			if ( items & Q1IT_ARMOR3 ) {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 2 ] );
			} else if ( items & Q1IT_ARMOR2 )     {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 1 ] );
			} else if ( items & Q1IT_ARMOR1 )     {
				SbarQ1_DrawPic( 0, 0, sbq1_armor[ 0 ] );
			}
		}
	}

	// face
	SbarQ1_DrawFace();

	// health
	SbarQ1_DrawNum( 136, 0, cl.qh_stats[ Q1STAT_HEALTH ], 3,
		cl.qh_stats[ Q1STAT_HEALTH ] <= 25 );

	// ammo icon
	if ( q1_rogue ) {
		if ( items & Q1RIT_SHELLS ) {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 0 ] );
		} else if ( items & Q1RIT_NAILS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 1 ] );
		} else if ( items & Q1RIT_ROCKETS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 2 ] );
		} else if ( items & Q1RIT_CELLS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 3 ] );
		} else if ( items & Q1RIT_LAVA_NAILS )     {
			SbarQ1_DrawPic( 224, 0, rsbq1_ammo[ 0 ] );
		} else if ( items & Q1RIT_PLASMA_AMMO )     {
			SbarQ1_DrawPic( 224, 0, rsbq1_ammo[ 1 ] );
		} else if ( items & Q1RIT_MULTI_ROCKETS )     {
			SbarQ1_DrawPic( 224, 0, rsbq1_ammo[ 2 ] );
		}
	} else   {
		if ( items & Q1IT_SHELLS ) {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 0 ] );
		} else if ( items & Q1IT_NAILS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 1 ] );
		} else if ( items & Q1IT_ROCKETS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 2 ] );
		} else if ( items & Q1IT_CELLS )     {
			SbarQ1_DrawPic( 224, 0, sbq1_ammo[ 3 ] );
		}
	}

	SbarQ1_DrawNum( 248, 0, cl.qh_stats[ Q1STAT_AMMO ], 3,
		cl.qh_stats[ Q1STAT_AMMO ] <= 10 );
}

static void SbarQ1_DeathmatchOverlay( int start ) {
	// request new ping times every two second
	if ( cls.realtime - cl.qh_last_ping_request * 1000 > 2000 ) {
		cl.qh_last_ping_request = cls.realtime * 0.001;
		CL_AddReliableCommand( "pings" );
	}

	int teamplay = GGameType & GAME_QuakeWorld && String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) );

	if ( !start ) {
		R_VerifyNoRenderCommands();
		image_t* pic = R_CachePic( "gfx/ranking.lmp" );
		UI_DrawPic( viddef.width / 2 - R_GetImageWidth( pic ) / 2, 8, pic );
		R_SyncRenderThread();
	}

	// scores
	SbarQ1_SortFrags( true );

	// draw the text
	int l = sbq1_scoreboardlines;

	int y;
	if ( start ) {
		y = start;
	} else if ( GGameType & GAME_QuakeWorld )     {
		y = 32;
	} else   {
		y = 40;
	}
	int x;
	if ( GGameType & GAME_QuakeWorld ) {
		if ( teamplay ) {
			x = 4 + ( ( viddef.width - 320 ) >> 1 );
			//                   0    40 64   104   152  192
			UI_DrawString( x, y, "ping pl time frags team name" );
			y += 8;
			UI_DrawString( x, y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f" );
			y += 8;
		} else   {
			x = 16 + ( ( viddef.width - 320 ) >> 1 );
			//                   0    40 64   104   152
			UI_DrawString( x, y, "ping pl time frags name" );
			y += 8;
			UI_DrawString( x, y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f" );
			y += 8;
		}
	} else   {
		x = 80 + ( ( viddef.width - 320 ) >> 1 );
	}

	int skip = 10;
	if ( sbq1_largegame ) {
		skip = 8;
	}

	for ( int i = 0; i < l && y <= ( int )viddef.height - 10; i++ ) {
		int k = sbq1_fragsort[ i ];
		q1player_info_t* s = &cl.q1_players[ k ];
		if ( !s->name[ 0 ] ) {
			continue;
		}

		if ( GGameType & GAME_QuakeWorld ) {
			// draw ping
			int p = s->ping;
			if ( p < 0 || p > 999 ) {
				p = 999;
			}
			char num[ 12 ];
			sprintf( num, "%4i", p );
			UI_DrawString( x, y, num );

			// draw pl
			p = s->pl;
			sprintf( num, "%3i", p );
			if ( p > 25 ) {
				UI_DrawString( x + 32, y, num, 0x80 );
			} else   {
				UI_DrawString( x + 32, y, num );
			}

			if ( s->spectator ) {
				UI_DrawString( x + 40, y, "(spectator)" );
				// draw name
				if ( teamplay ) {
					UI_DrawString( x + 152 + 40, y, s->name );
				} else   {
					UI_DrawString( x + 152, y, s->name );
				}
				y += skip;
				continue;
			}

			// draw time
			int total;
			if ( cl.qh_intermission ) {
				total = cl.qh_completed_time - s->entertime;
			} else   {
				total = cls.realtime * 0.001 - s->entertime;
			}
			int minutes = ( int )total / 60;
			sprintf( num, "%4i", minutes );
			UI_DrawString( x + 64, y, num );
		}
		R_VerifyNoRenderCommands();

		// draw background
		int top = SbarQ1_ColorForMap( s->topcolor );
		int bottom = SbarQ1_ColorForMap( s->bottomcolor );

		if ( GGameType & GAME_QuakeWorld ) {
			if ( sbq1_largegame ) {
				UI_FillPal( x + 104, y + 1, 40, 3, top );
			} else   {
				UI_FillPal( x + 104, y, 40, 4, top );
			}
			UI_FillPal( x + 104, y + 4, 40, 4, bottom );
		} else   {
			UI_FillPal( x, y, 40, 4, top );
			UI_FillPal( x, y + 4, 40, 4, bottom );
		}
		R_SyncRenderThread();

		// draw number
		char num[ 12 ];
		sprintf( num, "%3i", s->frags );

		if ( GGameType & GAME_QuakeWorld ) {
			UI_DrawString( x + 112, y, num );

			if ( k == cl.playernum ) {
				R_VerifyNoRenderCommands();
				UI_DrawChar( x + 104, y, 16 );
				R_SyncRenderThread();
				R_VerifyNoRenderCommands();
				UI_DrawChar( x + 136, y, 17 );
				R_SyncRenderThread();
			}

			// team
			if ( teamplay ) {
				char team[ 5 ];
				team[ 4 ] = 0;
				String::NCpy( team, Info_ValueForKey( s->userinfo, "team" ), 4 );
				UI_DrawString( x + 152, y, team );
			}

			// draw name
			if ( teamplay ) {
				UI_DrawString( x + 152 + 40, y, s->name );
			} else   {
				UI_DrawString( x + 152, y, s->name );
			}
		} else   {
			UI_DrawString( x + 8, y, num );

			if ( k == cl.viewentity - 1 ) {
				R_VerifyNoRenderCommands();
				UI_DrawChar( x - 8, y, 12 );
				R_SyncRenderThread();
			}

			// draw name
			UI_DrawString( x + 64, y, s->name );
		}

		y += skip;
	}

	if ( y >= ( int )viddef.height - 10 ) {	// we ran over the screen size, squish
		sbq1_largegame = true;
	}
}

//	team frags
static void SbarQW_TeamOverlay() {
	// request new ping times every two second
	int teamplay = String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) );

	if ( !teamplay ) {
		SbarQ1_DeathmatchOverlay( 0 );
		return;
	}

	R_VerifyNoRenderCommands();
	image_t* pic = R_CachePic( "gfx/ranking.lmp" );
	UI_DrawPic( 160 - R_GetImageWidth( pic ) / 2, 8, pic );
	R_SyncRenderThread();

	int y = 32;
	int x = 36 + ( ( viddef.width - 320 ) >> 1 );
	UI_DrawString( x, y, "low/avg/high team total players" );
	y += 8;
	UI_DrawString( x, y, "\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1f" );
	y += 8;

	// sort the teams
	SbarQW_SortTeams();

	// draw the text
	for ( int i = 0; i < sbq1_scoreboardteams && y <= ( int )viddef.height - 10; i++ ) {
		int k = sbq1_teamsort[ i ];
		team_t* tm = sbq1_teams + k;

		// draw pings
		int plow = tm->plow;
		if ( plow < 0 || plow > 999 ) {
			plow = 999;
		}
		int phigh = tm->phigh;
		if ( phigh < 0 || phigh > 999 ) {
			phigh = 999;
		}
		int pavg;
		if ( !tm->players ) {
			pavg = 999;
		} else   {
			pavg = tm->ptotal / tm->players;
		}
		if ( pavg < 0 || pavg > 999 ) {
			pavg = 999;
		}

		char num[ 12 ];
		sprintf( num, "%3i/%3i/%3i", plow, pavg, phigh );
		UI_DrawString( x, y, num );

		// draw team
		char team[ 5 ];
		team[ 4 ] = 0;
		String::NCpy( team, tm->team, 4 );
		UI_DrawString( x + 104, y, team );

		// draw total
		sprintf( num, "%5i", tm->frags );
		UI_DrawString( x + 104 + 40, y, num );

		// draw players
		sprintf( num, "%5i", tm->players );
		UI_DrawString( x + 104 + 88, y, num );

		if ( !String::NCmp( Info_ValueForKey( cl.q1_players[ cl.playernum ].userinfo,
					 "team" ), tm->team, 16 ) ) {
			R_VerifyNoRenderCommands();
			UI_DrawChar( x + 104 - 8, y, 16 );
			R_SyncRenderThread();
			R_VerifyNoRenderCommands();
			UI_DrawChar( x + 104 + 32, y, 17 );
			R_SyncRenderThread();
		}

		y += 8;
	}
	y += 8;
	SbarQ1_DeathmatchOverlay( y );
}

//	frags name
//	frags team name
//	displayed to right of status bar if there's room
static void SbarQ1_MiniDeathmatchOverlay() {
	int i, k;
	int top, bottom;
	int x, y, f;
	char num[ 12 ];
	q1player_info_t* s;
	int numlines;

	if ( viddef.width < 512 || !sbqh_lines ) {
		return;	// not enuff room
	}

	int teamplay = GGameType & GAME_QuakeWorld && String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) );

// scores
	SbarQ1_SortFrags( false );
	if ( GGameType & GAME_QuakeWorld && viddef.width >= 640 ) {
		SbarQW_SortTeams();
	}

	if ( !sbq1_scoreboardlines ) {
		return;	// no one there?
	}

// draw the text
	y = viddef.height - sbqh_lines - 1;
	numlines = sbqh_lines / 8;
	if ( numlines < 3 ) {
		return;	// not enough room
	}

	// find us
	for ( i = 0; i < sbq1_scoreboardlines; i++ )
		if ( sbq1_fragsort[ i ] == cl.viewentity - 1 ) {
			break;
		}

	if ( i == sbq1_scoreboardlines ) {	// we're not there, we are probably a spectator, just display top
		i = 0;
	} else   {	// figure out start
		i = i - numlines / 2;
	}

	if ( i > sbq1_scoreboardlines - numlines ) {
		i = sbq1_scoreboardlines - numlines;
	}
	if ( i < 0 ) {
		i = 0;
	}

	x = 324;
	for ( /* */; i < sbq1_scoreboardlines && y < ( int )viddef.height - 8 + 1; i++ ) {
		k = sbq1_fragsort[ i ];
		s = &cl.q1_players[ k ];
		if ( !s->name[ 0 ] ) {
			continue;
		}

		R_VerifyNoRenderCommands();
		// draw background
		top = s->topcolor;
		bottom = s->bottomcolor;
		top = SbarQ1_ColorForMap( top );
		bottom = SbarQ1_ColorForMap( bottom );

		UI_FillPal( x, y + 1, 40, 3, top );
		UI_FillPal( x, y + 4, 40, 4, bottom );
		R_SyncRenderThread();

		// draw number
		f = s->frags;
		sprintf( num, "%3i",f );

		UI_DrawString( x + 8, y, num );

		if ( k == cl.viewentity - 1 ) {
			R_VerifyNoRenderCommands();
			UI_DrawChar( x, y, 16 );
			R_SyncRenderThread();
			R_VerifyNoRenderCommands();
			UI_DrawChar( x + 32, y, 17 );
			R_SyncRenderThread();
		}

		// team
		if ( teamplay ) {
			char team[ 5 ];
			team[ 4 ] = 0;
			String::NCpy( team, Info_ValueForKey( s->userinfo, "team" ), 4 );
			UI_DrawString( x + 48, y, team );
		}

		// draw name
		char name[ 32 ];
		String::NCpyZ( name, s->name, sizeof ( name ) );
		if ( GGameType & GAME_QuakeWorld ) {
			name[ 16 ] = 0;
		}
		if ( teamplay ) {
			UI_DrawString( x + 48 + 40, y, name );
		} else   {
			UI_DrawString( x + 48, y, name );
		}
		y += 8;
	}

	// draw teams if room
	if ( viddef.width < 640 || !teamplay ) {
		return;
	}

	// draw seperator
	x += 208;
	for ( y = viddef.height - sbqh_lines; y < ( int )viddef.height - 6; y += 2 ) {
		R_VerifyNoRenderCommands();
		UI_DrawChar( x, y, 14 );
		R_SyncRenderThread();
	}

	x += 16;

	y = viddef.height - sbqh_lines;
	for ( i = 0; i < sbq1_scoreboardteams && y <= ( int )viddef.height; i++ ) {
		k = sbq1_teamsort[ i ];
		team_t* tm = sbq1_teams + k;

		// draw pings
		char team[ 5 ];
		team[ 4 ] = 0;
		String::NCpy( team, tm->team, 4 );
		UI_DrawString( x, y, team );

		// draw total
		sprintf( num, "%5i", tm->frags );
		UI_DrawString( x + 40, y, num );

		if ( !String::NCmp( Info_ValueForKey( cl.q1_players[ cl.playernum ].userinfo,
					 "team" ), tm->team, 16 ) ) {
			R_VerifyNoRenderCommands();
			UI_DrawChar( x - 8, y, 16 );
			R_SyncRenderThread();
			R_VerifyNoRenderCommands();
			UI_DrawChar( x + 32, y, 17 );
			R_SyncRenderThread();
		}

		y += 8;
	}
}

void SbarQ1_Draw() {
	if ( cls.state != CA_ACTIVE || ( !( GGameType & GAME_QuakeWorld ) && clc.qh_signon != SIGNONS ) || con.displayFrac == 1 ) {
		return;		// console is full screen
	}

	bool headsup = GGameType & GAME_QuakeWorld && !( clqh_sbar->value || scr_viewsize->value < 100 );

	// top line
	if ( sbqh_lines > 24 ) {
		if ( !( GGameType & GAME_QuakeWorld ) || !cl.qh_spectator || autocam == CAM_TRACK ) {
			SbarQ1_DrawInventory();
		}
		if ( cl.qh_maxclients != 1 && ( !headsup || viddef.width < 512 ) ) {
			SbarQ1_DrawFrags();
		}
	}

	// main area
	if ( !( GGameType & GAME_QuakeWorld ) && ( sbq1_showscores || cl.qh_stats[ Q1STAT_HEALTH ] <= 0 ) ) {
		SbarQ1_SoloScoreboard();
	} else if ( sbqh_lines > 0 )     {
		if ( GGameType & GAME_QuakeWorld && cl.qh_spectator ) {
			if ( autocam != CAM_TRACK ) {
				SbarQ1_DrawPic( 0, 0, sbq1_scorebar );
				SbarQ1_DrawString( 160 - 7 * 8,4, "SPECTATOR MODE" );
				SbarQ1_DrawString( 160 - 14 * 8 + 4, 12, "Press [ATTACK] for AutoCamera" );
			} else   {
				if ( sbq1_showscores || cl.qh_stats[ Q1STAT_HEALTH ] <= 0 ) {
					SbarQ1_SoloScoreboard();
				} else   {
					SbarQ1_DrawNormal();
				}

				char st[ 512 ];
				sprintf( st, "Tracking %-.13s, [JUMP] for next",
					cl.q1_players[ spec_track ].name );
				SbarQ1_DrawString( 0, -8, st );
			}
		} else if ( sbq1_showscores || cl.qh_stats[ Q1STAT_HEALTH ] <= 0 )       {
			SbarQ1_SoloScoreboard();
		} else   {
			SbarQ1_DrawNormal();
		}
	}

	// main screen deathmatch rankings
	// if we're dead show team scores in team games
	if ( cl.qh_stats[ Q1STAT_HEALTH ] <= 0 && ( !( GGameType & GAME_QuakeWorld ) || !cl.qh_spectator ) ) {
		if ( GGameType & GAME_QuakeWorld && String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) ) > 0 &&
			 !sbq1_showscores ) {
			SbarQW_TeamOverlay();
		} else if ( GGameType & GAME_QuakeWorld || cl.qh_gametype == QHGAME_DEATHMATCH )     {
			SbarQ1_DeathmatchOverlay( 0 );
		}
	} else if ( sbq1_showscores )     {
		if ( GGameType & GAME_QuakeWorld || cl.qh_gametype == QHGAME_DEATHMATCH ) {
			SbarQ1_DeathmatchOverlay( 0 );
		}
	} else if ( GGameType & GAME_QuakeWorld && sbq1_showteamscores )     {
		SbarQW_TeamOverlay();
	}

	if ( viddef.width > 320 && sbqh_lines > 0 ) {
		if ( GGameType & GAME_QuakeWorld || cl.qh_gametype == QHGAME_DEATHMATCH ) {
			SbarQ1_MiniDeathmatchOverlay();
		}
	}
}

static void SbarQ1_IntermissionNumber( int x, int y, int num, int digits, int color ) {
	R_VerifyNoRenderCommands();
	char str[ 12 ];
	char* ptr;
	int l, frame;

	l = SbarQH_itoa( num, str );
	ptr = str;
	if ( l > digits ) {
		ptr += ( l - digits );
	}
	if ( l < digits ) {
		x += ( digits - l ) * 24;
	}

	while ( *ptr ) {
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else   {
			frame = *ptr - '0';
		}

		UI_DrawPic( x,y,sbq1_nums[ color ][ frame ] );
		x += 24;
		ptr++;
	}
	R_SyncRenderThread();
}

void SbarQ1_IntermissionOverlay() {
	if ( GGameType & GAME_QuakeWorld ) {
		if ( String::Atoi( Info_ValueForKey( cl.qh_serverinfo, "teamplay" ) ) > 0 && !sbq1_showscores ) {
			SbarQW_TeamOverlay();
		} else   {
			SbarQ1_DeathmatchOverlay( 0 );
		}
		return;
	}

	if ( cl.qh_gametype == QHGAME_DEATHMATCH ) {
		SbarQ1_DeathmatchOverlay( 0 );
		return;
	}

	R_VerifyNoRenderCommands();
	image_t* pic = R_CachePic( "gfx/complete.lmp" );
	UI_DrawPic( 64, 24, pic );

	pic = R_CachePic( "gfx/inter.lmp" );
	UI_DrawPic( 0, 56, pic );
	R_SyncRenderThread();

	// time
	int dig = cl.qh_completed_time / 60;
	SbarQ1_IntermissionNumber( 160, 64, dig, 3, 0 );
	R_VerifyNoRenderCommands();
	int num = cl.qh_completed_time - dig * 60;
	UI_DrawPic( 234,64,sbq1_colon );
	UI_DrawPic( 246,64,sbq1_nums[ 0 ][ num / 10 ] );
	UI_DrawPic( 266,64,sbq1_nums[ 0 ][ num % 10 ] );
	R_SyncRenderThread();

	SbarQ1_IntermissionNumber( 160, 104, cl.qh_stats[ Q1STAT_SECRETS ], 3, 0 );
	R_VerifyNoRenderCommands();
	UI_DrawPic( 232,104,sbq1_slash );
	R_SyncRenderThread();
	SbarQ1_IntermissionNumber( 240, 104, cl.qh_stats[ Q1STAT_TOTALSECRETS ], 3, 0 );

	SbarQ1_IntermissionNumber( 160, 144, cl.qh_stats[ Q1STAT_MONSTERS ], 3, 0 );
	R_VerifyNoRenderCommands();
	UI_DrawPic( 232,144,sbq1_slash );
	R_SyncRenderThread();
	SbarQ1_IntermissionNumber( 240, 144, cl.qh_stats[ Q1STAT_TOTALMONSTERS ], 3, 0 );
}

void SbarQ1_FinaleOverlay() {
	R_VerifyNoRenderCommands();
	image_t* pic = R_CachePic( "gfx/finale.lmp" );
	UI_DrawPic( ( viddef.width - R_GetImageWidth( pic ) ) / 2, 16, pic );
	R_SyncRenderThread();
}
