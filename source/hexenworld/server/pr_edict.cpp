// sv_edict.c -- entity dictionary

#include "qwsvdef.h"

qboolean ignore_precache = false;


/*
===============
PR_Init
===============
*/
void PR_Init(void)
{
	PR_InitBuiltins();
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
	Cmd_AddCommand("edictcount", ED_Count);
	Cmd_AddCommand("profile", PR_Profile_f);
	max_temp_edicts = Cvar_Get("max_temp_edicts", "30", CVAR_ARCHIVE);
}
