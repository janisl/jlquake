// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "glquake.h"

int			skytexturenum;

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif


qboolean mtexenabled = false;

/*
===============
R_TextureAnimationQ1

Returns the proper texture for a given time and base texture
===============
*/
mbrush29_texture_t *R_TextureAnimationQ1 (mbrush29_texture_t *base)
{
	int		reletive;
	int		count;

	if (tr.currentEntity->e.frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}
	
	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;	
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimationQ1: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimationQ1: infinite cycle");
	}

	return base;
}


/*
=============================================================

	BRUSH MODELS

=============================================================
*/

void DrawGLWaterPoly (mbrush29_glpoly_t *p);
void DrawGLWaterPolyLightmap (mbrush29_glpoly_t *p);

/*
================
R_DrawSequentialPoly

Systems that have fast state and texture changes can
just do everything as it passes with no need to sort
================
*/
void R_DrawSequentialPoly (mbrush29_surface_t *s)
{
	mbrush29_glpoly_t	*p;
	float		*v;
	int			i;
	mbrush29_texture_t	*t;
	float		alpha_val = 1.0f;
	float		intensity = 1.0f;
	
	//
	// normal lightmaped poly
	//
	if (! (s->flags & (BRUSH29_SURF_DRAWSKY|BRUSH29_SURF_DRAWTURB|BRUSH29_SURF_UNDERWATER) ) )
	{
		if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
		{
			GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
//			qglColor4f (1,1,1,r_wateralpha.value);
			alpha_val = r_wateralpha->value;

			GL_TexEnv(GL_MODULATE);
			intensity = 1;
			// rjr
		}
		if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
		{
			// tr.currentEntity->abslight   0 - 255
			// rjr
			GL_TexEnv(GL_MODULATE);
			intensity = tr.currentEntity->e.radius;
//			intensity = 0;
		}

		qglColor4f( intensity, intensity, intensity, alpha_val );
		
		p = s->polys;

		t = R_TextureAnimationQ1 (s->texinfo->texture);
		GL_Bind (t->gl_texture);
		qglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
		{
			qglTexCoord2f (v[3], v[4]);
			qglVertex3fv (v);
		}
		qglEnd ();

		GL_Bind (tr.lightmaps[s->lightmaptexturenum]);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
		qglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
		{
			qglTexCoord2f (v[5], v[6]);
			qglVertex3fv (v);
		}
		qglEnd ();

		GL_State(GLS_DEFAULT);

		if ((tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT) ||
			(tr.currentEntity->e.renderfx & RF_WATERTRANS))
		{
			GL_TexEnv(GL_REPLACE);
		}
		return;
	}

	//
	// subdivided water surface warp
	//
	if (s->flags & BRUSH29_SURF_DRAWTURB)
	{
		GL_Bind (s->texinfo->texture->gl_texture);
		EmitWaterPolysQ1 (s);
		return;
	}

	//
	// subdivided sky warp
	//
	if (s->flags & BRUSH29_SURF_DRAWSKY)
	{
		GL_Bind (tr.solidskytexture);
		speedscale = realtime*8;
		speedscale -= (int)speedscale;

		EmitSkyPolys (s);

		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_Bind (tr.alphaskytexture);
		speedscale = realtime*16;
		speedscale -= (int)speedscale;
		EmitSkyPolys (s);

		GL_State(GLS_DEFAULT);
	}

	//
	// underwater warped with lightmap
	//
	p = s->polys;

	t = R_TextureAnimationQ1 (s->texinfo->texture);
	GL_Bind (t->gl_texture);
	DrawGLWaterPoly (p);

	GL_Bind (tr.lightmaps[s->lightmaptexturenum]);
	GL_State(GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
	DrawGLWaterPolyLightmap (p);
	GL_State(GLS_DEFAULT);
}


/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly (mbrush29_glpoly_t *p)
{
	int		i;
	float	*v;
	float	s, t, os, ot;
	vec3_t	nv;

	qglBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f (v[3], v[4]);

		nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[2] = v[2];

		qglVertex3fv (nv);
	}
	qglEnd ();
}
void DrawGLWaterPolyLightmap (mbrush29_glpoly_t *p)
{
	int		i;
	float	*v;
	float	s, t, os, ot;
	vec3_t	nv;

	qglBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f (v[5], v[6]);

		nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[2] = v[2];

		qglVertex3fv (nv);
	}
	qglEnd ();
}

