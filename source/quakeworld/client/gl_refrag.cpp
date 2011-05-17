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
// r_efrag.c

#include "quakedef.h"
#include "glquake.h"

//===========================================================================

/*
===============================================================================

					ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

efrag_t		**lastlink;

vec3_t		r_emins, r_emaxs;

entity_t	*r_addent;

/*
===================
R_SplitEntityOnNode
===================
*/
void R_SplitEntityOnNode (mnode_t *node)
{
	efrag_t		*ef;
	cplane_t	*splitplane;
	mleaf_t		*leaf;
	int			sides;
	
	if (node->contents == BSP29CONTENTS_SOLID)
	{
		return;
	}
	
// add an efrag if the node is a leaf

	if ( node->contents < 0)
	{
		leaf = (mleaf_t *)node;

// grab an efrag off the free list
		ef = cl.free_efrags;
		if (!ef)
		{
			Con_Printf ("Too many efrags!\n");
			return;		// no free fragments...
		}
		cl.free_efrags = cl.free_efrags->entnext;

		ef->entity = r_addent;
		
// add the entity link	
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;
		
// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;
			
		return;
	}
	
// NODE_MIXED

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);
	
// recurse down the contacted sides
	if (sides & 1)
		R_SplitEntityOnNode (node->children[0]);
		
	if (sides & 2)
		R_SplitEntityOnNode (node->children[1]);
}



/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags (entity_t *ent)
{
	model_t		*entmodel;
	int			i;
		
	if (!ent->model)
		return;

	r_addent = ent;
			
	lastlink = &ent->efrag;
	
	entmodel = ent->model;

	for (i=0 ; i<3 ; i++)
	{
		r_emins[i] = ent->origin[i] + entmodel->mins[i];
		r_emaxs[i] = ent->origin[i] + entmodel->maxs[i];
	}

	R_SplitEntityOnNode (cl.worldmodel->nodes);
}


/*
================
R_StoreEfrags

// FIXME: a lot of this goes away with edge-based
================
*/
void R_StoreEfrags (efrag_t **ppefrag)
{
	entity_t	*pent;
	model_t		*clmodel;
	efrag_t		*pefrag;


	while ((pefrag = *ppefrag) != NULL)
	{
		pent = pefrag->entity;
		clmodel = pent->model;

		switch (clmodel->type)
		{
		case mod_alias:
		case mod_brush:
		case mod_sprite:
			pent = pefrag->entity;

			if (pent->visframe != tr.frameCount)
			{
				refEntity_t rent;
				Com_Memset(&rent, 0, sizeof(rent));
				rent.reType = RT_MODEL;
				VectorCopy(pent->origin, rent.origin);
				rent.hModel = Mod_GetHandle(pent->model);
				CL_SetRefEntAxis(&rent, pent->angles);
				rent.frame = pent->frame;
				rent.skinNum = pent->skinnum;
				rent.shaderTime = pent->syncbase;
				R_AddRefEntToScene(&rent);

			// mark that we've recorded this entity for this frame
				pent->visframe = tr.frameCount;
			}

			ppefrag = &pefrag->leafnext;
			break;

		default:	
			Sys_Error ("R_StoreEfrags: Bad entity type %d\n", clmodel->type);
		}
	}
}


