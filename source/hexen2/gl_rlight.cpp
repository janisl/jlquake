// r_light.c

#include "quakedef.h"
#include "glquake.h"

int	r_dlightframecount;


//==========================================================================
//
// R_AnimateLight
//
//==========================================================================

void R_AnimateLight(void)
{
	int i;
	int v;
	int c;
	int defaultLocus;
	int locusHz[3];

	defaultLocus = locusHz[0] = (int)(cl.time*10);
	locusHz[1] = (int)(cl.time*20);
	locusHz[2] = (int)(cl.time*30);
	for(i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if(!cl_lightstyle[i].length)
		{ // No style def
			d_lightstylevalue[i] = 256;
			continue;
		}
		c = cl_lightstyle[i].map[0];
		if(c == '1' || c == '2' || c == '3')
		{ // Explicit anim rate
			if(cl_lightstyle[i].length == 1)
			{ // Bad style def
				d_lightstylevalue[i] = 256;
				continue;
			}
			v = locusHz[c-'1']%(cl_lightstyle[i].length-1);
			d_lightstylevalue[i] = (cl_lightstyle[i].map[v+1]-'a')*22;
			continue;
		}
		// Default anim rate (10 Hz)
		v = defaultLocus%cl_lightstyle[i].length;
		d_lightstylevalue[i] = (cl_lightstyle[i].map[v]-'a')*22;
	}
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int bit, mbrush29_node_t *node)
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
		R_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		R_MarkLights (light, bit, node->children[1]);
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

	R_MarkLights (light, bit, node->children[0]);
	R_MarkLights (light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	r_dlightframecount = tr.frameCount;
	l = tr.refdef.dlights;

	for (i=0 ; i<tr.refdef.num_dlights; i++, l++)
	{
		R_MarkLights ( l, 1<<i, tr.worldModel->brush29_nodes );
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

cplane_t		*lightplane;

int RecursiveLightPoint (mbrush29_node_t *node, vec3_t start, vec3_t end)
{
	int			r;
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	mbrush29_surface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mbrush29_texinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;
	int			maps;

	if (node->contents < 0)
		return -1;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = tr.worldModel->brush29_surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & BRUSH29_SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;
		
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			for (maps = 0 ; maps < BSP29_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
				lightmap += ((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
			
			r >>= 8;
		}
		
		return r;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

int R_LightPoint (vec3_t p)
{
	vec3_t		end;
	int			r;
	
	if (!tr.worldModel->brush29_lightdata)
		return 255;
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (tr.worldModel->brush29_nodes, p, end);
	
	if (r == -1)
		r = 0;

	return r;
}

