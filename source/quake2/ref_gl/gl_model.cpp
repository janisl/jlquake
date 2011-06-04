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

model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[BSP38MAX_MAP_LEAFS/8];

/*
===============
Mod_PointInLeaf
===============
*/
mbrush38_leaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mbrush38_node_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->brush38_nodes)
		ri.Sys_Error (ERR_DROP, "Mod_PointInLeaf: bad model");

	node = model->brush38_nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mbrush38_leaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP38MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->brush38_vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->brush38_vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->brush38_vis + model->brush38_vis->bitofs[cluster][BSP38DVIS_PVS],
		model);
}


//===============================================================================

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
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));

	tr.numModels = 1;
	tr.models[0] = new model_t;
	Com_Memset(tr.models[0], 0, sizeof(model_t));
	tr.models[0]->type = MOD_BAD;
}

model_t* Mod_AllocModel()
{
	if (tr.numModels == MAX_MOD_KNOWN)
	{
		ri.Sys_Error(ERR_DROP, "tr.numModels == MAX_MOD_KNOWN");
	}
	model_t* mod = new model_t;
	Com_Memset(mod, 0, sizeof(model_t));
	tr.models[tr.numModels] = mod;
	mod->index = tr.numModels;
	tr.numModels++;
	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	unsigned *buf;
	int		i;
	
	if (!name[0])
		ri.Sys_Error (ERR_DROP, "Mod_ForName: NULL name");

	//
	// search the currently loaded models
	//
	for (int i = 1; i < tr.numModels; i++)
	{
		if (!QStr::Cmp(tr.models[i]->name, name) )
			return tr.models[i];
	}
	
	mod = Mod_AllocModel();

	QStr::Cpy(mod->name, name);
	
	//
	// load the file
	//
	int modfilelen = FS_ReadFile(mod->name, (void**)&buf);
	if (!buf)
	{
		if (crash)
			ri.Sys_Error (ERR_DROP, "Mod_NumForName: %s not found", mod->name);
		Com_Memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}
	
	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case IDMESH2HEADER:
		Mod_LoadMd2Model(mod, buf);
		break;
		
	case IDSPRITE2HEADER:
		Mod_LoadSprite2Model (mod, buf, modfilelen);
		break;
	
	case BSP38_HEADER:
		if (loadmodel != tr.models[1])
			ri.Sys_Error (ERR_DROP, "Loaded a brush model after the world");
		Mod_LoadBrush38Model(mod, buf);
		break;

	default:
		ri.Sys_Error (ERR_DROP,"Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	FS_FreeFile (buf);

	return mod;
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

	r_worldmodel = Mod_ForName(fullname, true);

	r_viewcluster = -1;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
qhandle_t R_RegisterModel (char *name)
{
	model_t	*mod;

	mod = Mod_ForName (name, false);
	if (!mod)
	{
		return 0;
	}

	return mod->index;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
}


//=============================================================================

model_t* Mod_GetModel(qhandle_t handle)
{
	if (handle < 1 || handle >= tr.numModels)
	{
		return tr.models[0];
	}
	return tr.models[handle];
}
