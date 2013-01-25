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

#include "command_line_args.h"
#include "Common.h"
#include "common_defs.h"
#include "strings.h"
#include "command_buffer.h"

#define MAX_NUM_ARGVS       50
#define NUM_SAFE_ARGVS      4
#define MAX_CONSOLE_LINES   32

static int com_argc;
static const char* com_argv[ MAX_NUM_ARGVS + 1 ];

static const char* safeargvs[ NUM_SAFE_ARGVS ] = { "-nomidi", "-nolan", "-nosound", "-nocdaudio" };

static int com_numConsoleLines;
static char* com_consoleLines[ MAX_CONSOLE_LINES ];

int COM_Argc() {
	return com_argc;
}

const char* COM_Argv( int arg ) {
	if ( arg < 0 || arg >= com_argc || !com_argv[ arg ] ) {
		return "";
	}
	return com_argv[ arg ];
}

void COM_InitArgv( int argc, char** argv ) {
	if ( argc > MAX_NUM_ARGVS ) {
		common->FatalError( "argc > MAX_NUM_ARGVS" );
	}
	com_argc = argc;
	for ( int i = 0; i < argc; i++ ) {
		if ( !argv[ i ] ) {	// || String::Length(argv[i]) >= MAX_TOKEN_CHARS)
			com_argv[ i ] = "";
		} else   {
			com_argv[ i ] = argv[ i ];
		}
	}
}

//	Adds the given string at the end of the current argument list
void COM_AddParm( const char* parm ) {
	if ( com_argc == MAX_NUM_ARGVS ) {
		common->FatalError( "COM_AddParm: MAX_NUM)ARGS" );
	}
	com_argv[ com_argc++ ] = parm;
}

void COM_ClearArgv( int arg ) {
	if ( arg < 0 || arg >= com_argc || !com_argv[ arg ] ) {
		return;
	}
	com_argv[ arg ] = "";
}

void COM_InitArgv2( int argc, char** argv ) {
	COM_InitArgv( argc, argv );

	if ( COM_CheckParm( "-safe" ) ) {
		// force all the safe-mode switches. Note that we reserved extra space in
		// case we need to add these, so we don't need an overflow check
		for ( int i = 0; i < NUM_SAFE_ARGVS; i++ ) {
			COM_AddParm( safeargvs[ i ] );
		}
	}
}

//	Returns the position (1 to argc-1) in the program's argument list
// where the given parameter apears, or 0 if not present
int COM_CheckParm( const char* parm ) {
	for ( int i = 1; i < com_argc; i++ ) {
		if ( !String::Cmp( parm, com_argv[ i ] ) ) {
			return i;
		}
	}

	return 0;
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

//	Break it up into multiple console lines
void Com_ParseCommandLine( char* commandLine ) {
	int inq = 0;
	com_consoleLines[ 0 ] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		if ( *commandLine == '"' ) {
			inq = !inq;
		}
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( ( *commandLine == '+' && !inq ) || *commandLine == '\n' || *commandLine == '\r' ) {
			if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
				return;
			}
			com_consoleLines[ com_numConsoleLines ] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}

//	Check for "safe" on the command line, which will
// skip loading of q3config.cfg
bool Com_SafeMode() {
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		Cmd_TokenizeString( com_consoleLines[ i ] );
		if ( !String::ICmp( Cmd_Argv( 0 ), "safe" ) ||
			 !String::ICmp( Cmd_Argv( 0 ), "cvar_restart" ) ) {
			com_consoleLines[ i ][ 0 ] = 0;
			return true;
		}
	}
	return false;
}

//	Adds command line parameters as script statements
// Commands are seperated by + signs
//	Returns true if any late commands were added, which
// will keep the demoloop from immediately starting
bool Com_AddStartupCommands() {
	bool added = false;
	// quote every token, so args with semicolons can work
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		if ( !com_consoleLines[ i ] || !com_consoleLines[ i ][ 0 ] ) {
			continue;
		}
		//	Allow old style command line arguments before any + argument.
		if ( !( GGameType & GAME_Tech3 ) && i == 0 && com_consoleLines[ i ][ 0 ] == '-' ) {
			continue;
		}

		// set commands won't override menu startup
		if ( String::NICmp( com_consoleLines[ i ], "set", 3 ) ) {
			added = true;
		}
		Cbuf_AddText( com_consoleLines[ i ] );
		Cbuf_AddText( "\n" );
	}

	return added;
}

//	Adds command line parameters as script statements. Commands lead with
// a +, and continue until a - or another +
//	quake +prog jctest.qp +cmd amlev1
//	quake -nosound +cmd amlev1
void Cmd_StuffCmds_f() {
	//	Go backwards because text is inserted instead of added.
	for ( int i = com_numConsoleLines - 1; i >= 0; i-- ) {
		if ( !com_consoleLines[ i ] || !com_consoleLines[ i ][ 0 ] ) {
			continue;
		}
		//	Allow old style command line arguments before any + argument.
		if ( i == 0 && com_consoleLines[ i ][ 0 ] == '-' ) {
			continue;
		}

		Cbuf_InsertText( "\n" );
		Cbuf_InsertText( com_consoleLines[ i ] );
	}
}

//	Searches for command line parameters that are set commands.
// If match is not NULL, only that cvar will be looked for.
// That is necessary because cddir and basedir need to be set
// before the filesystem is started, but all other sets shouls
// be after execing the config and default.
void Com_StartupVariable( const char* match ) {
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		Cmd_TokenizeString( com_consoleLines[ i ] );
		if ( String::Cmp( Cmd_Argv( 0 ), "set" ) ) {
			continue;
		}

		const char* s = Cmd_Argv( 1 );
		if ( !match || !String::Cmp( s, match ) ) {
			Cvar_Set( s, Cmd_Argv( 2 ) );
			Cvar* cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}
