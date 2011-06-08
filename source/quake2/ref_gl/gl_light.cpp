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
// r_light.c

#include "gl_local.h"

int	r_dlightframecount;

#define	DLIGHT_CUTOFF	64

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
void R_MarkLights (dlight_t *light, int bit, mbrush38_node_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	mbrush38_surface_t	*surf;
	int			i;
	
	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
	
	if (dist > light->radius-DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius+DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[1]);
		return;
	}
		
// mark the polygons
	surf = tr.worldModel->brush38_surfaces + node->firstsurface;
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
	for (i=0 ; i<tr.refdef.num_dlights ; i++, l++)
		R_MarkLights ( l, 1<<i, tr.worldModel->brush38_nodes );
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

int RecursiveLightPoint (mbrush38_node_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	mbrush38_surface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mbrush38_texinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;

	if (node->contents != -1)
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

	surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&(BRUSH38_SURF_DRAWTURB|BRUSH38_SURF_DRAWSKY)) 
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
		VectorCopy (vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3*(dt * ((surf->extents[0]>>4)+1) + ds);

			for (maps = 0 ; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
				for (i=0 ; i<3 ; i++)
					scale[i] = gl_modulate->value*tr.refdef.lightstyles[surf->styles[maps]].rgb[i];

				pointcolor[0] += lightmap[0] * scale[0] * (1.0/255);
				pointcolor[1] += lightmap[1] * scale[1] * (1.0/255);
				pointcolor[2] += lightmap[2] * scale[2] * (1.0/255);
				lightmap += 3*((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
		}
		
		return 1;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t p, vec3_t color)
{
	vec3_t		end;
	float		r;
	int			lnum;
	dlight_t	*dl;
	float		light;
	vec3_t		dist;
	float		add;
	
	if (!tr.worldModel->brush38_lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (tr.worldModel->brush38_nodes, p, end);
	
	if (r == -1)
	{
		VectorCopy (vec3_origin, color);
	}
	else
	{
		VectorCopy (pointcolor, color);
	}

	//
	// add dynamic lights
	//
	light = 0;
	dl = tr.refdef.dlights;
	for (lnum=0 ; lnum<tr.refdef.num_dlights ; lnum++, dl++)
	{
		VectorSubtract (tr.currentEntity->e.origin,
						dl->origin,
						dist);
		add = dl->radius - VectorLength(dist);
		add *= (1.0/256);
		if (add > 0)
		{
			VectorMA (color, add, dl->color, color);
		}
	}

	VectorScale (color, gl_modulate->value, color);
}


//===================================================================

static float s_blocklights[34*34*3];
/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (mbrush38_surface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		fdist, frad, fminlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mbrush38_texinfo_t	*tex;
	dlight_t	*dl;
	float		*pfBL;
	float		fsacc, ftacc;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<tr.refdef.num_dlights ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		dl = &tr.refdef.dlights[lnum];
		frad = dl->radius;
		fdist = DotProduct (dl->origin, surf->plane->normal) -
				surf->plane->dist;
		frad -= fabs(fdist);
		// rad is now the highest intensity on the plane

		fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?
		if (frad < fminlight)
			continue;
		fminlight = frad - fminlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = dl->origin[i] -
					surf->plane->normal[i]*fdist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		pfBL = s_blocklights;
		for (t = 0, ftacc = 0 ; t<tmax ; t++, ftacc += 16)
		{
			td = local[1] - ftacc;
			if ( td < 0 )
				td = -td;

			for ( s=0, fsacc = 0 ; s<smax ; s++, fsacc += 16, pfBL += 3)
			{
				sd = Q_ftol( local[0] - fsacc );

				if ( sd < 0 )
					sd = -sd;

				if (sd > td)
					fdist = sd + (td>>1);
				else
					fdist = td + (sd>>1);

				if ( fdist < fminlight )
				{
					pfBL[0] += ( frad - fdist ) * dl->color[0];
					pfBL[1] += ( frad - fdist ) * dl->color[1];
					pfBL[2] += ( frad - fdist ) * dl->color[2];
				}
			}
		}
	}
}


/*
** R_SetCacheState
*/
void R_SetCacheState( mbrush38_surface_t *surf )
{
	int maps;

	for (maps = 0 ; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = tr.refdef.lightstyles[surf->styles[maps]].white;
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (mbrush38_surface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			r, g, b, a, max;
	int			i, j, size;
	byte		*lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;
	lightstyle_t	*style;

	if ( surf->texinfo->flags & (BSP38SURF_SKY|BSP38SURF_TRANS33|BSP38SURF_TRANS66|BSP38SURF_WARP) )
		ri.Sys_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	if (size > (sizeof(s_blocklights)>>4) )
		ri.Sys_Error (ERR_DROP, "Bad s_blocklights size");

// set to full bright if no light data
	if (!surf->samples)
	{
		int maps;

		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		for (maps = 0 ; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			style = &tr.refdef.lightstyles[surf->styles[maps]];
		}
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0 ; nummaps < BSP38_MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value * tr.refdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		Com_Memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );

		for (maps = 0 ; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value * tr.refdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == tr.frameCount)
		R_AddDynamicLights (surf);

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			
			r = Q_ftol( bl[0] );
			g = Q_ftol( bl[1] );
			b = Q_ftol( bl[2] );

			// catch negative lights
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;

			/*
			** determine the brightest of the three color components
			*/
			if (r > g)
				max = r;
			else
				max = g;
			if (b > max)
				max = b;

			/*
			** alpha is ONLY used for the mono lightmap case.  For this reason
			** we set it to the brightest of the color components so that 
			** things don't get too dim.
			*/
			a = max;

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255)
			{
				float t = 255.0F / max;

				r = r*t;
				g = g*t;
				b = b*t;
				a = a*t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}
}

