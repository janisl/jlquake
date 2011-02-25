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

#ifndef __cm_public_h__
#define __cm_public_h__

void		CM_LoadMap( const char *name, qboolean clientload, int *checksum);
void		CM_ClearMap( void );

void		CM_BoxTrace ( q3trace_t *results, const vec3_t start, const vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  clipHandle_t model, int brushmask, int capsule );
void		CM_TransformedBoxTrace( q3trace_t *results, const vec3_t start, const vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles, int capsule );

void		CM_AdjustAreaPortalState( int area1, int area2, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );

// cm_patch.c
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, float *points) );

#endif
