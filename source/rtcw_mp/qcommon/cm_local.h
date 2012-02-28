/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../wolfcore/clip_map/bsp46/local.h"
#include "../../core/file_formats/bsp47.h"

typedef struct {
	char name[MAX_QPATH];

	int numShaders;
	bsp46_dshader_t   *shaders;

	int numBrushSides;
	cbrushside_t *brushsides;

	int numPlanes;
	cplane_t    *planes;

	int numNodes;
	cNode_t     *nodes;

	int numLeafs;
	cLeaf_t     *leafs;

	int numLeafBrushes;
	int         *leafbrushes;

	int numLeafSurfaces;
	int         *leafsurfaces;

	int numSubModels;
	cmodel_t    *cmodels;

	int numBrushes;
	cbrush_t    *brushes;

	int numClusters;
	int clusterBytes;
	byte        *visibility;
	qboolean vised;             // if false, visibility is just a single cluster of ffs

	int numEntityChars;
	char        *entityString;

	int numAreas;
	cArea_t     *areas;
	int         *areaPortals;   // [ numAreas*numAreas ] reference counts

	int numSurfaces;
	cPatch_t    **surfaces;         // non-patches will be NULL

	int floodvalid;
	int checkcount;                         // incremented on each trace
} clipMap_t;


extern clipMap_t cm;

void CM_StoreLeafs( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t    *CM_ClipHandleToModel( clipHandle_t handle );

// cm_patch.c

struct patchCollide_t   *CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_t *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_t *pc );
void CM_ClearLevelPatches( void );