/*
================
DrawGLPolyQ1
================
*/
void DrawGLPolyQ1 (mbrush29_glpoly_t *p)
{
	int		i;
	float	*v;

	// hack
	qglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
	{
		// hack
//		qglColor3f( 1.0f, 0.0f, 0.0f );
		qglTexCoord2f (v[3], v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
}


/*
================
R_BlendLightmapsQ1
================
*/
void R_BlendLightmapsQ1 (qboolean Translucent)
{
	int			i, j;
	mbrush29_glpoly_t	*p;
	float		*v;

	if (r_fullbright->value)
		return;
	if (!r_texsort->value)
		return;

	int NewState = GLS_DEFAULT;
	if (!Translucent)
		NewState = 0;		// don't bother writing Z

	if (!r_lightmap->value)
	{
		NewState |= GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR;
	}
	GL_State(NewState);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		p = lightmap_polys[i];
		if (!p)
			continue;
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			R_ReUploadImage(tr.lightmaps[i], lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*4);
		}
		GL_Bind(tr.lightmaps[i]);
		for ( ; p ; p=p->chain)
		{
			if (p->flags & BRUSH29_SURF_UNDERWATER)
				DrawGLWaterPolyLightmap (p);
			else
			{
				qglBegin (GL_POLYGON);
				v = p->verts[0];
				for (j=0 ; j<p->numverts ; j++, v+= BRUSH29_VERTEXSIZE)
				{
					qglTexCoord2f (v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
		}
	}

	GL_State(GLS_DEPTHMASK_TRUE);		// back to normal Z buffering
}

/*
================
R_RenderBrushPolyQ1
================
*/
void R_RenderBrushPolyQ1 (mbrush29_surface_t *fa, qboolean override)
{
	mbrush29_texture_t	*t;
	byte		*base;
	int			maps;
	float		intensity = 1.0f, alpha_val = 1.0f;

	c_brush_polys++;

	if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
	{
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		//			qglColor4f (1,1,1,r_wateralpha.value);
		alpha_val = r_wateralpha->value;
		// rjr

		GL_TexEnv(GL_MODULATE);
		intensity = 1.0;

	}
	if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		// tr.currentEntity->abslight   0 - 255
		// rjr
		GL_TexEnv(GL_MODULATE);
		intensity = tr.currentEntity->e.radius;
//		intensity = 0;
	}
	
	if (!override)
		qglColor4f( intensity, intensity, intensity, alpha_val );
		
	if (fa->flags & BRUSH29_SURF_DRAWSKY)
	{	// warp texture, no lightmaps
		EmitBothSkyLayers (fa);
		return;
	}
		
	t = R_TextureAnimationQ1 (fa->texinfo->texture);
	GL_Bind (t->gl_texture);

	if (fa->flags & BRUSH29_SURF_DRAWTURB)
	{	// warp texture, no lightmaps
		EmitWaterPolysQ1 (fa);
		return;
	}

	if (fa->flags & BRUSH29_SURF_UNDERWATER)
		DrawGLWaterPoly (fa->polys);
	else
		DrawGLPolyQ1 (fa->polys);

	// add the poly to the proper lightmap chain

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (maps = 0 ; maps < BSP29_MAXLIGHTMAPS && fa->styles[maps] != 255 ;
		 maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == tr.frameCount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (r_dynamic->value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			base = lightmaps + fa->lightmaptexturenum*4*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1 (fa, base, BLOCK_WIDTH*4);
		}
	}
	if ((tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT) ||
	    (tr.currentEntity->e.renderfx & RF_WATERTRANS))
	{
		GL_TexEnv(GL_REPLACE);
	}

	if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
	{
		GL_State(GLS_DEFAULT);
	}
}

/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces (void)
{
	int			i;
	mbrush29_surface_t	*s;
	mbrush29_texture_t	*t;

	if (r_wateralpha->value == 1.0)
		return;

	//
	// go back to the world matrix
	//
    qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv(GL_MODULATE);

	for (i=0 ; i<tr.worldModel->brush29_numtextures ; i++)
	{
		t = tr.worldModel->brush29_textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		if ( !(s->flags & BRUSH29_SURF_DRAWTURB) )
			continue;

		if (s->flags & BRUSH29_SURF_TRANSLUCENT)
			qglColor4f (1,1,1,r_wateralpha->value);
		else
			qglColor4f (1,1,1,1);

		// set modulate mode explicitly
		GL_Bind (t->gl_texture);

		for ( ; s ; s=s->texturechain)
			R_RenderBrushPolyQ1 (s, true);

		t->texturechain = NULL;
	}

	GL_TexEnv(GL_REPLACE);

	qglColor4f (1,1,1,1);
	GL_State(GLS_DEFAULT);
}

/*
================
DrawTextureChainsQ1
================
*/
void DrawTextureChainsQ1 (void)
{
	int		i;
	mbrush29_surface_t	*s;
	mbrush29_texture_t	*t;

	for (i=0 ; i<tr.worldModel->brush29_numtextures ; i++)
	{
		t = tr.worldModel->brush29_textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		if (i == skytexturenum)
			R_DrawSkyChain (s);
		else
		{
			if ((s->flags & BRUSH29_SURF_DRAWTURB) && r_wateralpha->value != 1.0)
				continue;	// draw translucent water later
			for ( ; s ; s=s->texturechain)
				R_RenderBrushPolyQ1 (s, false);
		}

		t->texturechain = NULL;
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (trRefEntity_t *e, qboolean Translucent)
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

	if (!Translucent && !(tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT))
		R_BlendLightmapsQ1(Translucent);

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
				if ( !(surf->flags & BRUSH29_SURF_UNDERWATER) && ( (dot < 0) ^ !!(surf->flags & BRUSH29_SURF_PLANEBACK)) )
					continue;		// wrong side

				// if sorting by texture, just store it out
				if (r_texsort->value)
				{
					surf->texturechain = surf->texinfo->texture->texturechain;
					surf->texinfo->texture->texturechain = surf;
				}
				else
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

	if (r_texsort->value)
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
