/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.c

#include "tr_local.h"

/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *orient) {
	int		i;
	vec3_t	temp;

	for ( i = 0 ; i < count ; i++, dl++ ) {
		VectorSubtract( dl->origin, orient->origin, temp );
		dl->transformed[0] = DotProduct( temp, orient->axis[0] );
		dl->transformed[1] = DotProduct( temp, orient->axis[1] );
		dl->transformed[2] = DotProduct( temp, orient->axis[2] );
	}
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel( mbrush46_model_t *bmodel ) {
	int			i, j;
	dlight_t	*dl;
	int			mask;
	mbrush46_surface_t	*surf;

	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.orient );

	mask = 0;
	for ( i=0 ; i<tr.refdef.num_dlights ; i++ ) {
		dl = &tr.refdef.dlights[i];

		// see if the point is close enough to the bounds to matter
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( dl->transformed[j] - bmodel->bounds[1][j] > dl->radius ) {
				break;
			}
			if ( bmodel->bounds[0][j] - dl->transformed[j] > dl->radius ) {
				break;
			}
		}
		if ( j < 3 ) {
			continue;
		}

		// we need to check this light
		mask |= 1 << i;
	}

	tr.currentEntity->needDlights = (mask != 0);

	// set the dlight bits in all the surfaces
	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		surf = bmodel->firstSurface + i;

		if ( *surf->data == SF_FACE ) {
			((srfSurfaceFace_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_GRID ) {
			((srfGridMesh_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_TRIANGLES ) {
			((srfTriangles_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}
	}
}
