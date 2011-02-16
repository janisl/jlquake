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

#include "../../../libs/core/core.h"
#include "../../../libs/core/bsp46file.h"
#include "../../../libs/core/cm46_local.h"
#include "cm_public.h"

#define	BOX_MODEL_HANDLE		255
#define CAPSULE_MODEL_HANDLE	254


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

extern	int			c_pointcontents;
extern	int			c_traces, c_brush_traces, c_patch_traces;
extern	QCvar		*cm_noAreas;
extern	QCvar		*cm_noCurves;
extern	QCvar		*cm_playerCurveClip;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct
{
	qboolean	use;
	float		radius;
	float		halfheight;
	vec3_t		offset;
} sphere_t;

typedef struct {
	vec3_t		start;
	vec3_t		end;
	vec3_t		size[2];	// size of the box being swept through the model
	vec3_t		offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float		maxOffset;	// longest corner length from origin
	vec3_t		extents;	// greatest of abs(size[0]) and abs(size[1])
	vec3_t		bounds[2];	// enclosing box of start and end surrounding by size
	vec3_t		modelOrigin;// origin of the model tracing through
	int			contents;	// ored contents of the model tracing through
	qboolean	isPoint;	// optimized case
	trace_t		trace;		// returned from trace call
	sphere_t	sphere;		// sphere for oriendted capsule collision
} traceWork_t;

typedef struct leafList_s {
	int		count;
	int		maxcount;
	qboolean	overflowed;
	int		*list;
	vec3_t	bounds[2];
	int		lastLeaf;		// for overflows where each leaf can't be stored individually
	void	(*storeLeafs)( struct leafList_s *ll, int nodenum );
} leafList_t;


int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t **list, int listsize );

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle );

void CM_TraceThroughPatchCollide( traceWork_t *tw, const patchCollide_t* pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const patchCollide_t* pc );
void CM_ClearLevelPatches( void );

extern QClipMap46*	CMap;
