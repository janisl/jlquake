// cmd.c -- Quake script command processing module

#include "quakedef.h"

QCvar* cl_warncmd;

char *CopyString (char *in)
{
	char	*out;
	
	out = (char*)Z_Malloc (QStr::Length(in)+1);
	QStr::Cpy(out, in);
	return out;
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
Cmd_Init
============
*/
void Cmd_Init (void)
{
	Cmd_SharedInit();
#ifndef SERVERONLY
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
#endif

	cl_warncmd = Cvar_Get("cl_warncmd", "0", 0);
}

