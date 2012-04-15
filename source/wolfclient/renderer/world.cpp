//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

#if 0
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
#endif

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

//==========================================================================
//
//	R_DrawBrushModelQ1
//
//==========================================================================

void R_DrawBrushModelQ1(trRefEntity_t* e, bool Translucent)
{
	if ((e->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
	{
		return;
	}

	model_t* clmodel = R_GetModelByHandle(e->e.hModel);

	if (R_CullLocalBox(&clmodel->q1_mins) == CULL_OUT)
	{
		return;
	}

	qglColor3f(1, 1, 1);
	Com_Memset(lightmap_polys, 0, sizeof(lightmap_polys));

    qglPushMatrix();
	qglLoadMatrixf(tr.orient.modelMatrix);

	mbrush29_surface_t* psurf = &clmodel->brush29_surfaces[clmodel->brush29_firstmodelsurface];

	// calculate dynamic lighting for bmodel if it's not an
	// instanced model
	if (clmodel->brush29_firstmodelsurface != 0)
	{
		for (int k = 0; k < tr.refdef.num_dlights; k++)
		{
			R_MarkLightsQ1(&tr.refdef.dlights[k], 1 << k,
				clmodel->brush29_nodes + clmodel->brush29_firstnode);
		}
	}

	//
	// draw texture
	//
	for (int i = 0; i < clmodel->brush29_nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		cplane_t* pplane = psurf->plane;

		float dot = DotProduct(tr.orient.viewOrigin, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & BRUSH29_SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & BRUSH29_SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (r_texsort->value)
			{
				R_RenderBrushPolyQ1(psurf, false);
			}
			else
			{
				R_DrawSequentialPoly(psurf);
			}
		}
	}

	if (!Translucent && !(tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT))
	{
		R_BlendLightmapsQ1();
	}

	qglPopMatrix();
}

//==========================================================================
//
//	R_DrawInlineBModel
//
//==========================================================================

static void R_DrawInlineBModel()
{
	// calculate dynamic lighting for bmodel
	dlight_t* lt = tr.refdef.dlights;
	for (int k = 0; k < tr.refdef.num_dlights; k++, lt++)
	{
		R_MarkLightsQ2(lt, 1 << k, tr.currentModel->brush38_nodes + tr.currentModel->brush38_firstnode);
	}

	mbrush38_surface_t* psurf = &tr.currentModel->brush38_surfaces[tr.currentModel->brush38_firstmodelsurface];

	if (tr.currentEntity->e.renderfx & RF_TRANSLUCENT)
	{
		qglColor4f(1, 1, 1, 0.25);
		GL_TexEnv(GL_MODULATE);
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_State(GLS_DEFAULT);
	}

	//
	// draw texture
	//
	for (int i = 0; i < tr.currentModel->brush38_nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		cplane_t* pplane = psurf->plane;

		float dot = DotProduct(tr.orient.viewOrigin, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & BRUSH38_SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & BRUSH38_SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (BSP38SURF_TRANS33 | BSP38SURF_TRANS66))
			{
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (qglMultiTexCoord2fARB && !(psurf->flags & BRUSH38_SURF_DRAWTURB))
			{
				GL_RenderLightmappedPoly(psurf);
			}
			else
			{
				R_RenderBrushPolyQ2(psurf);
			}
		}
	}

	if (!(tr.currentEntity->e.renderfx & RF_TRANSLUCENT))
	{
		if (!qglMultiTexCoord2fARB)
		{
			R_BlendLightmapsQ2();
		}
	}
	else
	{
		GL_State(GLS_DEFAULT);
		qglColor4f(1, 1, 1, 1);
		GL_TexEnv(GL_REPLACE);
	}
}

//==========================================================================
//
//	R_DrawBrushModelQ2
//
//==========================================================================

void R_DrawBrushModelQ2(trRefEntity_t* e)
{
	if (tr.currentModel->brush38_nummodelsurfaces == 0)
	{
		return;
	}

	if (R_CullLocalBox(&tr.currentModel->q2_mins) == CULL_OUT)
	{
		return;
	}

	qglColor3f(1, 1, 1);
	Com_Memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

    qglPushMatrix();
	qglLoadMatrixf(tr.orient.modelMatrix);

	R_DrawInlineBModel();

	qglPopMatrix();
}

#if 0
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
		R_AddWorldSurface(bmodel->firstSurface + i, tr.currentEntity->dlightBits);
	}
}
#endif

