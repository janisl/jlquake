
// cl_null.c -- this file can stub out the entire client system
// for pure dedicated servers

#include "../qcommon/qcommon.h"

void Key_Bind_Null_f(void)
{
}

void CL_Init (void)
{
}

void CL_Drop (void)
{
}

void CL_Shutdown (void)
{
}

void CL_Frame (int msec)
{
}

void Con_Print (const char *text)
{
}

void Cmd_ForwardToServer (void)
{
	char *cmd;

	cmd = Cmd_Argv(0);
	Com_Printf ("Unknown command \"%s\"\n", cmd);
}

void SCR_DebugGraph (float value, int color)
{
}

void SCR_BeginLoadingPlaque (bool)
{
}

void SCR_EndLoadingPlaque (void)
{
}

void Key_Init (void)
{
	Cmd_AddCommand ("bind", Key_Bind_Null_f);
}

void S_ClearSoundBuffer()
{
}
