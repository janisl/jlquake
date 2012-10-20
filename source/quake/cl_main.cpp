/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_main.c  -- client main loop

#include "quakedef.h"
#include "../server/public.h"
#include "../client/game/quake_hexen2/demo.h"
#include "../client/game/quake_hexen2/connection.h"

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f(void)
{
	q1entity_t* ent;
	int i;

	for (i = 0,ent = clq1_entities; i < cl.qh_num_entities; i++,ent++)
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
}

float* CL_GetSimOrg()
{
	return clq1_entities[cl.viewentity].state.origin;
}
