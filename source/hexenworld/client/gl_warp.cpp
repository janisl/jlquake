// gl_warp.c -- sky and water polygons

#include "quakedef.h"
#include "glquake.h"

image_t*	solidskytexture;
image_t*	alphaskytexture;
float	speedscale;		// for top sky and bottom sky



// speed up sin calculations - Ed
float	turbsin[] =
{
	#include "gl_warp_sin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented mbrush29_glpoly_t chain
=============
*/
void EmitWaterPolys (mbrush29_surface_t *fa)
{
	mbrush29_glpoly_t	*p;
	float		*v;
	int			i;
	float		s, t, os, ot;


	for (p=fa->polys ; p ; p=p->next)
	{
		qglBegin (GL_POLYGON);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=BRUSH29_VERTEXSIZE)
		{
			os = v[3];
			ot = v[4];

			s = os + turbsin[(int)((ot*0.125+realtime) * TURBSCALE) & 255];
			s *= (1.0/64);

			t = ot + turbsin[(int)((os*0.125+realtime) * TURBSCALE) & 255];
			t *= (1.0/64);

			qglTexCoord2f (s, t);
			qglVertex3fv (v);
		}
		qglEnd ();
	}
}




/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys (mbrush29_surface_t *fa)
{
	mbrush29_glpoly_t	*p;
	float		*v;
	int			i;
	float	s, t;
	vec3_t	dir;
	float	length;

	for (p=fa->polys ; p ; p=p->next)
	{
		qglBegin (GL_POLYGON);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=BRUSH29_VERTEXSIZE)
		{
			VectorSubtract (v, tr.refdef.vieworg, dir);
			dir[2] *= 3;	// flatten the sphere

			length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
			length = sqrt (length);
			length = 6*63/length;

			dir[0] *= length;
			dir[1] *= length;

			s = (speedscale + dir[0]) * (1.0/128);
			t = (speedscale + dir[1]) * (1.0/128);

			qglTexCoord2f (s, t);
			qglVertex3fv (v);
		}
		qglEnd ();
	}
}

/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented mbrush29_glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitBothSkyLayers (mbrush29_surface_t *fa)
{
	int			i;
	int			lindex;
	float		*vec;

	GL_DisableMultitexture();

	GL_Bind (solidskytexture);
	speedscale = realtime*8;
	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_Bind (alphaskytexture);
	speedscale = realtime*16;
	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	GL_State(GLS_DEFAULT);
}

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain (mbrush29_surface_t *s)
{
	mbrush29_surface_t	*fa;

	GL_DisableMultitexture();

	// used when gl_texsort is on
	GL_Bind(solidskytexture);
	speedscale = realtime*8;
	speedscale -= (int)speedscale & ~127 ;

	for (fa=s ; fa ; fa=fa->texturechain)
		EmitSkyPolys (fa);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_Bind (alphaskytexture);
	speedscale = realtime*16;
	speedscale -= (int)speedscale & ~127 ;

	for (fa=s ; fa ; fa=fa->texturechain)
		EmitSkyPolys (fa);

	GL_State(GLS_DEFAULT);
}

//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (mbrush29_texture_t *mt)
{
	int			i, j, p;
	byte		*src;
	unsigned	trans[128*128];
	unsigned	transpix;
	int			r, g, b;
	unsigned	*rgba;
	extern	int			skytexturenum;

	src = (byte *)mt + mt->offsets[0];

	// make an average value for the back to avoid
	// a fringe on the top level

	r = g = b = 0;
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j + 128];
			rgba = &d_8to24table[p];
			trans[(i*128) + j] = *rgba;
			r += ((byte *)rgba)[0];
			g += ((byte *)rgba)[1];
			b += ((byte *)rgba)[2];
		}

	((byte *)&transpix)[0] = r/(128*128);
	((byte *)&transpix)[1] = g/(128*128);
	((byte *)&transpix)[2] = b/(128*128);
	((byte *)&transpix)[3] = 0;


	if (!solidskytexture)
	{
		solidskytexture = R_CreateImage("*solidsky", (byte*)trans, 128, 128, false, false, GL_REPEAT, false);
	}
	else
	{
		R_ReUploadImage(solidskytexture, (byte*)trans);
	}


	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j];
			if (p == 0)
				trans[(i*128) + j] = transpix;
			else
				trans[(i*128) + j] = d_8to24table[p];
		}

	if (!alphaskytexture)
	{
		alphaskytexture = R_CreateImage("*alphasky", (byte*)trans, 128, 128, false, false, GL_REPEAT, false);
	}
	else
	{
		R_ReUploadImage(alphaskytexture, (byte*)trans);
	}
}

