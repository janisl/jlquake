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
// r_light.c

#include "quakedef.h"
#include "glquake.h"

int	r_dlightframecount;


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j,k;
	
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl.time*10);
	for (j=0 ; j<MAX_LIGHTSTYLES_Q1 ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		k = k*22;
		d_lightstylevalue[j] = k;
	}	
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLightsQ1
=============
*/
void R_MarkLightsQ1 (dlight_t *light, int bit, mbrush29_node_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	mbrush29_surface_t	*surf;
	int			i;
	
	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
	
	if (dist > light->radius)
	{
		R_MarkLightsQ1 (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		R_MarkLightsQ1 (light, bit, node->children[1]);
		return;
	}
		
// mark the polygons
	surf = tr.worldModel->brush29_surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLightsQ1 (light, bit, node->children[0]);
	R_MarkLightsQ1 (light, bit, node->children[1]);
}


/*
=============
R_PushDlightsQ1
=============
*/
void R_PushDlightsQ1 (void)
{
	int		i;
	dlight_t	*l;

	r_dlightframecount = tr.frameCount;
	l = tr.refdef.dlights;

	for (i=0 ; i<tr.refdef.num_dlights; i++, l++)
	{
		R_MarkLightsQ1 ( l, 1<<i, tr.worldModel->brush29_nodes );
	}
}
