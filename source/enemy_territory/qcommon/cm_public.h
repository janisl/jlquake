/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "qfiles.h"

#include "../cgame/tr_types.h"

void        CM_LoadMap( const char *name, qboolean clientload, int *checksum );
void        CM_ClearMap( void );

void        CM_SetTempBoxModelContents( int contents );     // DHM - Nerve

void        CM_BoxTrace( q3trace_t *results, const vec3_t start, const vec3_t end,
						 const vec3_t mins, const vec3_t maxs,
						 clipHandle_t model, int brushmask, int capsule );
void        CM_TransformedBoxTrace( q3trace_t *results, const vec3_t start, const vec3_t end,
									const vec3_t mins, const vec3_t maxs,
									clipHandle_t model, int brushmask,
									const vec3_t origin, const vec3_t angles, int capsule );

// cm_tag.c
int         CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );


// cm_marks.c
int CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
					  int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );
