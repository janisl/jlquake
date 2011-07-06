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
along with thisv program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "glquake.h"

int			skytexturenum;

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif


unsigned		blocklights_q1[18*18];

typedef struct glRect_s {
	unsigned char l,t,w,h;
} glRect_t;

mbrush29_glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
qboolean	lightmap_modified[MAX_LIGHTMAPS];
glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[4*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

// For gl_texsort 0
mbrush29_surface_t  *skychain = NULL;
mbrush29_surface_t  *waterchain = NULL;

void R_RenderDynamicLightmaps (mbrush29_surface_t *fa);

/*
===============
R_AddDynamicLightsQ1
===============
*/
void R_AddDynamicLightsQ1 (mbrush29_surface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mbrush29_texinfo_t	*tex;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<tr.refdef.num_dlights; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by thisv light

		rad = tr.refdef.dlights[lnum].radius;
		dist = DotProduct (tr.refdef.dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = 0;//tr.refdef.dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = tr.refdef.dlights[lnum].origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		
		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
					blocklights_q1[t*smax + s] += (rad - dist)*256;
			}
		}
	}
}


/*
===============
R_BuildLightMapQ1

Combine and scale multiple lightmaps into the 8.8 format in blocklights_q1
===============
*/
void R_BuildLightMapQ1 (mbrush29_surface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			t;
	int			i, j, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	int			lightadj[4];
	unsigned	*bl;

	surf->cached_dlight = (surf->dlightframe == tr.frameCount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (r_fullbright->value || !tr.worldModel->brush29_lightdata)
	{
		for (i=0 ; i<size ; i++)
			blocklights_q1[i] = 255*256;
		goto store;
	}

// clear to no light
	for (i=0 ; i<size ; i++)
		blocklights_q1[i] = 0;

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < BSP29_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			for (i=0 ; i<size ; i++)
				blocklights_q1[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}

// add all the dynamic lights
	if (surf->dlightframe == tr.frameCount)
		R_AddDynamicLightsQ1 (surf);

// bound, invert, and shift
store:
	stride -= (smax<<2);
	bl = blocklights_q1;
	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			t = *bl++;
			t >>= 7;
			if (t > 255)
				t = 255;
			dest[0] = t;
			dest[1] = t;
			dest[2] = t;
			dest += 4;
		}
	}
}


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
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
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

	if (r_fullbright->value)
		return;
	if (!gl_texsort->value)
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

	if (fa->flags & BRUSH29_SURF_UNDERWATER)
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

	if (fa->dlightframe == tr.frameCount	// dynamic thisv frame
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
	mbrush29_texture_t	*t;
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

	if (fa->dlightframe == tr.frameCount	// dynamic thisv frame
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

	if (r_wateralpha->value == 1.0 && gl_texsort->value)
		return;

	//
	// go back to the world matrix
	//

    qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	if (r_wateralpha->value < 1.0) {
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglColor4f (1,1,1,r_wateralpha->value);
		GL_TexEnv(GL_MODULATE);
	}

	if (!gl_texsort->value) {
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

	if (!gl_texsort->value) {
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
			if (gl_texsort->value)
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
				if (gl_texsort->value)
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

	R_RecursiveWorldNode(tr.worldModel->brush29_nodes);

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



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		bestx;
	int		texnum;

	for (texnum=0 ; texnum<MAX_LIGHTMAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
			}
			if (j == w)
			{	// thisv is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("AllocBlock: full");
}


mbrush29_vertex_t	*r_pcurrentvertbase;

int	nColinElim;

/*
================
BuildSurfaceDisplayList
================
*/
void BuildSurfaceDisplayList (mbrush29_surface_t *fa)
{
	int			i, lindex, lnumverts, s_axis, t_axis;
	float		dist, lastdist, lzi, scale, u, v, frac;
	unsigned	mask;
	vec3_t		local, transformed;
	mbrush29_edge_t		*pedges, *r_pedge;
	cplane_t	*pplane;
	int			vertpage, newverts, newpage, lastvert;
	qboolean	visible;
	float		*vec;
	float		s, t;
	mbrush29_glpoly_t	*poly;

// reconstruct the polygon
	pedges = tr.currentModel->brush29_edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	//
	// draw texture
	//
	poly = (mbrush29_glpoly_t*)Hunk_Alloc (sizeof(mbrush29_glpoly_t) + (lnumverts-4) * BRUSH29_VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = tr.currentModel->brush29_surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!gl_keeptjunctions->value && !(fa->flags & BRUSH29_SURF_UNDERWATER) )
	{
		for (i = 0 ; i < lnumverts ; ++i)
		{
			vec3_t v1, v2;
			float *prev, *thisv, *next;
			float f;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			thisv = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract( thisv, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((fabs( v1[0] - v2[0] ) <= COLINEAR_EPSILON) &&
				(fabs( v1[1] - v2[1] ) <= COLINEAR_EPSILON) && 
				(fabs( v1[2] - v2[2] ) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < BRUSH29_VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmapQ1
========================
*/
void GL_CreateSurfaceLightmapQ1 (mbrush29_surface_t *surf)
{
	int		smax, tmax, s, t, l, i;
	byte	*base;

	if (surf->flags & (BRUSH29_SURF_DRAWSKY|BRUSH29_SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum*4*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * 4;
	R_BuildLightMapQ1 (surf, base, BLOCK_WIDTH*4);
}


/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int		i, j;
	model_t	*m;

	Com_Memset(allocated, 0, sizeof(allocated));

	tr.frameCount = 1;		// no dlightcache

	if (!tr.lightmaps[0])
	{
		for (int i = 0; i < MAX_LIGHTMAPS; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*4, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false);
		}
	}

	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = R_GetModelByHandle(cl.model_precache[j]);
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->brush29_vertexes;
		tr.currentModel = m;
		for (i=0 ; i<m->brush29_numsurfaces ; i++)
		{
			GL_CreateSurfaceLightmapQ1 (m->brush29_surfaces + i);
			if ( m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWTURB )
				continue;
			if ( m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWSKY )
				continue;
			BuildSurfaceDisplayList (m->brush29_surfaces + i);
		}
	}

 	if (!gl_texsort->value && qglActiveTextureARB)
 		GL_SelectTexture(1);

	//
	// upload all lightmaps that were filled
	//
	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		R_ReUploadImage(tr.lightmaps[i], lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*4);
	}

 	if (!gl_texsort->value && qglActiveTextureARB)
 		GL_SelectTexture(0);

}

