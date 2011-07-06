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
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "glquake.h"

int			skytexturenum;

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif


// For r_texsort 0
mbrush29_surface_t  *skychain = NULL;
mbrush29_surface_t  *waterchain = NULL;

void R_RenderDynamicLightmaps (mbrush29_surface_t *fa);

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
mbrush29_texture_t *R_TextureAnimation (mbrush29_texture_t *base)
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
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
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
	vec3_t		nv, dir;
	float		ss, ss2, length;
	float		s1, t1;
	glRect_t	*theRect;

	//
	// normal lightmaped poly
	//

	if (! (s->flags & (BRUSH29_SURF_DRAWSKY|BRUSH29_SURF_DRAWTURB|BRUSH29_SURF_UNDERWATER) ) )
	{
		R_RenderDynamicLightmaps (s);
		if (qglActiveTextureARB) {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
			// Binds world to texture env 0
			GL_SelectTexture(0);
			GL_Bind (t->gl_texture);
			GL_TexEnv(GL_REPLACE);
			// Binds lightmap to texenv 1
			GL_SelectTexture(1);
			qglEnable(GL_TEXTURE_2D);
			GL_Bind (tr.lightmaps[s->lightmaptexturenum]);
			i = s->lightmaptexturenum;
			if (lightmap_modified[i])
			{
				lightmap_modified[i] = false;
				theRect = &lightmap_rectchange[i];
				qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
					BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
					lightmaps+(i* BLOCK_HEIGHT + theRect->t) *BLOCK_WIDTH*4);
				theRect->l = BLOCK_WIDTH;
				theRect->t = BLOCK_HEIGHT;
				theRect->h = 0;
				theRect->w = 0;
			}
			GL_TexEnv(GL_MODULATE);
			qglBegin(GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
			{
				qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
				qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
				qglVertex3fv (v);
			}
			qglEnd ();
			qglDisable(GL_TEXTURE_2D);
			GL_SelectTexture(0);
			return;
		} else {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
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
		}

		return;
	}

	//
	// subdivided water surface warp
	//

	if (s->flags & BRUSH29_SURF_DRAWTURB)
	{
		GL_Bind (s->texinfo->texture->gl_texture);
		EmitWaterPolys (s);
		return;
	}

	//
	// subdivided sky warp
	//
	if (s->flags & BRUSH29_SURF_DRAWSKY)
	{
		GL_Bind (tr.solidskytexture);
		speedscale = realtime*8;
		speedscale -= (int)speedscale & ~127;

		EmitSkyPolys (s);

		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_Bind (tr.alphaskytexture);
		speedscale = realtime*16;
		speedscale -= (int)speedscale & ~127;
		EmitSkyPolys (s);

		GL_State(GLS_DEFAULT);
		return;
	}

	//
	// underwater warped with lightmap
	//
	R_RenderDynamicLightmaps (s);
	if (qglActiveTextureARB) {
		p = s->polys;

		t = R_TextureAnimation (s->texinfo->texture);
		GL_SelectTexture(0);
		GL_Bind (t->gl_texture);
		GL_TexEnv(GL_REPLACE);
		GL_SelectTexture(1);
		qglEnable(GL_TEXTURE_2D);
		GL_Bind (tr.lightmaps[s->lightmaptexturenum]);
		i = s->lightmaptexturenum;
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			theRect = &lightmap_rectchange[i];
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
				BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
				lightmaps+(i* BLOCK_HEIGHT + theRect->t) *BLOCK_WIDTH*4);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		GL_TexEnv(GL_MODULATE);
		qglBegin (GL_TRIANGLE_FAN);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
		{
			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);

			nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
			nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
			nv[2] = v[2];

			qglVertex3fv (nv);
		}
		qglEnd ();
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	} else {
		p = s->polys;

		t = R_TextureAnimation (s->texinfo->texture);
		GL_Bind (t->gl_texture);
		DrawGLWaterPoly (p);

		GL_Bind (tr.lightmaps[s->lightmaptexturenum]);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
		DrawGLWaterPolyLightmap (p);
		GL_State(GLS_DEFAULT);
	}
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
DrawGLPoly
================
*/
void DrawGLPoly (mbrush29_glpoly_t *p)
{
	int		i;
	float	*v;

	qglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f (v[3], v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
}


/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps (void)
{
	int			i, j;
	mbrush29_glpoly_t	*p;
	float		*v;
	glRect_t	*theRect;

#if 0
	if (r_fullbright.value)
		return;
#endif
	if (!r_texsort->value)
		return;

	if (!r_lightmap->value)
	{
		GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);		// don't bother writing Z
	}
	else
		GL_State(0);		// don't bother writing Z

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		p = lightmap_polys[i];
		if (!p)
			continue;
		GL_Bind(tr.lightmaps[i]);
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			theRect = &lightmap_rectchange[i];
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
				BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
				lightmaps+(i* BLOCK_HEIGHT + theRect->t) *BLOCK_WIDTH*4);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		for ( ; p ; p=p->chain)
		{
//			if (p->flags & BRUSH29_SURF_UNDERWATER)
//				DrawGLWaterPolyLightmap (p);
			if (((r_viewleaf->contents==BSP29CONTENTS_EMPTY && (p->flags & BRUSH29_SURF_UNDERWATER)) ||
				(r_viewleaf->contents!=BSP29CONTENTS_EMPTY && !(p->flags & BRUSH29_SURF_UNDERWATER)))
				&& !(p->flags & BRUSH29_SURF_DONTWARP))
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
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (mbrush29_surface_t *fa)
{
	mbrush29_texture_t	*t;
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;

	c_brush_polys++;

	if (fa->flags & BRUSH29_SURF_DRAWSKY)
	{	// warp texture, no lightmaps
		EmitBothSkyLayers (fa);
		return;
	}
		
	t = R_TextureAnimation (fa->texinfo->texture);
	GL_Bind (t->gl_texture);

	if (fa->flags & BRUSH29_SURF_DRAWTURB)
	{	// warp texture, no lightmaps
		EmitWaterPolys (fa);
		return;
	}

	if (((r_viewleaf->contents==BSP29CONTENTS_EMPTY && (fa->flags & BRUSH29_SURF_UNDERWATER)) ||
		(r_viewleaf->contents!=BSP29CONTENTS_EMPTY && !(fa->flags & BRUSH29_SURF_UNDERWATER)))
		&& !(fa->flags & BRUSH29_SURF_DONTWARP))
		DrawGLWaterPoly (fa->polys);
	else
		DrawGLPoly (fa->polys);

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
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*4*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1 (fa, base, BLOCK_WIDTH*4);
		}
	}
}

