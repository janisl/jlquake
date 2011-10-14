//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
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

mbrush29_glpoly_t*	lightmap_polys[MAX_LIGHTMAPS];

mbrush29_leaf_t*			r_viewleaf;
mbrush29_leaf_t*			r_oldviewleaf;

// For r_texsort 0
mbrush29_surface_t  *skychain = NULL;
mbrush29_surface_t  *waterchain = NULL;

int			skytexturenum;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int					allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];
static bool					lightmap_modified[MAX_LIGHTMAPS];
static glRect_t				lightmap_rectchange[MAX_LIGHTMAPS];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
static byte					lightmaps[4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];

static unsigned				blocklights_q1[18 * 18];

static mbrush29_vertex_t*	r_pcurrentvertbase;

// CODE --------------------------------------------------------------------

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

//==========================================================================
//
//	AllocBlock
//
// returns a texture number and the position inside it
//
//==========================================================================

static int AllocBlock(int w, int h, int* x, int* y)
{
	for (int texnum = 0; texnum < MAX_LIGHTMAPS; texnum++)
	{
		int best = BLOCK_HEIGHT;

		for (int i = 0; i < BLOCK_WIDTH - w; i++)
		{
			int best2 = 0;

			int j;
			for (j = 0; j < w; j++)
			{
				if (allocated[texnum][i + j] >= best)
				{
					break;
				}
				if (allocated[texnum][i + j] > best2)
				{
					best2 = allocated[texnum][i + j];
				}
			}
			if (j == w)
			{
				// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
		{
			continue;
		}

		for (int i = 0; i < w; i++)
		{
			allocated[texnum][*x + i] = best + h;
		}

		return texnum;
	}

	throw Exception("AllocBlock: full");
}

//==========================================================================
//
//	R_AddDynamicLightsQ1
//
//==========================================================================

static void R_AddDynamicLightsQ1(mbrush29_surface_t* surf)
{
	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	mbrush29_texinfo_t* tex = surf->texinfo;

	for (int lnum = 0; lnum < tr.refdef.num_dlights; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
		{
			continue;		// not lit by this light
		}

		float rad = tr.refdef.dlights[lnum].radius;
		float dist = DotProduct(tr.refdef.dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
		rad -= Q_fabs(dist);
		float minlight = 0;//tr.refdef.dlights[lnum].minlight;
		if (rad < minlight)
		{
			continue;
		}
		minlight = rad - minlight;

		vec3_t impact;
		for (int i = 0; i < 3; i++)
		{
			impact[i] = tr.refdef.dlights[lnum].origin[i] - surf->plane->normal[i] * dist;
		}

		vec3_t local;
		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		
		for (int t = 0; t < tmax; t++)
		{
			int td = local[1] - t * 16;
			if (td < 0)
			{
				td = -td;
			}
			for (int s = 0; s < smax; s++)
			{
				int sd = local[0] - s * 16;
				if (sd < 0)
				{
					sd = -sd;
				}
				if (sd > td)
				{
					dist = sd + (td >> 1);
				}
				else
				{
					dist = td + (sd >> 1);
				}
				if (dist < minlight)
				{
					blocklights_q1[t * smax + s] += (rad - dist) * 256;
				}
			}
		}
	}
}

//==========================================================================
//
//	R_BuildLightMapQ1
//
//	Combine and scale multiple lightmaps into the 8.8 format in blocklights_q1
//
//==========================================================================

static void R_BuildLightMapQ1(mbrush29_surface_t* surf, byte* dest, int stride)
{
	int			smax, tmax;
	int			t;
	int			i, j, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	unsigned	*bl;

	surf->cached_dlight = (surf->dlightframe == tr.frameCount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (!tr.worldModel->brush29_lightdata)
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
			scale = tr.refdef.lightstyles[surf->styles[maps]].rgb[0] * 256;
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

//==========================================================================
//
//	GL_CreateSurfaceLightmapQ1
//
//==========================================================================

static void GL_CreateSurfaceLightmapQ1(mbrush29_surface_t* surf)
{
	if (surf->flags & (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB))
	{
		return;
	}

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum = AllocBlock(smax, tmax, &surf->light_s, &surf->light_t);
	byte* base = lightmaps + surf->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * 4;
	R_BuildLightMapQ1(surf, base, BLOCK_WIDTH * 4);
}

//==========================================================================
//
//	BuildSurfaceDisplayList
//
//==========================================================================

static void BuildSurfaceDisplayList(mbrush29_surface_t* fa)
{
	// reconstruct the polygon
	mbrush29_edge_t* pedges = tr.currentModel->brush29_edges;
	int lnumverts = fa->numedges;

	//
	// draw texture
	//
	mbrush29_glpoly_t* poly = (mbrush29_glpoly_t*)Mem_Alloc(sizeof(mbrush29_glpoly_t) + (lnumverts - 4) * BRUSH29_VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	poly->chain = NULL;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (int i = 0; i < lnumverts; i++)
	{
		int lindex = tr.currentModel->brush29_surfedges[fa->firstedge + i];

		float* vec;
		if (lindex > 0)
		{
			mbrush29_edge_t* r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			mbrush29_edge_t* r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		float s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		float t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!r_keeptjunctions->value && !(fa->flags & BRUSH29_SURF_UNDERWATER))
	{
		for (int i = 0; i < lnumverts; ++i)
		{
			vec3_t v1, v2;
			float *prev, *thisv, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			thisv = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract(thisv, prev, v1);
			VectorNormalize(v1);
			VectorSubtract(next, prev, v2);
			VectorNormalize(v2);

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((Q_fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
				(Q_fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) && 
				(Q_fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
			{
				for (int j = i + 1; j < lnumverts; ++j)
				{
					for (int k = 0; k < BRUSH29_VERTEXSIZE; ++k)
					{
						poly->verts[j - 1][k] = poly->verts[j][k];
					}
				}
				--lnumverts;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;
}

//==========================================================================
//
//	GL_BuildLightmaps
//
//	Builds the lightmap texture with all the surfaces from all brush models
//
//==========================================================================

void GL_BuildLightmaps()
{
	Com_Memset(allocated, 0, sizeof(allocated));

	tr.frameCount = 1;		// no dlightcache

	for (int i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		backEndData[tr.smpFrame]->lightstyles[i].rgb[0] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[1] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[2] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].white = 3;
	}
	tr.refdef.lightstyles = backEndData[tr.smpFrame]->lightstyles;

	if (!tr.lightmaps[0])
	{
		for (int i = 0; i < MAX_LIGHTMAPS; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*4, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false);
		}
	}

	for (int j = 1; j < MAX_MOD_KNOWN; j++)
	{
		model_t* m = tr.models[j];
		if (!m)
		{
			break;
		}
		if (m->type != MOD_BRUSH29)
		{
			continue;
		}
		if (m->name[0] == '*')
		{
			continue;
		}
		r_pcurrentvertbase = m->brush29_vertexes;
		tr.currentModel = m;
		for (int i = 0; i < m->brush29_numsurfaces; i++)
		{
			GL_CreateSurfaceLightmapQ1(m->brush29_surfaces + i);
			if (m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWTURB)
			{
				continue;
			}
			if (m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWSKY)
			{
				continue;
			}
			BuildSurfaceDisplayList(m->brush29_surfaces + i);
		}
	}

	//
	// upload all lightmaps that were filled
	//
	for (int i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (!allocated[i][0])
		{
			break;		// no more used
		}
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		R_ReUploadImage(tr.lightmaps[i], lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * 4);
	}
}

//==========================================================================
//
//	R_TextureAnimationQ1
//
//	Returns the proper texture for a given time and base texture
//
//==========================================================================

static mbrush29_texture_t* R_TextureAnimationQ1(mbrush29_texture_t* base)
{
	if (tr.currentEntity->e.frame)
	{
		if (base->alternate_anims)
		{
			base = base->alternate_anims;
		}
	}
	
	if (!base->anim_total)
	{
		return base;
	}

	int reletive = (int)(tr.refdef.floatTime * 10) % base->anim_total;

	int count = 0;	
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
		{
			throw Exception("R_TextureAnimationQ1: broken cycle");
		}
		if (++count > 100)
		{
			throw Exception("R_TextureAnimationQ1: infinite cycle");
		}
	}

	return base;
}

//==========================================================================
//
//	R_RenderDynamicLightmaps
//
//	Multitexture
//
//==========================================================================

static void R_RenderDynamicLightmaps(mbrush29_surface_t* fa)
{
	c_brush_polys++;

	if (fa->flags & (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB))
	{
		return;
	}

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (int maps = 0; maps < BSP29_MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (tr.refdef.lightstyles[fa->styles[maps]].rgb[0] * 256 != fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	if (fa->dlightframe == tr.frameCount	// dynamic this frame
		|| fa->cached_dlight)				// dynamic previously
	{
dynamic:
		if (r_dynamic->value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			glRect_t* theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t)
			{
				if (theRect->h)
				{
					theRect->h += theRect->t - fa->light_t;
				}
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l)
			{
				if (theRect->w)
				{
					theRect->w += theRect->l - fa->light_s;
				}
				theRect->l = fa->light_s;
			}
			int smax = (fa->extents[0] >> 4) + 1;
			int tmax = (fa->extents[1] >> 4) + 1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
			{
				theRect->w = (fa->light_s - theRect->l) + smax;
			}
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
			{
				theRect->h = (fa->light_t - theRect->t) + tmax;
			}
			byte* base = lightmaps + fa->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1(fa, base, BLOCK_WIDTH * 4);
		}
	}
}

//==========================================================================
//
//	DrawGLPolyQ1
//
//==========================================================================

static void DrawGLPolyQ1(mbrush29_glpoly_t* p)
{
	qglBegin(GL_POLYGON);
	float* v = p->verts[0];
	for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);
		qglVertex3fv(v);
	}
	qglEnd();
}

//==========================================================================
//
//	DrawGLWaterPoly
//
//	Warp the vertex coordinates
//
//==========================================================================

static void DrawGLWaterPoly(mbrush29_glpoly_t* p)
{
	qglBegin(GL_TRIANGLE_FAN);
	float* v = p->verts[0];
	for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);

		vec3_t nv;
		nv[0] = v[0] + 8 * sin(v[1] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
		nv[1] = v[1] + 8 * sin(v[0] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
		nv[2] = v[2];

		qglVertex3fv(nv);
	}
	qglEnd();
}

//==========================================================================
//
//	DrawGLWaterPolyLightmap
//
//==========================================================================

static void DrawGLWaterPolyLightmap(mbrush29_glpoly_t* p)
{
	qglBegin(GL_TRIANGLE_FAN);
	float* v = p->verts[0];
	for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
	{
		qglTexCoord2f(v[5], v[6]);

		vec3_t nv;
		nv[0] = v[0] + 8 * sin(v[1] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
		nv[1] = v[1] + 8 * sin(v[0] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
		nv[2] = v[2];

		qglVertex3fv(nv);
	}
	qglEnd();
}

//==========================================================================
//
//	EmitWaterPolysQ1
//
//	Does a water warp on the pre-fragmented mbrush29_glpoly_t chain
//
//==========================================================================

void EmitWaterPolysQ1(mbrush29_surface_t* fa)
{
	for (mbrush29_glpoly_t* p = fa->polys; p; p = p->next)
	{
		qglBegin(GL_POLYGON);
		float* v = p->verts[0];
		for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
		{
			float os = v[3];
			float ot = v[4];

			float s = os + r_turbsin[(int)((ot * 0.125 + tr.refdef.floatTime) * TURBSCALE) & 255];
			s *= (1.0 / 64);

			float t = ot + r_turbsin[(int)((os * 0.125 + tr.refdef.floatTime) * TURBSCALE) & 255];
			t *= (1.0 / 64);

			qglTexCoord2f(s, t);
			qglVertex3fv(v);
		}
		qglEnd();
	}
}

//==========================================================================
//
//	R_RenderBrushPolyQ1
//
//==========================================================================

void R_RenderBrushPolyQ1(mbrush29_surface_t* fa, bool override)
{
	c_brush_polys++;

	float intensity = 1.0f, alpha_val = 1.0f;
	if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
	{
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		alpha_val = r_wateralpha->value;

		GL_TexEnv(GL_MODULATE);
		intensity = 1.0;
	}

	if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		// tr.currentEntity->abslight   0 - 255
		GL_TexEnv(GL_MODULATE);
		intensity = tr.currentEntity->e.radius;
	}
	
	if (!override)
	{
		qglColor4f(intensity, intensity, intensity, alpha_val);
	}
		
	if (fa->flags & BRUSH29_SURF_DRAWSKY)
	{
		// warp texture, no lightmaps
		EmitBothSkyLayers(fa);
		return;
	}
		
	mbrush29_texture_t* t = R_TextureAnimationQ1(fa->texinfo->texture);
	GL_Bind(t->gl_texture);

	if (fa->flags & BRUSH29_SURF_DRAWTURB)
	{
		// warp texture, no lightmaps
		EmitWaterPolysQ1(fa);
		return;
	}

	if (((r_viewleaf->contents==BSP29CONTENTS_EMPTY && (fa->flags & BRUSH29_SURF_UNDERWATER)) ||
		(r_viewleaf->contents!=BSP29CONTENTS_EMPTY && !(fa->flags & BRUSH29_SURF_UNDERWATER)))
		&& !(fa->flags & BRUSH29_SURF_DONTWARP))
	{
		DrawGLWaterPoly(fa->polys);
	}
	else
	{
		DrawGLPolyQ1(fa->polys);
	}

	// add the poly to the proper lightmap chain

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (int maps = 0; maps < BSP29_MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (tr.refdef.lightstyles[fa->styles[maps]].rgb[0] * 256 != fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	if (fa->dlightframe == tr.frameCount	// dynamic this frame
		|| fa->cached_dlight)				// dynamic previously
	{
dynamic:
		if (r_dynamic->value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			glRect_t* theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t)
			{
				if (theRect->h)
				{
					theRect->h += theRect->t - fa->light_t;
				}
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l)
			{
				if (theRect->w)
				{
					theRect->w += theRect->l - fa->light_s;
				}
				theRect->l = fa->light_s;
			}
			int smax = (fa->extents[0] >> 4) + 1;
			int tmax = (fa->extents[1] >> 4) + 1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
			{
				theRect->w = (fa->light_s - theRect->l) + smax;
			}
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
			{
				theRect->h = (fa->light_t - theRect->t) + tmax;
			}
			byte* base = lightmaps + fa->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1(fa, base, BLOCK_WIDTH * 4);
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

//==========================================================================
//
//	R_DrawSequentialPoly
//
//	Systems that have fast state and texture changes can just do everything
// as it passes with no need to sort
//
//==========================================================================

void R_DrawSequentialPoly(mbrush29_surface_t* s)
{
	//
	// normal lightmaped poly
	//
	if (!(s->flags & (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_UNDERWATER)))
	{
		R_RenderDynamicLightmaps(s);
		if (qglActiveTextureARB  && !(tr.currentEntity->e.renderfx & RF_WATERTRANS) &&
			!(tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT))
		{
			mbrush29_glpoly_t* p = s->polys;

			mbrush29_texture_t* t = R_TextureAnimationQ1(s->texinfo->texture);
			// Binds world to texture env 0
			GL_SelectTexture(0);
			GL_Bind(t->gl_texture);
			GL_TexEnv(GL_REPLACE);
			// Binds lightmap to texenv 1
			GL_SelectTexture(1);
			qglEnable(GL_TEXTURE_2D);
			GL_Bind(tr.lightmaps[s->lightmaptexturenum]);
			int i = s->lightmaptexturenum;
			if (lightmap_modified[i])
			{
				lightmap_modified[i] = false;
				glRect_t* theRect = &lightmap_rectchange[i];
				qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
					BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
					lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * 4);
				theRect->l = BLOCK_WIDTH;
				theRect->t = BLOCK_HEIGHT;
				theRect->h = 0;
				theRect->w = 0;
			}
			GL_TexEnv(GL_MODULATE);
			qglBegin(GL_POLYGON);
			float* v = p->verts[0];
			for (i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
			{
				qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
				qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
				qglVertex3fv(v);
			}
			qglEnd();
			qglDisable(GL_TEXTURE_2D);
			GL_SelectTexture(0);
			return;
		}
		else
		{
			float alpha_val = 1.0f;
			float intensity = 1.0f;
			if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
			{
				GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
				alpha_val = r_wateralpha->value;

				GL_TexEnv(GL_MODULATE);
				intensity = 1;
			}
			if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
			{
				// tr.currentEntity->abslight   0 - 255
				GL_TexEnv(GL_MODULATE);
				intensity = tr.currentEntity->e.radius;
			}

			qglColor4f(intensity, intensity, intensity, alpha_val);

			mbrush29_glpoly_t* p = s->polys;

			mbrush29_texture_t* t = R_TextureAnimationQ1(s->texinfo->texture);
			GL_Bind(t->gl_texture);
			qglBegin(GL_POLYGON);
			float* v = p->verts[0];
			for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
			{
				qglTexCoord2f(v[3], v[4]);
				qglVertex3fv(v);
			}
			qglEnd();

			if (!(tr.currentEntity->e.renderfx & RF_WATERTRANS) &&
				!(tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT))
			{
				GL_Bind(tr.lightmaps[s->lightmaptexturenum]);
				GL_State(GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
				qglBegin(GL_POLYGON);
				v = p->verts[0];
				for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
				{
					qglTexCoord2f(v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}

			GL_State(GLS_DEFAULT);

			if ((tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT) ||
				(tr.currentEntity->e.renderfx & RF_WATERTRANS))
			{
				GL_TexEnv(GL_REPLACE);
				qglColor4f(1, 1, 1, 1);
			}
		}
		return;
	}

	//
	// subdivided water surface warp
	//
	if (s->flags & BRUSH29_SURF_DRAWTURB)
	{
		GL_Bind(s->texinfo->texture->gl_texture);
		EmitWaterPolysQ1(s);
		return;
	}

	//
	// subdivided sky warp
	//
	if (s->flags & BRUSH29_SURF_DRAWSKY)
	{
		GL_Bind(tr.solidskytexture);
		speedscale = tr.refdef.floatTime * 8;
		speedscale -= (int)speedscale & ~127;

		EmitSkyPolys(s);

		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_Bind(tr.alphaskytexture);
		speedscale = tr.refdef.floatTime * 16;
		speedscale -= (int)speedscale & ~127;
		EmitSkyPolys(s);

		GL_State(GLS_DEFAULT);
		return;
	}

	//
	// underwater warped with lightmap
	//
	R_RenderDynamicLightmaps(s);
	if (qglActiveTextureARB)
	{
		mbrush29_glpoly_t* p = s->polys;

		mbrush29_texture_t* t = R_TextureAnimationQ1(s->texinfo->texture);
		GL_SelectTexture(0);
		GL_Bind(t->gl_texture);
		GL_TexEnv(GL_REPLACE);
		GL_SelectTexture(1);
		qglEnable(GL_TEXTURE_2D);
		GL_Bind(tr.lightmaps[s->lightmaptexturenum]);
		int i = s->lightmaptexturenum;
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			glRect_t* theRect = &lightmap_rectchange[i];
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
				BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
				lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * 4);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		GL_TexEnv(GL_MODULATE);
		qglBegin(GL_TRIANGLE_FAN);
		float* v = p->verts[0];
		for (i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
		{
			qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
			qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);

			vec3_t nv;
			nv[0] = v[0] + 8 * sin(v[1] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
			nv[1] = v[1] + 8 * sin(v[0] * 0.05 + tr.refdef.floatTime) * sin(v[2] * 0.05 + tr.refdef.floatTime);
			nv[2] = v[2];

			qglVertex3fv(nv);
		}
		qglEnd();
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}
	else
	{
		mbrush29_glpoly_t* p = s->polys;

		mbrush29_texture_t* t = R_TextureAnimationQ1(s->texinfo->texture);
		GL_Bind(t->gl_texture);
		DrawGLWaterPoly(p);

		GL_Bind(tr.lightmaps[s->lightmaptexturenum]);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
		DrawGLWaterPolyLightmap(p);
		GL_State(GLS_DEFAULT);
	}
}

//==========================================================================
//
//	R_BlendLightmapsQ1
//
//==========================================================================

void R_BlendLightmapsQ1()
{
	if (r_fullbright->value)
	{
		return;
	}
	if (!r_texsort->value)
	{
		return;
	}

	int NewState = 0;		// don't bother writing Z
	if (!r_lightmap->value)
	{
		NewState |= GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR;
	}
	GL_State(NewState);

	for (int i = 0; i < MAX_LIGHTMAPS; i++)
	{
		mbrush29_glpoly_t* p = lightmap_polys[i];
		if (!p)
		{
			continue;
		}
		GL_Bind(tr.lightmaps[i]);
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			glRect_t* theRect = &lightmap_rectchange[i];
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, theRect->t, 
				BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
				lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * 4);
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}
		for (; p; p = p->chain)
		{
			if (((r_viewleaf->contents == BSP29CONTENTS_EMPTY && (p->flags & BRUSH29_SURF_UNDERWATER)) ||
				(r_viewleaf->contents != BSP29CONTENTS_EMPTY && !(p->flags & BRUSH29_SURF_UNDERWATER)))
				&& !(p->flags & BRUSH29_SURF_DONTWARP))
			{
				DrawGLWaterPolyLightmap(p);
			}
			else
			{
				qglBegin(GL_POLYGON);
				float* v = p->verts[0];
				for (int j = 0; j < p->numverts; j++, v += BRUSH29_VERTEXSIZE)
				{
					qglTexCoord2f(v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
	}

	GL_State(GLS_DEPTHMASK_TRUE);		// back to normal Z buffering
}

//==========================================================================
//
//	DrawTextureChainsQ1
//
//==========================================================================

void DrawTextureChainsQ1()
{
	if (!r_texsort->value)
	{
		if (skychain)
		{
			R_DrawSkyChain(skychain);
			skychain = NULL;
		}

		return;
	} 

	for (int i = 0; i < tr.worldModel->brush29_numtextures; i++)
	{
		mbrush29_texture_t* t = tr.worldModel->brush29_textures[i];
		if (!t)
		{
			continue;
		}
		mbrush29_surface_t* s = t->texturechain;
		if (!s)
		{
			continue;
		}
		if (i == skytexturenum)
		{
			R_DrawSkyChain(s);
		}
		else
		{
			if ((s->flags & BRUSH29_SURF_DRAWTURB) && r_wateralpha->value != 1.0)
			{
				continue;	// draw translucent water later
			}

			if (s->flags & BRUSH29_SURF_TRANSLUCENT)
			{
				qglColor4f(1, 1, 1, r_wateralpha->value);
			}
			else
			{
				qglColor4f(1, 1, 1, 1);
			}

			for (; s; s = s->texturechain)
			{
				R_RenderBrushPolyQ1(s, true);
			}
		}

		t->texturechain = NULL;
	}
}

//==========================================================================
//
//	R_DrawWaterSurfaces
//
//==========================================================================

void R_DrawWaterSurfaces()
{
	if (r_wateralpha->value == 1.0 && r_texsort->value)
	{
		return;
	}

	//
	// go back to the world matrix
	//
	qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	if (r_wateralpha->value < 1.0)
	{
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_TexEnv(GL_MODULATE);
	}

	if (!r_texsort->value)
	{
		if (!waterchain)
		{
			return;
		}

		for (mbrush29_surface_t* s = waterchain; s; s = s->texturechain)
		{
			if ((GGameType & GAME_Quake) || (s->flags & BRUSH29_SURF_TRANSLUCENT))
			{
				qglColor4f(1, 1, 1, r_wateralpha->value);
			}
			else
			{
				qglColor4f(1, 1, 1, 1);
			}

			GL_Bind(s->texinfo->texture->gl_texture);
			EmitWaterPolysQ1(s);
		}
		
		waterchain = NULL;
	}
	else
	{
		for (int i = 0; i < tr.worldModel->brush29_numtextures; i++)
		{
			mbrush29_texture_t* t = tr.worldModel->brush29_textures[i];
			if (!t)
			{
				continue;
			}
			mbrush29_surface_t* s = t->texturechain;
			if (!s)
			{
				continue;
			}
			if (!(s->flags & BRUSH29_SURF_DRAWTURB))
			{
				continue;
			}

			if ((GGameType & GAME_Quake) || (s->flags & BRUSH29_SURF_TRANSLUCENT))
			{
				qglColor4f(1, 1, 1, r_wateralpha->value);
			}
			else
			{
				qglColor4f(1, 1, 1, 1);
			}

			// set modulate mode explicitly
			GL_Bind(t->gl_texture);

			for (; s; s = s->texturechain)
			{
				EmitWaterPolysQ1(s);
			}

			t->texturechain = NULL;
		}
	}

	if (r_wateralpha->value < 1.0)
	{
		GL_TexEnv(GL_REPLACE);

		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEFAULT);
	}
}