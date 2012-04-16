/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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

// ydnar: the original implementation of this function is a bit flaky...
int R_BmodelFogNum(trRefEntity_t* re, mbrush46_model_t* bmodel)

#if 1

{
	int i, j;
	mbrush46_fog_t* fog;

	for (i = 1; i < tr.world->numfogs; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0; j < 3; j++)
		{
			if (re->e.origin[j] + bmodel->bounds[0][j] >= fog->bounds[1][j])
			{
				break;
			}
			if (re->e.origin[j] + bmodel->bounds[1][j] <= fog->bounds[0][j])
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

#else

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

#endif

//----(SA) done



/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t* ent)
{
	int i, clip, fognum, decalBits;
	vec3_t mins, maxs;
	model_t* pModel;
	mbrush46_model_t* bmodel;
	int savedNumDecalProjectors, numLocalProjectors;
	decalProjector_t* savedDecalProjectors, localProjectors[MAX_DECAL_PROJECTORS];


	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->q3_bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT)
	{
		return;
	}

	// ydnar: set current brush model to world
	tr.currentBModel = bmodel;

	// ydnar: set model state for decals and dynamic fog
	VectorCopy(ent->e.origin, bmodel->orientation[tr.smpFrame].origin);
	VectorCopy(ent->e.axis[0], bmodel->orientation[tr.smpFrame].axis[0]);
	VectorCopy(ent->e.axis[1], bmodel->orientation[tr.smpFrame].axis[1]);
	VectorCopy(ent->e.axis[2], bmodel->orientation[tr.smpFrame].axis[2]);
	bmodel->visible[tr.smpFrame] = qtrue;
	bmodel->entityNum[tr.smpFrame] = tr.currentEntityNum;

	R_DlightBmodel(bmodel);

	// determine if in fog
	fognum = R_BmodelFogNum(ent, bmodel);

	// ydnar: project any decals
	decalBits = 0;
	numLocalProjectors = 0;
	for (i = 0; i < tr.refdef.numDecalProjectors; i++)
	{
		// early out
		if (tr.refdef.decalProjectors[i].shader == NULL)
		{
			continue;
		}

		// transform entity bbox (fixme: rotated entities have invalid bounding boxes)
		VectorAdd(bmodel->bounds[0], tr.orient.origin, mins);
		VectorAdd(bmodel->bounds[1], tr.orient.origin, maxs);

		// set bit
		if (R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], mins, maxs))
		{
			R_TransformDecalProjector(&tr.refdef.decalProjectors[i], tr.orient.axis, tr.orient.origin, &localProjectors[numLocalProjectors]);
			numLocalProjectors++;
			decalBits <<= 1;
			decalBits |= 1;
		}
	}

	// ydnar: save old decal projectors
	savedNumDecalProjectors = tr.refdef.numDecalProjectors;
	savedDecalProjectors = tr.refdef.decalProjectors;

	// set local decal projectors
	tr.refdef.numDecalProjectors = numLocalProjectors;
	tr.refdef.decalProjectors = localProjectors;

	// add model surfaces
	for (i = 0; i < bmodel->numSurfaces; i++)
	{
		(bmodel->firstSurface + i)->fogIndex = fognum;
		// Arnout: custom shader support for brushmodels
		if (ent->e.customShader)
		{
			R_AddWorldSurface(bmodel->firstSurface + i, R_GetShaderByHandle(ent->e.customShader), tr.currentEntity->dlightBits, decalBits);
		}
		else
		{
			R_AddWorldSurface(bmodel->firstSurface + i, ((mbrush46_surface_t*)(bmodel->firstSurface + i))->shader, tr.currentEntity->dlightBits, decalBits);
		}
	}

	// ydnar: restore old decal projectors
	tr.refdef.numDecalProjectors = savedNumDecalProjectors;
	tr.refdef.decalProjectors = savedDecalProjectors;

	// ydnar: add decal surfaces
	R_AddDecalSurfaces(bmodel);

	// ydnar: clear current brush model
	tr.currentBModel = NULL;
}


/*
=============================================================

    WORLD MODEL

=============================================================
*/


/*
R_AddLeafSurfaces() - ydnar
adds a leaf's drawsurfaces
*/

