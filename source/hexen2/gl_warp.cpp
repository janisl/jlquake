// gl_warp.c -- sky and water polygons

#include "quakedef.h"
#include "glquake.h"

/*
=============
EmitWaterPolysQ1

Does a water warp on the pre-fragmented mbrush29_glpoly_t chain
=============
*/
void EmitWaterPolysQ1 (mbrush29_surface_t *fa)
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

			s = os + r_turbsin[(int)((ot*0.125+realtime) * TURBSCALE) & 255];
			s *= (1.0/64);

			t = ot + r_turbsin[(int)((os*0.125+realtime) * TURBSCALE) & 255];
			t *= (1.0/64);

			qglTexCoord2f (s, t);
			qglVertex3fv (v);
		}
		qglEnd ();
	}
}