/*
=============================================================

	WORLD MODEL

=============================================================
*/

//==========================================================================
//
//	R_RecursiveWorldNodeQ1
//
//==========================================================================

static void R_RecursiveWorldNodeQ1(mbrush29_node_t* node)
{
	if (node->contents == BSP29CONTENTS_SOLID)
	{
		return;		// solid
	}

	if (node->visframe != tr.visCount)
	{
		return;
	}
	if (R_CullLocalBox((vec3_t*)node->minmaxs) == CULL_OUT)
	{
		return;
	}
	
	// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		mbrush29_leaf_t* pleaf = (mbrush29_leaf_t*)node;

		mbrush29_surface_t** mark = pleaf->firstmarksurface;
		int c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = tr.viewCount;
				mark++;
			}
			while (--c);
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	cplane_t* plane = node->plane;

	double dot;
	switch (plane->type)
	{
	case PLANE_X:
		dot = tr.orient.viewOrigin[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = tr.orient.viewOrigin[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = tr.orient.viewOrigin[2] - plane->dist;
		break;
	default:
		dot = DotProduct (tr.orient.viewOrigin, plane->normal) - plane->dist;
		break;
	}

	int side;
	if (dot >= 0)
	{
		side = 0;
	}
	else
	{
		side = 1;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNodeQ1(node->children[side]);

	// draw stuff
	int c = node->numsurfaces;

	if (c)
	{
		mbrush29_surface_t* surf = tr.worldModel->brush29_surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
		{
			side = BRUSH29_SURF_PLANEBACK;
		}
		else if (dot > BACKFACE_EPSILON)
		{
			side = 0;
		}
		for (; c; c--, surf++)
		{
			if (surf->visframe != tr.viewCount)
			{
				continue;
			}

			// don't backface underwater surfaces, because they warp
			if (!(((r_viewleaf->contents == BSP29CONTENTS_EMPTY && (surf->flags & BRUSH29_SURF_UNDERWATER)) ||
				(r_viewleaf->contents != BSP29CONTENTS_EMPTY && !(surf->flags & BRUSH29_SURF_UNDERWATER))) &&
				!(surf->flags & BRUSH29_SURF_DONTWARP)) && ((dot < 0) ^ !!(surf->flags & BRUSH29_SURF_PLANEBACK)))
			{
				continue;		// wrong side
			}

			// if sorting by texture, just store it out
			if (r_texsort->value)
			{
				surf->texturechain = surf->texinfo->texture->texturechain;
				surf->texinfo->texture->texturechain = surf;
			}
			else if (surf->flags & BRUSH29_SURF_DRAWSKY)
			{
				surf->texturechain = skychain;
				skychain = surf;
			}
			else if (surf->flags & BRUSH29_SURF_DRAWTURB)
			{
				surf->texturechain = waterchain;
				waterchain = surf;
			}
			else
			{
				R_DrawSequentialPoly(surf);
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNodeQ1(node->children[!side]);
}

//==========================================================================
//
//	R_MarkLeavesQ1
//
//==========================================================================

static void R_MarkLeavesQ1()
{
	// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeafQ1(tr.viewParms.orient.origin, tr.worldModel);

	if (r_oldviewleaf == r_viewleaf && !r_novis->value)
	{
		return;
	}
	
	tr.visCount++;
	r_oldviewleaf = r_viewleaf;

	byte* vis;
	byte solid[4096];
	if (r_novis->value)
	{
		vis = solid;
		Com_Memset(solid, 0xff, (tr.worldModel->brush29_numleafs + 7) >> 3);
	}
	else
	{
		vis = Mod_LeafPVS(r_viewleaf, tr.worldModel);
	}

	for (int i = 0; i < tr.worldModel->brush29_numleafs; i++)
	{
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			mbrush29_node_t* node = (mbrush29_node_t*)&tr.worldModel->brush29_leafs[i + 1];
			do
			{
				if (node->visframe == tr.visCount)
				{
					break;
				}
				node->visframe = tr.visCount;
				node = node->parent;
			}
			while (node);
		}
	}
}

//==========================================================================
//
//	R_DrawWorldQ1
//
//==========================================================================

void R_DrawWorldQ1()
{
	R_MarkLeavesQ1();

	VectorCopy(tr.viewParms.orient.origin, tr.orient.viewOrigin);

	tr.currentEntity = &tr.worldEntity;

	qglColor3f(1, 1, 1);
	Com_Memset(lightmap_polys, 0, sizeof(lightmap_polys));

	R_RecursiveWorldNodeQ1(tr.worldModel->brush29_nodes);

	DrawTextureChainsQ1();

	R_BlendLightmapsQ1();
}

//==========================================================================
//
//	R_RecursiveWorldNodeQ2
//
//==========================================================================

static void R_RecursiveWorldNodeQ2(mbrush38_node_t* node)
{
	if (node->contents == BSP38CONTENTS_SOLID)
	{
		return;		// solid
	}

	if (node->visframe != tr.visCount)
	{
		return;
	}
	if (R_CullLocalBox((vec3_t*)node->minmaxs) == CULL_OUT)
	{
		return;
	}
	
	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		mbrush38_leaf_t* pleaf = (mbrush38_leaf_t*)node;

		// check for door connected areas
		if (tr.refdef.areamask[pleaf->area >> 3] & (1 << (pleaf->area & 7)))
		{
			return;		// not visible
		}

		mbrush38_surface_t** mark = pleaf->firstmarksurface;
		int c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = tr.viewCount;
				mark++;
			} while (--c);
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	cplane_t* plane = node->plane;

	float dot;
	switch (plane->type)
	{
	case PLANE_X:
		dot = tr.orient.viewOrigin[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = tr.orient.viewOrigin[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = tr.orient.viewOrigin[2] - plane->dist;
		break;
	default:
		dot = DotProduct(tr.orient.viewOrigin, plane->normal) - plane->dist;
		break;
	}

	int side, sidebit;
	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = BRUSH38_SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNodeQ2(node->children[side]);

	// draw stuff
	mbrush38_surface_t* surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for (int c = node->numsurfaces; c; c--, surf++)
	{
		if (surf->visframe != tr.viewCount)
		{
			continue;
		}

		if ((surf->flags & BRUSH38_SURF_PLANEBACK) != sidebit)
		{
			continue;		// wrong side
		}

		if (surf->texinfo->flags & BSP38SURF_SKY)
		{
			// just adds to visible sky bounds
			R_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & (BSP38SURF_TRANS33 | BSP38SURF_TRANS66))
		{
			// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if (qglMultiTexCoord2fARB && !(surf->flags & BRUSH38_SURF_DRAWTURB))
			{
				GL_RenderLightmappedPoly(surf);
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image_t* image = R_TextureAnimationQ2(surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNodeQ2(node->children[!side]);
}

//==========================================================================
//
//	R_FindViewCluster
//
//==========================================================================

static void R_FindViewCluster()
{
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;
	mbrush38_leaf_t* leaf = Mod_PointInLeafQ2(tr.viewParms.orient.origin, tr.worldModel);
	r_viewcluster = r_viewcluster2 = leaf->cluster;

	// check above and below so crossing solid water doesn't draw wrong
	if (!leaf->contents)
	{
		// look down a bit
		vec3_t temp;
		VectorCopy(tr.viewParms.orient.origin, temp);
		temp[2] -= 16;
		leaf = Mod_PointInLeafQ2(temp, tr.worldModel);
		if (!(leaf->contents & BSP38CONTENTS_SOLID) && (leaf->cluster != r_viewcluster2))
		{
			r_viewcluster2 = leaf->cluster;
		}
	}
	else
	{
		// look up a bit
		vec3_t	temp;
		VectorCopy(tr.viewParms.orient.origin, temp);
		temp[2] += 16;
		leaf = Mod_PointInLeafQ2(temp, tr.worldModel);
		if (!(leaf->contents & BSP38CONTENTS_SOLID) && (leaf->cluster != r_viewcluster2))
		{
			r_viewcluster2 = leaf->cluster;
		}
	}
}

//==========================================================================
//
//	R_MarkLeavesQ2
//
//	Mark the leaves and nodes that are in the PVS for the current cluster
//
//==========================================================================

static void R_MarkLeavesQ2()
{
	// current viewcluster
	R_FindViewCluster();

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
	{
		return;
	}

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (r_lockpvs->value)
	{
		return;
	}

	tr.visCount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !tr.worldModel->brush38_vis)
	{
		// mark everything
		for (int i = 0; i < tr.worldModel->brush38_numleafs; i++)
		{
			tr.worldModel->brush38_leafs[i].visframe = tr.visCount;
		}
		for (int i = 0; i < tr.worldModel->brush38_numnodes; i++)
		{
			tr.worldModel->brush38_nodes[i].visframe = tr.visCount;
		}
		return;
	}

	byte* vis = Mod_ClusterPVS(r_viewcluster, tr.worldModel);
	byte fatvis[BSP38MAX_MAP_LEAFS/8];
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		Com_Memcpy(fatvis, vis, (tr.worldModel->brush38_numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, tr.worldModel);
		int c = (tr.worldModel->brush38_numleafs + 31) / 32;
		for (int i = 0; i < c; i++)
		{
			((int*)fatvis)[i] |= ((int*)vis)[i];
		}
		vis = fatvis;
	}

	mbrush38_leaf_t* leaf = tr.worldModel->brush38_leafs;
	for (int i = 0; i < tr.worldModel->brush38_numleafs; i++, leaf++)
	{
		int cluster = leaf->cluster;
		if (cluster == -1)
		{
			continue;
		}
		if (vis[cluster >> 3] & (1 << (cluster & 7)))
		{
			mbrush38_node_t* node = (mbrush38_node_t*)leaf;
			do
			{
				if (node->visframe == tr.visCount)
				{
					break;
				}
				node->visframe = tr.visCount;
				node = node->parent;
			}
			while (node);
		}
	}
}

//==========================================================================
//
//	R_DrawWorldQ2
//
//==========================================================================

void R_DrawWorldQ2()
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	R_MarkLeavesQ2();

	if (!r_drawworld->value)
	{
		return;
	}

	tr.currentModel = tr.worldModel;

	VectorCopy(tr.viewParms.orient.origin, tr.orient.viewOrigin);

	// auto cycle the world frame for texture animation
	tr.worldEntity.e.frame = (int)(tr.refdef.floatTime * 2);
	tr.currentEntity = &tr.worldEntity;

	qglColor3f(1, 1, 1);
	Com_Memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));
	R_ClearSkyBox();

	R_RecursiveWorldNodeQ2(tr.worldModel->brush38_nodes);

	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled
	*/
	DrawTextureChainsQ2();
	R_BlendLightmapsQ2();
	
	R_DrawSkyBoxQ2();

	R_DrawTriangleOutlines();
}

#if 0
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
			Log::write("cluster:%i  area:%i\n", cluster, leaf->area);
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
#endif
