// cmd.c -- Quake script command processing module

#include "quakedef.h"

void Cmd_ForwardToServer (void);

QCvar* cl_warncmd;

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
		Con_Printf ("Cbuf_AddText: overflow\n");
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
	cmd_text.WriteData("\n", 1);
// add the copied off data
	if (templen)
	{
		cmd_text.WriteData(temp, templen);
		Z_Free (temp);
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
			Com_Memcpy(text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line);
		
		if (cmd_wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds_f (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;
		
// build the combined string to parse from
	s = 0;
	for (i=1 ; i<COM_Argc() ; i++)
	{
		s += QStr::Length(COM_Argv(i)) + 1;
	}
	if (!s)
		return;
		
	text = (char*)Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<COM_Argc() ; i++)
	{
		QStr::Cat(text, s + 1,COM_Argv(i));
		if (i != COM_Argc()-1)
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
	
	if (build[0])
		Cbuf_InsertText (build);
	
	Z_Free (text);
	Z_Free (build);
}


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	// FIXME: is this safe freeing the hunk here???
	mark = Hunk_LowMark ();
	f = (char *)COM_LoadHunkFile (Cmd_Argv(1));
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	if (!Cvar_Command () && (cl_warncmd->value || developer->value))
		Con_Printf ("execing %s\n",Cmd_Argv(1));
	
	Cbuf_InsertText (f);
	Hunk_FreeToLowMark (mark);
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
		Con_Printf ("%s ",Cmd_Argv(i));
	Con_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/

char *CopyString (char *in)
{
	char	*out;
	
	out = (char*)Z_Malloc (QStr::Length(in)+1);
	QStr::Cpy(out, in);
	return out;
}

void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (QStr::Length(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias allready exists, reuse it
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
		if (i != c)
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

#ifndef SERVERONLY		// FIXME
/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.Print(Cmd_Argv(0));
	if (Cmd_Argc() > 1)
	{
		cls.netchan.message.Print(" ");
		cls.netchan.message.Print(Cmd_Args());
	}
}

// don't forward the first argument
void Cmd_ForwardToServer_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	if (Cmd_Argc() > 1)
	{
		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.Print(Cmd_Args());
	}
}
#else
void Cmd_ForwardToServer (void)
{
}
#endif

bool Cmd_HandleNullCommand(const char* text)
{
	Cmd_ForwardToServer();
    return true;
}

void Cmd_HandleUnknownCommand()
{
    if (cl_warncmd->value || developer->value)
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
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
				cmd->function ();
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
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/
int Cmd_CheckParm (char *parm)
{
	int i;
	
	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! QStr::ICmp(parm, Cmd_Argv (i)))
			return i;
			
	return 0;
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
#ifndef SERVERONLY
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
#endif

	cl_warncmd = Cvar_Get("cl_warncmd", "0", 0);
}

