// sv_user.c -- server code for moving users

/*
 * $Header: /H2 Mission Pack/SV_USER.C 6     3/13/98 1:51p Mgummelt $
 */

#include "quakedef.h"
#include "../client/public.h"

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s);

	for (ucmd_t* u = h2_ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			return;
		}
	}

	common->DPrintf("%s tried to %s\n", cl->name, s);
}
