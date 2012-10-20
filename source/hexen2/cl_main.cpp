// cl_main.c  -- client main loop

/*
 * $Header: /H2 Mission Pack/CL_MAIN.C 12    3/16/98 5:32p Jweier $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include "../server/public.h"
#include "../client/game/quake_hexen2/demo.h"
#include "../client/game/quake_hexen2/connection.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

static float save_sensitivity;

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f(void)
{
	h2entity_t* ent;
	int i;

	for (i = 0,ent = h2cl_entities; i < cl.qh_num_entities; i++,ent++)
	{
		common->Printf("%3i:",i);
		if (!ent->state.modelindex)
		{
			common->Printf("EMPTY\n");
			continue;
		}
		common->Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
			R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

void CL_Sensitivity_save_f(void)
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = cl_sensitivity->value;
	}
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue("sensitivity", save_sensitivity);
	}
}
/*
=================
CL_Init
=================
*/
void CL_Init(void)
{
	CL_SharedInit();


//
// register our commands
//
	Cmd_AddCommand("entities", CL_PrintEntities_f);
	Cmd_AddCommand("sensitivity_save", CL_Sensitivity_save_f);
}
