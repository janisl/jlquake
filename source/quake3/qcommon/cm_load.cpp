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
#include "cm_patch.h"

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)


clipMap_t	cm;
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


byte		*cmod_base;

QCvar		*cm_noAreas;
QCvar		*cm_noCurves;
QCvar		*cm_playerCurveClip;

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;



void	CM_InitBoxHull (void);
void	CM_FloodAreaConnections (void);


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders( bsp46_lump_t *l ) {
	bsp46_dshader_t	*in, *out;
	int			i, count;

	in = (bsp46_dshader_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		throw QDropException("CMod_LoadShaders: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	if (count < 1) {
		throw QDropException("Map with no shaders");
	}
	cm.shaders = new bsp46_dshader_t[count];
	cm.numShaders = count;

	Com_Memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );

	out = cm.shaders;
	for ( i=0 ; i<count ; i++, in++, out++ ) {
		out->contentFlags = LittleLong( out->contentFlags );
		out->surfaceFlags = LittleLong( out->surfaceFlags );
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( bsp46_lump_t *l ) {
	bsp46_dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	int			*indexes;

	in = (bsp46_dmodel_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("CMod_LoadSubmodels: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		throw QDropException("Map with no models");
	cm.cmodels = new cmodel_t[count];
	Com_Memset(cm.cmodels, 0, sizeof(cmodel_t) * count);
	cm.numSubModels = count;

	if ( count > MAX_SUBMODELS ) {
		throw QDropException("MAX_SUBMODELS exceeded" );
	}

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &cm.cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}

		if ( i == 0 ) {
			continue;	// world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = new int[out->leaf.numLeafBrushes];
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for ( j = 0 ; j < out->leaf.numLeafBrushes ; j++ ) {
			indexes[j] = LittleLong( in->firstBrush ) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
		indexes = new int[out->leaf.numLeafSurfaces];
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for ( j = 0 ; j < out->leaf.numLeafSurfaces ; j++ ) {
			indexes[j] = LittleLong( in->firstSurface ) + j;
		}
	}
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes( bsp46_lump_t *l ) {
	bsp46_dnode_t		*in;
	int			child;
	cNode_t		*out;
	int			i, j, count;
	
	in = (bsp46_dnode_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		throw QDropException("Map has no nodes");
	cm.nodes = new cNode_t[count];
	Com_Memset(cm.nodes, 0, sizeof(cNode_t) * count);
	cm.numNodes = count;

	out = cm.nodes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = cm.planes + LittleLong( in->planeNum );
		for (j=0 ; j<2 ; j++)
		{
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
	}

}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b ) {
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes( bsp46_lump_t *l ) {
	bsp46_dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;

	in = (bsp46_dbrush_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushes = new cbrush_t[BOX_BRUSHES + count];
	Com_Memset(cm.brushes, 0, sizeof(cbrush_t) * (BOX_BRUSHES + count));
	cm.numBrushes = count;

	out = cm.brushes;

	for ( i=0 ; i<count ; i++, out++, in++ ) {
		out->sides = cm.brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			throw QDropException(va("CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum));
		}
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush( out );
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs (bsp46_lump_t *l)
{
	int			i;
	cLeaf_t		*out;
	bsp46_dleaf_t 	*in;
	int			count;
	
	in = (bsp46_dleaf_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		throw QDropException("Map with no leafs");

	cm.leafs = new cLeaf_t[BOX_LEAFS + count];
	Com_Memset(cm.leafs, 0, sizeof(cLeaf_t) * (BOX_LEAFS + count));
	cm.numLeafs = count;

	out = cm.leafs;	
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->cluster = LittleLong (in->cluster);
		out->area = LittleLong (in->area);
		out->firstLeafBrush = LittleLong (in->firstLeafBrush);
		out->numLeafBrushes = LittleLong (in->numLeafBrushes);
		out->firstLeafSurface = LittleLong (in->firstLeafSurface);
		out->numLeafSurfaces = LittleLong (in->numLeafSurfaces);

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
		if (out->area >= cm.numAreas)
			cm.numAreas = out->area + 1;
	}

	cm.areas = new cArea_t[cm.numAreas];
	Com_Memset(cm.areas, 0, sizeof(cArea_t) * cm.numAreas);
	cm.areaPortals = new int[cm.numAreas * cm.numAreas];
	Com_Memset(cm.areaPortals, 0, sizeof(int) * cm.numAreas * cm.numAreas);
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes (bsp46_lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	bsp46_dplane_t 	*in;
	int			count;
	
	in = (bsp46_dplane_t*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		throw QDropException("Map with no planes");
	cm.planes = new cplane_t[BOX_PLANES + count];
	Com_Memset(cm.planes, 0, sizeof(cplane_t) * (BOX_PLANES + count));
	cm.numPlanes = count;

	out = cm.planes;	

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
		}
		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal( out->normal );

		SetPlaneSignbits(out);
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes (bsp46_lump_t *l)
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;
	
	in = (int*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafbrushes = new int[count + BOX_BRUSHES];
	Com_Memset(cm.leafbrushes, 0, sizeof(int) * (count + BOX_BRUSHES));
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for ( i=0 ; i<count ; i++, in++, out++) {
		*out = LittleLong (*in);
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces( bsp46_lump_t *l )
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;
	
	in = (int*)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafsurfaces = new int[count];
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for ( i=0 ; i<count ; i++, in++, out++) {
		*out = LittleLong (*in);
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides (bsp46_lump_t *l)
{
	int				i;
	cbrushside_t	*out;
	bsp46_dbrushside_t 	*in;
	int				count;
	int				num;

	in = (bsp46_dbrushside_t*)(cmod_base + l->fileofs);
	if ( l->filelen % sizeof(*in) ) {
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushsides = new cbrushside_t[BOX_SIDES + count];
	Com_Memset(cm.brushsides, 0, sizeof(cbrushside_t) * (BOX_SIDES + count));
	cm.numBrushSides = count;

	out = cm.brushsides;	

	for ( i=0 ; i<count ; i++, in++, out++) {
		num = LittleLong( in->planeNum );
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			throw QDropException(va("CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum));
		}
		out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( bsp46_lump_t *l )
{
	cm.entityString = new char[l->filelen];
	cm.numEntityChars = l->filelen;
	Com_Memcpy (cm.entityString, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadVisibility
=================
*/
#define	VIS_HEADER	8
void CMod_LoadVisibility( bsp46_lump_t *l ) {
	int		len;
	byte	*buf;

    len = l->filelen;
	if ( !len ) {
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = new byte[cm.clusterBytes];
		Com_Memset(cm.visibility, 255, cm.clusterBytes);
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = true;
	cm.visibility = new byte[len];
	cm.numClusters = LittleLong( ((int *)buf)[0] );
	cm.clusterBytes = LittleLong( ((int *)buf)[1] );
	Com_Memcpy(cm.visibility, buf + VIS_HEADER, len - VIS_HEADER);
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define	MAX_PATCH_VERTS		1024
void CMod_LoadPatches( bsp46_lump_t *surfs, bsp46_lump_t *verts ) {
	bsp46_drawVert_t	*dv, *dv_p;
	bsp46_dsurface_t	*in;
	int			count;
	int			i, j;
	int			c;
	cPatch_t	*patch;
	vec3_t		points[MAX_PATCH_VERTS];
	int			width, height;
	int			shaderNum;

	in = (bsp46_dsurface_t*)(cmod_base + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		throw QDropException("MOD_LoadBmodel: funny lump size");
	cm.numSurfaces = count = surfs->filelen / sizeof(*in);
	cm.surfaces = new cPatch_t*[cm.numSurfaces];
	Com_Memset(cm.surfaces, 0, sizeof(cPatch_t*) * cm.numSurfaces);

	dv = (bsp46_drawVert_t*)(cmod_base + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		throw QDropException("MOD_LoadBmodel: funny lump size");

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for ( i = 0 ; i < count ; i++, in++ ) {
		if ( LittleLong( in->surfaceType ) != BSP46MST_PATCH ) {
			continue;		// ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cm.surfaces[ i ] = patch = new cPatch_t;
		Com_Memset(patch, 0, sizeof(*patch));

		// load the full drawverts onto the stack
		width = LittleLong( in->patchWidth );
		height = LittleLong( in->patchHeight );
		c = width * height;
		if ( c > MAX_PATCH_VERTS ) {
			throw QDropException("ParseMesh: MAX_PATCH_VERTS" );
		}

		dv_p = dv + LittleLong( in->firstVert );
		for ( j = 0 ; j < c ; j++, dv_p++ ) {
			points[j][0] = LittleFloat( dv_p->xyz[0] );
			points[j][1] = LittleFloat( dv_p->xyz[1] );
			points[j][2] = LittleFloat( dv_p->xyz[2] );
		}

		shaderNum = LittleLong( in->shaderNum );
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points );
	}
}

//==================================================================

unsigned CM_LumpChecksum(bsp46_lump_t *lump) {
	return LittleLong (Com_BlockChecksum (cmod_base + lump->fileofs, lump->filelen));
}

unsigned CM_Checksum(bsp46_dheader_t *header) {
	unsigned checksums[16];
	checksums[0] = CM_LumpChecksum(&header->lumps[BSP46LUMP_SHADERS]);
	checksums[1] = CM_LumpChecksum(&header->lumps[BSP46LUMP_LEAFS]);
	checksums[2] = CM_LumpChecksum(&header->lumps[BSP46LUMP_LEAFBRUSHES]);
	checksums[3] = CM_LumpChecksum(&header->lumps[BSP46LUMP_LEAFSURFACES]);
	checksums[4] = CM_LumpChecksum(&header->lumps[BSP46LUMP_PLANES]);
	checksums[5] = CM_LumpChecksum(&header->lumps[BSP46LUMP_BRUSHSIDES]);
	checksums[6] = CM_LumpChecksum(&header->lumps[BSP46LUMP_BRUSHES]);
	checksums[7] = CM_LumpChecksum(&header->lumps[BSP46LUMP_MODELS]);
	checksums[8] = CM_LumpChecksum(&header->lumps[BSP46LUMP_NODES]);
	checksums[9] = CM_LumpChecksum(&header->lumps[BSP46LUMP_SURFACES]);
	checksums[10] = CM_LumpChecksum(&header->lumps[BSP46LUMP_DRAWVERTS]);

	return LittleLong(Com_BlockChecksum(checksums, 11 * 4));
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	int				*buf;
	int				i;
	bsp46_dheader_t		header;
	int				length;
	static unsigned	last_checksum;

	if ( !name || !name[0] ) {
		throw QDropException("CM_LoadMap: NULL name" );
	}

	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
	GLog.DWrite("CM_LoadMap( %s, %i )\n", name, clientload);

	if ( !QStr::Cmp( cm.name, name ) && clientload ) {
		*checksum = last_checksum;
		return;
	}

	// free old stuff
	CM_ClearMap();

	if ( !name[0] ) {
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = new cmodel_t[1];
		Com_Memset(cm.cmodels, 0, sizeof(cmodel_t));
		*checksum = 0;
		return;
	}

	//
	// load the file
	//
	length = FS_ReadFile( name, (void **)&buf );

	if ( !buf ) {
		throw QDropException(va("Couldn't load %s", name));
	}

	last_checksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(bsp46_dheader_t *)buf;
	for (i=0 ; i<sizeof(bsp46_dheader_t)/4 ; i++) {
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);
	}

	if ( header.version != BSP46_VERSION ) {
		throw QDropException(va("CM_LoadMap: %s has wrong version number (%i should be %i)"
		, name, header.version, BSP46_VERSION));
	}

	cmod_base = (byte *)buf;

	// load into heap
	CMod_LoadShaders( &header.lumps[BSP46LUMP_SHADERS] );
	CMod_LoadLeafs (&header.lumps[BSP46LUMP_LEAFS]);
	CMod_LoadLeafBrushes (&header.lumps[BSP46LUMP_LEAFBRUSHES]);
	CMod_LoadLeafSurfaces (&header.lumps[BSP46LUMP_LEAFSURFACES]);
	CMod_LoadPlanes (&header.lumps[BSP46LUMP_PLANES]);
	CMod_LoadBrushSides (&header.lumps[BSP46LUMP_BRUSHSIDES]);
	CMod_LoadBrushes (&header.lumps[BSP46LUMP_BRUSHES]);
	CMod_LoadSubmodels (&header.lumps[BSP46LUMP_MODELS]);
	CMod_LoadNodes (&header.lumps[BSP46LUMP_NODES]);
	CMod_LoadEntityString (&header.lumps[BSP46LUMP_ENTITIES]);
	CMod_LoadVisibility( &header.lumps[BSP46LUMP_VISIBILITY] );
	CMod_LoadPatches( &header.lumps[BSP46LUMP_SURFACES], &header.lumps[BSP46LUMP_DRAWVERTS] );

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile (buf);

	CM_InitBoxHull ();

	CM_FloodAreaConnections ();

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		QStr::NCpyZ( cm.name, name, sizeof( cm.name ) );
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
	delete[] cm.shaders;
	for (int i = 1; i < cm.numSubModels; i++)
	{
		delete[] (cm.leafbrushes + cm.cmodels[i].leaf.firstLeafBrush);
		delete[] (cm.leafsurfaces + cm.cmodels[i].leaf.firstLeafSurface);
	}
	delete[] cm.cmodels;
	delete[] cm.nodes;
	delete[] cm.brushes;
	delete[] cm.leafs;
	delete[] cm.areas;
	delete[] cm.areaPortals;
	delete[] cm.planes;
	delete[] cm.leafbrushes;
	delete[] cm.leafsurfaces;
	delete[] cm.brushsides;
	delete[] cm.entityString;
	delete[] cm.visibility;
	for (int i = 0; i < cm.numSurfaces; i++)
	{
		if (!cm.surfaces[i])
		{
			continue;
		}
		delete[] cm.surfaces[i]->pc->facets;
		delete[] cm.surfaces[i]->pc->planes;
		delete cm.surfaces[i]->pc;
		delete cm.surfaces[i];
	}
	delete[] cm.surfaces;
	Com_Memset(&cm, 0, sizeof(cm));
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
	if ( handle < cm.numSubModels ) {
		return &cm.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE ) {
		return &box_model;
	}
	if ( handle < MAX_SUBMODELS ) {
		throw QDropException(va("CM_ClipHandleToModel: bad handle %i < %i < %i", 
			cm.numSubModels, handle, MAX_SUBMODELS));
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
	if ( index < 0 || index >= cm.numSubModels ) {
		throw QDropException("CM_InlineModel: bad number");
	}
	return index;
}

int		CM_NumClusters( void ) {
	return cm.numClusters;
}

int		CM_NumInlineModels( void ) {
	return cm.numSubModels;
}

char	*CM_EntityString( void ) {
	return cm.entityString;
}

int		CM_LeafCluster( int leafnum ) {
	if (leafnum < 0 || leafnum >= cm.numLeafs) {
		throw QDropException("CM_LeafCluster: bad number");
	}
	return cm.leafs[leafnum].cluster;
}

int		CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cm.numLeafs ) {
		throw QDropException("CM_LeafArea: bad number");
	}
	return cm.leafs[leafnum].area;
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

	box_planes = &cm.planes[cm.numPlanes];

	box_brush = &cm.brushes[cm.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = BSP46CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cm.numBrushes;
	box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
	cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cm.brushsides[cm.numBrushSides+i];
		s->plane = 	cm.planes + (cm.numPlanes+i*2+side);
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


