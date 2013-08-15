//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "SurfaceFlare.h"
#include "surfaces.h"

void idSurfaceFlare::Draw() {
	srfFlare_t* surf = ( srfFlare_t* )data;
#if 0
	// calculate the xyz locations for the four corners
	float radius = 30;
	vec3_t left, up;
	VectorScale( backEnd.viewParms.orient.axis[ 1 ], radius, left );
	VectorScale( backEnd.viewParms.orient.axis[ 2 ], radius, up );
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	byte color[ 4 ];
	color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 255;

	vec3_t origin;
	VectorMA( surf->origin, 3, surf->normal, origin );
	vec3_t dir;
	VectorSubtract( origin, backEnd.viewParms.orient.origin, dir );
	VectorNormalize( dir );
	VectorMA( origin, r_ignore->value, dir, origin );

	float d = -DotProduct( dir, surf->normal );
	if ( d < 0 ) {
		return;
	}
#if 0
	color[ 0 ] *= d;
	color[ 1 ] *= d;
	color[ 2 ] *= d;
#endif

	RB_AddQuadStamp( origin, left, up, color );
#elif 0
	byte color[ 4 ];
	color[ 0 ] = surf->color[ 0 ] * 255;
	color[ 1 ] = surf->color[ 1 ] * 255;
	color[ 2 ] = surf->color[ 2 ] * 255;
	color[ 3 ] = 255;

	vec3_t left, up;
	VectorClear( left );
	VectorClear( up );

	left[ 0 ] = r_ignore->value;

	up[ 1 ] = r_ignore->value;

	RB_AddQuadStampExt( surf->origin, left, up, color, 0, 0, 1, 1 );
#endif
}
