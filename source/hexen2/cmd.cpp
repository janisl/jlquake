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
void Cmd_Init(void)
{
	Cmd_SharedInit();
}

bool Cmd_HandleNullCommand(const char* text)
{
	common->FatalError("NULL command");
	return false;
}

void Cmd_HandleUnknownCommand()
{
	common->Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer(void)
{
#ifndef DEDICATED
	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (clc.demoplaying)
	{
		return;		// not really connected

	}
	CL_AddReliableCommand(Cmd_Cmd());
#endif
}
