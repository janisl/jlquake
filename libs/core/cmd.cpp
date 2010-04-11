//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

#define	MAX_CMD_LINE	1024

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

char* __CopyString(const char* in);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

cmd_source_t	cmd_source;
bool            cmd_macroExpand;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

cmdalias_t	*cmd_alias;

int		alias_count;		// for detecting runaway loops

int			cmd_wait;

QCmd		cmd_text;
byte		cmd_text_buf[MAX_CMD_BUFFER];

byte		defer_text_buf[MAX_CMD_BUFFER];

cmd_function_t	*cmd_functions;		// possible commands to execute

int			cmd_argc;
char		*cmd_argv[MAX_ARGS];		// points into cmd_tokenized
char		cmd_tokenized[8192+1024];	// will have 0 bytes inserted
char		cmd_cmd[8192]; // the original command we received (no token processing)
char*		cmd_args;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QCmd::Clear
//
//==========================================================================

void QCmd::Clear()
{
	cursize = 0;
}

//==========================================================================
//
//	QCmd::WriteData
//
//==========================================================================

void QCmd::WriteData(const void* Buffer, int Length)
{
	if (maxsize - cursize < Length)
	{
		throw QException("QCmd::WriteData: overflow");
	}

	Com_Memcpy(data + cursize, Buffer, Length);
	cursize += Length;
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
void Cbuf_AddText(const char *text)
{
	int		l;
	
	l = QStr::Length(text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		GLog.Write("Cbuf_AddText: overflow\n");
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
		GLog.Write("Cbuf_InsertText overflowed\n");
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
		throw QException("Cbuf_ExecuteText: bad exec_when");
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
		if (cmd_wait)
		{
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

		if (i >= (MAX_CMD_LINE - 1))
		{
			i = MAX_CMD_LINE - 1;
		}

		Com_Memcpy(line, text, i);
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
===============
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
void Cbuf_AddEarlyCommands(bool clear)
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
bool Cbuf_AddLateCommands()
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
		
	text = new char[s+1];
	text[0] = 0;
	for (i=1 ; i<argc ; i++)
	{
		QStr::Cat(text, s + 1,COM_Argv(i));
		if (i != argc-1)
			QStr::Cat(text, s + 1, " ");
	}
	
// pull out the commands
	build = new char[s+1];
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
	
	delete[] text;
	delete[] build;

	return ret;
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function )
{
	cmd_function_t	*cmd;
	
	// fail if the command already exists
	for (cmd = cmd_functions; cmd; cmd=cmd->next)
    {
		if (!QStr::Cmp(cmd_name, cmd->name))
        {
			// allow completion-only commands to be silently doubled
			if (function != NULL)
            {
                GLog.Write("Cmd_AddCommand: %s already defined\n", cmd_name);
			}
			return;
		}
	}

	cmd = new cmd_function_t;
	cmd->name = __CopyString(cmd_name);
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand(const char *cmd_name)
{
	cmd_function_t	*cmd, **back;

	back = &cmd_functions;
	while (1)
    {
		cmd = *back;
		if (!cmd)
        {
			// command wasn't active
			return;
		}
		if (!QStr::Cmp(cmd_name, cmd->name))
        {
			*back = cmd->next;
			if (cmd->name)
            {
				Mem_Free(cmd->name);
			}
			delete cmd;
			return;
		}
		back = &cmd->next;
	}
}

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc( void ) {
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= cmd_argc ) {
		return "";
	}
	return cmd_argv[arg];	
}

/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength ) {
	QStr::NCpyZ( buffer, Cmd_Argv( arg ), bufferLength );
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char* Cmd_ArgsUnmodified()
{
	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char	*Cmd_Args( void ) {
	static	char		cmd_args[MAX_STRING_CHARS];
	int		i;

	cmd_args[0] = 0;
	for ( i = 1 ; i < cmd_argc ; i++ ) {
		QStr::Cat( cmd_args, sizeof(cmd_args), cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			QStr::Cat( cmd_args, sizeof(cmd_args), " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char *Cmd_ArgsFrom( int arg ) {
	static	char		cmd_args[BIG_INFO_STRING];
	int		i;

	cmd_args[0] = 0;
	if (arg < 0)
		arg = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		QStr::Cat( cmd_args, sizeof(cmd_args), cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			QStr::Cat( cmd_args, sizeof(cmd_args), " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgsBuffer( char *buffer, int bufferLength ) {
	QStr::NCpyZ( buffer, Cmd_Args(), bufferLength );
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
============
*/
char *Cmd_Cmd()
{
	return cmd_cmd;
}

/*
======================
Cmd_MacroExpandString
======================
*/
const char* Cmd_MacroExpandString(const char *text)
{
	int		i, j, count, len;
	qboolean	inquote;
	const char	*scan;
	static	char	expanded[MAX_STRING_CHARS];
	char	temporary[MAX_STRING_CHARS];
	const char	*token;
	const char*	start;

	inquote = false;
	scan = text;

	len = QStr::Length(scan);
	if (len >= MAX_STRING_CHARS)
	{
        GLog.Write("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return NULL;
	}

	count = 0;

	for (i=0 ; i<len ; i++)
	{
		if (scan[i] == '"')
			inquote ^= 1;
		if (inquote)
			continue;	// don't expand inside quotes
		if (scan[i] != '$')
			continue;
		// scan out the complete macro
		start = scan+i+1;
		token = QStr::Parse2 (&start);
		if (!start)
			continue;
	
		token = Cvar_VariableString (token);

		j = QStr::Length(token);
		len += j;
		if (len >= MAX_STRING_CHARS)
		{
			GLog.Write("Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		QStr::NCpy(temporary, scan, i);
		QStr::Cpy(temporary+i, token);
		QStr::Cpy(temporary+i+j, start);

		QStr::Cpy(expanded, temporary);
		scan = expanded;
		i--;

		if (++count == 100)
		{
			GLog.Write("Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (inquote)
	{
		GLog.Write("Line has unmatched quote, discarded.\n");
		return NULL;
	}

	return scan;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
$Cvars will be expanded unless they are in a quoted token
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
void Cmd_TokenizeString(const char *text_in, bool MacroExpand )
{
	const char	*text;
	char	*textOut;

#ifdef TKN_DBG
    // FIXME TTimo blunt hook to try to find the tokenization of userinfo
    Com_DPrintf("Cmd_TokenizeString: %s\n", text_in);
#endif

	// clear previous args
	cmd_argc = 0;
	cmd_args = "";

	// macro expand the text
	if (MacroExpand)
    {
		text_in = Cmd_MacroExpandString(text_in);
    }
	if (!text_in)
    {
		return;
	}
	
	QStr::NCpyZ( cmd_cmd, text_in, sizeof(cmd_cmd) );

	text = text_in;
	textOut = cmd_tokenized;

	while (1)
    {
		if (cmd_argc == MAX_ARGS)
        {
			return;			// this is usually something malicious
		}

		while (1)
        {
			// skip whitespace
			while (*text && *text <= ' ')
            {
				text++;
			}
			if (!*text)
            {
				return;			// all tokens parsed
			}

			// skip // comments
			if (text[0] == '/' && text[1] == '/')
            {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] =='*')
            {
				while (*text && ( text[0] != '*' || text[1] != '/'))
                {
					text++;
				}
				if (!*text)
                {
					return;		// all tokens parsed
				}
				text += 2;
			}
            else
            {
				break;			// we are ready to parse a token
			}
		}

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			int		l;

			cmd_args = const_cast<char*>(text);

			// strip off any trailing whitespace
			l = QStr::Length(cmd_args) - 1;
			for ( ; l >= 0 ; l--)
				if (cmd_args[l] <= ' ')
					cmd_args[l] = 0;
				else
					break;
		}

		// handle quoted strings
        // NOTE TTimo this doesn't handle \" escaping
		if (*text == '"')
        {
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while (*text && *text != '"')
            {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if (!*text)
            {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		// skip until whitespace, quote, or comment
		while (*text > ' ')
        {
			if (text[0] == '"')
            {
				break;
			}

			if (text[0] == '/' && text[1] == '/')
            {
				break;
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] =='*')
            {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if (!*text)
        {
			return;		// all tokens parsed
		}
	}
}

/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_CompleteCommand (const char *partial)
{
	cmd_function_t	*cmd;
	int				len;
	cmdalias_t		*a;
	
	len = QStr::Length(partial);
	
	if (!len)
		return NULL;
		
// check for exact match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!QStr::Cmp(partial,cmd->name))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!QStr::Cmp(partial, a->name))
			return a->name;

// check for partial match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!QStr::NCmp(partial,cmd->name, len))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!QStr::NCmp(partial, a->name, len))
			return a->name;

	return NULL;
}

/*
============
Cmd_CommandCompletion
============
*/
void Cmd_CommandCompletion(void(*callback)(const char* s))
{
	for (cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next)
    {
		callback(cmd->name);
	}
	for (cmdalias_t* a = cmd_alias; a; a = a->next)
    {
		callback(a->name);
	}
}
