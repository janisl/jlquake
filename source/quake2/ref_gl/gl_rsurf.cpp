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

mbrush38_surface_t	*r_alpha_surfaces;

#define DYNAMIC_LIGHT_WIDTH  128
#define DYNAMIC_LIGHT_HEIGHT 128

#define LIGHTMAP_BYTES 4

int		c_visible_lightmaps;
int		c_visible_textures;

typedef struct
{
	int	current_lightmap_texture;

	mbrush38_surface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;


static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

extern void R_SetCacheState( mbrush38_surface_t *surf );
extern void R_BuildLightMap (mbrush38_surface_t *surf, byte *dest, int stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mbrush38_texinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = tr.currentEntity->e.frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

#if 0
/*
=================
WaterWarpPolyVerts

Mangles the x and y coordinates in a copy of the poly
so that any drawing routine can be water warped
=================
*/
mbrush38_glpoly_t *WaterWarpPolyVerts (mbrush38_glpoly_t *p)
{
	int		i;
	float	*v, *nv;
	static byte	buffer[1024];
	mbrush38_glpoly_t *out;

	out = (mbrush38_glpoly_t *)buffer;

	out->numverts = p->numverts;
	v = p->verts[0];
	nv = out->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH38_VERTEXSIZE, nv+=BRUSH38_VERTEXSIZE)
	{
		nv[0] = v[0] + 4*sin(v[1]*0.05+tr.refdef.floatTime)*sin(v[2]*0.05+tr.refdef.floatTime);
		nv[1] = v[1] + 4*sin(v[0]*0.05+tr.refdef.floatTime)*sin(v[2]*0.05+tr.refdef.floatTime);

		nv[2] = v[2];
		nv[3] = v[3];
		nv[4] = v[4];
		nv[5] = v[5];
		nv[6] = v[6];
	}

	return out;
}

/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly (mbrush38_glpoly_t *p)
{
	int		i;
	float	*v;

	p = WaterWarpPolyVerts (p);
	qglBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f (v[3], v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
}
void DrawGLWaterPolyLightmap (mbrush38_glpoly_t *p)
{
	int		i;
	float	*v;

	p = WaterWarpPolyVerts (p);
	qglBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f (v[5], v[6]);
		qglVertex3fv (v);
	}
	qglEnd ();
}
#endif

/*
================
DrawGLPoly
================
*/
void DrawGLPoly (mbrush38_glpoly_t *p)
{
	int		i;
	float	*v;

	qglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f (v[3], v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
}

//============
//PGM
/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
void DrawGLFlowingPoly (mbrush38_surface_t *fa)
{
	int		i;
	float	*v;
	mbrush38_glpoly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
	if(scroll == 0.0)
		scroll = -64.0;

	qglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= BRUSH38_VERTEXSIZE)
	{
		qglTexCoord2f ((v[3] + scroll), v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
}
//PGM
//============

/*
** R_DrawTriangleOutlines
*/
void R_DrawTriangleOutlines (void)
{
	int			i, j;
	mbrush38_glpoly_t	*p;

	if (!gl_showtris->value)
		return;

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE);
	qglDisable (GL_TEXTURE_2D);
	qglColor4f (1,1,1,1);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		mbrush38_surface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for ( ; p ; p=p->chain)
			{
				for (j=2 ; j<p->numverts ; j++ )
				{
					qglBegin (GL_LINE_STRIP);
					qglVertex3fv (p->verts[0]);
					qglVertex3fv (p->verts[j-1]);
					qglVertex3fv (p->verts[j]);
					qglVertex3fv (p->verts[0]);
					qglEnd ();
				}
			}
		}
	}

	GL_State(GLS_DEFAULT);
	qglEnable (GL_TEXTURE_2D);
}

/*
** DrawGLPolyChain
*/
void DrawGLPolyChain( mbrush38_glpoly_t *p, float soffset, float toffset )
{
	if ( soffset == 0 && toffset == 0 )
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= BRUSH38_VERTEXSIZE)
			{
				qglTexCoord2f (v[5], v[6] );
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
	else
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= BRUSH38_VERTEXSIZE)
			{
				qglTexCoord2f (v[5] - soffset, v[6] - toffset );
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
}

