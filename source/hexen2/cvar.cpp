// cvar.c -- dynamic variable tracking

#include "quakedef.h"

void Cvar_Changed(Cvar* var)
{
	if ((var->flags & CVAR_SERVERINFO))
	{
		if (sv.active)
		{
			SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var->name, var->string);
		}
	}
}
