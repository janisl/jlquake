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
#include "../../../common/Common.h"
#include "../../../common/strings.h"

#define STAT_MINUS      10	// num frame for '-' stats digit
#define CHAR_WIDTH      16

#define Q2STAT_LAYOUTS  13

const char* sb_nums[ 2 ][ 11 ] =
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	 "num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	 "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

static void SCRQ2_DrawField( int x, int y, int color, int width, int value ) {
	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	char num[ 16 ];
	String::Sprintf( num, sizeof ( num ), "%i", value );
	int l = String::Length( num );
	if ( l > width ) {
		l = width;
	}
	x += 2 + CHAR_WIDTH * ( width - l );

	char* ptr = num;
	while ( *ptr && l ) {
		int frame;
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else   {
			frame = *ptr - '0';
		}

		UI_DrawNamedPic( x,y,sb_nums[ color ][ frame ] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

static void SCRQ2_DrawHUDString( const char* string, int x, int y, int centerwidth, int _xor ) {
	int margin = x;

	while ( *string ) {
		// scan out one line of text from the string
		int width = 0;
		char line[ 1024 ];
		while ( *string && *string != '\n' ) {
			line[ width++ ] = *string++;
		}
		line[ width ] = 0;

		if ( centerwidth ) {
			x = margin + ( centerwidth - width * 8 ) / 2;
		} else   {
			x = margin;
		}
		UI_DrawString( x, y, line, _xor );
		if ( *string ) {
			string++;	// skip the \n
			x = margin;
			y += 8;
		}
	}
}

static void SCRQ2_ExecuteLayoutString( const char* s ) {
	if ( cls.state != CA_ACTIVE || !cl.q2_refresh_prepped ) {
		return;
	}

	if ( !s[ 0 ] ) {
		return;
	}

	int x = 0;
	int y = 0;
	int width = 3;

	while ( s ) {
		const char* token = String::Parse2( &s );
		if ( !String::Cmp( token, "xl" ) ) {
			token = String::Parse2( &s );
			x = String::Atoi( token );
			continue;
		}
		if ( !String::Cmp( token, "xr" ) ) {
			token = String::Parse2( &s );
			x = viddef.width + String::Atoi( token );
			continue;
		}
		if ( !String::Cmp( token, "xv" ) ) {
			token = String::Parse2( &s );
			x = viddef.width / 2 - 160 + String::Atoi( token );
			continue;
		}

		if ( !String::Cmp( token, "yt" ) ) {
			token = String::Parse2( &s );
			y = String::Atoi( token );
			continue;
		}
		if ( !String::Cmp( token, "yb" ) ) {
			token = String::Parse2( &s );
			y = viddef.height + String::Atoi( token );
			continue;
		}
		if ( !String::Cmp( token, "yv" ) ) {
			token = String::Parse2( &s );
			y = viddef.height / 2 - 120 + String::Atoi( token );
			continue;
		}

		if ( !String::Cmp( token, "pic" ) ) {	// draw a pic from a stat number
			token = String::Parse2( &s );
			int value = cl.q2_frame.playerstate.stats[ String::Atoi( token ) ];
			if ( value >= MAX_IMAGES_Q2 ) {
				common->Error( "Pic >= MAX_IMAGES_Q2" );
			}
			if ( cl.q2_configstrings[ Q2CS_IMAGES + value ] ) {
				UI_DrawNamedPic( x, y, cl.q2_configstrings[ Q2CS_IMAGES + value ] );
			}
			continue;
		}

		if ( !String::Cmp( token, "client" ) ) {// draw a deathmatch client block
			int score, ping, time;

			token = String::Parse2( &s );
			x = viddef.width / 2 - 160 + String::Atoi( token );
			token = String::Parse2( &s );
			y = viddef.height / 2 - 120 + String::Atoi( token );

			token = String::Parse2( &s );
			int value = String::Atoi( token );
			if ( value >= MAX_CLIENTS_Q2 || value < 0 ) {
				common->Error( "client >= MAX_CLIENTS_Q2" );
			}
			q2clientinfo_t* ci = &cl.q2_clientinfo[ value ];

			token = String::Parse2( &s );
			score = String::Atoi( token );

			token = String::Parse2( &s );
			ping = String::Atoi( token );

			token = String::Parse2( &s );
			time = String::Atoi( token );

			UI_DrawString( x + 32, y, ci->name, 0x80 );
			UI_DrawString( x + 32, y + 8,  "Score: " );
			UI_DrawString( x + 32 + 7 * 8, y + 8,  va( "%i", score ), 0x80 );
			UI_DrawString( x + 32, y + 16, va( "Ping:  %i", ping ) );
			UI_DrawString( x + 32, y + 24, va( "Time:  %i", time ) );

			if ( !ci->icon ) {
				ci = &cl.q2_baseclientinfo;
			}
			UI_DrawNamedPic( x, y, ci->iconname );
			continue;
		}

		if ( !String::Cmp( token, "ctf" ) ) {	// draw a ctf client block
			int score, ping;
			char block[ 80 ];

			token = String::Parse2( &s );
			x = viddef.width / 2 - 160 + String::Atoi( token );
			token = String::Parse2( &s );
			y = viddef.height / 2 - 120 + String::Atoi( token );

			token = String::Parse2( &s );
			int value = String::Atoi( token );
			if ( value >= MAX_CLIENTS_Q2 || value < 0 ) {
				common->Error( "client >= MAX_CLIENTS_Q2" );
			}
			q2clientinfo_t* ci = &cl.q2_clientinfo[ value ];

			token = String::Parse2( &s );
			score = String::Atoi( token );

			token = String::Parse2( &s );
			ping = String::Atoi( token );
			if ( ping > 999 ) {
				ping = 999;
			}

			sprintf( block, "%3d %3d %-12.12s", score, ping, ci->name );

			if ( value == cl.playernum ) {
				UI_DrawString( x, y, block, 0x80 );
			} else {
				UI_DrawString( x, y, block );
			}
			continue;
		}

		if ( !String::Cmp( token, "picn" ) ) {	// draw a pic from a name
			token = String::Parse2( &s );
			UI_DrawNamedPic( x, y, token );
			continue;
		}

		if ( !String::Cmp( token, "num" ) ) {	// draw a number
			token = String::Parse2( &s );
			width = String::Atoi( token );
			token = String::Parse2( &s );
			int value = cl.q2_frame.playerstate.stats[ String::Atoi( token ) ];
			SCRQ2_DrawField( x, y, 0, width, value );
			continue;
		}

		if ( !String::Cmp( token, "hnum" ) ) {	// health number
			int color;

			width = 3;
			int value = cl.q2_frame.playerstate.stats[ Q2STAT_HEALTH ];
			if ( value > 25 ) {
				color = 0;	// green
			} else if ( value > 0 )     {
				color = ( cl.q2_frame.serverframe >> 2 ) & 1;			// flash
			} else   {
				color = 1;
			}

			if ( cl.q2_frame.playerstate.stats[ Q2STAT_FLASHES ] & 1 ) {
				UI_DrawNamedPic( x, y, "field_3" );
			}

			SCRQ2_DrawField( x, y, color, width, value );
			continue;
		}

		if ( !String::Cmp( token, "anum" ) ) {	// ammo number
			int color;

			width = 3;
			int value = cl.q2_frame.playerstate.stats[ Q2STAT_AMMO ];
			if ( value > 5 ) {
				color = 0;	// green
			} else if ( value >= 0 )     {
				color = ( cl.q2_frame.serverframe >> 2 ) & 1;			// flash
			} else   {
				continue;	// negative number = don't show
			}

			if ( cl.q2_frame.playerstate.stats[ Q2STAT_FLASHES ] & 4 ) {
				UI_DrawNamedPic( x, y, "field_3" );
			}

			SCRQ2_DrawField( x, y, color, width, value );
			continue;
		}

		if ( !String::Cmp( token, "rnum" ) ) {	// armor number
			int color;

			width = 3;
			int value = cl.q2_frame.playerstate.stats[ Q2STAT_ARMOR ];
			if ( value < 1 ) {
				continue;
			}

			color = 0;	// green

			if ( cl.q2_frame.playerstate.stats[ Q2STAT_FLASHES ] & 2 ) {
				UI_DrawNamedPic( x, y, "field_3" );
			}

			SCRQ2_DrawField( x, y, color, width, value );
			continue;
		}


		if ( !String::Cmp( token, "stat_string" ) ) {
			token = String::Parse2( &s );
			int index = String::Atoi( token );
			if ( index < 0 || index >= MAX_CONFIGSTRINGS_Q2 ) {
				common->Error( "Bad stat_string index" );
			}
			index = cl.q2_frame.playerstate.stats[ index ];
			if ( index < 0 || index >= MAX_CONFIGSTRINGS_Q2 ) {
				common->Error( "Bad stat_string index" );
			}
			UI_DrawString( x, y, cl.q2_configstrings[ index ] );
			continue;
		}

		if ( !String::Cmp( token, "cstring" ) ) {
			token = String::Parse2( &s );
			SCRQ2_DrawHUDString( token, x, y, 320, 0 );
			continue;
		}

		if ( !String::Cmp( token, "string" ) ) {
			token = String::Parse2( &s );
			UI_DrawString( x, y, token );
			continue;
		}

		if ( !String::Cmp( token, "cstring2" ) ) {
			token = String::Parse2( &s );
			SCRQ2_DrawHUDString( token, x, y, 320,0x80 );
			continue;
		}

		if ( !String::Cmp( token, "string2" ) ) {
			token = String::Parse2( &s );
			UI_DrawString( x, y, token, 0x80 );
			continue;
		}

		if ( !String::Cmp( token, "if" ) ) {
			// draw a number
			token = String::Parse2( &s );
			int value = cl.q2_frame.playerstate.stats[ String::Atoi( token ) ];
			if ( !value ) {
				// skip to endif
				while ( s && String::Cmp( token, "endif" ) ) {
					token = String::Parse2( &s );
				}
			}
			continue;
		}
	}
}

//	The status bar is a small layout program that
// is based on the stats array
static void SCRQ2_DrawStats() {
	SCRQ2_ExecuteLayoutString( cl.q2_configstrings[ Q2CS_STATUSBAR ] );
}

static void SCRQ2_DrawLayout() {
	if ( !cl.q2_frame.playerstate.stats[ Q2STAT_LAYOUTS ] ) {
		return;
	}
	SCRQ2_ExecuteLayoutString( cl.q2_layout );
}

void CLQ2_ParseInventory( QMsg& message ) {
	for ( int i = 0; i < MAX_ITEMS_Q2; i++ ) {
		cl.q2_inventory[ i ] = message.ReadShort();
	}
}

static void CLQ2_DrawInventory() {
	enum { DISPLAY_ITEMS = 17 };
	int selected = cl.q2_frame.playerstate.stats[ Q2STAT_SELECTED_ITEM ];

	int num = 0;
	int selected_num = 0;
	int index[ MAX_ITEMS_Q2 ];
	for ( int i = 0; i < MAX_ITEMS_Q2; i++ ) {
		if ( i == selected ) {
			selected_num = num;
		}
		if ( cl.q2_inventory[ i ] ) {
			index[ num ] = i;
			num++;
		}
	}

	// determine scroll point
	int top = selected_num - DISPLAY_ITEMS / 2;
	if ( num - top < DISPLAY_ITEMS ) {
		top = num - DISPLAY_ITEMS;
	}
	if ( top < 0 ) {
		top = 0;
	}

	int x = ( viddef.width - 256 ) / 2;
	int y = ( viddef.height - 240 ) / 2;

	UI_DrawNamedPic( x, y + 8, "inventory" );

	y += 24;
	x += 24;
	UI_DrawString( x, y, "hotkey ### item" );
	UI_DrawString( x, y + 8, "------ --- ----" );
	y += 16;
	for ( int i = top; i < num && i < top + DISPLAY_ITEMS; i++ ) {
		int item = index[ i ];
		// search for a binding
		char binding[ 1024 ];
		String::Sprintf( binding, sizeof ( binding ), "use %s", cl.q2_configstrings[ Q2CS_ITEMS + item ] );
		int key = Key_GetKey( binding );
		const char* bind = key < 0 ? "" : Key_KeynumToString( key, true );

		char string[ 1024 ];
		String::Sprintf( string, sizeof ( string ), "%s%6s %3i %s", item != selected ? S_COLOR_GREEN : "",
			bind, cl.q2_inventory[ item ], cl.q2_configstrings[ Q2CS_ITEMS + item ] );
		// draw a blinky cursor by the selected item
		if ( item == selected && ( cls.realtime * 10 ) & 1 ) {
			UI_DrawChar( x - 8, y, 15 );
		}
		UI_DrawString( x, y, string );
		y += 8;
	}
}

void SCRQ2_DrawHud() {
	R_VerifyNoRenderCommands();
	SCRQ2_DrawStats();
	if ( cl.q2_frame.playerstate.stats[ Q2STAT_LAYOUTS ] & 1 ) {
		SCRQ2_DrawLayout();
	}
	if ( cl.q2_frame.playerstate.stats[ Q2STAT_LAYOUTS ] & 2 ) {
		CLQ2_DrawInventory();
	}
	R_SyncRenderThread();

	SCR_DrawNet();
	SCR_CheckDrawCenterString();
}
