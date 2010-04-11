/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmd.c -- Quake script command processing module

#include "qcommon.h"

void Cmd_ForwardToServer (void);

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
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
	cmd_text.Init(cmd_text_buf, sizeof(cmd_text_buf));
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		l;
	
	l = QStr::Length(text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	cmd_text.WriteData(text, QStr::Length(text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = (char*)Z_Malloc (templen);
		Com_Memcpy(temp, cmd_text.data, templen);
		cmd_text.Clear();
	}
	else
		temp = NULL;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText (text);
	
// add the copied off data
	if (templen)
	{
		cmd_text.WriteData(temp, templen);
		Z_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	Com_Memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	Cbuf_InsertText ((char*)defer_text_buf);
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		Cmd_ExecuteString (text);
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
	char	line[1024];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}
			
				
		Com_Memcpy(line, text, i);
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
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
		
		if (cmd_wait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}


/*
===============
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
void Cbuf_AddEarlyCommands (qboolean clear)
{
	int			i;
	const char	*s;

	for (i=0 ; i<COM_Argc() ; i++)
	{
		s = COM_Argv(i);
		if (QStr::Cmp(s, "+set"))
			continue;
		Cbuf_AddText (va("set %s %s\n", COM_Argv(i+1), COM_Argv(i+2)));
		if (clear)
		{
			COM_ClearArgv(i);
			COM_ClearArgv(i+1);
			COM_ClearArgv(i+2);
		}
		i+=2;
	}
}

/*
=================
Cbuf_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Cbuf_AddLateCommands (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;
	int		argc;
	qboolean	ret;

// build the combined string to parse from
	s = 0;
	argc = COM_Argc();
	for (i=1 ; i<argc ; i++)
	{
		s += QStr::Length(COM_Argv(i)) + 1;
	}
	if (!s)
		return false;
		
	text = (char*)Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<argc ; i++)
	{
		QStr::Cat(text, s + 1,COM_Argv(i));
		if (i != argc-1)
			QStr::Cat(text, s + 1, " ");
	}
	
// pull out the commands
	build = (char*)Z_Malloc (s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			QStr::Cat(build, s + 1, text+i);
			QStr::Cat(build, s + 1, "\n");
			text[j] = c;
			i = j-1;
		}
	}

	ret = (build[0] != 0);
	if (ret)
		Cbuf_AddText (build);
	
	Z_Free (text);
	Z_Free (build);

	return ret;
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
void Cmd_Exec_f (void)
{
	char	*f, *f2;
	int		len;

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv(1), (void **)&f);
	if (!f)
	{
		Com_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));
	
	// the file doesn't have a trailing 0, so we need to copy it off
	f2 = (char*)Z_Malloc(len+1);
	Com_Memcpy(f2, f, len);
	f2[len] = 0;

	Cbuf_InsertText (f2);

	Z_Free (f2);
	FS_FreeFile (f);
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
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Com_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Com_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (QStr::Length(s) >= MAX_ALIAS_NAME)
	{
		Com_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!QStr::Cmp(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = (cmdalias_t*)Z_Malloc (sizeof(cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;
	}
	QStr::Cpy(a->name, s);	

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		QStr::Cat(cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
			QStr::Cat(cmd, sizeof(cmd), " ");
	}
	QStr::Cat(cmd, sizeof(cmd), "\n");
	
	a->value = CopyString (cmd);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

bool Cmd_HandleNullCommand(const char* text)
{
    // forward to server command
	Cmd_ExecuteString(va("cmd %s", text));
    return true;
}

void Cmd_HandleUnknownCommand()
{
	// send it as a server command if we are connected
	Cmd_ForwardToServer ();
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void	Cmd_ExecuteString (const char *text, cmd_source_t src)
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
		if (!QStr::ICmp(cmd_argv[0],cmd->name))
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

	i = 0;
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next, i++)
		Com_Printf ("%s\n", cmd->name);
	Com_Printf ("%i commands\n", i);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
    cmd_macroExpand = true;
//
// register our commands
//
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

