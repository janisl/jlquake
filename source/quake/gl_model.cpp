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

static char	loadname[32];	// for hunk tags

static model_t *Mod_LoadModel (model_t *mod, qboolean crash);

static byte	mod_novis[BSP29_MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
static model_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));

	//	Reserve 0 for default model.
	mod_numknown = 1;
	mod_known[0].type = MOD_BAD;
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
===============
Mod_PointInLeaf
===============
*/
mbrush29_leaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mbrush29_node_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->brush29_nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->brush29_nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mbrush29_leaf_t *)node;
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
static byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP29_MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->brush29_numleafs+7)>>3;	
	out = decompressed;

#if 0
	Com_Memcpy(out, in, row);
#else
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
#endif
	
	return decompressed;
}

byte *Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model)
{
	if (leaf == model->brush29_leafs)
		return mod_novis;
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	model_t	*mod;
	
	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
		if (mod->type != MOD_MESH1)
			mod->q1_needload = true;
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (const char *name)
{
	int		i;
	model_t	*mod;
	
	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
		if (!QStr::Cmp(mod->name, name) )
			break;
			
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			Sys_Error ("mod_numknown == MAX_MOD_KNOWN");
		QStr::Cpy(mod->name, name);
		mod->q1_needload = true;
		mod->index = mod_numknown;
		mod_numknown++;
	}

	return mod;
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
static model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	void	*d;
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

	if (!mod->q1_needload)
	{
		return mod;
	}

//
// because the world is so huge, load it one piece at a time
//
	if (!crash)
	{
	
	}
	
//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return NULL;
	}
	
//
// allocate a new model
//
	QStr::FileBase (mod->name, loadname);
	
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	mod->q1_needload = false;
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadMdlModel (mod, buf);
		break;
		
	case IDSPRITE1HEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;
	
	default:
		Mod_LoadBrush29Model (mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qhandle_t Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	
	mod = Mod_FindName (name);
	
	mod = Mod_LoadModel (mod, crash);
	if (!mod)
	{
		return 0;
	}
	return mod - mod_known;
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
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s\n",mod->q1_cache, mod->name);
	}
}

model_t* Mod_GetModel(qhandle_t handle)
{
	if (handle < 1 || handle >= mod_numknown)
	{
		return &mod_known[0];
	}
	return &mod_known[handle];
}

int Mod_GetNumFrames(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_flags;
}

void Mod_PrintFrameName (qhandle_t m, int frame)
{
	mesh1hdr_t 			*hdr;
	mmesh1framedesc_t	*pframedesc;

	hdr = (mesh1hdr_t *)Mod_Extradata (Mod_GetModel(m));
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];
	
	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->name;
}

int Mod_GetSyncType(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_synctype;
}