/*
** R_BlendLightMaps
**
** This routine takes all the given light mapped surfaces in the world and
** blends them into the framebuffer.
*/
void R_BlendLightmaps (void)
{
	int			i;
	mbrush38_surface_t	*surf, *newdrawsurf = 0;

	// don't bother if we're set to fullbright
	if (r_fullbright->value)
		return;
	if (!tr.worldModel->brush38_lightdata)
		return;

	/*
	** set the appropriate blending mode unless we're only looking at the
	** lightmaps.
	*/
	if (!r_lightmap->value)
	{
		if ( gl_saturatelighting->value )
		{
			// don't bother writing Z
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			// don't bother writing Z
			GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR);
		}
	}
	else
	{
		// don't bother writing Z
		GL_State(0);
	}

	if ( tr.currentModel == tr.worldModel )
		c_visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for ( i = 1; i < MAX_LIGHTMAPS; i++ )
	{
		if ( gl_lms.lightmap_surfaces[i] )
		{
			if (tr.currentModel == tr.worldModel)
				c_visible_lightmaps++;
			GL_Bind( tr.lightmaps[i]);

			for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
			{
				if ( surf->polys )
					DrawGLPolyChain( surf->polys, 0, 0 );
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if ( gl_dynamic->value )
	{
		LM_InitBlock();

		GL_Bind( tr.lightmaps[0]);

		if (tr.currentModel == tr.worldModel)
			c_visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for ( surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain )
		{
			int		smax, tmax;
			byte	*base;

			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			if ( LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
			{
				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
			else
			{
				mbrush38_surface_t *drawsurf;

				// upload what we have so far
				LM_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for ( drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if ( drawsurf->polys )
						DrawGLPolyChain( drawsurf->polys, 
							              ( drawsurf->light_s - drawsurf->dlight_s ) * ( 1.0 / 128.0 ), 
										( drawsurf->light_t - drawsurf->dlight_t ) * ( 1.0 / 128.0 ) );
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if ( !LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
				{
					ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax );
				}

				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if ( newdrawsurf )
			LM_UploadBlock( true );

		for ( surf = newdrawsurf; surf != 0; surf = surf->lightmapchain )
		{
			if ( surf->polys )
				DrawGLPolyChain( surf->polys, ( surf->light_s - surf->dlight_s ) * ( 1.0 / 128.0 ), ( surf->light_t - surf->dlight_t ) * ( 1.0 / 128.0 ) );
		}
	}

	/*
	** restore state
	*/
	GL_State(GLS_DEPTHMASK_TRUE);
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (mbrush38_surface_t *fa)
{
	int			maps;
	image_t		*image;
	qboolean is_dynamic = false;

	c_brush_polys++;

	image = R_TextureAnimation (fa->texinfo);

	if (fa->flags & BRUSH38_SURF_DRAWTURB)
	{	
		GL_Bind( image);

		// warp texture, no lightmaps
		GL_TexEnv( GL_MODULATE );
		qglColor4f(tr.identityLight, tr.identityLight, tr.identityLight, 1.0f);
		EmitWaterPolys (fa);
		GL_TexEnv( GL_REPLACE );

		return;
	}
	else
	{
		GL_Bind( image);

		GL_TexEnv( GL_REPLACE );
	}

//======
//PGM
	if(fa->texinfo->flags & BSP38SURF_FLOWING)
		DrawGLFlowingPoly (fa);
	else
		DrawGLPoly (fa->polys);
//PGM
//======

	/*
	** check for lightmap modification
	*/
	for ( maps = 0; maps < BSP38_MAXLIGHTMAPS && fa->styles[maps] != 255; maps++ )
	{
		if (tr.refdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( ( fa->dlightframe == tr.frameCount ) )
	{
dynamic:
		if ( gl_dynamic->value )
		{
			if (!( fa->texinfo->flags & (BSP38SURF_SKY|BSP38SURF_TRANS33|BSP38SURF_TRANS66|BSP38SURF_WARP ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		if ( ( fa->styles[maps] >= 32 || fa->styles[maps] == 0 ) && ( fa->dlightframe != tr.frameCount ) )
		{
			unsigned	temp[34*34];
			int			smax, tmax;

			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;

			R_BuildLightMap( fa, (byte*)temp, smax*4 );
			R_SetCacheState( fa );

			GL_Bind( tr.lightmaps[fa->lightmaptexturenum]);

			qglTexSubImage2D( GL_TEXTURE_2D, 0,
							  fa->light_s, fa->light_t, 
							  smax, tmax, 
							  GL_RGBA, 
							  GL_UNSIGNED_BYTE, temp );

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


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	mbrush38_surface_t	*s;
	float		intens;

	//
	// go back to the world matrix
	//
    qglLoadMatrixf (tr.viewParms.world.modelMatrix);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = tr.identityLight;

	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		GL_Bind(s->texinfo->image);
		c_brush_polys++;
		if (s->texinfo->flags & BSP38SURF_TRANS33)
			qglColor4f (intens,intens,intens,0.33);
		else if (s->texinfo->flags & BSP38SURF_TRANS66)
			qglColor4f (intens,intens,intens,0.66);
		else
			qglColor4f (intens,intens,intens,1);
		if (s->flags & BRUSH38_SURF_DRAWTURB)
			EmitWaterPolys (s);
		else
			DrawGLPoly (s->polys);
	}

	GL_TexEnv( GL_REPLACE );
	qglColor4f (1,1,1,1);
	GL_State(GLS_DEFAULT);

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	mbrush38_surface_t	*s;

	c_visible_textures = 0;

//	GL_TexEnv( GL_REPLACE );

	if ( !qglActiveTextureARB )
	{
		for ( i = 0; i<tr.numImages; i++)
		{
			if (!tr.images[i])
				continue;
			s = tr.images[i]->texturechain;
			if (!s)
				continue;
			c_visible_textures++;

			for ( ; s ; s=s->texturechain)
				R_RenderBrushPoly (s);

			tr.images[i]->texturechain = NULL;
		}
	}
	else
	{
		for ( i = 0; i<tr.numImages; i++)
		{
			if (!tr.images[i])
				continue;
			if (!tr.images[i]->texturechain)
				continue;
			c_visible_textures++;

			for ( s = tr.images[i]->texturechain; s ; s=s->texturechain)
			{
				if ( !( s->flags & BRUSH38_SURF_DRAWTURB ) )
					R_RenderBrushPoly (s);
			}
		}

		for ( i = 0; i<tr.numImages; i++)
		{
			if (!tr.images[i])
				continue;
			s = tr.images[i]->texturechain;
			if (!s)
				continue;

			for ( ; s ; s=s->texturechain)
			{
				if ( s->flags & BRUSH38_SURF_DRAWTURB )
					R_RenderBrushPoly (s);
			}

			tr.images[i]->texturechain = NULL;
		}
	}

	GL_TexEnv( GL_REPLACE );
}


static void GL_MBind( int target, image_t* image)
{
	GL_SelectTexture( target );
	GL_Bind(image);
}

static void GL_RenderLightmappedPoly( mbrush38_surface_t *surf )
{
	int		i, nv = surf->polys->numverts;
	int		map;
	float	*v;
	image_t *image = R_TextureAnimation( surf->texinfo );
	qboolean is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	mbrush38_glpoly_t *p;

	GL_SelectTexture(0);
	GL_TexEnv( GL_REPLACE );
	GL_SelectTexture(1);

	GL_SelectTexture(1);
	qglEnable( GL_TEXTURE_2D );

	if ( r_lightmap->value )
		GL_TexEnv( GL_REPLACE );
	else 
		GL_TexEnv( GL_MODULATE );

	for ( map = 0; map < BSP38_MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
	{
		if (tr.refdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( ( surf->dlightframe == tr.frameCount ) )
	{
dynamic:
		if ( gl_dynamic->value )
		{
			if ( !(surf->texinfo->flags & (BSP38SURF_SKY|BSP38SURF_TRANS33|BSP38SURF_TRANS66|BSP38SURF_WARP ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		unsigned	temp[128*128];
		int			smax, tmax;

		if ( ( surf->styles[map] >= 32 || surf->styles[map] == 0 ) && ( surf->dlightframe != tr.frameCount ) )
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap( surf, (byte*)temp, smax*4 );
			R_SetCacheState( surf );

			GL_MBind( 1, tr.lightmaps[surf->lightmaptexturenum]);

			lmtex = surf->lightmaptexturenum;

			qglTexSubImage2D( GL_TEXTURE_2D, 0,
							  surf->light_s, surf->light_t, 
							  smax, tmax, 
							  GL_RGBA, 
							  GL_UNSIGNED_BYTE, temp );

		}
		else
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap( surf, (byte*)temp, smax*4 );

			GL_MBind( 1, tr.lightmaps[0]);

			lmtex = 0;

			qglTexSubImage2D( GL_TEXTURE_2D, 0,
							  surf->light_s, surf->light_t, 
							  smax, tmax, 
							  GL_RGBA, 
							  GL_UNSIGNED_BYTE, temp );

		}

		c_brush_polys++;

		GL_MBind( 0, image);
		GL_MBind( 1, tr.lightmaps[lmtex]);

//==========
//PGM
		if (surf->texinfo->flags & BSP38SURF_FLOWING)
		{
			float scroll;
		
			scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
			if(scroll == 0.0)
				scroll = -64.0;

			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				qglBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, (v[3]+scroll), v[4]);
					qglMultiTexCoord2fARB( GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
		}
		else
		{
			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				qglBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, v[3], v[4]);
					qglMultiTexCoord2fARB( GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
		}
//PGM
//==========
	}
	else
	{
		c_brush_polys++;

		GL_MBind( 0, image);
		GL_MBind( 1, tr.lightmaps[lmtex]);

//==========
//PGM
		if (surf->texinfo->flags & BSP38SURF_FLOWING)
		{
			float scroll;
		
			scroll = -64 * ((tr.refdef.floatTime / 40.0) - (int)(tr.refdef.floatTime / 40.0));
			if(scroll == 0.0)
				scroll = -64.0;

			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				qglBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, (v[3]+scroll), v[4]);
					qglMultiTexCoord2fARB( GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
		}
		else
		{
//PGM
//==========
			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				qglBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= BRUSH38_VERTEXSIZE)
				{
					qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, v[3], v[4]);
					qglMultiTexCoord2fARB( GL_TEXTURE1_ARB, v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
//==========
//PGM
		}
//PGM
//==========
	}

	GL_SelectTexture(1);
	qglDisable( GL_TEXTURE_2D );
	GL_TexEnv( GL_REPLACE );
	GL_SelectTexture(0);
	GL_TexEnv( GL_REPLACE );
}

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
		R_MarkLights (lt, 1<<k, tr.currentModel->brush38_nodes + tr.currentModel->brush38_firstnode);
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
				R_RenderBrushPoly( psurf );
			}
		}
	}

	if ( !(tr.currentEntity->e.renderfx & RF_TRANSLUCENT) )
	{
		if ( !qglMultiTexCoord2fARB )
			R_BlendLightmaps ();
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
				image = R_TextureAnimation (surf->texinfo);
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

	VectorCopy(tr.refdef.vieworg, tr.orient.viewOrigin);

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
	DrawTextureChains ();
	R_BlendLightmaps ();
	
	R_DrawSkyBox ();

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



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock( void )
{
	Com_Memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind(tr.lightmaps[texture]);

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		qglTexSubImage2D( GL_TEXTURE_2D, 
						  0,
						  0, 0,
						  BLOCK_WIDTH, height,
						  GL_RGBA,
						  GL_UNSIGNED_BYTE,
						  gl_lms.lightmap_buffer );
	}
	else
	{
		R_ReUploadImage(tr.lightmaps[texture], gl_lms.lightmap_buffer);
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			ri.Sys_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (mbrush38_surface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (BRUSH38_SURF_DRAWSKY|BRUSH38_SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	unsigned		dummy[128*128];

	Com_Memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );

	tr.frameCount = 1;		// no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	tr.refdef.lightstyles = lightstyles;

	if (!tr.lightmaps[0])
	{
		for (int i = 0; i < MAX_LIGHTMAPS; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), (byte*)dummy, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false);
		}
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** initialize the dynamic lightmap texture
	*/
	R_ReUploadImage(tr.lightmaps[0], (byte*)dummy);
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
}
