/*
Copyright (C) 1996-1997 Id Software, Inc.

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

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

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
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	return mod->q1_cache;
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	R_FreeModels();
	Mod_Init();
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qhandle_t Mod_LoadWorld(const char *name)
{
	int		i;
	model_t	*mod;
	
	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
	mod = R_AllocModel();
	QStr::Cpy(mod->name, name);

	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		return 0;
	}
	
//
// allocate a new model
//
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	Mod_LoadBrush29Model (mod, buf);

	tr.worldModel = mod;
	return mod->index;
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	Con_Printf ("Cached models:\n");
	for (int i = 0; i < tr.numModels; i++)
	{
		Con_Printf("%8p : %s\n", tr.models[i]->q1_cache, tr.models[i]->name);
	}
}

int Mod_GetNumFrames(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_flags;
}

void Mod_PrintFrameName (qhandle_t m, int frame)
{
	mesh1hdr_t 			*hdr;
	mmesh1framedesc_t	*pframedesc;

	hdr = (mesh1hdr_t *)Mod_Extradata (R_GetModelByHandle(m));
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];
	
	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->name;
}

int Mod_GetSyncType(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_synctype;
}
