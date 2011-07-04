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
// gl_warp.c -- sky and water polygons

#include "gl_local.h"

// speed up sin calculations - Ed
float	r_turbsin[] =
{
	#include "warpsin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented mbrush38_glpoly_t chain
=============
*/
void EmitWaterPolys (mbrush38_surface_t *fa)
{
	mbrush38_glpoly_t	*p, *bp;
	float		*v;
	int			i;
	float		s, t, os, ot;
	float		scroll;
	float		rdt = tr.refdef.floatTime;

	if (fa->texinfo->flags & BSP38SURF_FLOWING)
		scroll = -64 * ((tr.refdef.floatTime * 0.5) - (int)(tr.refdef.floatTime * 0.5));
	else
		scroll = 0;
	for (bp=fa->polys ; bp ; bp=bp->next)
	{
		p = bp;

		qglBegin (GL_TRIANGLE_FAN);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=BRUSH38_VERTEXSIZE)
		{
			os = v[3];
			ot = v[4];

#if !id386
			s = os + r_turbsin[(int)((ot*0.125+rdt) * TURBSCALE) & 255];
#else
			s = os + r_turbsin[Q_ftol( ((ot*0.125+rdt) * TURBSCALE) ) & 255];
#endif
			s += scroll;
			s *= (1.0/64);

#if !id386
			t = ot + r_turbsin[(int)((os*0.125+rdt) * TURBSCALE) & 255];
#else
			t = ot + r_turbsin[Q_ftol( ((os*0.125+rdt) * TURBSCALE) ) & 255];
#endif
			t *= (1.0/64);

			qglTexCoord2f (s, t);
			qglVertex3fv (v);
		}
		qglEnd ();
	}
}


//===================================================================

void EmitSkyVertex (float s, float t, int axis)
{
	vec3_t		v;
	float		St[2];	
	MakeSkyVec(s, t, axis, St, v);
	qglTexCoord2f(St[0], St[1]);
	qglVertex3fv(v);
}

/*
==============
R_DrawSkyBoxQ2
==============
*/
void R_DrawSkyBoxQ2 (void)
{
	int		i;

	if (skyrotate)
	{	// check for no sky at all
		for (i=0 ; i<6 ; i++)
			if (sky_mins[0][i] < sky_maxs[0][i]
			&& sky_mins[1][i] < sky_maxs[1][i])
				break;
		if (i == 6)
			return;		// nothing visible
	}

qglPushMatrix ();
qglTranslatef (tr.refdef.vieworg[0], tr.refdef.vieworg[1], tr.refdef.vieworg[2]);
qglRotatef (tr.refdef.floatTime * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);

	for (i=0 ; i<6 ; i++)
	{
		if (skyrotate)
		{	// hack, forces full sky to draw when rotating
			sky_mins[0][i] = -1;
			sky_mins[1][i] = -1;
			sky_maxs[0][i] = 1;
			sky_maxs[1][i] = 1;
		}

		if (sky_mins[0][i] >= sky_maxs[0][i]
		|| sky_mins[1][i] >= sky_maxs[1][i])
			continue;

		GL_Bind (sky_images[sky_texorder[i]]);

		qglBegin (GL_QUADS);
		EmitSkyVertex (sky_mins[0][i], sky_mins[1][i], i);
		EmitSkyVertex (sky_mins[0][i], sky_maxs[1][i], i);
		EmitSkyVertex (sky_maxs[0][i], sky_maxs[1][i], i);
		EmitSkyVertex (sky_maxs[0][i], sky_mins[1][i], i);
		qglEnd ();
	}
qglPopMatrix ();
}
