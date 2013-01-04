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

// HEADER FILES ------------------------------------------------------------

#include "../../qcommon.h"
#include "../../common_defs.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define BOX_BRUSHES         1
#define BOX_SIDES           6
#define BOX_LEAFS           2
#define BOX_PLANES          12

#define VIS_HEADER          8

#define MAX_PATCH_VERTS     1024

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap46::LoadMap
//
//==========================================================================

void QClipMap46::LoadMap(const char* AName, const Array<quint8>& Buffer)
{
	cm_noAreas = Cvar_Get("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get("cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT);
	if (GGameType & GAME_ET)
	{
		cm_optimize = Cvar_Get("cm_optimize", "1", CVAR_CHEAT);
	}

	CheckSum = LittleLong(Com_BlockChecksum(Buffer.Ptr(), Buffer.Num()));
	CheckSum2 = CheckSum;

	bsp46_dheader_t header = *(bsp46_dheader_t*)Buffer.Ptr();
	for (int i = 0; i < (int)sizeof(bsp46_dheader_t) / 4; i++)
	{
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);
	}

	if (header.version != BSP46_VERSION && header.version != BSP47_VERSION)
	{
		common->Error("CM_LoadMap: %s has wrong version number (%i should be %i)",
				AName, header.version, BSP46_VERSION);
	}

	const byte* cmod_base = Buffer.Ptr();

	// load into heap
	LoadShaders(cmod_base, &header.lumps[BSP46LUMP_SHADERS]);
	LoadLeafs(cmod_base, &header.lumps[BSP46LUMP_LEAFS]);
	LoadLeafBrushes(cmod_base, &header.lumps[BSP46LUMP_LEAFBRUSHES]);
	LoadLeafSurfaces(cmod_base, &header.lumps[BSP46LUMP_LEAFSURFACES]);
	LoadPlanes(cmod_base, &header.lumps[BSP46LUMP_PLANES]);
	LoadBrushSides(cmod_base, &header.lumps[BSP46LUMP_BRUSHSIDES]);
	LoadBrushes(cmod_base, &header.lumps[BSP46LUMP_BRUSHES]);
	LoadNodes(cmod_base, &header.lumps[BSP46LUMP_NODES]);
	LoadVisibility(cmod_base, &header.lumps[BSP46LUMP_VISIBILITY]);
	LoadEntityString(cmod_base, &header.lumps[BSP46LUMP_ENTITIES]);
	LoadPatches(cmod_base, &header.lumps[BSP46LUMP_SURFACES], &header.lumps[BSP46LUMP_DRAWVERTS]);
	LoadSubmodels(cmod_base, &header.lumps[BSP46LUMP_MODELS]);

	InitBoxHull();

	FloodAreaConnections();

	Name = AName;
}

//==========================================================================
//
//	QClipMap46::ReloadMap
//
//==========================================================================

void QClipMap46::ReloadMap(bool ClientLoad)
{
	if (!ClientLoad)
	{
		Com_Memset(areaPortals, 0, sizeof(int) * numAreas * numAreas);
		FloodAreaConnections();
	}
}

//==========================================================================
//
//	QClipMap46::LoadShaders
//
//==========================================================================

void QClipMap46::LoadShaders(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dshader_t* in = (const bsp46_dshader_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("CMod_LoadShaders: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		common->Error("Map with no shaders");
	}

	shaders = new bsp46_dshader_t[count];
	numShaders = count;

	Com_Memcpy(shaders, in, count * sizeof(*shaders));

	bsp46_dshader_t* out = shaders;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->contentFlags = LittleLong(out->contentFlags);
		out->surfaceFlags = LittleLong(out->surfaceFlags);
	}
}

//==========================================================================
//
//	QClipMap46::LoadLeafs
//
//==========================================================================

void QClipMap46::LoadLeafs(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dleaf_t* in = (const bsp46_dleaf_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		common->Error("Map with no leafs");
	}

	leafs = new cLeaf_t[BOX_LEAFS + count];
	Com_Memset(leafs, 0, sizeof(cLeaf_t) * (BOX_LEAFS + count));
	numLeafs = count;

	cLeaf_t* out = leafs;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);
		out->firstLeafBrush = LittleLong(in->firstLeafBrush);
		out->numLeafBrushes = LittleLong(in->numLeafBrushes);
		out->firstLeafSurface = LittleLong(in->firstLeafSurface);
		out->numLeafSurfaces = LittleLong(in->numLeafSurfaces);

		if (out->cluster >= numClusters)
		{
			numClusters = out->cluster + 1;
		}
		if (out->area >= numAreas)
		{
			numAreas = out->area + 1;
		}
	}

	areas = new cArea_t[numAreas];
	Com_Memset(areas, 0, sizeof(cArea_t) * numAreas);
	areaPortals = new int[numAreas * numAreas];
	Com_Memset(areaPortals, 0, sizeof(int) * numAreas * numAreas);
}

