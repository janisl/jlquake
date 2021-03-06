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
//**
//**	tty console routines
//**	NOTE: if the user is editing a line when something gets printed to
//**  the early console then it won't look good so we provide tty_Clear and
//**  tty_Show to be called before and after a stdout or stderr output
//**
//**************************************************************************

#include "Common.h"
#include "common_defs.h"
#include "strings.h"
#include "command_buffer.h"
#include "system_unix.h"
#include <signal.h>
#include <unistd.h>
#include <errno.h>

// FIXME TTimo should we gard this? most *nix system should comply?
#include <termios.h>

#define TTY_HISTORY     32

// enable/disabled tty input mode
// NOTE TTimo this is used during startup, cannot be changed during run
Cvar* ttycon = NULL;
// general flag to tell about tty console mode
bool ttycon_on = false;
// when printing general stuff to stdout stderr (Sys_Print)
//   we need to disable the tty console stuff
// this increments so we can recursively disable
static int ttycon_hide = 0;
// some key codes that the terminal may be using
// TTimo NOTE: I'm not sure how relevant this is
static int tty_erase;
static int tty_eof;

static termios tty_tc;

static field_t tty_con;
static int tty_acLength = 0;

// history
// NOTE TTimo this is a bit duplicate of the graphical console history
//   but it's safer and faster to write our own here
static field_t ttyEditLines[ TTY_HISTORY ];
static int hist_current = -1;
static int hist_count = 0;

static char returnedText[ 256 ];

// initialize the console input (tty mode if wanted and possible)
void Sys_ConsoleInputInit() {
	termios tc;

	// TTimo
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=390
	// ttycon 0 or 1, if the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not catched, turns into a SIGSTP
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	// FIXME TTimo initialize this in Sys_Init or something?
	ttycon = Cvar_Get( "ttycon", "1", 0 );
	if ( !ttycon->value ) {
		ttycon_on = false;
		return;
	}

	if ( isatty( STDIN_FILENO ) != 1 ) {
		common->Printf( "stdin is not a tty, tty console mode failed\n" );
		Cvar_Set( "ttycon", "0" );
		ttycon_on = false;
		return;
	}

	Field_Clear( &tty_con );
	if ( tcgetattr( 0, &tty_tc ) == -1 ) {
		common->Printf( "tcgetattr failed: %s\n", strerror( errno ) );
		Cvar_Set( "ttycon", "0" );
		ttycon_on = false;
		return;
	}
	tty_erase = tty_tc.c_cc[ VERASE ];
	tty_eof = tty_tc.c_cc[ VEOF ];
	tc = tty_tc;
	/*
	 ECHO: don't echo input characters
	 ICANON: enable canonical mode.  This  enables  the  special
	          characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	          STATUS, and WERASE, and buffers by lines.
	 ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	          DSUSP are received, generate the corresponding sig-
	          nal
	*/
	tc.c_lflag &= ~( ECHO | ICANON );
	/*
	 ISTRIP strip off bit 8
	 INPCK enable input parity checking
	 */
	tc.c_iflag &= ~( ISTRIP | INPCK );
	tc.c_cc[ VMIN ] = 1;
	tc.c_cc[ VTIME ] = 0;
	if ( tcsetattr( 0, TCSADRAIN, &tc ) == -1 ) {
		common->Printf( "tcsetattr failed: %s\n", strerror( errno ) );
		Cvar_Set( "ttycon", "0" );
		ttycon_on = false;
		return;
	}
	ttycon_on = true;
	common->Printf( "Started tty console (use +set ttycon 0 to disable)\n" );
}

// never exit without calling this, or your terminal will be left in a pretty bad state
void Sys_ConsoleInputShutdown() {
	if ( ttycon_on ) {
		common->Printf( "Shutdown tty console\n" );
		tcsetattr( 0, TCSADRAIN, &tty_tc );
		ttycon_on = false;
	}
}

//	flush stdin, I suspect some terminals are sending a LOT of shit
//	FIXME TTimo relevant?
static void tty_FlushIn() {
	char key;
	while ( read( 0, &key, 1 ) != -1 )
		;
}

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
static void tty_Back() {
	char key;
	key = '\b';
	write( 1, &key, 1 );
	key = ' ';
	write( 1, &key, 1 );
	key = '\b';
	write( 1, &key, 1 );
}

// clear the display of the line currently edited
// bring cursor back to beginning of line
void tty_Hide() {
	assert( ttycon_on );
	if ( ttycon_hide ) {
		ttycon_hide++;
		return;
	}
	if ( tty_con.cursor > 0 ) {
		for ( int i = 0; i < tty_con.cursor; i++ ) {
			tty_Back();
		}
	}
	ttycon_hide++;
}

//	show the current line
//	FIXME TTimo need to position the cursor if needed??
void tty_Show() {
	assert( ttycon_on );
	assert( ttycon_hide > 0 );
	ttycon_hide--;
	if ( ttycon_hide == 0 ) {
		if ( tty_con.cursor ) {
			for ( int i = 0; i < tty_con.cursor; i++ ) {
				write( 1, tty_con.buffer + i, 1 );
			}
		}
	}
}

static void Hist_Add( field_t* field ) {
	assert( hist_count <= TTY_HISTORY );
	assert( hist_count >= 0 );
	assert( hist_current >= -1 );
	assert( hist_current <= hist_count );
	// make some room
	for ( int i = TTY_HISTORY - 1; i > 0; i-- ) {
		ttyEditLines[ i ] = ttyEditLines[ i - 1 ];
	}
	ttyEditLines[ 0 ] = *field;
	if ( hist_count < TTY_HISTORY ) {
		hist_count++;
	}
	hist_current = -1;	// re-init
}

