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

// cmd.c -- Quake script command processing module

#include "../game/q_shared.h"
#include "qcommon.h"

#define MAX_CMD_BUFFER  131072
#define MAX_CMD_LINE    1024

struct QCmd
{
	byte    *data;
	int maxsize;
	int cursize;
};

extern int cmd_wait;
extern QCmd cmd_text;
extern byte cmd_text_buf[MAX_CMD_BUFFER];


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText( int exec_when, const char *text ) {
	switch ( exec_when )
	{
	case EXEC_NOW:
		if ( text && String::Length( text ) > 0 ) {
			Cmd_ExecuteString( text );
		} else {
			Cbuf_Execute();
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText( text );
		break;
	case EXEC_APPEND:
		Cbuf_AddText( text );
		break;
	default:
		Com_Error( ERR_FATAL, "Cbuf_ExecuteText: bad exec_when" );
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute( void ) {
	int i;
	char    *text;
	char line[MAX_CMD_LINE];
	int quotes;

	while ( cmd_text.cursize )
	{
		if ( cmd_wait ) {
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for ( i = 0 ; i < cmd_text.cursize ; i++ )
		{
			if ( text[i] == '"' ) {
				quotes++;
			}
			if ( !( quotes & 1 ) &&  text[i] == ';' ) {
				break;  // don't break if inside a quoted string
			}
			if ( text[i] == '\n' || text[i] == '\r' ) {
				break;
			}
		}

		if ( i >= ( MAX_CMD_LINE - 1 ) ) {
			i = MAX_CMD_LINE - 1;
		}

		memcpy( line, text, i );
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if ( i == cmd_text.cursize ) {
			cmd_text.cursize = 0;
		} else
		{
			i++;
			cmd_text.cursize -= i;
			memmove( text, text + i, cmd_text.cursize );
		}

// execute the command line

		Cmd_ExecuteString( line );
	}
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f( void ) {
	char    *f;
	int len;
	char filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "exec <filename> : execute a script file\n" );
		return;
	}

	String::NCpyZ( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	String::DefaultExtension( filename, sizeof( filename ), ".cfg" );
	len = FS_ReadFile( filename, (void **)&f );
	if ( !f ) {
		Com_Printf( "couldn't exec %s\n",Cmd_Argv( 1 ) );
		return;
	}
	Com_Printf( "execing %s\n",Cmd_Argv( 1 ) );

	Cbuf_InsertText( f );

	FS_FreeFile( f );
}


/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
void Cmd_Vstr_f( void ) {
	char    *v;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "vstr <variablename> : execute a variable command\n" );
		return;
	}

	v = Cvar_VariableString( Cmd_Argv( 1 ) );
	Cbuf_InsertText( va( "%s\n", v ) );
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

struct cmd_function_t
{
	cmd_function_t*next;
	char                    *name;
	xcommand_t function;
};


extern int cmd_argc;
extern char        *cmd_argv[1024];        // points into cmd_tokenized
extern char cmd_tokenized[BIG_INFO_STRING + 1024];         // will have 0 bytes inserted
extern char cmd_cmd[BIG_INFO_STRING];         // the original command we received (no token processing)

extern cmd_function_t  *cmd_functions;      // possible commands to execute

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void    Cmd_ExecuteString( const char *text ) {
	cmd_function_t  *cmd, **prev;

	// execute the command line
	Cmd_TokenizeString( text );
	if ( !Cmd_Argc() ) {
		return;     // no tokens
	}

	// check registered command functions
	for ( prev = &cmd_functions ; *prev ; prev = &cmd->next ) {
		cmd = *prev;
		if ( !String::ICmp( cmd_argv[0],cmd->name ) ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = cmd_functions;
			cmd_functions = cmd;

			// perform the action
			if ( !cmd->function ) {
				// let the cgame or game handle it
				break;
			} else {
				cmd->function();
			}
			return;
		}
	}

	// check cvars
	if ( Cvar_Command() ) {
		return;
	}

	// check client game commands
	if ( com_cl_running && com_cl_running->integer && CL_GameCommand() ) {
		return;
	}

	// check server game commands
	if ( com_sv_running && com_sv_running->integer && SV_GameCommand() ) {
		return;
	}

	// check ui commands
	if ( com_cl_running && com_cl_running->integer && UI_GameCommand() ) {
		return;
	}

	// send it as a server command if we are connected
	// this will usually result in a chat message
	CL_ForwardCommandToServer( text );
}

/*
============
Cmd_Init
============
*/
void Cmd_Init( void ) {
	Cmd_SharedInit();
	Cmd_AddCommand( "exec",Cmd_Exec_f );
	Cmd_AddCommand( "vstr",Cmd_Vstr_f );
}
