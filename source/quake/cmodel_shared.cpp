//**************************************************************************
//**
//**	$Id: endian.cpp 101 2010-04-03 23:06:31Z dj_jl $
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../../libs/core/cm29_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void CM_LoadAliasModelNew(cmodel_t* mod, void* buffer);
static void CM_LoadSpriteModel(cmodel_t* mod, void* buffer);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte			mod_novis[BSP29_MAX_MAP_LEAFS/8];

static QClipMap29*	CMap;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_Init
//
//==========================================================================

void CM_Init()
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));
}

//==========================================================================
//
//	CM_PointLeafnum
//
//==========================================================================

int CM_PointLeafnum(const vec3_t p)
{
	if (!CMap)
	{
		return 0;		// sound may call this without map loaded
	}

	cnode_t* node = CMap->Map.map_models[0].nodes;
	while (1)
	{
		if (node->contents < 0)
		{
			return (cleaf_t*)node - CMap->Map.map_models[0].leafs;
		}
		cplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return 0;	// never reached
}

//==========================================================================
//
//	CM_DecompressVis
//
//==========================================================================

static byte* CM_DecompressVis(byte* in, cmodel_t* model)
{
	static byte		decompressed[BSP29_MAX_MAP_LEAFS/8];

	if (!in)
	{
		// no vis info
		return mod_novis;
	}

	int row = (model->numleafs + 7) >> 3;
	byte* out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		int c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

//==========================================================================
//
//	CM_ClusterPVS
//
//==========================================================================

byte* CM_ClusterPVS(int Cluster)
{
	if (Cluster < 0)
	{
		return mod_novis;
	}
	return CM_DecompressVis(CMap->Map.map_models[0].leafs[Cluster + 1].compressed_vis,
		&CMap->Map.map_models[0]);
}

//==========================================================================
//
//	CM_ClusterPHS
//
//==========================================================================

byte* CM_ClusterPHS(int Cluster)
{
	return CMap->Map.map_models[0].phs + (Cluster + 1) * 4 * ((CM_NumClusters() + 31) >> 5);
}

//==========================================================================
//
//	CM_LoadMap
//
//==========================================================================

cmodel_t* CM_LoadMap(const char* name, bool clientload, int* checksum)
{
	if (!name[0])
	{
		Sys_Error("CM_ForName: NULL name");
	}

	if (CMap)
	{
		if (!QStr::Cmp(CMap->Map.map_models[0].name, name))
		{
			//	Already got it.
			return &CMap->Map.map_models[0];
		}

		delete CMap;
	}

	CMap = new QClipMap29;
	CMap->LoadModel(name);

	return &CMap->Map.map_models[0];
}

//==========================================================================
//
//	CM_InlineModel
//
//==========================================================================

cmodel_t* CM_InlineModel(const char* name)
{
	if (name[0] != '*')
	{
		throw QDropException("Submodel name mus start with *");
	}
	int i = QStr::Atoi(name + 1);
	if (i < 1 || i > CMap->Map.map_models[0].numsubmodels)
	{
		throw QDropException("Bad submodel index");
	}
	return &CMap->Map.map_models[i];
}

//==========================================================================
//
//	CM_PrecacheModel
//
//==========================================================================

cmodel_t* CM_PrecacheModel(const char* name)
{
	if (!name[0])
	{
		Sys_Error("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (int i = 0; i < CMap->numknown; i++)
	{
		if (!QStr::Cmp(CMap->known[i]->model.name, name))
		{
			return &CMap->known[i]->model;
		}
	}

	if (CMap->numknown == MAX_CMOD_KNOWN)
	{
		Sys_Error("mod_numknown == MAX_CMOD_KNOWN");
	}
	CMap->known[CMap->numknown] = new QClipModelNonMap29;
	Com_Memset(&CMap->known[CMap->numknown]->model, 0, sizeof(CMap->known[CMap->numknown]->model));
	cmodel_t* mod = &CMap->known[CMap->numknown]->model;
	QStr::Cpy(mod->name, name);
	QClipModelNonMap29* LoadCMap = CMap->known[CMap->numknown];
	CMap->numknown++;

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(mod->name, Buffer) <= 0)
	{
		Sys_Error("CM_PrecacheModel: %s not found", mod->name);
	}

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)Buffer.Ptr()))
	{
	case RAPOLYHEADER:
		LoadCMap->LoadAliasModelNew(mod, Buffer.Ptr());
		break;

	case IDPOLYHEADER:
		LoadCMap->LoadAliasModel(mod, Buffer.Ptr());
		break;

	case IDSPRITE1HEADER:
		CM_LoadSpriteModel(mod, Buffer.Ptr());
		break;

	default:
		LoadCMap->LoadBrushModelNonMap(mod, Buffer.Ptr());
		break;
	}

	return mod;
}

