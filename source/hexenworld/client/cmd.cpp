// cmd.c -- Quake script command processing module

#include "quakedef.h"

Cvar* cl_warncmd;

/*
=============================================================================

                    COMMAND EXECUTION

=============================================================================
*/

bool Cmd_HandleNullCommand(const char* text)
{
	CL_ForwardKnownCommandToServer();
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

	cl_warncmd = Cvar_Get("cl_warncmd", "0", 0);
}
