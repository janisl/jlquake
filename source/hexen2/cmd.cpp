// cmd.c -- Quake script command processing module

/*
 * $Header: /H2 Mission Pack/CMD.C 3     3/01/98 8:20p Jmonroe $
 */

#include "quakedef.h"

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	Cmd_SharedInit();
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer);
}

bool Cmd_HandleNullCommand(const char* text)
{
    throw Exception("NULL command");
}

void Cmd_HandleUnknownCommand()
{
	Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	cls.message.WriteByte(h2clc_stringcmd);
	if (String::ICmp(Cmd_Argv(0), "cmd") != 0)
	{
		cls.message.Print(Cmd_Argv(0));
		cls.message.Print(" ");
	}
	if (Cmd_Argc() > 1)
		cls.message.Print(Cmd_ArgsUnmodified());
	else
		cls.message.Print("\n");
}