//==========================================================================
//
//	QClipMap46::LoadLeafBrushes
//
//==========================================================================

void QClipMap46::LoadLeafBrushes(const quint8* base, const bsp46_lump_t* l)
{
	const int* in = (const int*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	leafbrushes = new int[count + BOX_BRUSHES];
	Com_Memset(leafbrushes, 0, sizeof(int) * (count + BOX_BRUSHES));
	numLeafBrushes = count;

	int* out = leafbrushes;
	for (int i = 0; i < count; i++, in++, out++)
	{
		*out = LittleLong(*in);
	}
}

//==========================================================================
//
//	QClipMap46::LoadLeafSurfaces
//
//==========================================================================

void QClipMap46::LoadLeafSurfaces(const quint8* base, const bsp46_lump_t* l)
{
	const int* in = (const int*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	leafsurfaces = new int[count];
	numLeafSurfaces = count;

	int* out = leafsurfaces;
	for (int i = 0; i < count; i++, in++, out++)
	{
		*out = LittleLong(*in);
	}
}

//==========================================================================
//
//	QClipMap46::LoadPlanes
//
//==========================================================================

void QClipMap46::LoadPlanes(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dplane_t* in = (const bsp46_dplane_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		common->Error("Map with no planes");
	}

	planes = new cplane_t[BOX_PLANES + count];
	Com_Memset(planes, 0, sizeof(cplane_t) * (BOX_PLANES + count));
	numPlanes = count;

	cplane_t* out = planes;
	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}
		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	QClipMap46::LoadBrushSides
//
//==========================================================================

void QClipMap46::LoadBrushSides(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dbrushside_t* in = (const bsp46_dbrushside_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	brushsides = new cbrushside_t[BOX_SIDES + count];
	Com_Memset(brushsides, 0, sizeof(cbrushside_t) * (BOX_SIDES + count));
	numBrushSides = count;

	cbrushside_t* out = brushsides;
	for (int i = 0; i < count; i++, in++, out++)
	{
		int num = LittleLong(in->planeNum);
		out->plane = &planes[num];
		out->shaderNum = LittleLong(in->shaderNum);
		if (out->shaderNum < 0 || out->shaderNum >= numShaders)
		{
			common->Error("CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum);
		}
		out->surfaceFlags = shaders[out->shaderNum].surfaceFlags;
	}
}

//==========================================================================
//
//	QClipMap46::LoadBrushes
//
//==========================================================================

void QClipMap46::LoadBrushes(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dbrush_t* in = (const bsp46_dbrush_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	brushes = new cbrush_t[BOX_BRUSHES + count];
	Com_Memset(brushes, 0, sizeof(cbrush_t) * (BOX_BRUSHES + count));
	numBrushes = count;

	cbrush_t* out = brushes;
	for (int i = 0; i < count; i++, out++, in++)
	{
		out->sides = brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong(in->shaderNum);
		if (out->shaderNum < 0 || out->shaderNum >= numShaders)
		{
			common->Error("CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum);
		}
		out->contents = shaders[out->shaderNum].contentFlags;

		out->bounds[0][0] = -out->sides[0].plane->dist;
		out->bounds[1][0] = out->sides[1].plane->dist;

		out->bounds[0][1] = -out->sides[2].plane->dist;
		out->bounds[1][1] = out->sides[3].plane->dist;

		out->bounds[0][2] = -out->sides[4].plane->dist;
		out->bounds[1][2] = out->sides[5].plane->dist;
	}
}

//==========================================================================
//
//	QClipMap46::LoadNodes
//
//==========================================================================

void QClipMap46::LoadNodes(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dnode_t* in = (const bsp46_dnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		common->Error("Map has no nodes");
	}

	nodes = new cNode_t[count];
	Com_Memset(nodes, 0, sizeof(cNode_t) * count);
	numNodes = count;

	cNode_t* out = nodes;
	for (int i = 0; i < count; i++, out++, in++)
	{
		out->plane = planes + LittleLong(in->planeNum);
		for (int j = 0; j < 2; j++)
		{
			int child = LittleLong(in->children[j]);
			out->children[j] = child;
		}
	}
}

//==========================================================================
//
//	QClipMap46::LoadVisibility
//
//==========================================================================

void QClipMap46::LoadVisibility(const quint8* base, const bsp46_lump_t* l)
{
	int len = l->filelen;
	if (!len)
	{
		clusterBytes = (numClusters + 31) & ~31;
		visibility = new byte[clusterBytes];
		Com_Memset(visibility, 255, clusterBytes);
		return;
	}

	const byte* buf = base + l->fileofs;

	vised = true;
	visibility = new byte[len - VIS_HEADER];
	numClusters = LittleLong(((const int*)buf)[0]);
	clusterBytes = LittleLong(((const int*)buf)[1]);
	Com_Memcpy(visibility, buf + VIS_HEADER, len - VIS_HEADER);
}

//==========================================================================
//
//	QClipMap46::LoadEntityString
//
//==========================================================================

void QClipMap46::LoadEntityString(const quint8* base, const bsp46_lump_t* l)
{
	entityString = new char[l->filelen];
	numEntityChars = l->filelen;
	Com_Memcpy(entityString, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipMap46::LoadPatches
//
//==========================================================================

void QClipMap46::LoadPatches(const quint8* base, const bsp46_lump_t* surfs, const bsp46_lump_t* verts)
{
	const bsp46_dsurface_t* in = (const bsp46_dsurface_t*)(base + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = surfs->filelen / sizeof(*in);
	numSurfaces = count;
	surfaces = new cPatch_t*[numSurfaces];
	Com_Memset(surfaces, 0, sizeof(cPatch_t*) * numSurfaces);

	const bsp46_drawVert_t* dv = (const bsp46_drawVert_t*)(base + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for (int i = 0; i < count; i++, in++)
	{
		if (LittleLong(in->surfaceType) != BSP46MST_PATCH)
		{
			continue;		// ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cPatch_t* patch = new cPatch_t;
		surfaces[i] = patch;
		Com_Memset(patch, 0, sizeof(*patch));

		// load the full drawverts onto the stack
		int width = LittleLong(in->patchWidth);
		int height = LittleLong(in->patchHeight);
		int c = width * height;
		if (c > MAX_PATCH_VERTS)
		{
			common->Error("ParseMesh: MAX_PATCH_VERTS");
		}

		const bsp46_drawVert_t* dv_p = dv + LittleLong(in->firstVert);
		vec3_t points[MAX_PATCH_VERTS];
		for (int j = 0; j < c; j++, dv_p++)
		{
			points[j][0] = LittleFloat(dv_p->xyz[0]);
			points[j][1] = LittleFloat(dv_p->xyz[1]);
			points[j][2] = LittleFloat(dv_p->xyz[2]);
		}

		int shaderNum = LittleLong(in->shaderNum);
		patch->contents = shaders[shaderNum].contentFlags;
		patch->surfaceFlags = shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = GeneratePatchCollide(width, height, points);
	}
}

//==========================================================================
//
//	QClipMap46::LoadSubmodels
//
//==========================================================================

void QClipMap46::LoadSubmodels(const quint8* base, const bsp46_lump_t* l)
{
	const bsp46_dmodel_t* in = (const bsp46_dmodel_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("CMod_LoadSubmodels: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		common->Error("Map with no models");
	}

	cmodels = new cmodel_t[count];
	Com_Memset(cmodels, 0, sizeof(cmodel_t) * count);
	numSubModels = count;

	if (count > MAX_SUBMODELS)
	{
		common->Error("MAX_SUBMODELS exceeded");
	}

	for (int i = 0; i < count; i++, in++)
	{
		cmodel_t* out = &cmodels[i];

		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}

		if (i == 0)
		{
			continue;	// world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong(in->numBrushes);
		int* indexes = new int[out->leaf.numLeafBrushes];
		out->leaf.firstLeafBrush = indexes - leafbrushes;
		for (int j = 0; j < out->leaf.numLeafBrushes; j++)
		{
			indexes[j] = LittleLong(in->firstBrush) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong(in->numSurfaces);
		indexes = new int[out->leaf.numLeafSurfaces];
		out->leaf.firstLeafSurface = indexes - leafsurfaces;
		for (int j = 0; j < out->leaf.numLeafSurfaces; j++)
		{
			indexes[j] = LittleLong(in->firstSurface) + j;
		}
	}
}