static field_t* Hist_Prev() {
	assert( hist_count <= TTY_HISTORY );
	assert( hist_count >= 0 );
	assert( hist_current >= -1 );
	assert( hist_current <= hist_count );
	int hist_prev = hist_current + 1;
	if ( hist_prev >= hist_count ) {
		return NULL;
	}
	hist_current++;
	return &( ttyEditLines[ hist_current ] );
}

static field_t* Hist_Next() {
	assert( hist_count <= TTY_HISTORY );
	assert( hist_count >= 0 );
	assert( hist_current >= -1 );
	assert( hist_current <= hist_count );
	if ( hist_current >= 0 ) {
		hist_current--;
	}
	if ( hist_current == -1 ) {
		return NULL;
	}
	return &( ttyEditLines[ hist_current ] );
}

//	Checks for a complete line of text typed in at the console, then forwards
// it to the host command processor
char* Sys_ConsoleInput() {
	if ( ttycon && ttycon->value ) {
		char key;
		int avail = read( 0, &key, 1 );
		if ( avail == -1 ) {
			return NULL;
		}

		// we have something
		// backspace?
		// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
		if ( ( key == tty_erase ) || ( key == 127 ) || ( key == 8 ) ) {
			if ( tty_con.cursor > 0 ) {
				tty_con.cursor--;
				tty_con.buffer[ tty_con.cursor ] = '\0';
				tty_Back();
				tty_acLength = 0;
			}
			return NULL;
		}

		// check if this is a control char
		if ( key && key < ' ' ) {
			if ( key == '\n' ) {
				// push it in history
				Hist_Add( &tty_con );
				String::Cpy( returnedText, tty_con.buffer );
				Field_Clear( &tty_con );
				key = '\n';
				write( 1, &key, 1 );
				tty_acLength = 0;
				return returnedText;
			}
			if ( key == '\t' ) {
				tty_Hide();
				Field_CompleteCommand( &tty_con, tty_acLength );
				// Field_CompleteCommand does weird things to the string, do a cleanup
				//   it adds a '\' at the beginning of the string
				//   cursor doesn't reflect actual length of the string that's sent back
				tty_con.cursor = String::Length( tty_con.buffer );
				if ( tty_con.cursor > 0 ) {
					if ( tty_con.buffer[ 0 ] == '\\' ) {
						for ( int i = 0; i <= tty_con.cursor; i++ ) {
							tty_con.buffer[ i ] = tty_con.buffer[ i + 1 ];
						}
						tty_con.cursor--;
					}
				}
				tty_Show();
				return NULL;
			}
			avail = read( 0, &key, 1 );
			if ( avail != -1 ) {
				// VT 100 keys
				if ( key == '[' || key == 'O' ) {
					avail = read( 0, &key, 1 );
					if ( avail != -1 ) {
						field_t* history;
						switch ( key ) {
						case 'A':
							history = Hist_Prev();
							if ( history ) {
								tty_Hide();
								tty_con = *history;
								tty_Show();
							}
							tty_FlushIn();
							tty_acLength = 0;
							return NULL;
						case 'B':
							history = Hist_Next();
							tty_Hide();
							if ( history ) {
								tty_con = *history;
							} else {
								Field_Clear( &tty_con );
							}
							tty_Show();
							tty_FlushIn();
							tty_acLength = 0;
							return NULL;
						case 'C':
							return NULL;
						case 'D':
							return NULL;
						}
					}
				}
			}
			common->DPrintf( "droping ISCTL sequence: %d, tty_erase: %d\n", key, tty_erase );
			tty_FlushIn();
			return NULL;
		}

		// push regular character
		tty_con.buffer[ tty_con.cursor ] = key;
		tty_con.cursor++;
		// print the current line (this is differential)
		write( 1, &key, 1 );
		tty_acLength = 0;
		return NULL;
	} else {
		if ( !com_dedicated || !com_dedicated->value ) {
			return NULL;
		}

		if ( !stdin_active ) {
			return NULL;
		}

		fd_set fdset;
		FD_ZERO( &fdset );
		FD_SET( 0, &fdset );	// stdin
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if ( select( 1, &fdset, NULL, NULL, &timeout ) == -1 || !FD_ISSET( 0, &fdset ) ) {
			return NULL;
		}

		int len = read( 0, returnedText, sizeof ( returnedText ) );
		if ( len == 0 ) {
			// eof!
			stdin_active = false;
			return NULL;
		}

		if ( len < 1 ) {
			return NULL;
		}
		returnedText[ len - 1 ] = 0;		// rip off the \n and terminate

		return returnedText;
	}
}

void Sys_ShowConsole( int visLevel, bool quitOnClose ) {
}

//	Print text to the dedicated console
void  Sys_Print( const char* msg ) {
	if ( ttycon_on ) {
		tty_Hide();
	}
	for ( quint8* p = ( quint8* )msg; *p; p++ ) {
		if ( ( *p >= 128 || *p < 32 ) && *p != 10 && *p != 13 && *p != 9 ) {
			fprintf( stderr, "[%02x]", *p );
		} else {
			putc( *p, stderr );
		}
	}
	if ( ttycon_on ) {
		tty_Show();
	}
}
