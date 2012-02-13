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
//**	Command text buffering and command execution
//**
//**************************************************************************

//==========================================================================
//
//	Any number of commands can be added in a frame, from several different
// sources. Most commands come from either keybindings or console line input,
// but entire text files can be execed.
//
//==========================================================================

// paramters for command buffer stuffing
enum
{
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
};

void Cbuf_Init();
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText(const char* Text);
// Adds command text at the end of the buffer, does NOT add a final \n

void Cbuf_InsertText(const char* Text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void Cbuf_ExecuteText(int ExecWhen, const char* Text);
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute();
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function, or current args will be destroyed.

void Cbuf_CopyToDefer();
void Cbuf_InsertFromDefer();
// These two functions are used to defer any pending commands while a map
// is being loaded

void Cbuf_AddEarlyCommands(bool clear);
// adds all the +set commands from the command line

bool Cbuf_AddLateCommands(bool Insert = false);
// adds all the remaining + commands from the command line
// Returns true if any late commands were added, which
// will keep the demoloop from immediately starting

//==========================================================================
//
//	Command execution takes a null terminated string, breaks it into tokens,
// then searches for a command or variable that matches the first token.
//
//==========================================================================

typedef void (*xcommand_t) (void);

enum cmd_source_t
{
	src_command,	// from the command buffer
	src_client,		// came in over a net connection as a clc_stringcmd
					// host_client will be valid during this state.
};

void Cmd_AddCommand(const char* CmdName, xcommand_t Function);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_clientCommand instead of executed locally

void Cmd_RemoveCommand(const char* CmdName);

int Cmd_Argc();
char* Cmd_Argv(int Arg);
void Cmd_ArgvBuffer(int Arg, char* Buffer, int BufferLength);
char* Cmd_ArgsUnmodified();
char* Cmd_Args();
char* Cmd_ArgsFrom(int Arg);
void Cmd_ArgsBuffer(char* Buffer, int BufferLength);
char* Cmd_Cmd();
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

void Cmd_TokenizeString(const char* Text, bool MacroExpand = false);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

char* Cmd_CompleteCommand(const char* partial);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

void Cmd_CommandCompletion(void(*callback)(const char* s));
// callback with each valid string

void Cmd_ExecuteString(const char* text, cmd_source_t src = src_command);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console

void Cmd_SharedInit();

extern	cmd_source_t	cmd_source;

//==========================================================================
//
//	Edit fields and command line history/completion
//
//==========================================================================

#define	MAX_EDIT_LINE	256

struct field_t
{
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
};

void Field_Clear(field_t* Edit);
void Field_CompleteCommand(field_t* Edit);
