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
// cmodel.c -- model loading

#include "cm_local.h"

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)


QClipMap46*	CMap;
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


QCvar		*cm_noAreas;
QCvar		*cm_noCurves;
QCvar		*cm_playerCurveClip;

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;



void	CM_InitBoxHull (void);
void	CM_FloodAreaConnections (void);

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	if ( !name || !name[0] ) {
		throw QDropException("CM_LoadMap: NULL name" );
	}

	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
	GLog.DWrite("CM_LoadMap( %s, %i )\n", name, clientload);

	if (CMap && !QStr::Cmp(CMap->name, name) && clientload)
	{
		*checksum = CMap->checksum;
		return;
	}

	// free old stuff
	CM_ClearMap();

	CMap = new QClipMap46;

	if (!name[0])
	{
		CMap->numLeafs = 1;
		CMap->numClusters = 1;
		CMap->numAreas = 1;
		CMap->cmodels = new cmodel_t[1];
		Com_Memset(CMap->cmodels, 0, sizeof(cmodel_t));
		*checksum = 0;
		return;
	}

	CMap->LoadMap(name);
	*checksum = CMap->checksum;

	CM_InitBoxHull();

	CM_FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if (!clientload)
	{
		QStr::NCpyZ(CMap->name, name, sizeof(CMap->name));
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
	if (CMap)
	{
		delete CMap;
		CMap = NULL;
	}
	CM_ClearLevelPatches();
}


/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle ) {
	if ( handle < 0 ) {
		throw QDropException(va("CM_ClipHandleToModel: bad handle %i", handle));
	}
	if ( handle < CMap->numSubModels ) {
		return &CMap->cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE ) {
		return &box_model;
	}
	if ( handle < MAX_SUBMODELS ) {
		throw QDropException(va("CM_ClipHandleToModel: bad handle %i < %i < %i", 
			CMap->numSubModels, handle, MAX_SUBMODELS));
	}
	throw QDropException(va("CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS));

	return NULL;

}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t	CM_InlineModel( int index ) {
	if ( index < 0 || index >= CMap->numSubModels ) {
		throw QDropException("CM_InlineModel: bad number");
	}
	return index;
}

int		CM_NumClusters( void ) {
	return CMap->numClusters;
}

int		CM_NumInlineModels( void ) {
	return CMap->numSubModels;
}

char	*CM_EntityString( void ) {
	return CMap->entityString;
}

int		CM_LeafCluster( int leafnum ) {
	if (leafnum < 0 || leafnum >= CMap->numLeafs) {
		throw QDropException("CM_LeafCluster: bad number");
	}
	return CMap->leafs[leafnum].cluster;
}

int		CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= CMap->numLeafs ) {
		throw QDropException("CM_LeafArea: bad number");
	}
	return CMap->leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cplane_t	*p;
	cbrushside_t	*s;

	box_planes = &CMap->planes[CMap->numPlanes];

	box_brush = &CMap->brushes[CMap->numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = CMap->brushsides + CMap->numBrushSides;
	box_brush->contents = BSP46CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cm.numBrushes;
	box_model.leaf.firstLeafBrush = CMap->numLeafBrushes;
	CMap->leafbrushes[CMap->numLeafBrushes] = CMap->numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &CMap->brushsides[CMap->numBrushSides + i];
		s->plane = CMap->planes + (CMap->numPlanes+i*2+side);
		s->surfaceFlags = 0;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;

		SetPlaneSignbits( p );
	}	
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {

	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t	*cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}