//==========================================================================
//
//	CM_MapChecksums
//
//==========================================================================

void CM_MapChecksums(int& checksum, int& checksum2)
{
	checksum = CMap->Map.map_models[0].checksum;
	checksum2 = CMap->Map.map_models[0].checksum2;
}

//==========================================================================
//
//	CM_ModelBounds
//
//==========================================================================

void CM_ModelBounds(cmodel_t* model, vec3_t mins, vec3_t maxs)
{
	VectorCopy(model->mins, mins);
	VectorCopy(model->maxs, maxs);
}

//==========================================================================
//
//	CM_NumClusters
//
//==========================================================================

int CM_NumClusters()
{
	return CMap->Map.map_models[0].numleafs;
}

//==========================================================================
//
//	CM_NumInlineModels
//
//==========================================================================

int CM_NumInlineModels()
{
	return CMap->Map.map_models[0].numsubmodels;
}

//==========================================================================
//
//	CM_EntityString
//
//==========================================================================

const char* CM_EntityString()
{
	return CMap->Map.map_models[0].entities;
}

//==========================================================================
//
//	CM_LeafCluster
//
//==========================================================================

int CM_LeafCluster(int LeafNum)
{
	//	-1 is because pvs rows are 1 based, not 0 based like leafs
	return LeafNum - 1;
}

//==========================================================================
//
//	CM_LeafAmbientSoundLevel
//
//==========================================================================

byte* CM_LeafAmbientSoundLevel(int LeafNum)
{
	if (!CMap)
	{
		return NULL;		// sound may call this without map loaded
	}
	return CMap->Map.map_models[0].leafs[LeafNum].ambient_sound_level;
}

//==========================================================================
//
//	CM_LoadSpriteModel
//
//==========================================================================

static void CM_LoadSpriteModel(cmodel_t* mod, void* buffer)
{
	dsprite1_t* pin = (dsprite1_t*)buffer;

	int version = LittleLong (pin->version);
	if (version != SPRITE1_VERSION)
	{
		Sys_Error("%s has wrong version number (%i should be %i)", mod->name, version, SPRITE1_VERSION);
	}

	mod->type = cmod_sprite;

	mod->mins[0] = mod->mins[1] = -LittleLong(pin->width) / 2;
	mod->maxs[0] = mod->maxs[1] = LittleLong(pin->width) / 2;
	mod->mins[2] = -LittleLong(pin->height) / 2;
	mod->maxs[2] = LittleLong(pin->height) / 2;
}

//==========================================================================
//
//	CM_HullPointContents
//
//==========================================================================

int CM_HullPointContents(chull_t* hull, int num, vec3_t p)
{
	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
		{
			throw QException("SV_HullPointContents: bad node number");
		}
	
		bsp29_dclipnode_t* node = hull->clipnodes + num;
		cplane_t* plane = hull->planes + node->planenum;

		float d;
		if (plane->type < 3)
		{
			d = p[plane->type] - plane->dist;
		}
		else
		{
			d = DotProduct(plane->normal, p) - plane->dist;
		}
		if (d < 0)
		{
			num = node->children[1];
		}
		else
		{
			num = node->children[0];
		}
	}

	return num;
}

//==========================================================================
//
//	CM_HullPointContents
//
//==========================================================================

int CM_HullPointContents(chull_t* hull, vec3_t p)
{
	return CM_HullPointContents(hull, hull->firstclipnode, p);
}

