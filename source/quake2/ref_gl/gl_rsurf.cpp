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
// GL_RSURF.C: surface-related refresh code
#include <assert.h>

#include "gl_local.h"

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel (void)
{
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	mbrush38_surface_t	*psurf;
	dlight_t	*lt;

	// calculate dynamic lighting for bmodel
	lt = tr.refdef.dlights;
	for (k=0 ; k<tr.refdef.num_dlights ; k++, lt++)
	{
		R_MarkLightsQ2 (lt, 1<<k, tr.currentModel->brush38_nodes + tr.currentModel->brush38_firstnode);
	}

	psurf = &tr.currentModel->brush38_surfaces[tr.currentModel->brush38_firstmodelsurface];

	if ( tr.currentEntity->e.renderfx & RF_TRANSLUCENT )
	{
		qglColor4f (1,1,1,0.25);
		GL_TexEnv( GL_MODULATE );
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_State(GLS_DEFAULT);
	}

	//
	// draw texture
	//
	for (i=0 ; i<tr.currentModel->brush38_nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (tr.orient.viewOrigin, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & BRUSH38_SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & BRUSH38_SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (BSP38SURF_TRANS33|BSP38SURF_TRANS66) )
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if ( qglMultiTexCoord2fARB && !( psurf->flags & BRUSH38_SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( psurf );
			}
			else
			{
				R_RenderBrushPolyQ2( psurf );
			}
		}
	}

	if ( !(tr.currentEntity->e.renderfx & RF_TRANSLUCENT) )
	{
		if ( !qglMultiTexCoord2fARB )
			R_BlendLightmapsQ2 ();
	}
	else
	{
		GL_State(GLS_DEFAULT);
		qglColor4f (1,1,1,1);
		GL_TexEnv( GL_REPLACE );
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (trRefEntity_t *e)
{
	if (tr.currentModel->brush38_nummodelsurfaces == 0)
		return;

	R_RotateForEntity(e, &tr.viewParms, &tr.orient);

	if (R_CullLocalBox(&tr.currentModel->q2_mins) == CULL_OUT)
	{
		return;
	}

	qglColor3f (1,1,1);
	Com_Memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

    qglPushMatrix ();
	qglLoadMatrixf(tr.orient.modelMatrix);

	R_DrawInlineBModel ();

	qglPopMatrix ();
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
void R_RecursiveWorldNode (mbrush38_node_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	mbrush38_surface_t	*surf, **mark;
	mbrush38_leaf_t		*pleaf;
	float		dot;
	image_t		*image;

	if (node->contents == BSP38CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != tr.visCount)
		return;
	if (R_CullLocalBox((vec3_t*)node->minmaxs) == CULL_OUT)
	{
		return;
	}
	
// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mbrush38_leaf_t *)node;

		// check for door connected areas
		if (tr.refdef.areamask[pleaf->area >> 3] & (1 << (pleaf->area & 7)))
			return;		// not visible

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

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
	plane = node->plane;

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
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = tr.worldModel->brush38_surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != tr.viewCount)
			continue;

		if ( (surf->flags & BRUSH38_SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & BSP38SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & (BSP38SURF_TRANS33|BSP38SURF_TRANS66))
		{	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if ( qglMultiTexCoord2fARB && !( surf->flags & BRUSH38_SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( surf );
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimationQ2 (surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	if (!r_drawworld->value)
		return;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	tr.currentModel = tr.worldModel;

	VectorCopy(tr.viewParms.orient.origin, tr.orient.viewOrigin);

	// auto cycle the world frame for texture animation
	tr.worldEntity.e.frame = (int)(tr.refdef.floatTime * 2);
	tr.currentEntity = &tr.worldEntity;

	qglColor3f (1,1,1);
	Com_Memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));
	R_ClearSkyBox ();

	R_RecursiveWorldNode (tr.worldModel->brush38_nodes);

	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled
	*/
	DrawTextureChainsQ2 ();
	R_BlendLightmapsQ2 ();
	
	R_DrawSkyBoxQ2 ();

	R_DrawTriangleOutlines ();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	byte	fatvis[BSP38MAX_MAP_LEAFS/8];
	mbrush38_node_t	*node;
	int		i, c;
	mbrush38_leaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	tr.visCount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !tr.worldModel->brush38_vis)
	{
		// mark everything
		for (i=0 ; i<tr.worldModel->brush38_numleafs ; i++)
			tr.worldModel->brush38_leafs[i].visframe = tr.visCount;
		for (i=0 ; i<tr.worldModel->brush38_numnodes ; i++)
			tr.worldModel->brush38_nodes[i].visframe = tr.visCount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, tr.worldModel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		Com_Memcpy(fatvis, vis, (tr.worldModel->brush38_numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, tr.worldModel);
		c = (tr.worldModel->brush38_numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=tr.worldModel->brush38_leafs ; i<tr.worldModel->brush38_numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mbrush38_node_t *)leaf;
			do
			{
				if (node->visframe == tr.visCount)
					break;
				node->visframe = tr.visCount;
				node = node->parent;
			} while (node);
		}
	}
}