static void R_AddLeafSurfaces(mbrush46_node_t* node, int dlightBits, int decalBits)
{
	int c;
	mbrush46_surface_t* surf, ** mark;


	// add to count
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
		R_AddWorldSurface(surf, surf->shader, dlightBits, decalBits);
		mark++;
	}
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(mbrush46_node_t* node, int planeBits, int dlightBits, int decalBits)
{
	int i, r;
	dlight_t* dl;


	do
	{
		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (!r_nocull->integer)
		{
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

			// ydnar: farplane culling
			if (planeBits & 16)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
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

		// ydnar: cull dlights
		if (dlightBits)      //%	&& node->contents != -1 )
		{
			for (i = 0; i < tr.refdef.num_dlights; i++)
			{
				if (dlightBits & (1 << i))
				{
					// directional dlights don't get culled
					if (tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT)
					{
						continue;
					}

					// test dlight bounds against node surface bounds
					dl = &tr.refdef.dlights[i];
					if (node->surfMins[0] >= (dl->origin[0] + dl->radius) || node->surfMaxs[0] <= (dl->origin[0] - dl->radius) ||
						node->surfMins[1] >= (dl->origin[1] + dl->radius) || node->surfMaxs[1] <= (dl->origin[1] - dl->radius) ||
						node->surfMins[2] >= (dl->origin[2] + dl->radius) || node->surfMaxs[2] <= (dl->origin[2] - dl->radius))
					{
						dlightBits &= ~(1 << i);
					}
				}
			}
		}

		// ydnar: cull decals
		if (decalBits)
		{
			for (i = 0; i < tr.refdef.numDecalProjectors; i++)
			{
				if (decalBits & (1 << i))
				{
					// test decal bounds against node surface bounds
					if (tr.refdef.decalProjectors[i].shader == NULL ||
						!R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], node->surfMins, node->surfMaxs))
					{
						decalBits &= ~(1 << i);
					}
				}
			}
		}

		// handle leaf nodes
		if (node->contents != -1)
		{
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, dlightBits, decalBits);

		// tail recurse
		node = node->children[1];
	}
	while (1);

	// short circuit
	if (node->nummarksurfaces == 0)
	{
		return;
	}

	// ydnar: moved off to separate function
	R_AddLeafSurfaces(node, dlightBits, decalBits);
}


/*
=================
R_inPVS
=================
*/
qboolean R_inPVS(const vec3_t p1, const vec3_t p2)
{
	mbrush46_node_t* leaf;
	byte* vis;

	leaf = R_PointInLeaf(p1);
	vis = CM_ClusterPVS(leaf->cluster);
	leaf = R_PointInLeaf(p2);

	if (!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
	{
		return qfalse;
	}
	return qtrue;
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

		// ydnar: don't want to walk the entire bsp to add skybox surfaces
		if (tr.refdef.rdflags & RDF_SKYBOXPORTAL)
		{
			// this only happens once, as game/cgame know the origin of the skybox
			// this also means the skybox portal cannot move, as this list is calculated once and never again
			if (tr.world->numSkyNodes < WORLD_MAX_SKY_NODES)
			{
				tr.world->skyNodes[tr.world->numSkyNodes++] = leaf;
			}
			R_AddLeafSurfaces(leaf, 0, 0);
			continue;
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

	// ydnar: set current brush model to world
	tr.currentBModel = &tr.world->bmodels[0];

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// render sky or world?
	if (tr.refdef.rdflags & RDF_SKYBOXPORTAL && tr.world->numSkyNodes > 0)
	{
		int i;
		mbrush46_node_t** node;

		for (i = 0, node = tr.world->skyNodes; i < tr.world->numSkyNodes; i++, node++)
			R_AddLeafSurfaces(*node, tr.refdef.dlightBits, 0);      // no decals on skybox nodes
	}
	else
	{
		// determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// perform frustum culling and add all the potentially visible surfaces
		R_RecursiveWorldNode(tr.world->nodes, 255, tr.refdef.dlightBits, tr.refdef.decalBits);

		// ydnar: add decal surfaces
		R_AddDecalSurfaces(tr.world->bmodels);
	}

	// clear brush model
	tr.currentBModel = NULL;
}

