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

#define MAX_CMD_LINE		1024
#define MAX_ARGS			1024
#define MAX_CMD_BUFFER		16384
#define MAX_ALIAS_NAME		32
#define ALIAS_LOOP_COUNT	16

// TYPES -------------------------------------------------------------------

struct QCmd
{
	byte*				data;
	int					maxsize;
	int					cursize;
};

struct cmdalias_t
{
	cmdalias_t*			next;
	char				name[MAX_ALIAS_NAME];
	char*				value;
};

struct cmd_function_t
{
	cmd_function_t*		next;
	char*				name;
	xcommand_t			function;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

//	Game specific.
bool Cmd_HandleNullCommand(const char* text);
void Cmd_HandleUnknownCommand();

char* __CopyString(const char* in);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

cmd_source_t			cmd_source;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static cmdalias_t*		cmd_alias;

static int				alias_count;		// for detecting runaway loops

static int				cmd_wait;

static QCmd				cmd_text;
static byte				cmd_text_buf[MAX_CMD_BUFFER];

static byte				defer_text_buf[MAX_CMD_BUFFER];

static cmd_function_t*	cmd_functions;		// possible commands to execute

static int				cmd_argc;
static char*			cmd_argv[MAX_ARGS];			// points into cmd_tokenized
static char				cmd_tokenized[8192+1024];	// will have 0 bytes inserted
static char				cmd_cmd[8192]; // the original command we received (no token processing)
static char				cmd_args[8192];

// CODE --------------------------------------------------------------------

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

//==========================================================================
//
//	Cbuf_Init
//
//==========================================================================

void Cbuf_Init()
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

//==========================================================================
//
//	Cbuf_AddText
//
//	Adds command text at the end of the buffer, does NOT add a final \n
//
//==========================================================================

void Cbuf_AddText(const char* Text)
{
	int L = QStr::Length(Text);

	if (cmd_text.cursize + L >= cmd_text.maxsize)
	{
		GLog.Write("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], Text, L);
	cmd_text.cursize += L;
}

//==========================================================================
//
//	Cbuf_InsertText
//
//	Adds command text immediately after the current command
//	Adds a \n to the text
//
//==========================================================================

void Cbuf_InsertText(const char* Text)
{
	int Len = QStr::Length(Text) + 1;
	if (Len + cmd_text.cursize > cmd_text.maxsize)
	{
		GLog.Write("Cbuf_InsertText overflowed\n");
		return;
	}

	// move the existing command text
	for (int i = cmd_text.cursize - 1; i >= 0; i--)
	{
		cmd_text.data[i + Len] = cmd_text.data[i];
	}

	// copy the new text in
	Com_Memcpy(cmd_text.data, Text, Len - 1);

	// add a \n
	cmd_text.data[Len - 1] = '\n';

	cmd_text.cursize += Len;
}

//==========================================================================
//
//	Cbuf_ExecuteText
//
//==========================================================================

void Cbuf_ExecuteText(int ExecWhen, const char* Text)
{
	switch (ExecWhen)
	{
	case EXEC_NOW:
		if (Text && QStr::Length(Text) > 0)
		{
			Cmd_ExecuteString(Text);
		}
		else
		{
			Cbuf_Execute();
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText(Text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText(Text);
		break;
	default:
		throw QException("Cbuf_ExecuteText: bad exec_when");
	}
}

//==========================================================================
//
//	Cbuf_Execute
//
//==========================================================================

void Cbuf_Execute()
{
	char	Line[MAX_CMD_LINE];

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
		char* text = (char*)cmd_text.data;

		int quotes = 0;
		int i;
		for (i = 0; i < cmd_text.cursize; i++)
		{
			if (text[i] == '"')
			{
				quotes++;
			}
			if (!(quotes & 1) &&  text[i] == ';')
			{
				break;	// don't break if inside a quoted string
			}
			if (text[i] == '\n' || text[i] == '\r')
			{
				break;
			}
		}

		if (i >= (MAX_CMD_LINE - 1))
		{
			i = MAX_CMD_LINE - 1;
		}

		Com_Memcpy(Line, text, i);
		Line[i] = 0;

		//	Delete the text from the command buffer and move remaining commands
		// down. This is necessary because commands (exec) can insert data at
		// the beginning of the text buffer.

		if (i == cmd_text.cursize)
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove(text, text + i, cmd_text.cursize);
		}

		// execute the command line
		Cmd_ExecuteString(Line);
	}
}

//==========================================================================
//
//	Cbuf_CopyToDefer
//
//==========================================================================

void Cbuf_CopyToDefer()
{
	Com_Memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
}

//==========================================================================
//
//	Cbuf_InsertFromDefer
//
//==========================================================================

void Cbuf_InsertFromDefer()
{
	Cbuf_InsertText((char*)defer_text_buf);
	defer_text_buf[0] = 0;
}

//==========================================================================
//
//	Cbuf_AddEarlyCommands
//
//	Adds command line parameters as script statements
//	Commands lead with a +, and continue until another +
//
//	Set commands are added early, so they are guaranteed to be set before
// the client and server initialize for the first time.
//
//	Other commands are added late, after all initialization is complete.
//
//==========================================================================

void Cbuf_AddEarlyCommands(bool Clear)
{
	for (int i = 0; i < COM_Argc(); i++)
	{
		const char* s = COM_Argv(i);
		if (QStr::Cmp(s, "+set"))
		{
			continue;
		}
		Cbuf_AddText(va("set %s %s\n", COM_Argv(i + 1), COM_Argv(i + 2)));
		if (Clear)
		{
			COM_ClearArgv(i);
			COM_ClearArgv(i + 1);
			COM_ClearArgv(i + 2);
		}
		i += 2;
	}
}

//==========================================================================
//
//	Cbuf_AddLateCommands
//
//	Adds command line parameters as script statements. Commands lead with
// a + and continue until another + or -
//	quake +vid_ref gl +map amlev1
//
//	Returns true if any late commands were added, which will keep the
// demoloop from immediately starting
//
//==========================================================================

bool Cbuf_AddLateCommands(bool Insert)
{
	// build the combined string to parse from
	int s = 0;
	int argc = COM_Argc();
	for (int i = 1; i < argc; i++)
	{
		s += QStr::Length(COM_Argv(i)) + 1;
	}
	if (!s)
	{
		return false;
	}

	char* Text = new char[s + 1];
	Text[0] = 0;
	for (int i = 1; i < argc; i++)
	{
		QStr::Cat(Text, s + 1, COM_Argv(i));
		if (i != argc - 1)
		{
			QStr::Cat(Text, s + 1, " ");
		}
	}

	// pull out the commands
	char* Build = new char[s + 1];
	Build[0] = 0;

	for (int i = 0; i < s - 1; i++)
	{
		if (Text[i] == '+')
		{
			i++;

			int j;
			for (j = i; (Text[j] != '+') && (Text[j] != '-') && (Text[j] != 0); j++)
				;

			char c = Text[j];
			Text[j] = 0;

			QStr::Cat(Build, s + 1, Text + i);
			QStr::Cat(Build, s + 1, "\n");
			Text[j] = c;
			i = j - 1;
		}
	}

	bool Ret = (Build[0] != 0);
	if (Ret)
	{
		if (Insert)
		{
			Cbuf_InsertText(Build);
		}
		else
		{
			Cbuf_AddText(Build);
		}
	}
	
	delete[] Text;
	delete[] Build;

	return Ret;
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

//==========================================================================
//
//	Cmd_Wait_f
//
//	Causes execution of the remainder of the command buffer to be delayed
// until next frame.  This allows commands like:
//	bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
//
//==========================================================================

static void Cmd_Wait_f()
{
	if (Cmd_Argc() == 2)
	{
		cmd_wait = QStr::Atoi(Cmd_Argv(1));
	}
	else
	{
		cmd_wait = 1;
	}
}

//==========================================================================
//
//	Cmd_StuffCmds_f
//
//	Adds command line parameters as script statements. Commands lead with
// a +, and continue until a - or another +
//	quake +prog jctest.qp +cmd amlev1
//	quake -nosound +cmd amlev1
//
//==========================================================================

static void Cmd_StuffCmds_f()
{
	Cbuf_AddLateCommands(true);
}

//==========================================================================
//
//	Cmd_Echo_f
//
//	Just prints the rest of the line to the console
//
//==========================================================================

static void Cmd_Echo_f()
{
	for (int i = 1; i < Cmd_Argc(); i++)
	{
		GLog.Write("%s ",Cmd_Argv(i));
	}
	GLog.Write("\n");
}

//==========================================================================
//
//	Cmd_Alias_f
//
//	Creates a new command that executes a command string (possibly ; seperated)
//
//==========================================================================

static void Cmd_Alias_f()
{
	cmdalias_t	*a;
	char		cmd[1024];

	if (Cmd_Argc() == 1)
	{
		GLog.Write("Current alias commands:\n");
		for (a = cmd_alias; a; a = a->next)
		{
			GLog.Write("%s : %s\n", a->name, a->value);
		}
		return;
	}

	const char* s = Cmd_Argv(1);
	if (QStr::Length(s) >= MAX_ALIAS_NAME)
	{
		GLog.Write("Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a = cmd_alias; a; a = a->next)
	{
		if (!QStr::Cmp(s, a->name))
		{
			Mem_Free(a->value);
			break;
		}
	}

	if (!a)
	{
		a = new cmdalias_t;
		a->next = cmd_alias;
		cmd_alias = a;
	}
	QStr::Cpy(a->name, s);

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	int c = Cmd_Argc();
	for (int i = 2; i < c; i++)
	{
		QStr::Cat(cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
		{
			QStr::Cat(cmd, sizeof(cmd), " ");
		}
	}
	QStr::Cat(cmd, sizeof(cmd), "\n");

	a->value = __CopyString(cmd);
}

//==========================================================================
//
//	Cmd_Vstr_f
//
//	Inserts the current value of a variable as command text
//
//==========================================================================

static void Cmd_Vstr_f()
{
	if (Cmd_Argc () != 2)
	{
		GLog.Write("vstr <variablename> : execute a variable command\n");
		return;
	}

	const char* v = Cvar_VariableString(Cmd_Argv(1));
	Cbuf_InsertText(va("%s\n", v));
}

//==========================================================================
//
//	Cmd_Exec_f
//
//==========================================================================

static void Cmd_Exec_f()
{
	if (Cmd_Argc() != 2)
	{
		GLog.Write("exec <filename> : execute a script file\n");
		return;
	}

	char filename[MAX_QPATH];
	QStr::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	QStr::DefaultExtension(filename, sizeof(filename), ".cfg");

	QArray<byte> Buffer;
	FS_ReadFile(filename, Buffer);
	if (!Buffer.Num())
	{
		GLog.Write("couldn't exec %s\n", Cmd_Argv(1));
		return;
	}
	GLog.Write("execing %s\n", Cmd_Argv(1));

	Cbuf_InsertText((char*)Buffer.Ptr());
}

//==========================================================================
//
//	Cmd_List_f
//
//==========================================================================

static void Cmd_List_f()
{
	char			*match;

	if (Cmd_Argc() > 1)
	{
		match = Cmd_Argv(1);
	}
	else
	{
		match = NULL;
	}

	int i = 0;
	for (cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (match && !QStr::Filter(match, cmd->name, false))
		{
			continue;
		}

		GLog.Write("%s\n", cmd->name);
		i++;
	}
	GLog.Write("%i commands\n", i);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

//==========================================================================
//
//	Cmd_Init
//
//==========================================================================

void Cmd_SharedInit()
{
	//
	// register our commands
	//
	Cmd_AddCommand("wait", Cmd_Wait_f);
	Cmd_AddCommand("echo", Cmd_Echo_f);
	Cmd_AddCommand("stuffcmds", Cmd_StuffCmds_f);
	Cmd_AddCommand("alias", Cmd_Alias_f);
	Cmd_AddCommand("exec",Cmd_Exec_f);
	Cmd_AddCommand("vstr", Cmd_Vstr_f);
	Cmd_AddCommand("cmdlist", Cmd_List_f);
}

//==========================================================================
//
//	Cmd_AddCommand
//
//==========================================================================

void Cmd_AddCommand(const char* CmdName, xcommand_t Function)
{
	cmd_function_t*		cmd;

	//	Fail if the command already exists.
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!QStr::Cmp(CmdName, cmd->name))
		{
			//	Allow completion-only commands to be silently doubled.
			if (Function != NULL)
			{
				GLog.Write("Cmd_AddCommand: %s already defined\n", CmdName);
			}
			return;
		}
	}

	cmd = new cmd_function_t;
	cmd->name = __CopyString(CmdName);
	cmd->function = Function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

//==========================================================================
//
//	Cmd_RemoveCommand
//
//==========================================================================

void Cmd_RemoveCommand(const char* CmdName)
{
	cmd_function_t** back = &cmd_functions;
	while (1)
	{
		cmd_function_t* cmd = *back;
		if (!cmd)
		{
			// command wasn't active
			return;
		}
		if (!QStr::Cmp(CmdName, cmd->name))
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

//==========================================================================
//
//	Cmd_Argc
//
//==========================================================================

int Cmd_Argc()
{
	return cmd_argc;
}

//==========================================================================
//
//	Cmd_Argv
//
//==========================================================================

char* Cmd_Argv(int Arg)
{
	if ((unsigned)Arg >= cmd_argc)
	{
		return "";
	}
	return cmd_argv[Arg];
}

//==========================================================================
//
//	Cmd_ArgvBuffer
//
//	The interpreted versions use this because they can't have pointers
// returned to them
//
//==========================================================================

void Cmd_ArgvBuffer(int Arg, char* Buffer, int BufferLength)
{
	QStr::NCpyZ(Buffer, Cmd_Argv(Arg), BufferLength);
}

//==========================================================================
//
//	Cmd_ArgsUnmodified
//
//	Returns a single string containing argv(1) to argv(argc()-1)
//
//==========================================================================

char* Cmd_ArgsUnmodified()
{
	return cmd_args;
}

//==========================================================================
//
//	Cmd_Args
//
//	Returns a single string containing argv(1) to argv(argc()-1)
//
//==========================================================================

char* Cmd_Args()
{
	static	char	cmd_args[MAX_STRING_CHARS];

	cmd_args[0] = 0;
	for (int i = 1; i < cmd_argc; i++)
	{
		QStr::Cat(cmd_args, sizeof(cmd_args), cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			QStr::Cat(cmd_args, sizeof(cmd_args), " ");
		}
	}

	return cmd_args;
}

//==========================================================================
//
//	Cmd_ArgsFrom
//
//	Returns a single string containing argv(arg) to argv(argc()-1)
//
//==========================================================================

char* Cmd_ArgsFrom(int Arg)
{
	static char		cmd_args[BIG_INFO_STRING];

	cmd_args[0] = 0;
	if (Arg < 0)
	{
		Arg = 0;
	}
	for (int i = Arg; i < cmd_argc; i++)
	{
		QStr::Cat(cmd_args, sizeof(cmd_args), cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			QStr::Cat(cmd_args, sizeof(cmd_args), " ");
		}
	}

	return cmd_args;
}

//==========================================================================
//
//	Cmd_ArgsBuffer
//
//	The interpreted versions use this because they can't have pointers
// returned to them
//
//==========================================================================

void Cmd_ArgsBuffer(char* Buffer, int BufferLength)
{
	QStr::NCpyZ(Buffer, Cmd_Args(), BufferLength);
}

//==========================================================================
//
//	Cmd_Cmd
//
//	Retrieve the unmodified command string. For rcon use when you want to
// transmit without altering quoting.
//
//==========================================================================

char* Cmd_Cmd()
{
	return cmd_cmd;
}

//==========================================================================
//
//	Cmd_MacroExpandString
//
//==========================================================================

static const char* Cmd_MacroExpandString(const char* Text)
{
	static char		Expanded[MAX_STRING_CHARS];

	bool InQuote = false;
	const char* Scan = Text;

	int Len = QStr::Length(Scan);
	if (Len >= MAX_STRING_CHARS)
	{
		GLog.Write("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return NULL;
	}

	int Count = 0;

	for (int i = 0; i < Len; i++)
	{
		if (Scan[i] == '"')
		{
			InQuote ^= 1;
		}
		if (InQuote)
		{
			continue;	// don't expand inside quotes
		}
		if (Scan[i] != '$')
		{
			continue;
		}
		// scan out the complete macro
		const char* Start = Scan + i + 1;
		const char* Token = QStr::Parse2(&Start);
		if (!Start)
		{
			continue;
		}

		Token = Cvar_VariableString(Token);

		int j = QStr::Length(Token);
		Len += j;
		if (Len >= MAX_STRING_CHARS)
		{
			GLog.Write("Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		char Temporary[MAX_STRING_CHARS];
		QStr::NCpy(Temporary, Scan, i);
		QStr::Cpy(Temporary + i, Token);
		QStr::Cpy(Temporary + i + j, Start);

		QStr::Cpy(Expanded, Temporary);
		Scan = Expanded;
		i--;

		if (++Count == 100)
		{
			GLog.Write("Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (InQuote)
	{
		GLog.Write("Line has unmatched quote, discarded.\n");
		return NULL;
	}

	return Scan;
}

//==========================================================================
//
//	Cmd_TokenizeString
//
//	Parses the given string into command line tokens. The text is copied to
// a seperate buffer and 0 characters are inserted in the apropriate place.
// The argv array will point into this temporary buffer. $Cvars will be
// expanded unless they are in a quoted token
//
//==========================================================================

void Cmd_TokenizeString(const char* TextIn, bool MacroExpand)
{
	// clear previous args
	cmd_argc = 0;
	cmd_args[0] = 0;

	// macro expand the text
	if (MacroExpand)
	{
		TextIn = Cmd_MacroExpandString(TextIn);
	}
	if (!TextIn)
	{
		return;
	}

	QStr::NCpyZ(cmd_cmd, TextIn, sizeof(cmd_cmd));

	const char* Text = TextIn;
	char* TextOut = cmd_tokenized;

	while (1)
	{
		if (cmd_argc == MAX_ARGS)
		{
			return;			// this is usually something malicious
		}

		while (1)
		{
			// skip whitespace
			while (*Text && *Text <= ' ')
			{
				Text++;
			}
			if (!*Text)
			{
				return;			// all tokens parsed
			}

			// skip // comments
			if (Text[0] == '/' && Text[1] == '/')
			{
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if (Text[0] == '/' && Text[1] =='*')
			{
				while (*Text && (Text[0] != '*' || Text[1] != '/'))
				{
					Text++;
				}
				if (!*Text)
				{
					return;		// all tokens parsed
				}
				Text += 2;
			}
			else
			{
				break;			// we are ready to parse a token
			}
		}

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			QStr::Cpy(cmd_args, Text);

			// strip off any trailing whitespace
			int l = QStr::Length(cmd_args) - 1;
			for (; l >= 0; l--)
			{
				if (cmd_args[l] <= ' ')
				{
					cmd_args[l] = 0;
				}
				else
				{
					break;
				}
			}
		}

		// handle quoted strings
		// NOTE TTimo this doesn't handle \" escaping
		if (*Text == '"')
		{
			cmd_argv[cmd_argc] = TextOut;
			cmd_argc++;
			Text++;
			while (*Text && *Text != '"')
			{
				*TextOut++ = *Text++;
			}
			*TextOut++ = 0;
			if (!*Text)
			{
				return;		// all tokens parsed
			}
			Text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = TextOut;
		cmd_argc++;

		// skip until whitespace, quote, or comment
		while (*Text > ' ')
		{
			if (Text[0] == '"')
			{
				break;
			}

			if (Text[0] == '/' && Text[1] == '/')
			{
				break;
			}

			// skip /* */ comments
			if (Text[0] == '/' && Text[1] =='*')
			{
				break;
			}

			*TextOut++ = *Text++;
		}

		*TextOut++ = 0;

		if (!*Text)
		{
			return;		// all tokens parsed
		}
	}
}

//==========================================================================
//
//	Cmd_CompleteCommand
//
//==========================================================================

char* Cmd_CompleteCommand(const char* Partial)
{
	int Len = QStr::Length(Partial);

	if (!Len)
	{
		return NULL;
	}

	// check for exact match
	for (cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!QStr::Cmp(Partial, cmd->name))
		{
			return cmd->name;
		}
	}
	for (cmdalias_t* a = cmd_alias; a; a = a->next)
	{
		if (!QStr::Cmp(Partial, a->name))
		{
			return a->name;
		}
	}

	// check for partial match
	for (cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!QStr::NCmp(Partial, cmd->name, Len))
		{
			return cmd->name;
		}
	}
	for (cmdalias_t* a = cmd_alias; a; a = a->next)
	{
		if (!QStr::NCmp(Partial, a->name, Len))
		{
			return a->name;
		}
	}

	return NULL;
}

//==========================================================================
//
//	Cmd_CommandCompletion
//
//==========================================================================

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

//==========================================================================
//
//	Cmd_ExecuteString
//
//	A complete command line has been parsed, so try to execute it
//
//==========================================================================

void Cmd_ExecuteString(const char* Text, cmd_source_t Src)
{
	// execute the command line
	cmd_source = Src;
	Cmd_TokenizeString(Text, !!(GGameType & GAME_Quake2));
	if (!Cmd_Argc())
	{
		return;		// no tokens
	}

	// check registered command functions
	cmd_function_t* cmd;
	for (cmd_function_t** prev = &cmd_functions; *prev; prev = &cmd->next)
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
				if (!Cmd_HandleNullCommand(Text))
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
	for (cmdalias_t* a = cmd_alias; a; a = a->next)
	{
		if (!QStr::ICmp(cmd_argv[0], a->name))
		{
			if (++alias_count == ALIAS_LOOP_COUNT)
			{
				GLog.Write("ALIAS_LOOP_COUNT\n");
				return;
			}
			Cbuf_InsertText(a->value);
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
