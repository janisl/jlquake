// cmd.c -- Quake script command processing module

#include "quakedef.h"

Cvar* cl_warncmd;

/*
=============================================================================

                    COMMAND EXECUTION

=============================================================================
*/

#ifndef SERVERONLY		// FIXME
/*
===================
Cmd_ForwardToServer

adds the current command line as a h2clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer(void)
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (clc.demoplaying)
	{
		return;		// not really connected

	}
	clc.netchan.message.WriteByte(h2clc_stringcmd);
	clc.netchan.message.Print(Cmd_Argv(0));
	if (Cmd_Argc() > 1)
	{
		clc.netchan.message.Print(" ");
		clc.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}

// don't forward the first argument
void Cmd_ForwardToServer_f(void)
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (clc.demoplaying)
	{
		return;		// not really connected

	}
	if (Cmd_Argc() > 1)
	{
		clc.netchan.message.WriteByte(h2clc_stringcmd);
		clc.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}
#else
void Cmd_ForwardToServer(void)
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
	if (cl_warncmd->value || com_developer->value)
	{
		common->Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
	}
}

/*
============
Cmd_Init
============
*/
void Cmd_Init(void)
{
	Cmd_SharedInit();
#ifndef SERVERONLY
	Cmd_AddCommand("cmd", Cmd_ForwardToServer_f);
#endif

	cl_warncmd = Cvar_Get("cl_warncmd", "0", 0);
}
