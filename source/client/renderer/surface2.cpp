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

gllightmapstate_t	gl_lms;

mbrush38_surface_t*	r_alpha_surfaces;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static float	s_blocklights_q2[34 * 34 * 3];

// CODE --------------------------------------------------------------------

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

//==========================================================================
//
//	LM_InitBlock
//
//==========================================================================

static void LM_InitBlock()
{
	Com_Memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));
}

//==========================================================================
//
//	LM_AllocBlock
//
//	Returns a texture number and the position inside it
//
//==========================================================================

static bool LM_AllocBlock(int w, int h, int* x, int* y)
{
	int best = BLOCK_HEIGHT;

	for (int i = 0; i < BLOCK_WIDTH - w; i++)
	{
		int best2 = 0;

		int j;
		for (j = 0; j < w; j++)
		{
			if (gl_lms.allocated[i + j] >= best)
			{
				break;
			}
			if (gl_lms.allocated[i + j] > best2)
			{
				best2 = gl_lms.allocated[i + j];
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
		return false;
	}

	for (int i = 0; i < w; i++)
	{
		gl_lms.allocated[*x + i] = best + h;
	}

	return true;
}

//==========================================================================
//
//	LM_UploadBlock
//
//==========================================================================

static void LM_UploadBlock(bool dynamic)
{
	int texture;
	if (dynamic)
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind(tr.lightmaps[texture]);

	if (dynamic)
	{
		int height = 0;
		for (int i = 0; i < BLOCK_WIDTH; i++)
		{
			if (gl_lms.allocated[i] > height)
			{
				height = gl_lms.allocated[i];
			}
		}

		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BLOCK_WIDTH, height,
			GL_RGBA, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffer);
	}
	else
	{
		R_ReUploadImage(tr.lightmaps[texture], gl_lms.lightmap_buffer);
		if (++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS)
		{
			throw DropException("LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n");
		}
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

//==========================================================================
//
//	R_SetCacheState
//
//==========================================================================

static void R_SetCacheState(mbrush38_surface_t* surf)
{
	for (int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
	{
		surf->cached_light[maps] = tr.refdef.lightstyles[surf->styles[maps]].white;
	}
}

//==========================================================================
//
//	R_AddDynamicLightsQ2
//
//==========================================================================

static void R_AddDynamicLightsQ2(mbrush38_surface_t* surf)
{
	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	mbrush38_texinfo_t* tex = surf->texinfo;

	for (int lnum = 0; lnum < tr.refdef.num_dlights; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
		{
			continue;		// not lit by this light
		}

		dlight_t* dl = &tr.refdef.dlights[lnum];
		float frad = dl->radius;
		float fdist = DotProduct(dl->origin, surf->plane->normal) - surf->plane->dist;
		frad -= fabs(fdist);
		// rad is now the highest intensity on the plane

		float fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?
		if (frad < fminlight)
		{
			continue;
		}
		fminlight = frad - fminlight;

		vec3_t impact;
		for (int i = 0; i < 3; i++)
		{
			impact[i] = dl->origin[i] - surf->plane->normal[i] * fdist;
		}

		vec3_t local;
		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		float* pfBL = s_blocklights_q2;
		float ftacc = 0;
		for (int t = 0; t < tmax ; t++, ftacc += 16)
		{
			int td = local[1] - ftacc;
			if (td < 0)
			{
				td = -td;
			}

			float fsacc = 0;
			for (int s = 0; s < smax; s++, fsacc += 16, pfBL += 3)
			{
				int sd = Q_ftol(local[0] - fsacc);

				if (sd < 0)
				{
					sd = -sd;
				}

				if (sd > td)
				{
					fdist = sd + (td >> 1);
				}
				else
				{
					fdist = td + (sd >> 1);
				}

				if (fdist < fminlight)
				{
					pfBL[0] += (frad - fdist) * dl->color[0];
					pfBL[1] += (frad - fdist) * dl->color[1];
					pfBL[2] += (frad - fdist) * dl->color[2];
				}
			}
		}
	}
}

//==========================================================================
//
//	R_BuildLightMapQ2
//
//	Combine and scale multiple lightmaps into the floating format in blocklights
//
//==========================================================================

static void R_BuildLightMapQ2(mbrush38_surface_t* surf, byte* dest, int stride)
{
	if (surf->texinfo->flags & (BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP))
	{
		throw DropException("R_BuildLightMapQ2 called for non-lit surface");
	}

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	int size = smax * tmax;
	if (size > ((int)sizeof(s_blocklights_q2) >> 4))
	{
		throw DropException("Bad s_blocklights_q2 size");
	}

	// set to full bright if no light data
	if (!surf->samples)
	{
		for (int i = 0; i < size * 3; i++)
		{
			s_blocklights_q2[i] = 255;
		}
	}
	else
	{
		// count the # of maps
		int nummaps = 0;
		while (nummaps < BSP38_MAXLIGHTMAPS && surf->styles[nummaps] != 255)
		{
			nummaps++;
		}

		byte* lightmap = surf->samples;

		// add all the lightmaps
		if (nummaps == 1)
		{
			for (int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				float* bl = s_blocklights_q2;

				float scale[4];
				for (int i = 0; i < 3; i++)
				{
					scale[i] = r_modulate->value * tr.refdef.lightstyles[surf->styles[maps]].rgb[i];
				}

				if (scale[0] == 1.0F && scale[1] == 1.0F && scale[2] == 1.0F)
				{
					for (int i = 0; i < size; i++, bl += 3)
					{
						bl[0] = lightmap[i * 3 + 0];
						bl[1] = lightmap[i * 3 + 1];
						bl[2] = lightmap[i * 3 + 2];
					}
				}
				else
				{
					for (int i = 0; i < size; i++, bl += 3)
					{
						bl[0] = lightmap[i * 3 + 0] * scale[0];
						bl[1] = lightmap[i * 3 + 1] * scale[1];
						bl[2] = lightmap[i * 3 + 2] * scale[2];
					}
				}
				lightmap += size * 3;		// skip to next lightmap
			}
		}
		else
		{
			Com_Memset(s_blocklights_q2, 0, sizeof(s_blocklights_q2[0]) * size * 3);

			for (int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				float* bl = s_blocklights_q2;

				float scale[4];
				for (int i = 0; i < 3; i++)
				{
					scale[i] = r_modulate->value * tr.refdef.lightstyles[surf->styles[maps]].rgb[i];
				}

				if (scale[0] == 1.0F && scale[1] == 1.0F && scale[2] == 1.0F)
				{
					for (int i = 0; i < size; i++, bl += 3)
					{
						bl[0] += lightmap[i * 3 + 0];
						bl[1] += lightmap[i * 3 + 1];
						bl[2] += lightmap[i * 3 + 2];
					}
				}
				else
				{
					for (int i = 0; i < size; i++, bl += 3)
					{
						bl[0] += lightmap[i * 3 + 0] * scale[0];
						bl[1] += lightmap[i * 3 + 1] * scale[1];
						bl[2] += lightmap[i * 3 + 2] * scale[2];
					}
				}
				lightmap += size * 3;		// skip to next lightmap
			}
		}

		// add all the dynamic lights
		if (surf->dlightframe == tr.frameCount)
		{
			R_AddDynamicLightsQ2(surf);
		}
	}

	// put into texture format
	stride -= (smax<<2);
	float* bl = s_blocklights_q2;

	for (int i = 0; i < tmax; i++, dest += stride)
	{
		for (int j = 0; j < smax; j++)
		{
			int r = Q_ftol(bl[0]);
			int g = Q_ftol(bl[1]);
			int b = Q_ftol(bl[2]);

			// catch negative lights
			if (r < 0)
			{
				r = 0;
			}
			if (g < 0)
			{
				g = 0;
			}
			if (b < 0)
			{
				b = 0;
			}

			/*
			** determine the brightest of the three color components
			*/
			int max;
			if (r > g)
			{
				max = r;
			}
			else
			{
				max = g;
			}
			if (b > max)
			{
				max = b;
			}

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255)
			{
				float t = 255.0F / max;

				r = r * t;
				g = g * t;
				b = b * t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = 255;

			bl += 3;
			dest += 4;
		}
	}
}

//==========================================================================
//
//	GL_BeginBuildingLightmaps
//
//==========================================================================

void GL_BeginBuildingLightmaps(model_t* m)
{
	Com_Memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

	tr.frameCount = 1;		// no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (int i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		backEndData[tr.smpFrame]->lightstyles[i].rgb[0] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[1] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[2] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].white = 3;
	}
	tr.refdef.lightstyles = backEndData[tr.smpFrame]->lightstyles;

	byte dummy[128 * 128 * 4];
	if (!tr.lightmaps[0])
	{
		for (int i = 0; i < MAX_LIGHTMAPS; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), dummy, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false);
		}
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** initialize the dynamic lightmap texture
	*/
	R_ReUploadImage(tr.lightmaps[0], dummy);
}

//==========================================================================
//
//	GL_CreateSurfaceLightmapQ2
//
//==========================================================================

void GL_CreateSurfaceLightmapQ2(mbrush38_surface_t* surf)
{
	if (surf->flags & (BRUSH38_SURF_DRAWSKY | BRUSH38_SURF_DRAWTURB))
	{
		return;
	}

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;

	if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		LM_UploadBlock(false);
		LM_InitBlock();
		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			throw Exception(va("Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax));
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	byte* base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState(surf);
	R_BuildLightMapQ2(surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}

//==========================================================================
//
//	GL_EndBuildingLightmaps
//
//==========================================================================

void GL_EndBuildingLightmaps()
{
	LM_UploadBlock(false);
}

//==========================================================================
//
//	EmitWaterPolysQ2
//
//	Does a water warp on the pre-fragmented mbrush38_glpoly_t chain
//
//==========================================================================

static void EmitWaterPolysQ2(mbrush38_surface_t* fa)
{
	float scroll;
	if (fa->texinfo->flags & BSP38SURF_FLOWING)
	{
		scroll = -64 * ((tr.refdef.floatTime * 0.5) - (int)(tr.refdef.floatTime * 0.5));
	}
	else
	{
		scroll = 0;
	}
	for (mbrush38_glpoly_t* bp = fa->polys; bp; bp = bp->next)
	{
		mbrush38_glpoly_t* p = bp;

		qglBegin(GL_TRIANGLE_FAN);
		float* v = p->verts[0];
		for (int i = 0; i < p->numverts; i++, v += BRUSH38_VERTEXSIZE)
		{
			float os = v[3];
			float ot = v[4];

			float s = os + r_turbsin[Q_ftol( ((ot * 0.125 + tr.refdef.floatTime) * TURBSCALE)) & 255] * 0.5;
			s += scroll;
			s *= (1.0 / 64);

			float t = ot + r_turbsin[Q_ftol( ((os * 0.125 + tr.refdef.floatTime) * TURBSCALE)) & 255] * 0.5;
			t *= (1.0 / 64);

			qglTexCoord2f(s, t);
			qglVertex3fv(v);
		}
		qglEnd();
	}
}

//==========================================================================
//
//	R_TextureAnimationQ2
//
//	Returns the proper texture for a given time and base texture
//
//==========================================================================

image_t* R_TextureAnimationQ2(mbrush38_texinfo_t* tex)
{
	if (!tex->next)
	{
		return tex->image;
	}

	int c = tr.currentEntity->e.frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

//==========================================================================
//
//	DrawGLPolyQ2
//
//==========================================================================

static void DrawGLPolyQ2(mbrush38_glpoly_t* p)
{
	qglBegin(GL_POLYGON);
	float* v = p->verts[0];
	for (int i = 0; i < p->numverts; i++, v += BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);
		qglVertex3fv(v);
	}
	qglEnd();
}

//==========================================================================
//
//	DrawGLFlowingPoly
//
//	Version of DrawGLPolyQ2 that handles scrolling texture
//
//==========================================================================

static void DrawGLFlowingPoly(mbrush38_surface_t* fa)
{
	mbrush38_glpoly_t* p = fa->polys;

	float scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
	if (scroll == 0.0)
	{
		scroll = -64.0;
	}

	qglBegin(GL_POLYGON);
	float* v = p->verts[0];
	for (int i = 0; i < p->numverts; i++, v += BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f((v[3] + scroll), v[4]);
		qglVertex3fv(v);
	}
	qglEnd();
}

//==========================================================================
//
//	R_RenderBrushPolyQ2
//
//==========================================================================

void R_RenderBrushPolyQ2(mbrush38_surface_t* fa)
{
	c_brush_polys++;

	image_t* image = R_TextureAnimationQ2(fa->texinfo);

	if (fa->flags & BRUSH38_SURF_DRAWTURB)
	{	
		GL_Bind(image);

		// warp texture, no lightmaps
		GL_TexEnv(GL_MODULATE);
		qglColor4f(tr.identityLight, tr.identityLight, tr.identityLight, 1.0f);
		EmitWaterPolysQ2(fa);
		GL_TexEnv(GL_REPLACE);

		return;
	}
	else
	{
		GL_Bind(image);

		GL_TexEnv(GL_REPLACE);
	}

	if (fa->texinfo->flags & BSP38SURF_FLOWING)
	{
		DrawGLFlowingPoly(fa);
	}
	else
	{
		DrawGLPolyQ2(fa->polys);
	}

	/*
	** check for lightmap modification
	*/
	bool is_dynamic = false;
	int maps;
	for (maps = 0; maps < BSP38_MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (tr.refdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	// dynamic this frame or dynamic previously
	if (fa->dlightframe == tr.frameCount)
	{
dynamic:
		if (r_dynamic->value)
		{
			if (!(fa->texinfo->flags & (BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if ((fa->styles[maps] >= 32 || fa->styles[maps] == 0) && (fa->dlightframe != tr.frameCount))
		{
			int smax = (fa->extents[0] >> 4) + 1;
			int tmax = (fa->extents[1] >> 4) + 1;

			byte temp[34 * 34 * 4];
			R_BuildLightMapQ2(fa, temp, smax * 4);
			R_SetCacheState(fa);

			GL_Bind(tr.lightmaps[fa->lightmaptexturenum]);

			qglTexSubImage2D(GL_TEXTURE_2D, 0, fa->light_s, fa->light_t, smax, tmax, GL_RGBA, GL_UNSIGNED_BYTE, temp);

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}

//==========================================================================
//
//	DrawTextureChainsQ2
//
//==========================================================================

void DrawTextureChainsQ2()
{
	int		i;

	c_visible_textures = 0;

	if (!qglActiveTextureARB)
	{
		for (i = 0; i < tr.numImages; i++)
		{
			mbrush38_surface_t* s = tr.images[i]->texturechain;
			if (!s)
			{
				continue;
			}
			c_visible_textures++;

			for (; s; s = s->texturechain)
			{
				R_RenderBrushPolyQ2(s);
			}

			tr.images[i]->texturechain = NULL;
		}
	}
	else
	{
		for (i = 0; i < tr.numImages; i++)
		{
			if (!tr.images[i]->texturechain)
			{
				continue;
			}
			c_visible_textures++;

			for (mbrush38_surface_t* s = tr.images[i]->texturechain; s; s = s->texturechain)
			{
				if (!(s->flags & BRUSH38_SURF_DRAWTURB))
				{
					R_RenderBrushPolyQ2(s);
				}
			}
		}

		for (i = 0; i < tr.numImages; i++)
		{
			mbrush38_surface_t* s = tr.images[i]->texturechain;
			if (!s)
			{
				continue;
			}

			for (; s; s = s->texturechain)
			{
				if (s->flags & BRUSH38_SURF_DRAWTURB)
				{
					R_RenderBrushPolyQ2(s);
				}
			}

			tr.images[i]->texturechain = NULL;
		}
	}

	GL_TexEnv(GL_REPLACE);
}

//==========================================================================
//
//	DrawGLPolyChainQ2
//
//==========================================================================

static void DrawGLPolyChainQ2(mbrush38_glpoly_t* p, float soffset, float toffset)
{
	if (soffset == 0 && toffset == 0)
	{
		for (; p != 0; p = p->chain)
		{
			qglBegin(GL_POLYGON);
			float* v = p->verts[0];
			for (int j = 0; j < p->numverts; j++, v += BRUSH38_VERTEXSIZE)
			{
				qglTexCoord2f(v[5], v[6]);
				qglVertex3fv(v);
			}
			qglEnd();
		}
	}
	else
	{
		for (; p != 0; p = p->chain)
		{
			qglBegin(GL_POLYGON);
			float* v = p->verts[0];
			for (int j = 0; j < p->numverts; j++, v += BRUSH38_VERTEXSIZE)
			{
				qglTexCoord2f(v[5] - soffset, v[6] - toffset);
				qglVertex3fv(v);
			}
			qglEnd();
		}
	}
}

//==========================================================================
//
//	R_BlendLightmapsQ2
//
//	This routine takes all the given light mapped surfaces in the world and
// blends them into the framebuffer.
//
//==========================================================================

void R_BlendLightmapsQ2()
{
	// don't bother if we're set to fullbright
	if (r_fullbright->value)
	{
		return;
	}
	if (!tr.worldModel->brush38_lightdata)
	{
		return;
	}

	/*
	** set the appropriate blending mode unless we're only looking at the
	** lightmaps.
	*/
	// don't bother writing Z
	if (!r_lightmap->value)
	{
		if (r_saturatelighting->value)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
		}
	}
	else
	{
		GL_State(0);
	}

	if (tr.currentModel == tr.worldModel)
	{
		c_visible_lightmaps = 0;
	}

	/*
	** render static lightmaps first
	*/
	for (int i = 1; i < MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_surfaces[i])
		{
			if (tr.currentModel == tr.worldModel)
			{
				c_visible_lightmaps++;
			}
			GL_Bind(tr.lightmaps[i]);

			for (mbrush38_surface_t* surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
			{
				if (surf->polys)
				{
					DrawGLPolyChainQ2(surf->polys, 0, 0);
				}
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if (r_dynamic->value)
	{
		LM_InitBlock();

		GL_Bind(tr.lightmaps[0]);

		if (tr.currentModel == tr.worldModel)
		{
			c_visible_lightmaps++;
		}

		mbrush38_surface_t* newdrawsurf = gl_lms.lightmap_surfaces[0];

		for (mbrush38_surface_t* surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain)
		{
			int smax = (surf->extents[0] >> 4) + 1;
			int tmax = (surf->extents[1] >> 4) + 1;

			if (LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
			{
				byte* base = gl_lms.lightmap_buffer;
				base += (surf->dlight_t * BLOCK_WIDTH + surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMapQ2(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
			}
			else
			{
				// upload what we have so far
				LM_UploadBlock(true);

				// draw all surfaces that use this lightmap
				mbrush38_surface_t* drawsurf;
				for (drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain)
				{
					if (drawsurf->polys)
					{
						DrawGLPolyChainQ2(drawsurf->polys, 
							(drawsurf->light_s - drawsurf->dlight_s) * (1.0 / 128.0), 
							(drawsurf->light_t - drawsurf->dlight_t) * (1.0 / 128.0));
					}
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if (!LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
				{
					throw Exception(va("Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax));
				}

				byte* base = gl_lms.lightmap_buffer;
				base += (surf->dlight_t * BLOCK_WIDTH + surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMapQ2(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if (newdrawsurf)
		{
			LM_UploadBlock(true);
		}

		for (mbrush38_surface_t* surf = newdrawsurf; surf != 0; surf = surf->lightmapchain)
		{
			if (surf->polys)
			{
				DrawGLPolyChainQ2(surf->polys, (surf->light_s - surf->dlight_s) * (1.0 / 128.0), (surf->light_t - surf->dlight_t) * (1.0 / 128.0));
			}
		}
	}

	/*
	** restore state
	*/
	GL_State(GLS_DEPTHMASK_TRUE);
}

//==========================================================================
//
//	GL_MBind
//
//==========================================================================

static void GL_MBind(int target, image_t* image)
{
	GL_SelectTexture(target);
	GL_Bind(image);
}

//==========================================================================
//
//	GL_RenderLightmappedPoly
//
//==========================================================================

void GL_RenderLightmappedPoly(mbrush38_surface_t* surf)
{
	int		i, nv = surf->polys->numverts;
	int		map;
	float	*v;
	image_t *image = R_TextureAnimationQ2( surf->texinfo );
	qboolean is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	mbrush38_glpoly_t *p;

	GL_SelectTexture(0);
	GL_TexEnv(GL_REPLACE);
	GL_SelectTexture(1);

	GL_SelectTexture(1);
	qglEnable(GL_TEXTURE_2D);

	if (r_lightmap->value)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else 
	{
		GL_TexEnv(GL_MODULATE);
	}

	for (map = 0; map < BSP38_MAXLIGHTMAPS && surf->styles[map] != 255; map++)
	{
		if (tr.refdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
		{
			goto dynamic;
		}
	}

	// dynamic this frame or dynamic previously
	if ((surf->dlightframe == tr.frameCount))
	{
dynamic:
		if (r_dynamic->value)
		{
			if (!(surf->texinfo->flags & (BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		unsigned	temp[128*128];
		int			smax, tmax;

		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != tr.frameCount))
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMapQ2(surf, (byte*)temp, smax * 4);
			R_SetCacheState(surf);

			GL_MBind(1, tr.lightmaps[surf->lightmaptexturenum]);

			lmtex = surf->lightmaptexturenum;

			qglTexSubImage2D(GL_TEXTURE_2D, 0, surf->light_s, surf->light_t, smax, tmax,
				GL_RGBA, GL_UNSIGNED_BYTE, temp);
		}
		else
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMapQ2(surf, (byte*)temp, smax * 4);

			GL_MBind(1, tr.lightmaps[0]);

			lmtex = 0;

			qglTexSubImage2D(GL_TEXTURE_2D, 0, surf->light_s, surf->light_t, smax, tmax, 
				GL_RGBA, GL_UNSIGNED_BYTE, temp);
		}

		c_brush_polys++;

		GL_MBind(0, image);
		GL_MBind(1, tr.lightmaps[lmtex]);

		if (surf->texinfo->flags & BSP38SURF_FLOWING)
		{
			float scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
			if (scroll == 0.0)
			{
				scroll = -64.0;
			}

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, (v[3] + scroll), v[4]);
					qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
		else
		{
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
					qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
	}
	else
	{
		c_brush_polys++;

		GL_MBind(0, image);
		GL_MBind(1, tr.lightmaps[lmtex]);

		if (surf->texinfo->flags & BSP38SURF_FLOWING)
		{
			float scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
			if (scroll == 0.0)
			{
				scroll = -64.0;
			}

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, (v[3] + scroll), v[4]);
					qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
		else
		{
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
					qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
	}

	GL_SelectTexture(1);
	qglDisable(GL_TEXTURE_2D);
	GL_TexEnv(GL_REPLACE);
	GL_SelectTexture(0);
	GL_TexEnv(GL_REPLACE);
}

//==========================================================================
//
//	R_DrawAlphaSurfaces
//
//	Draw water surfaces and windows.
//	The BSP tree is waled front to back, so unwinding the chain of
// alpha_surfaces will draw back to front, giving proper ordering.
//
//==========================================================================

void R_DrawAlphaSurfaces()
{
	//
	// go back to the world matrix
	//
    qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv(GL_MODULATE);

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	float intens = tr.identityLight;

	for (mbrush38_surface_t* s = r_alpha_surfaces; s; s = s->texturechain)
	{
		GL_Bind(s->texinfo->image);
		c_brush_polys++;
		if (s->texinfo->flags & BSP38SURF_TRANS33)
		{
			qglColor4f(intens, intens, intens, 0.33);
		}
		else if (s->texinfo->flags & BSP38SURF_TRANS66)
		{
			qglColor4f(intens, intens, intens, 0.66);
		}
		else
		{
			qglColor4f(intens, intens, intens, 1);
		}
		if (s->flags & BRUSH38_SURF_DRAWTURB)
		{
			EmitWaterPolysQ2(s);
		}
		else
		{
			DrawGLPolyQ2(s->polys);
		}
	}

	GL_TexEnv(GL_REPLACE);
	qglColor4f(1, 1, 1, 1);
	GL_State(GLS_DEFAULT);

	r_alpha_surfaces = NULL;
}

//==========================================================================
//
//	R_DrawTriangleOutlines
//
//==========================================================================

void R_DrawTriangleOutlines()
{
	if (!r_showtris->value)
	{
		return;
	}

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE);
	qglDisable(GL_TEXTURE_2D);
	qglColor4f(1, 1, 1, 1);

	for (int i = 0; i < MAX_LIGHTMAPS; i++)
	{
		for (mbrush38_surface_t* surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
		{
			for (mbrush38_glpoly_t* p = surf->polys; p; p = p->chain)
			{
				for (int j = 2; j < p->numverts; j++)
				{
					qglBegin(GL_LINE_STRIP);
					qglVertex3fv(p->verts[0]);
					qglVertex3fv(p->verts[j - 1]);
					qglVertex3fv(p->verts[j]);
					qglVertex3fv(p->verts[0]);
					qglEnd();
				}
			}
		}
	}

	GL_State(GLS_DEFAULT);
	qglEnable(GL_TEXTURE_2D);
}
