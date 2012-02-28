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

// cmodel.c -- model loading

#include "cm_local.h"

clipMap_t cm;


byte        *cmod_base;

cmodel_t box_model;
cplane_t    *box_planes;
cbrush_t    *box_brush;



void    CM_InitBoxHull( void );
void    CM_FloodAreaConnections( void );


//==================================================================


#if 0 //BSPC
/*
==================
CM_FreeMap

Free any loaded map and all submodels
==================
*/
void CM_FreeMap( void ) {
	memset( &cm, 0, sizeof( cm ) );
	Hunk_ClearHigh();
	CM_ClearLevelPatches();
}
#endif //BSPC

unsigned CM_LumpChecksum( bsp46_lump_t *lump ) {
	return LittleLong( Com_BlockChecksum( cmod_base + lump->fileofs, lump->filelen ) );
}

unsigned CM_Checksum( bsp46_dheader_t *header ) {
	unsigned checksums[16];
	checksums[0] = CM_LumpChecksum( &header->lumps[BSP46LUMP_SHADERS] );
	checksums[1] = CM_LumpChecksum( &header->lumps[BSP46LUMP_LEAFS] );
	checksums[2] = CM_LumpChecksum( &header->lumps[BSP46LUMP_LEAFBRUSHES] );
	checksums[3] = CM_LumpChecksum( &header->lumps[BSP46LUMP_LEAFSURFACES] );
	checksums[4] = CM_LumpChecksum( &header->lumps[BSP46LUMP_PLANES] );
	checksums[5] = CM_LumpChecksum( &header->lumps[BSP46LUMP_BRUSHSIDES] );
	checksums[6] = CM_LumpChecksum( &header->lumps[BSP46LUMP_BRUSHES] );
	checksums[7] = CM_LumpChecksum( &header->lumps[BSP46LUMP_MODELS] );
	checksums[8] = CM_LumpChecksum( &header->lumps[BSP46LUMP_NODES] );
	checksums[9] = CM_LumpChecksum( &header->lumps[BSP46LUMP_SURFACES] );
	checksums[10] = CM_LumpChecksum( &header->lumps[BSP46LUMP_DRAWVERTS] );

	return LittleLong( Com_BlockChecksum( checksums, 11 * 4 ) );
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	int             *buf;
	int i;
	bsp46_dheader_t header;
	int length;
	static unsigned last_checksum;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get( "cm_noAreas", "0", CVAR_CHEAT );
	cm_noCurves = Cvar_Get( "cm_noCurves", "0", CVAR_CHEAT );
	cm_playerCurveClip = Cvar_Get( "cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !String::Cmp( cm.name, name ) && clientload ) {
		*checksum = last_checksum;
		return;
	}

	// free old stuff
	memset( &cm, 0, sizeof( cm ) );
	if (cm46)
		delete cm46;
	cm46 = new QClipMap46();
	CMapShared = cm46;

	if ( !name[0] ) {
		cm46->numLeafs = 1;
		cm46->numClusters = 1;
		cm46->numAreas = 1;
		cm46->cmodels = (cmodel_t*)Hunk_Alloc( sizeof( *cm46->cmodels ), h_high );
		*checksum = 0;
		return;
	}

	//
	// load the file
	//
#ifndef BSPC
	length = FS_ReadFile( name, (void **)&buf );
#else
	length = LoadQuakeFile( (quakefile_t *) name, (void **)&buf );
#endif

	if ( !buf ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	last_checksum = LittleLong( Com_BlockChecksum( buf, length ) );
	*checksum = last_checksum;

	header = *(bsp46_dheader_t *)buf;
	for ( i = 0 ; i < sizeof( bsp46_dheader_t ) / 4 ; i++ ) {
		( (int *)&header )[i] = LittleLong( ( (int *)&header )[i] );
	}

	if ( header.version != BSP47_VERSION ) {
		Com_Error( ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
				   , name, header.version, BSP47_VERSION );
	}

	cmod_base = (byte *)buf;

	// load into heap
	cm46->LoadShaders(cmod_base, &header.lumps[BSP46LUMP_SHADERS] );
	cm46->LoadLeafs(cmod_base, &header.lumps[BSP46LUMP_LEAFS] );
	cm46->LoadLeafBrushes(cmod_base, &header.lumps[BSP46LUMP_LEAFBRUSHES] );
	cm46->LoadLeafSurfaces(cmod_base, &header.lumps[BSP46LUMP_LEAFSURFACES] );
	cm46->LoadPlanes(cmod_base, &header.lumps[BSP46LUMP_PLANES] );
	cm46->LoadBrushSides(cmod_base, &header.lumps[BSP46LUMP_BRUSHSIDES] );
	cm46->LoadBrushes(cmod_base, &header.lumps[BSP46LUMP_BRUSHES] );
	cm46->LoadSubmodels(cmod_base, &header.lumps[BSP46LUMP_MODELS] );
	cm46->LoadNodes(cmod_base, &header.lumps[BSP46LUMP_NODES] );
	cm46->LoadEntityString(cmod_base, &header.lumps[BSP46LUMP_ENTITIES] );
	cm46->LoadVisibility(cmod_base, &header.lumps[BSP46LUMP_VISIBILITY] );
	cm46->LoadPatches(cmod_base, &header.lumps[BSP46LUMP_SURFACES], &header.lumps[BSP46LUMP_DRAWVERTS] );

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile( buf );

	CM_InitBoxHull();

	CM_FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		String::NCpyZ( cm.name, name, sizeof( cm.name ) );
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void ) {
	Com_Memset( &cm, 0, sizeof( cm ) );
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
		return &box_model;
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
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull( void ) {
	int i;
	int side;
	cplane_t    *p;
	cbrushside_t    *s;

	box_planes = &cm46->planes[cm46->numPlanes];

	box_brush = &cm46->brushes[cm46->numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cm46->brushsides + cm46->numBrushSides;
	box_brush->contents = BSP46CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cm46->numBrushes;
	box_model.leaf.firstLeafBrush = cm46->numLeafBrushes;
	cm46->leafbrushes[cm46->numLeafBrushes] = cm46->numBrushes;

	for ( i = 0 ; i < 6 ; i++ )
	{
		side = i & 1;

		// brush sides
		s = &cm46->brushsides[cm46->numBrushSides + i];
		s->plane =  cm46->planes + ( cm46->numPlanes + i * 2 + side );
		s->surfaceFlags = 0;

		// planes
		p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + ( i >> 1 );
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = -1;

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

// DHM - Nerve
void CM_SetTempBoxModelContents( int contents ) {

	box_brush->contents = contents;
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


