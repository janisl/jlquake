/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// console.c

#include "client.h"


int g_console_field_width = 78;

#define COLNSOLE_COLOR  COLOR_WHITE //COLOR_BLACK

Cvar      *con_debug;
Cvar      *con_conspeed;
Cvar      *con_notifytime;
Cvar      *con_autoclear;

// DHM - Nerve :: Must hold CTRL + SHIFT + ~ to get console
Cvar      *con_restricted;

#define DEFAULT_CONSOLE_WIDTH   78

vec4_t console_color = {1.0, 1.0, 1.0, 1.0};
vec4_t console_highlightcolor = {0.5, 0.5, 0.2, 0.45};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void ) {
	con.acLength = 0;

	if ( con_restricted->integer && ( !keys[K_CTRL].down || !keys[K_SHIFT].down ) ) {
		return;
	}

	// ydnar: persistent console input is more useful
	// Arnout: added cvar
	if ( con_autoclear->integer ) {
		Field_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	// ydnar: multiple console size support
	if ( in_keyCatchers & KEYCATCH_CONSOLE ) {
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		con.desiredFrac = 0.0;
	} else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if ( keys[ K_CTRL ].down ) {
			con.desiredFrac = ( 5.0 * SMALLCHAR_HEIGHT ) / cls.glconfig.vidHeight;
		}
		// full console
		else if ( keys[ K_ALT ].down ) {
			con.desiredFrac = 1.0;
		}
		// normal half-screen console
		else {
			con.desiredFrac = 0.5;
		}
	}
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f( void ) {
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f( void ) {
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f( void ) {
	chat_team = qfalse;
	chat_buddy = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 26;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}


/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void ) {
	int i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
	}

	Con_Bottom();       // go to end
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f( void ) {
	int l, x, i;
	short   *line;
	fileHandle_t f;
	char buffer[1024];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: condump <filename>\n" );
		return;
	}

	Com_Printf( "Dumped console text to %s.\n", Cmd_Argv( 1 ) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if ( !f ) {
		Com_Printf( "ERROR: couldn't open.\n" );
		return;
	}

	// skip empty lines
	for ( l = con.current - con.totallines + 1 ; l <= con.current ; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for ( x = 0 ; x < con.linewidth ; x++ )
			if ( ( line[x] & 0xff ) != ' ' ) {
				break;
			}
		if ( x != con.linewidth ) {
			break;
		}
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for ( i = 0; i < con.linewidth; i++ )
			buffer[i] = line[i] & 0xff;
		for ( x = con.linewidth - 1 ; x >= 0 ; x-- )
		{
			if ( buffer[x] == ' ' ) {
				buffer[x] = 0;
			} else {
				break;
			}
		}
		strcat( buffer, "\n" );
		FS_Write( buffer, String::Length( buffer ), f );
	}

	FS_FCloseFile( f );
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void ) {
	int i, j, width, oldwidth, oldtotallines, numlines, numchars;
	MAC_STATIC short tbuf[CON_TEXTSIZE];

	// ydnar: wasn't allowing for larger consoles
	// width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	width = ( cls.glconfig.vidWidth / SMALLCHAR_WIDTH ) - 2;

	if ( width == con.linewidth ) {
		return;
	}

	if ( width < 1 ) {        // video hasn't been initialized yet
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for ( i = 0; i < CON_TEXTSIZE; i++ )

			con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
	} else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if ( con.totallines < numlines ) {
			numlines = con.totallines;
		}

		numchars = oldwidth;

		if ( con.linewidth < numchars ) {
			numchars = con.linewidth;
		}

		memcpy( tbuf, con.text, CON_TEXTSIZE * sizeof( short ) );
		for ( i = 0; i < CON_TEXTSIZE; i++ )

			con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';


		for ( i = 0 ; i < numlines ; i++ )
		{
			for ( j = 0 ; j < numchars ; j++ )
			{
				con.text[( con.totallines - 1 - i ) * con.linewidth + j] =
					tbuf[( ( con.current - i + oldtotallines ) %
						   oldtotallines ) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init( void ) {
	int i;

	con_notifytime = Cvar_Get( "con_notifytime", "7", 0 ); // JPW NERVE increased per id req for obits
	con_conspeed = Cvar_Get( "scr_conspeed", "3", 0 );
	con_debug = Cvar_Get( "con_debug", "0", CVAR_ARCHIVE );   //----(SA)	added
	con_autoclear = Cvar_Get( "con_autoclear", "1", CVAR_ARCHIVE );
	con_restricted = Cvar_Get( "con_restricted", "0", CVAR_INIT );        // DHM - Nerve

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand( "toggleConsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );

	// ydnar: these are deprecated in favor of cgame/ui based version
	Cmd_AddCommand( "clMessageMode", Con_MessageMode_f );
	Cmd_AddCommand( "clMessageMode2", Con_MessageMode2_f );
	Cmd_AddCommand( "clMessageMode3", Con_MessageMode3_f );
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed( qboolean skipnotify ) {
	int i;

	// mark time for transparent overlay
	if ( con.current >= 0 ) {
		if ( skipnotify ) {
			con.times[con.current % NUM_CON_TIMES] = 0;
		} else {
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}

	con.x = 0;
	if ( con.display == con.current ) {
		con.display++;
	}
	con.current++;
	for ( i = 0; i < con.linewidth; i++ )
		con.text[( con.current % con.totallines ) * con.linewidth + i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", off ) // SMF - msvc totally screws this function up with optimize on
#endif

void CL_ConsolePrint( char *txt ) {
	int y;
	int c, l;
	int color;
	qboolean skipnotify = qfalse;       // NERVE - SMF
	int prev;                           // NERVE - SMF

	// NERVE - SMF - work around for text that shows up in console but not in notify
	if ( !String::NCmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	if ( !con.initialized ) {
		con.color[0] =
			con.color[1] =
				con.color[2] =
					con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	color = ColorIndex( COLNSOLE_COLOR );

	while ( ( c = *txt ) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			if ( *( txt + 1 ) == COLOR_NULL ) {
				color = ColorIndex( COLNSOLE_COLOR );
			} else {
				color = ColorIndex( *( txt + 1 ) );
			}
			txt += 2;
			continue;
		}

		// count word length
		for ( l = 0 ; l < con.linewidth ; l++ ) {
			if ( txt[l] <= ' ' ) {
				break;
			}

		}

		// word wrap
		if ( l != con.linewidth && ( con.x + l >= con.linewidth ) ) {
			Con_Linefeed( skipnotify );

		}

		txt++;

		switch ( c )
		{
		case '\n':
			Con_Linefeed( skipnotify );
			break;
		case '\r':
			con.x = 0;
			break;
		default:    // display character and advance
			y = con.current % con.totallines;
			// rain - sign extension caused the character to carry over
			// into the color info for high ascii chars; casting c to unsigned
			con.text[y * con.linewidth + con.x] = ( color << 8 ) | (unsigned char)c;
			con.x++;
			if ( con.x >= con.linewidth ) {

				Con_Linefeed( skipnotify );
				con.x = 0;
			}
			break;
		}
	}

	// mark time for transparent overlay
	if ( con.current >= 0 ) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 ) {
				prev = NUM_CON_TIMES - 1;
			}
			con.times[prev] = 0;
		} else {
			// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
}

#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", on ) // SMF - re-enabled optimization
#endif

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( void ) {
	int y;

	if ( cls.state != CA_DISCONNECTED && !( in_keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * 2 );

	// hightlight the current autocompleted part
	if ( con.acLength ) {
		Cmd_TokenizeString( g_consoleField.buffer );

		if ( String::Length( Cmd_Argv( 0 ) ) - con.acLength > 0 ) {
			R_SetColor( console_highlightcolor );
			R_StretchPic( con.xadjust + ( 2 + con.acLength ) * SMALLCHAR_WIDTH,
							   y + 2,
							   ( String::Length( Cmd_Argv( 0 ) ) - con.acLength ) * SMALLCHAR_WIDTH,
							   SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, cls.whiteShader );
		}
	}

	R_SetColor( con.color );

	SCR_DrawSmallChar( con.xadjust + 1 * SMALLCHAR_WIDTH, y, ']' );

	Field_Draw( &g_consoleField, con.xadjust + 2 * SMALLCHAR_WIDTH, y,
				SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void ) {
	int x, v;
	short   *text;
	int i;
	int time;
	int skip;
	int currentColor;

	currentColor = 7;
	R_SetColor( g_color_table[currentColor] );

	v = 0;
	for ( i = con.current - NUM_CON_TIMES + 1 ; i <= con.current ; i++ )
	{
		if ( i < 0 ) {
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = cls.realtime - time;
		if ( time > con_notifytime->value * 1000 ) {
			continue;
		}
		text = con.text + ( i % con.totallines ) * con.linewidth;

		if ( cl.et_snap.ps.pm_type != PM_INTERMISSION && in_keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) {
			continue;
		}

		for ( x = 0 ; x < con.linewidth ; x++ ) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( ( text[x] >> 8 ) & COLOR_BITS ) != currentColor ) {
				currentColor = ( text[x] >> 8 ) & COLOR_BITS;
				R_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	R_SetColor( NULL );

	if ( in_keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) {
		return;
	}

	// draw the chat line
	if ( in_keyCatchers & KEYCATCH_MESSAGE ) {
		if ( chat_team ) {
			char buf[128];
			CL_TranslateString( "say_team:", buf );
			SCR_DrawBigString( 8, v, buf, 1.0f );
			skip = String::Length( buf ) + 2;
		} else if ( chat_buddy ) {
			char buf[128];
			CL_TranslateString( "say_fireteam:", buf );
			SCR_DrawBigString( 8, v, buf, 1.0f );
			skip = String::Length( buf ) + 2;
		} else {
			char buf[128];
			CL_TranslateString( "say:", buf );
			SCR_DrawBigString( 8, v, buf, 1.0f );
			skip = String::Length( buf ) + 1;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
					   SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/

void Con_DrawSolidConsole( float frac ) {
	int i, x, y;
	int rows;
	short           *text;
	int row;
	int lines;
	int currentColor;
	vec4_t color;

	lines = cls.glconfig.vidHeight * frac;
	if ( lines <= 0 ) {
		return;
	}

	if ( lines > cls.glconfig.vidHeight ) {
		lines = cls.glconfig.vidHeight;
	}

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	} else {
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, y, cls.consoleShader );

		// NERVE - SMF - merged from WolfSP
		if ( frac >= 0.5f ) {
			color[0] = color[1] = color[2] = frac * 2.0f;
			color[3] = 1.0f;
			R_SetColor( color );

			// draw the logo
			SCR_DrawPic( 192, 70, 256, 128, cls.consoleShader2 );
			R_SetColor( NULL );
		}
		// -NERVE - SMF
	}

	// ydnar: matching light text
	color[0] = 0.75;
	color[1] = 0.75;
	color[2] = 0.75;
	color[3] = 1.0f;
	if ( frac < 1.0 ) {
		SCR_FillRect( 0, y, SCREEN_WIDTH, 1.25, color );
	}


	// draw the version number

	R_SetColor( g_color_table[ColorIndex( COLNSOLE_COLOR )] );

	i = String::Length( Q3_VERSION );

	for ( x = 0 ; x < i ; x++ ) {

		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH,

						   ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ), Q3_VERSION[x] );

	}


	// draw the text
	con.vislines = lines;
	rows = ( lines - SMALLCHAR_WIDTH ) / SMALLCHAR_WIDTH;     // rows of text to draw

	y = lines - ( SMALLCHAR_HEIGHT * 3 );

	// draw from the bottom up
	if ( con.display != con.current ) {
		// draw arrows to show the buffer is backscrolled
		R_SetColor( g_color_table[ColorIndex( COLOR_WHITE )] );
		for ( x = 0 ; x < con.linewidth ; x += 4 )
			SCR_DrawSmallChar( con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	R_SetColor( g_color_table[currentColor] );

	for ( i = 0 ; i < rows ; i++, y -= SMALLCHAR_HEIGHT, row-- )
	{
		if ( row < 0 ) {
			break;
		}
		if ( con.current - row >= con.totallines ) {
			// past scrollback wrap point
			continue;
		}

		text = con.text + ( row % con.totallines ) * con.linewidth;

		for ( x = 0 ; x < con.linewidth ; x++ ) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( ( text[x] >> 8 ) & COLOR_BITS ) != currentColor ) {
				currentColor = ( text[x] >> 8 ) & COLOR_BITS;
				R_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, y, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();

	R_SetColor( NULL );
}

extern Cvar   *con_drawnotify;

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( in_keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE && con_drawnotify->integer ) {
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void ) {
	// decide on the destination height of the console
	// ydnar: added short console support (via shift+~)
	if ( in_keyCatchers & KEYCATCH_CONSOLE ) {
		con.finalFrac = con.desiredFrac;
	} else {
		con.finalFrac = 0;  // none visible

	}
	// scroll towards the destination height
	if ( con.finalFrac < con.displayFrac ) {
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;
		if ( con.finalFrac > con.displayFrac ) {
			con.displayFrac = con.finalFrac;
		}

	} else if ( con.finalFrac > con.displayFrac )     {
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;
		if ( con.finalFrac < con.displayFrac ) {
			con.displayFrac = con.finalFrac;
		}
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if ( con.display > con.current ) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;              // none visible
	con.displayFrac = 0;
}
