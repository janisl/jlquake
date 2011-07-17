/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// models.c -- model loading and caching

#include "gl_local.h"

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int total = 0;
	ri.Con_Printf (PRINT_ALL,"Loaded models:\n");
	for (int i = 0; i < tr.numModels; i++)
	{
		ri.Con_Printf(PRINT_ALL, "%8i : %s\n", tr.models[i]->q2_extradatasize, tr.models[i]->name);
		total += tr.models[i]->q2_extradatasize;
	}
	ri.Con_Printf (PRINT_ALL, "Total resident: %i\n", total);
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (const char *model)
{
	char	fullname[MAX_QPATH];

	r_oldviewcluster = -1;		// force markleafs

	QStr::Sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	R_FreeModels();
	R_ModelInit();

	R_LoadWorld(fullname);

	r_viewcluster = -1;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
}