//==========================================================================
//
//	CM_PointContents
//
//==========================================================================

int CM_PointContents(vec3_t p, int HullNum)
{
	chull_t* hull = &CMap->Map.map_models[0].hulls[HullNum];
	return CM_HullPointContents(hull, hull->firstclipnode, p);
}

//==========================================================================
//
//	CM_HullForBox
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
//
//==========================================================================

chull_t* CM_HullForBox(vec3_t mins, vec3_t maxs)
{
	CMap->box_planes[0].dist = maxs[0];
	CMap->box_planes[1].dist = mins[0];
	CMap->box_planes[2].dist = maxs[1];
	CMap->box_planes[3].dist = mins[1];
	CMap->box_planes[4].dist = maxs[2];
	CMap->box_planes[5].dist = mins[2];

	return &CMap->box_hull;
}

//==========================================================================
//
//	CM_RecursiveHullCheck
//
//==========================================================================

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

bool CM_RecursiveHullCheck(chull_t* hull, int num, float p1f, float p2f,
	vec3_t p1, vec3_t p2, trace_t* trace)
{
	// check for empty
	if (num < 0)
	{
		if (num != BSP29CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == BSP29CONTENTS_EMPTY)
			{
				trace->inopen = true;
			}
			else
			{
				trace->inwater = true;
			}
		}
		else
		{
			trace->startsolid = true;
		}
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
	{
		Sys_Error("SV_RecursiveHullCheck: bad node number");
	}

	//
	// find the point distances
	//
	bsp29_dclipnode_t* node = hull->clipnodes + num;
	cplane_t* plane = hull->planes + node->planenum;

	float t1, t2;
	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct(plane->normal, p1) - plane->dist;
		t2 = DotProduct(plane->normal, p2) - plane->dist;
	}

	if (t1 >= 0 && t2 >= 0)
	{
		return CM_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
	}
	if (t1 < 0 && t2 < 0)
	{
		return CM_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	float frac;
	if (t1 < 0)
	{
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		frac = (t1 - DIST_EPSILON)/ (t1 - t2);
	}
	if (frac < 0)
	{
		frac = 0;
	}
	if (frac > 1)
	{
		frac = 1;
	}

	float midf = p1f + (p2f - p1f)*frac;
	vec3_t mid;
	for (int i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	int side = (t1 < 0);

	// move up to the node
	if (!CM_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
	{
		return false;
	}

#ifdef PARANOID
	if (CM_HullPointContents(hull, node->children[side], mid) == BSP29CONTENTS_SOLID)
	{
		GLog.Write("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (CM_HullPointContents(hull, node->children[side ^ 1], mid) != BSP29CONTENTS_SOLID)
	{
		// go past the node
		return CM_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
	}

	if (trace->allsolid)
	{
		return false;		// never got out of the solid area
	}

	//==================
	// the other side of the node is solid, this is the impact point
	//==================
	if (!side)
	{
		VectorCopy(plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorSubtract(vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (CM_HullPointContents (hull, hull->firstclipnode, mid) == BSP29CONTENTS_SOLID)
	{
		// shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy(mid, trace->endpos);
			GLog.DWrite("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f) * frac;
		for (int i = 0; i < 3; i++)
		{
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
		}
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return false;
}

//==========================================================================
//
//	CM_HullCheck
//
//==========================================================================

bool CM_HullCheck(chull_t* hull, vec3_t p1, vec3_t p2, trace_t* trace)
{
	return CM_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, p1, p2, trace);
}

//==========================================================================
//
//	CM_TraceLine
//
//==========================================================================

int CM_TraceLine(vec3_t start, vec3_t end, trace_t* trace)
{
	return CM_RecursiveHullCheck(&CMap->Map.map_models[0].hulls[0], 0, 0, 1, start, end, trace);
}

//==========================================================================
//
//	CM_ModelHull
//
//==========================================================================

chull_t* CM_ModelHull(cmodel_t* model, int HullNum, vec3_t clip_mins, vec3_t clip_maxs)
{
	if (!model || model->type != mod_brush)
	{
		throw QException("Non bsp model");
	}
	VectorCopy(model->hulls[HullNum].clip_mins, clip_mins);
	VectorCopy(model->hulls[HullNum].clip_maxs, clip_maxs);
	return &model->hulls[HullNum];
}

//==========================================================================
//
//	CM_BoxLeafnums_r
//
//==========================================================================

static int leaf_count;
static int* leaf_list;
static int leaf_maxcount;

static void CM_BoxLeafnums_r(vec3_t mins, vec3_t maxs, cnode_t *node)
{
	if (node->contents == BSP29CONTENTS_SOLID)
	{
		return;
	}

	if (node->contents < 0)
	{
		if (leaf_count == leaf_maxcount)
		{
			return;
		}

		cleaf_t* leaf = (cleaf_t*)node;
		int leafnum = leaf - CMap->Map.map_models[0].leafs;

		leaf_list[leaf_count] = leafnum;
		leaf_count++;
		return;
	}

	// NODE_MIXED

	cplane_t* splitplane = node->plane;
	int sides = BOX_ON_PLANE_SIDE(mins, maxs, splitplane);

	// recurse down the contacted sides
	if (sides & 1)
	{
		CM_BoxLeafnums_r(mins, maxs, node->children[0]);
	}

	if (sides & 2)
	{
		CM_BoxLeafnums_r(mins, maxs, node->children[1]);
	}
}

//==========================================================================
//
//	CM_BoxLeafnums
//
//==========================================================================

int CM_BoxLeafnums(vec3_t mins, vec3_t maxs, int* list, int maxcount)
{
	leaf_count = 0;
	leaf_list = list;
	leaf_maxcount = maxcount;
	CM_BoxLeafnums_r(mins, maxs, CMap->Map.map_models[0].nodes);
	return leaf_count;
}

//==========================================================================
//
//	CM_CalcPHS
//
//	Calculates the PHS (Potentially Hearable Set)
//
//==========================================================================

void CM_CalcPHS()
{
	int		rowbytes, rowwords;
	int		i, j, k, l, index, num;
	int		bitbyte;
	unsigned	*dest, *src;
	byte	*scan;
	int		count, vcount;

	Con_Printf ("Building PHS...\n");

	num = CMap->Map.map_models[0].numleafs;
	rowwords = (num+31)>>5;
	rowbytes = rowwords*4;

	byte* pvs = (byte*)Hunk_Alloc (rowbytes*num);
	scan = pvs;
	vcount = 0;
	for (i=0 ; i<num ; i++, scan+=rowbytes)
	{
		Com_Memcpy(scan, CM_ClusterPVS(CM_LeafCluster(i)), rowbytes);
		if (i == 0)
			continue;
		for (j=0 ; j<num ; j++)
		{
			if ( scan[j>>3] & (1<<(j&7)) )
			{
				vcount++;
			}
		}
	}


	CMap->Map.map_models[0].phs = (byte*)Hunk_Alloc (rowbytes*num);
	count = 0;
	scan = pvs;
	dest = (unsigned *)CMap->Map.map_models[0].phs;
	for (i=0 ; i<num ; i++, dest += rowwords, scan += rowbytes)
	{
		Com_Memcpy(dest, scan, rowbytes);
		for (j=0 ; j<rowbytes ; j++)
		{
			bitbyte = scan[j];
			if (!bitbyte)
				continue;
			for (k=0 ; k<8 ; k++)
			{
				if (! (bitbyte & (1<<k)) )
					continue;
				// or this pvs row into the phs
				// +1 because pvs is 1 based
				index = ((j<<3)+k+1);
				if (index >= num)
					continue;
				src = (unsigned *)pvs + index*rowwords;
				for (l=0 ; l<rowwords ; l++)
					dest[l] |= src[l];
			}
		}

		if (i == 0)
			continue;
		for (j=0 ; j<num ; j++)
			if ( ((byte *)dest)[j>>3] & (1<<(j&7)) )
				count++;
	}

	Con_Printf ("Average leafs visible / hearable / total: %i / %i / %i\n"
		, vcount/num, count/num, num);
}
