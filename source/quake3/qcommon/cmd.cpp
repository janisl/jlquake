/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmd.c -- Quake script command processing module

#include "../game/q_shared.h"
#include "qcommon.h"

#define	MAX_CMD_LINE	1024

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void Cmd_Wait_f( void ) {
	if ( Cmd_Argc() == 2 ) {
		cmd_wait = QStr::Atoi( Cmd_Argv( 1 ) );
	} else {
		cmd_wait = 1;
	}
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void Cbuf_AddText( const char *text ) {
	int		l;
	
	l = QStr::Length(text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText( const char *text ) {
	int		len;
	int		i;

	len = QStr::Length( text ) + 1;
	if ( len + cmd_text.cursize > cmd_text.maxsize ) {
		Com_Printf( "Cbuf_InsertText overflowed\n" );
		return;
	}

	// move the existing command text
	for ( i = cmd_text.cursize - 1 ; i >= 0 ; i-- ) {
		cmd_text.data[ i + len ] = cmd_text.data[ i ];
	}

	// copy the new text in
	Com_Memcpy( cmd_text.data, text, len - 1 );

	// add a \n
	cmd_text.data[ len - 1 ] = '\n';

	cmd_text.cursize += len;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, const char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		if (text && QStr::Length(text) > 0) {
			Cmd_ExecuteString (text);
		} else {
			Cbuf_Execute();
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[MAX_CMD_LINE];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
		if ( cmd_wait )	{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n' || text[i] == '\r' )
				break;
		}

		if( i >= (MAX_CMD_LINE - 1)) {
			i = MAX_CMD_LINE - 1;
		}
				
		Com_Memcpy (line, text, i);
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line

		Cmd_ExecuteString (line);		
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
	char	*f;
	int		len;
	char	filename[MAX_QPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	QStr::NCpyZ( filename, Cmd_Argv(1), sizeof( filename ) );
	QStr::DefaultExtension( filename, sizeof( filename ), ".cfg" ); 
	len = FS_ReadFile( filename, (void **)&f);
	if (!f) {
		Com_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));
	
	Cbuf_InsertText (f);

	FS_FreeFile (f);
}


/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
void Cmd_Vstr_f( void ) {
	const char	*v;

	if (Cmd_Argc () != 2) {
		Com_Printf ("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = Cvar_VariableString( Cmd_Argv( 1 ) );
	Cbuf_InsertText( va("%s\n", v ) );
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Com_Printf ("%s ",Cmd_Argv(i));
	Com_Printf ("\n");
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

bool Cmd_HandleNullCommand(const char* text)
{
    // let the cgame or game handle it
	return false;
}

void Cmd_HandleUnknownCommand()
{
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
	CL_ForwardCommandToServer(Cmd_Cmd());
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void	Cmd_ExecuteString( const char *text, cmd_source_t src)
{	
	cmd_function_t	*cmd, **prev;
	cmdalias_t		*a;

	// execute the command line
	cmd_source = src;
	Cmd_TokenizeString(text, cmd_macroExpand);		
	if (!Cmd_Argc())
    {
		return;		// no tokens
	}

	// check registered command functions	
	for (prev = &cmd_functions; *prev; prev = &cmd->next)
    {
		cmd = *prev;
		if (!QStr::ICmp(cmd_argv[0], cmd->name))
        {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = cmd_functions;
			cmd_functions = cmd;

			// perform the action
			if (!cmd->function)
            {
				if (!Cmd_HandleNullCommand(text))
                {
                    break;
                }
			}
            else
            {
				cmd->function();
			}
			return;
		}
	}

	// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!QStr::ICmp(cmd_argv[0], a->name))
		{
			if (++alias_count == ALIAS_LOOP_COUNT)
			{
                GLog.Write("ALIAS_LOOP_COUNT\n");
				return;
			}
			Cbuf_InsertText (a->value);
			return;
		}
	}
	
	// check cvars
	if (Cvar_Command())
    {
		return;
	}

    Cmd_HandleUnknownCommand();
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int				i;
	char			*match;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	} else {
		match = NULL;
	}

	i = 0;
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		if (match && !Com_Filter(match, cmd->name, qfalse)) continue;

		Com_Printf ("%s\n", cmd->name);
		i++;
	}
	Com_Printf ("%i commands\n", i);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void) {
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("vstr",Cmd_Vstr_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

