/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"

void R_AddWorldSurface(mbrush46_surface_t* surf, shader_t* shader, int dlightBits, int decalBits);

/*
=============================================================

    BRUSH MODELS

=============================================================
*/

//----(SA) added

/*
=================
R_BmodelFogNum

See if a sprite is inside a fog volume
Return positive with /any part/ of the brush falling within a fog volume
=================
*/
int R_BmodelFogNum(trRefEntity_t* re, mbrush46_model_t* bmodel)
{
	int i, j;
	mbrush46_fog_t* fog;

	for (i = 1; i < tr.world->numfogs; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0; j < 3; j++)
		{
			if (re->e.origin[j] + bmodel->bounds[0][j] > fog->bounds[1][j])
			{
				break;
			}
			if (re->e.origin[j] + bmodel->bounds[0][j] < fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
		for (j = 0; j < 3; j++)
		{
			if (re->e.origin[j] + bmodel->bounds[1][j] > fog->bounds[1][j])
			{
				break;
			}
			if (bmodel->bounds[1][j] < fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

//----(SA) done


/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t* ent)
{
	mbrush46_model_t* bmodel;
	int clip;
	model_t* pModel;
	int i;
	int fognum;

	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->q3_bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT)
	{
		return;
	}

	R_DlightBmodel(bmodel);

//----(SA) modified
	// determine if in fog
	fognum = R_BmodelFogNum(ent, bmodel);

	for (i = 0; i < bmodel->numSurfaces; i++)
	{
		(bmodel->firstSurface + i)->fogIndex = fognum;
		R_AddWorldSurface(bmodel->firstSurface + i, (bmodel->firstSurface + i)->shader, tr.currentEntity->dlightBits, 0);
	}
//----(SA) end
}


/*
=============================================================

    WORLD MODEL

=============================================================
*/


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(mbrush46_node_t* node, int planeBits, int dlightBits)
{

	do
	{
		int newDlights[2];

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (!r_nocull->integer)
		{
			int r;

			if (planeBits & 1)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~1;            // all descendants will also be in front
				}
			}

			if (planeBits & 2)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~2;            // all descendants will also be in front
				}
			}

			if (planeBits & 4)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~4;            // all descendants will also be in front
				}
			}

			if (planeBits & 8)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~8;            // all descendants will also be in front
				}
			}

		}

		if (node->contents != -1)
		{
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative
		// determine which dlights are needed
		newDlights[0] = 0;
		newDlights[1] = 0;
/*
//		if ( dlightBits )
        {
            int	i;

            for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
                dlight_t	*dl;
                float		dist;

//				if ( dlightBits & ( 1 << i ) ) {
                    dl = &tr.refdef.dlights[i];
                    dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;

                    if ( dist > -dl->radius ) {
                        newDlights[0] |= ( 1 << i );
                    }
                    if ( dist < dl->radius ) {
                        newDlights[1] |= ( 1 << i );
                    }
//				}
            }
        }
*/
		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, newDlights[0]);

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
	}
	while (1);

	{
		// leaf node, so add mark surfaces
		int c;
		mbrush46_surface_t* surf, ** mark;

		// RF, hack, dlight elimination above is unreliable
		dlightBits = 0xffffffff;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		if (node->mins[0] < tr.viewParms.visBounds[0][0])
		{
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if (node->mins[1] < tr.viewParms.visBounds[0][1])
		{
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if (node->mins[2] < tr.viewParms.visBounds[0][2])
		{
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if (node->maxs[0] > tr.viewParms.visBounds[1][0])
		{
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if (node->maxs[1] > tr.viewParms.visBounds[1][1])
		{
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if (node->maxs[2] > tr.viewParms.visBounds[1][2])
		{
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}

		// add the individual surfaces
		mark = node->firstmarksurface;
		c = node->nummarksurfaces;
		while (c--)
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_AddWorldSurface(surf, surf->shader, dlightBits, 0);
			mark++;
		}
	}

}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void)
{
	const byte* vis;
	mbrush46_node_t* leaf, * parent;
	int i;
	int cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockpvs->integer)
	{
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if (tr.viewCluster == cluster && !tr.refdef.areamaskModified
		&& !r_showcluster->modified)
	{
		return;
	}

	if (r_showcluster->modified || r_showcluster->integer)
	{
		r_showcluster->modified = qfalse;
		if (r_showcluster->integer)
		{
			ri.Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if (r_novis->integer || tr.viewCluster == -1)
	{
		for (i = 0; i < tr.world->numnodes; i++)
		{
			if (tr.world->nodes[i].contents != BSP46CONTENTS_SOLID)
			{
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	vis = R_ClusterPVS(tr.viewCluster);

	for (i = 0,leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster < 0 || cluster >= tr.world->numClusters)
		{
			continue;
		}

		// check general pvs
		if (!(vis[cluster >> 3] & (1 << (cluster & 7))))
		{
			continue;
		}

		// check for door connection
		if ((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7))))
		{
			continue;       // not visible
		}

		parent = leaf;
		do
		{
			if (parent->visframe == tr.visCount)
			{
				break;
			}
			parent->visframe = tr.visCount;
			parent = parent->parent;
		}
		while (parent);
	}
}


/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces(void)
{
	if (!r_drawworld->integer)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// perform frustum culling and add all the potentially visible surfaces
	if (tr.refdef.num_dlights > 32)
	{
		tr.refdef.num_dlights = 32;
	}
	R_RecursiveWorldNode(tr.world->nodes, 15, (1 << tr.refdef.num_dlights) - 1);
}
