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

// cmodel.c -- model loading

#include "cm_local.h"

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	int length;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if (cm46 && cm46->Name == name && clientload ) {
		*checksum = cm46->CheckSum;
		return;
	}

	// free old stuff
	if (cm46)
		delete cm46;
	cm46 = new QClipMap46();
	CMapShared = cm46;

	//
	// load the file
	//
	Array<quint8> buffer;
	length = FS_ReadFile( name, buffer);

	if ( !length ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	cm46->LoadMap(name, buffer);
	*checksum = cm46->CheckSum;
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void ) {
	if (cm46)
		delete cm46;
	cm46 = NULL;
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t    *CM_ClipHandleToModel( clipHandle_t handle ) {
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cm46->numSubModels ) {
		return &cm46->cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE ) {
		return &cm46->box_model;
	}
	if ( handle < MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i",
				   cm46->numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;

}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t    CM_InlineModel( int index ) {
	if ( index < 0 || index >= cm46->numSubModels ) {
		Com_Error( ERR_DROP, "CM_InlineModel: bad number" );
	}
	return index;
}

int     CM_NumClusters( void ) {
	return cm46->numClusters;
}

int     CM_NumInlineModels( void ) {
	return cm46->numSubModels;
}

char    *CM_EntityString( void ) {
	return cm46->entityString;
}

int     CM_LeafCluster( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cm46->numLeafs ) {
		Com_Error( ERR_DROP, "CM_LeafCluster: bad number" );
	}
	return cm46->leafs[leafnum].cluster;
}

int     CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cm46->numLeafs ) {
		Com_Error( ERR_DROP, "CM_LeafArea: bad number" );
	}
	return cm46->leafs[leafnum].area;
}

//=======================================================================

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {

	VectorCopy( mins, cm46->box_model.mins );
	VectorCopy( maxs, cm46->box_model.maxs );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	cm46->box_planes[0].dist = maxs[0];
	cm46->box_planes[1].dist = -maxs[0];
	cm46->box_planes[2].dist = mins[0];
	cm46->box_planes[3].dist = -mins[0];
	cm46->box_planes[4].dist = maxs[1];
	cm46->box_planes[5].dist = -maxs[1];
	cm46->box_planes[6].dist = mins[1];
	cm46->box_planes[7].dist = -mins[1];
	cm46->box_planes[8].dist = maxs[2];
	cm46->box_planes[9].dist = -maxs[2];
	cm46->box_planes[10].dist = mins[2];
	cm46->box_planes[11].dist = -mins[2];

	VectorCopy( mins, cm46->box_brush->bounds[0] );
	VectorCopy( maxs, cm46->box_brush->bounds[1] );

	return BOX_MODEL_HANDLE;
}

// DHM - Nerve
void CM_SetTempBoxModelContents( int contents ) {

	cm46->box_brush->contents = contents;
}
// dhm

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t    *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}


