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
	CL_ForwardKnownCommandToServer();
	return true;
}

void Cmd_HandleUnknownCommand()
{
	common->Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
}
