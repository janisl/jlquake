// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "glquake.h"

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (trRefEntity_t* e, qboolean Translucent)
{
	int			j, k;
	int			i, numsurfaces;
	mbrush29_surface_t	*psurf;
	float		dot;
	cplane_t	*pplane;
	model_t		*clmodel;

	R_RotateForEntity(e, &tr.viewParms, &tr.orient);

	clmodel = R_GetModelByHandle(e->e.hModel);

	if (R_CullLocalBox(&clmodel->q1_mins) == CULL_OUT)
	{
		return;
	}

	qglColor3f (1,1,1);
	Com_Memset(lightmap_polys, 0, sizeof(lightmap_polys));

    qglPushMatrix ();
	qglLoadMatrixf(tr.orient.modelMatrix);

	psurf = &clmodel->brush29_surfaces[clmodel->brush29_firstmodelsurface];

// calculate dynamic lighting for bmodel if it's not an
// instanced model
	if (clmodel->brush29_firstmodelsurface != 0)
	{
		for (k=0 ; k<tr.refdef.num_dlights; k++)
		{
			R_MarkLights (&tr.refdef.dlights[k], 1<<k,
				clmodel->brush29_nodes + clmodel->brush29_firstnode);
		}
	}

	GL_State(GLS_DEPTHMASK_TRUE);

	//
	// draw texture
	//
	for (i=0 ; i<clmodel->brush29_nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (tr.orient.viewOrigin, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & BRUSH29_SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & BRUSH29_SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (r_texsort->value)
				R_RenderBrushPolyQ1 (psurf, false);
			else
				R_DrawSequentialPoly (psurf);
		}
	}

	if (!Translucent &&  !(tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT))
		R_BlendLightmapsQ1 (Translucent);

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
void R_RecursiveWorldNode (mbrush29_node_t *node)
{
	int			i, c, side, *pindex;
	vec3_t		acceptpt, rejectpt;
	cplane_t	*plane;
	mbrush29_surface_t	*surf, **mark;
	mbrush29_leaf_t		*pleaf;
	double		d, dot;
	vec3_t		mins, maxs;

	if (node->contents == BSP29CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != tr.visCount)
		return;
	if (R_CullLocalBox((vec3_t*)node->minmaxs) == CULL_OUT)
	{
		return;
	}
	
// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mbrush29_leaf_t *)node;

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
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = tr.worldModel->brush29_surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = BRUSH29_SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			for ( ; c ; c--, surf++)
			{
				if (surf->visframe != tr.viewCount)
					continue;

				// don't backface underwater surfaces, because they warp
//				if ( !(surf->flags & BRUSH29_SURF_UNDERWATER) && ( (dot < 0) ^ !!(surf->flags & BRUSH29_SURF_PLANEBACK)) )
//					continue;		// wrong side
				if ( !(((r_viewleaf->contents==BSP29CONTENTS_EMPTY && (surf->flags & BRUSH29_SURF_UNDERWATER)) ||
					(r_viewleaf->contents!=BSP29CONTENTS_EMPTY && !(surf->flags & BRUSH29_SURF_UNDERWATER)))
					&& !(surf->flags & BRUSH29_SURF_DONTWARP)) && ( (dot < 0) ^ !!(surf->flags & BRUSH29_SURF_PLANEBACK)) )
					continue;		// wrong side

				// if sorting by texture, just store it out
				if (r_texsort->value)
				{
					surf->texturechain = surf->texinfo->texture->texturechain;
					surf->texinfo->texture->texturechain = surf;
				} else if (surf->flags & BRUSH29_SURF_DRAWSKY) {
					surf->texturechain = skychain;
					skychain = surf;
				} else if (surf->flags & BRUSH29_SURF_DRAWTURB) {
					surf->texturechain = waterchain;
					waterchain = surf;
				} else
					R_DrawSequentialPoly (surf);

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
	int			i;

	VectorCopy (tr.viewParms.orient.origin, tr.orient.viewOrigin);

	tr.currentEntity = &tr.worldEntity;

	qglColor3f (1,1,1);
	Com_Memset(lightmap_polys, 0, sizeof(lightmap_polys));

	R_RecursiveWorldNode (tr.worldModel->brush29_nodes);

		DrawTextureChainsQ1 ();

	R_BlendLightmapsQ1 (false);
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mbrush29_node_t	*node;
	int		i;
	byte	solid[4096];

	if (r_oldviewleaf == r_viewleaf && !r_novis->value)
		return;
	
	tr.visCount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis->value)
	{
		vis = solid;
		Com_Memset(solid, 0xff, (tr.worldModel->brush29_numleafs+7)>>3);
	}
	else
		vis = Mod_LeafPVS (r_viewleaf, tr.worldModel);
		
	for (i=0 ; i<tr.worldModel->brush29_numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mbrush29_node_t *)&tr.worldModel->brush29_leafs[i+1];
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
