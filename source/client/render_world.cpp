//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_CullTriSurf
//
//==========================================================================

static bool R_CullTriSurf(srfTriangles_t* cv)
{
	int boxCull = R_CullLocalBox(cv->bounds);

	return boxCull == CULL_OUT;
}

//==========================================================================
//
//	R_CullGrid
//
//	Returns true if the grid is completely culled away.
//	Also sets the clipped hint bit in tess
//
//==========================================================================

static bool R_CullGrid(srfGridMesh_t* cv)
{
	if (r_nocurves->integer)
	{
		return true;
	}

	int sphereCull;
	if (tr.currentEntityNum != REF_ENTITYNUM_WORLD)
	{
		sphereCull = R_CullLocalPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	else
	{
		sphereCull = R_CullPointAndRadius(cv->localOrigin, cv->meshRadius);
	}

	// check for trivial reject
	if (sphereCull == CULL_OUT)
	{
		tr.pc.c_sphere_cull_patch_out++;
		return true;
	}
	// check bounding box if necessary
	else if (sphereCull == CULL_CLIP)
	{
		tr.pc.c_sphere_cull_patch_clip++;

		int boxCull = R_CullLocalBox(cv->meshBounds);

		if (boxCull == CULL_OUT) 
		{
			tr.pc.c_box_cull_patch_out++;
			return true;
		}
		else if (boxCull == CULL_IN)
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return false;
}

//==========================================================================
//
//	R_CullSurface
//
//	Tries to back face cull surfaces before they are lighted or added to the
// sorting list.
//
//	This will also allow mirrors on both sides of a model without recursion.
//
//==========================================================================

static bool R_CullSurface(surfaceType_t* surface, shader_t* shader)
{
	if (r_nocull->integer)
	{
		return false;
	}

	if (*surface == SF_GRID)
	{
		return R_CullGrid((srfGridMesh_t*)surface);
	}

	if (*surface == SF_TRIANGLES)
	{
		return R_CullTriSurf((srfTriangles_t*)surface);
	}

	if (*surface != SF_FACE)
	{
		return false;
	}

	if (shader->cullType == CT_TWO_SIDED)
	{
		return false;
	}

	// face culling
	if (!r_facePlaneCull->integer)
	{
		return false;
	}

	srfSurfaceFace_t* sface = (srfSurfaceFace_t*)surface;
	float d = DotProduct(tr.orient.viewOrigin, sface->plane.normal);

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here 
	if (shader->cullType == CT_FRONT_SIDED)
	{
		if (d < sface->plane.dist - 8)
		{
			return true;
		}
	}
	else
	{
		if (d > sface->plane.dist + 8)
		{
			return true;
		}
	}

	return false;
}

//==========================================================================
//
//	R_DlightFace
//
//==========================================================================

static int R_DlightFace(srfSurfaceFace_t* face, int dlightBits)
{
	for (int i = 0; i < tr.refdef.num_dlights; i++)
	{
		if (!(dlightBits & (1 << i)))
		{
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[i];
		float d = DotProduct(dl->origin, face->plane.normal) - face->plane.dist;
		if (d < -dl->radius || d > dl->radius)
		{
			// dlight doesn't reach the plane
			dlightBits &= ~(1 << i);
		}
	}

	if (!dlightBits)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}

//==========================================================================
//
//	R_DlightGrid
//
//==========================================================================

static int R_DlightGrid(srfGridMesh_t* grid, int dlightBits)
{
	for (int i = 0; i < tr.refdef.num_dlights; i++)
	{
		if (!(dlightBits & (1 << i)))
		{
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[i];
		if (dl->origin[0] - dl->radius > grid->meshBounds[1][0] ||
			dl->origin[0] + dl->radius < grid->meshBounds[0][0] ||
			dl->origin[1] - dl->radius > grid->meshBounds[1][1] ||
			dl->origin[1] + dl->radius < grid->meshBounds[0][1] ||
			dl->origin[2] - dl->radius > grid->meshBounds[1][2] ||
			dl->origin[2] + dl->radius < grid->meshBounds[0][2])
		{
			// dlight doesn't reach the bounds
			dlightBits &= ~(1 << i);
		}
	}

	if (!dlightBits)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}

//==========================================================================
//
//	R_DlightTrisurf
//
//==========================================================================

static int R_DlightTrisurf(srfTriangles_t* surf, int dlightBits)
{
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}

//==========================================================================
//
//	R_DlightSurface
//
//	The given surface is going to be drawn, and it touches a leaf that is
// touched by one or more dlights, so try to throw out more dlights if possible.
//
//==========================================================================

static int R_DlightSurface(mbrush46_surface_t* surf, int dlightBits)
{
	if (*surf->data == SF_FACE)
	{
		dlightBits = R_DlightFace((srfSurfaceFace_t*)surf->data, dlightBits);
	}
	else if (*surf->data == SF_GRID)
	{
		dlightBits = R_DlightGrid((srfGridMesh_t*)surf->data, dlightBits);
	}
	else if (*surf->data == SF_TRIANGLES)
	{
		dlightBits = R_DlightTrisurf((srfTriangles_t*)surf->data, dlightBits);
	}
	else
	{
		dlightBits = 0;
	}

	if (dlightBits)
	{
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}

//==========================================================================
//
//	R_AddWorldSurface
//
//==========================================================================

static void R_AddWorldSurface(mbrush46_surface_t* surf, int dlightBits)
{
	if (surf->viewCount == tr.viewCount)
	{
		return;		// already in this view
	}

	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if (R_CullSurface(surf->data, surf->shader))
	{
		return;
	}

	// check for dlighting
	if (dlightBits)
	{
		dlightBits = R_DlightSurface(surf, dlightBits);
		dlightBits = (dlightBits != 0);
	}

	R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex, dlightBits);
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

//==========================================================================
//
//	R_AddBrushModelSurfaces
//
//==========================================================================

void R_AddBrushModelSurfaces(trRefEntity_t* ent)
{
	model_t* pModel = R_GetModelByHandle(ent->e.hModel);

	mbrush46_model_t* bmodel = pModel->q3_bmodel;

	int clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT)
	{
		return;
	}
	
	R_DlightBmodel(bmodel);

	for (int i = 0; i < bmodel->numSurfaces; i++)
	{
		R_AddWorldSurface(bmodel->firstSurface + i, tr.currentEntity->needDlights);
	}
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

//==========================================================================
//
//	R_RecursiveWorldNodeQ3
//
//==========================================================================

static void R_RecursiveWorldNodeQ3(mbrush46_node_t* node, int planeBits, int dlightBits)
{
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
				int r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2)
				{
					return;						// culled
				}
				if (r == 1)
				{
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if (planeBits & 2)
			{
				int r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2)
				{
					return;						// culled
				}
				if (r == 1)
				{
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if (planeBits & 4)
			{
				int r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2)
				{
					return;						// culled
				}
				if (r == 1)
				{
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if (planeBits & 8)
			{
				int r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2)
				{
					return;						// culled
				}
				if (r == 1)
				{
					planeBits &= ~8;			// all descendants will also be in front
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
		int newDlights[2];
		newDlights[0] = 0;
		newDlights[1] = 0;
		if (dlightBits)
		{
			for (int i = 0; i < tr.refdef.num_dlights; i++)
			{
				if (dlightBits & (1 << i))
				{
					dlight_t* dl = &tr.refdef.dlights[i];
					float dist = DotProduct(dl->origin, node->plane->normal) - node->plane->dist;
					
					if (dist > -dl->radius)
					{
						newDlights[0] |= (1 << i);
					}
					if (dist < dl->radius)
					{
						newDlights[1] |= (1 << i);
					}
				}
			}
		}

		// recurse down the children, front side first
		R_RecursiveWorldNodeQ3(node->children[0], planeBits, newDlights[0]);

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
	}
	while (1);

	// leaf node, so add mark surfaces
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
	mbrush46_surface_t** mark = node->firstmarksurface;
	int c = node->nummarksurfaces;
	while (c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		mbrush46_surface_t* surf = *mark;
		R_AddWorldSurface(surf, dlightBits);
		mark++;
	}
}

//==========================================================================
//
//	R_MarkLeavesQ3
//
//	Mark the leaves and nodes that are in the PVS for the current cluster
//
//==========================================================================

static void R_MarkLeavesQ3()
{
	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockpvs->integer)
	{
		return;
	}

	// current viewcluster
	mbrush46_node_t* leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	int cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything 
	if (tr.viewCluster == cluster && !tr.refdef.areamaskModified && !r_showcluster->modified)
	{
		return;
	}

	if (r_showcluster->modified || r_showcluster->integer)
	{
		r_showcluster->modified = false;
		if (r_showcluster->integer)
		{
			GLog.Write("cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if (r_novis->integer || tr.viewCluster == -1)
	{
		for (int i = 0; i < tr.world->numnodes; i++)
		{
			if (tr.world->nodes[i].contents != BSP46CONTENTS_SOLID)
			{
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	const byte* vis = R_ClusterPVS(tr.viewCluster);

	leaf = tr.world->nodes;
	for (int i = 0; i < tr.world->numnodes; i++, leaf++)
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
			continue;		// not visible
		}

		mbrush46_node_t* parent = leaf;
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

//==========================================================================
//
//	R_AddWorldSurfaces
//
//==========================================================================

void R_AddWorldSurfaces()
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
	R_MarkLeavesQ3();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// perform frustum culling and add all the potentially visible surfaces
	if (tr.refdef.num_dlights > 32)
	{
		tr.refdef.num_dlights = 32;
	}
	R_RecursiveWorldNodeQ3(tr.world->nodes, 15, (1 << tr.refdef.num_dlights) - 1);
}