/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps (mbrush29_surface_t *fa)
{
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;

	c_brush_polys++;

	if (fa->flags & ( BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB) )
		return;
		
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
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*4*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1 (fa, base, BLOCK_WIDTH*4);
		}
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

	if (r_wateralpha->value == 1.0 && r_texsort->value)
		return;

	//
	// go back to the world matrix
	//

    qglLoadMatrixf (tr.viewParms.world.modelMatrix);

	if (r_wateralpha->value < 1.0) {
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglColor4f (1,1,1,r_wateralpha->value);
		GL_TexEnv(GL_MODULATE);
	}

	if (!r_texsort->value) {
		if (!waterchain)
			return;

		for ( s = waterchain ; s ; s=s->texturechain) {
			GL_Bind (s->texinfo->texture->gl_texture);
			EmitWaterPolys (s);
		}
		
		waterchain = NULL;
	} else {

		for (i=0 ; i<tr.worldModel->brush29_numtextures ; i++)
		{
			t = tr.worldModel->brush29_textures[i];
			if (!t)
				continue;
			s = t->texturechain;
			if (!s)
				continue;
			if ( !(s->flags & BRUSH29_SURF_DRAWTURB ) )
				continue;

			// set modulate mode explicitly
			
			GL_Bind (t->gl_texture);

			for ( ; s ; s=s->texturechain)
				EmitWaterPolys (s);
			
			t->texturechain = NULL;
		}

	}

	if (r_wateralpha->value < 1.0) {
		GL_TexEnv(GL_REPLACE);

		qglColor4f (1,1,1,1);
		GL_State(GLS_DEFAULT);
	}

}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	mbrush29_surface_t	*s;
	mbrush29_texture_t	*t;

	if (!r_texsort->value) {
		if (skychain) {
			R_DrawSkyChain(skychain);
			skychain = NULL;
		}

		return;
	} 

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
				R_RenderBrushPoly (s);
		}

		t->texturechain = NULL;
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (trRefEntity_t *e)
{
	int			i;
	int			k;
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
				R_RenderBrushPoly (psurf);
			else
				R_DrawSequentialPoly (psurf);
		}
	}

	R_BlendLightmaps ();

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
	int			c, side;
	cplane_t	*plane;
	mbrush29_surface_t	*surf, **mark;
	mbrush29_leaf_t		*pleaf;
	double		dot;

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
	VectorCopy (tr.viewParms.orient.origin, tr.orient.viewOrigin);

	tr.currentEntity = &tr.worldEntity;

	qglColor3f (1,1,1);
	Com_Memset(lightmap_polys, 0, sizeof(lightmap_polys));

	R_RecursiveWorldNode (tr.worldModel->brush29_nodes);

		DrawTextureChains ();

	R_BlendLightmaps ();
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
