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
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	R_ModelInit();
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
static void Mod_LoadWorld(const char* name)
{
	unsigned *buf;
	
	model_t* mod = R_AllocModel();

	QStr::Cpy(mod->name, name);
	
	//
	// load the file
	//
	int modfilelen = FS_ReadFile(mod->name, (void**)&buf);
	if (!buf)
	{
		ri.Sys_Error (ERR_DROP, "Mod_NumForName: %s not found", mod->name);
	}
	
	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case BSP38_HEADER:
		Mod_LoadBrush38Model(mod, buf);
		break;

	default:
		ri.Sys_Error (ERR_DROP,"Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	FS_FreeFile (buf);

	tr.worldModel = mod;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (char *model)
{
	char	fullname[MAX_QPATH];

	r_oldviewcluster = -1;		// force markleafs

	QStr::Sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	R_FreeModels();
	Mod_Init();

	Mod_LoadWorld(fullname);

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
