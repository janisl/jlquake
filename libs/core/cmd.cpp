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
//	QCmd::Init
//
//==========================================================================

void QCmd::Init(byte* NewData, int Length)
{
	data = NewData;
	maxsize = Length;
	cursize = 0;
}

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
		callback(cmd->name);
	}
}
