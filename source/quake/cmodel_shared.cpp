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

#include "cm_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void CM_LoadBrushModel(cmodel_t* mod, void* buffer);
static void CM_LoadBrushModelNonMap(cmodel_t* mod, void* buffer);
static void CM_LoadAliasModel(cmodel_t* mod, void* buffer);
static void CM_LoadAliasModelNew(cmodel_t* mod, void* buffer);
static void CM_LoadSpriteModel(cmodel_t* mod, void* buffer);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static cmodel_t	*loadcmodel;

static byte		mod_novis[BSP29_MAX_MAP_LEAFS/8];

#define	MAX_MAP_MODELS		256
static cmodel_t	cm_map_models[MAX_MAP_MODELS];

#define MAX_MOD_KNOWN	2048
static cmodel_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

static byte*	mod_base;

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
	if (!cm_map_models[0].numplanes)
	{
		return 0;		// sound may call this without map loaded
	}

	cnode_t* node = cm_map_models[0].nodes;
	while (1)
	{
		if (node->contents < 0)
		{
			return (cleaf_t*)node - cm_map_models[0].leafs;
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
	return CM_DecompressVis(cm_map_models[0].leafs[Cluster + 1].compressed_vis,
		&cm_map_models[0]);
}

//==========================================================================
//
//	CM_ClearAll
//
//==========================================================================

void CM_ClearAll()
{
	cmodel_t* mod = mod_known;
	for (int i = 0; i < mod_numknown; i++, mod++)
	{
		if (mod->type != mod_alias)
		{
			mod->needload = true;
		}
		mod->Free();
	}
	cm_map_models[0].Free();
	Com_Memset(cm_map_models, 0, sizeof(cm_map_models));
}

//==========================================================================
//
//	CM_FindName
//
//==========================================================================

static cmodel_t* CM_FindName(const char* name)
{
	int		i;
	cmodel_t	*mod;

	if (!name[0])
	{
		Sys_Error("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!QStr::Cmp(mod->name, name))
		{
			break;
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
			Sys_Error("mod_numknown == MAX_MOD_KNOWN");
		}
		QStr::Cpy(mod->name, name);
		mod->needload = true;
		mod_numknown++;
	}

	return mod;
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

	//
	// search the currently loaded models
	//
	cmodel_t* mod = &cm_map_models[0];
	if (!QStr::Cmp(mod->name, name))
	{
		if (mod->needload)
		{
			throw QException("Curent map not loaded");
		}
		return mod;
	}

	QStr::Cpy(mod->name, name);
	mod->needload = true;

	if (!mod->needload)
	{
		return mod;
	}

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(mod->name, Buffer) <= 0)
	{
		return NULL;
	}

	//
	// allocate a new model
	//
	loadcmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader
	mod->needload = false;

	CM_LoadBrushModel(mod, Buffer.Ptr());
	return mod;
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
	if (i < 1 || i > cm_map_models[0].numsubmodels)
	{
		throw QDropException("Bad submodel index");
	}
	return &cm_map_models[i];
}

//==========================================================================
//
//	CM_PrecacheModel
//
//==========================================================================

cmodel_t* CM_PrecacheModel(const char* name)
{
	cmodel_t* mod = CM_FindName(name);

	if (!mod->needload)
	{
		return mod;
	}

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(mod->name, Buffer) <= 0)
	{
		Sys_Error("CM_PrecacheModel: %s not found", mod->name);
	}

	//
	// allocate a new model
	//
	loadcmodel = mod;

	// call the apropriate loader
	mod->needload = false;

	switch (LittleLong(*(unsigned*)Buffer.Ptr()))
	{
	case RAPOLYHEADER:
		CM_LoadAliasModelNew(mod, Buffer.Ptr());
		break;

	case IDPOLYHEADER:
		CM_LoadAliasModel(mod, Buffer.Ptr());
		break;

	case IDSPRITEHEADER:
		CM_LoadSpriteModel(mod, Buffer.Ptr());
		break;

	default:
		CM_LoadBrushModelNonMap(mod, Buffer.Ptr());
		break;
	}

	return mod;
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
	return cm_map_models[0].numleafs;
}

//==========================================================================
//
//	CM_NumInlineModels
//
//==========================================================================

int CM_NumInlineModels()
{
	return cm_map_models[0].numsubmodels;
}

//==========================================================================
//
//	CM_EntityString
//
//==========================================================================

const char* CM_EntityString()
{
	return cm_map_models[0].entities;
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
	if (!cm_map_models[0].numplanes)
	{
		return NULL;		// sound may call this without map loaded
	}
	return cm_map_models[0].leafs[LeafNum].ambient_sound_level;
}

//==========================================================================
//
//	cmodel_t::Free
//
//==========================================================================

void cmodel_t::Free()
{
	delete[] visdata;
	visdata = NULL;
	delete[] entities;
	entities = NULL;
	delete[] planes;
	planes = NULL;
	delete[] nodes;
	nodes = NULL;
	delete[] leafs;
	leafs = NULL;
	delete[] clipnodes;
	clipnodes = NULL;
	delete[] hulls[0].clipnodes;
	hulls[0].clipnodes = NULL;
	delete[] submodels;
	submodels = NULL;
}

//**************************************************************************
//
//	BRUSHMODEL LOADING
//
//**************************************************************************

//==========================================================================
//
//	CM_LoadVisibility
//
//==========================================================================

static void CM_LoadVisibility(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadcmodel->visdata = NULL;
		return;
	}
	loadcmodel->visdata = new byte[l->filelen];
	Com_Memcpy(loadcmodel->visdata, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	CM_LoadEntities
//
//==========================================================================

static void CM_LoadEntities(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadcmodel->entities = NULL;
		return;
	}
	loadcmodel->entities = new char[l->filelen];
	Com_Memcpy(loadcmodel->entities, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	CM_LoadPlanes
//
//==========================================================================

static void CM_LoadPlanes(bsp29_lump_t* l)
{
	int			i, j;
	cplane_t	*out;
	bsp29_dplane_t 	*in;
	int			count;

	in = (bsp29_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadcmodel->name);
	count = l->filelen / sizeof(*in);
	out = new cplane_t[count];

	loadcmodel->planes = out;
	loadcmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
		}
		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	CM_LoadNodes
//
//==========================================================================

static void CM_LoadNodes(bsp29_lump_t* l)
{
	int			i, j, count, p;
	bsp29_dnode_t		*in;
	cnode_t 	*out;

	in = (bsp29_dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadcmodel->name);
	count = l->filelen / sizeof(*in);
	out = new cnode_t[count];

	loadcmodel->nodes = out;
	loadcmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->contents = 0;
		p = LittleLong(in->planenum);
		out->plane = loadcmodel->planes + p;

		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = loadcmodel->nodes + p;
			else
				out->children[j] = (cnode_t *)(loadcmodel->leafs + (-1 - p));
		}
	}
}

//==========================================================================
//
//	CM_LoadLeafs
//
//==========================================================================

static void CM_LoadLeafs(bsp29_lump_t* l)
{
	bsp29_dleaf_t 	*in;
	cleaf_t 	*out;
	int			i, j, count, p;

	in = (bsp29_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadcmodel->name);
	count = l->filelen / sizeof(*in);
	out = new cleaf_t[count];

	loadcmodel->leafs = out;
	loadcmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		p = LittleLong(in->contents);
		out->contents = p;

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadcmodel->visdata + p;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

//==========================================================================
//
//	CM_LoadClipnodes
//
//==========================================================================

static void CM_LoadClipnodes(bsp29_lump_t* l)
{
	bsp29_dclipnode_t* in = (bsp29_dclipnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name);
	}
	int count = l->filelen / sizeof(*in);
	bsp29_dclipnode_t* out = new bsp29_dclipnode_t[count];

	loadcmodel->clipnodes = out;
	loadcmodel->numclipnodes = count;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

//==========================================================================
//
//	CM_MakeHull0
//
//	Deplicate the drawing hull structure as a clipping hull
//
//==========================================================================

static void CM_MakeHull0()
{
	chull_t* hull = &loadcmodel->hulls[0];

	cnode_t* in = loadcmodel->nodes;
	int count = loadcmodel->numnodes;
	bsp29_dclipnode_t* out = new bsp29_dclipnode_t[count];

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadcmodel->planes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadcmodel->planes;
		for (int j = 0; j < 2; j++)
		{
			cnode_t* child = in->children[j];
			if (child->contents < 0)
			{
				out->children[j] = child->contents;
			}
			else
			{
				out->children[j] = child - loadcmodel->nodes;
			}
		}
	}
}

//==========================================================================
//
//	CM_MakeHulls
//
//==========================================================================

static void CM_MakeHulls()
{
	for (int j = 1; j < MAX_MAP_HULLS_; j++)
	{
		loadcmodel->hulls[j].clipnodes = loadcmodel->clipnodes;
		loadcmodel->hulls[j].firstclipnode = 0;
		loadcmodel->hulls[j].lastclipnode = loadcmodel->numclipnodes - 1;
		loadcmodel->hulls[j].planes = loadcmodel->planes;
	}

	chull_t* hull = &loadcmodel->hulls[1];
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadcmodel->hulls[2];
#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
	hull->clip_mins[0] = -24;
	hull->clip_mins[1] = -24;
	hull->clip_mins[2] = -20;
	hull->clip_maxs[0] = 24;
	hull->clip_maxs[1] = 24;
	hull->clip_maxs[2] = 20;

	hull = &loadcmodel->hulls[3];
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -12;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 16;

	hull = &loadcmodel->hulls[4];
	//	In Hexen 2 this was #if 0. After looking in progs source I found that
	// it was changed for mission pack.
#if defined HEXENWORLD_HULLS || !defined MISSIONPACK
	hull->clip_mins[0] = -40;
	hull->clip_mins[1] = -40;
	hull->clip_mins[2] = -42;
	hull->clip_maxs[0] = 40;
	hull->clip_maxs[1] = 40;
	hull->clip_maxs[2] = 42;
#else
	hull->clip_mins[0] = -8;
	hull->clip_mins[1] = -8;
	hull->clip_mins[2] = -8;
	hull->clip_maxs[0] = 8;
	hull->clip_maxs[1] = 8;
	hull->clip_maxs[2] = 8;
#endif

	hull = &loadcmodel->hulls[5];
	hull->clip_mins[0] = -48;
	hull->clip_mins[1] = -48;
	hull->clip_mins[2] = -50;
	hull->clip_maxs[0] = 48;
	hull->clip_maxs[1] = 48;
	hull->clip_maxs[2] = 50;
#else
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;
#endif
}

//==========================================================================
//
//	CM_LoadSubmodels
//
//==========================================================================

static void CM_LoadSubmodels(bsp29_lump_t* l)
{
	int			i, j, count;

#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
	bsp29_dmodel_h2_t* in = (bsp29_dmodel_h2_t*)(mod_base + l->fileofs);
#else
	bsp29_dmodel_q1_t* in = (bsp29_dmodel_q1_t*)(mod_base + l->fileofs);
#endif
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadcmodel->name);
	count = l->filelen / sizeof(*in);
	bsp29_dmodel_h2_t* out = new bsp29_dmodel_h2_t[count];

	loadcmodel->submodels = out;
	loadcmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
		for (j=0 ; j<BSP29_MAX_MAP_HULLS_H2 ; j++)
#else
		for (j=0 ; j<BSP29_MAX_MAP_HULLS_Q1 ; j++)
#endif
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

//==========================================================================
//
//	CM_LoadBrushModel
//
//==========================================================================

static void CM_LoadBrushModel(cmodel_t* mod, void* buffer)
{
	loadcmodel->type = mod_brush;

	bsp29_dheader_t* header = (bsp29_dheader_t*)buffer;

	int version = LittleLong(header->version);
	if (version != BSP29_VERSION)
	{
		Sys_Error("CM_LoadBrushModel: %s has wrong version number (%i should be %i)",
			mod->name, version, BSP29_VERSION);
	}

	// swap all the lumps
	mod_base = (byte *)header;

	for (int i = 0; i < sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	mod->checksum = 0;
	mod->checksum2 = 0;

	// checksum all of the map, except for entities
	for (int i = 0; i < BSP29_HEADER_LUMPS; i++)
	{
		if (i == BSP29LUMP_ENTITIES)
		{
			continue;
		}
		mod->checksum ^= LittleLong(Com_BlockChecksum(mod_base + header->lumps[i].fileofs, 
			header->lumps[i].filelen));

		if (i == BSP29LUMP_VISIBILITY || i == BSP29LUMP_LEAFS || i == BSP29LUMP_NODES)
		{
			continue;
		}
		mod->checksum2 ^= LittleLong(Com_BlockChecksum(mod_base + header->lumps[i].fileofs, 
			header->lumps[i].filelen));
	}

	// load into heap
	CM_LoadPlanes(&header->lumps[BSP29LUMP_PLANES]);
	CM_LoadVisibility(&header->lumps[BSP29LUMP_VISIBILITY]);
	CM_LoadLeafs(&header->lumps[BSP29LUMP_LEAFS]);
	CM_LoadNodes(&header->lumps[BSP29LUMP_NODES]);
	CM_LoadClipnodes(&header->lumps[BSP29LUMP_CLIPNODES]);
	CM_LoadEntities(&header->lumps[BSP29LUMP_ENTITIES]);

	CM_MakeHull0();
	CM_MakeHulls();

	CM_LoadSubmodels(&header->lumps[BSP29LUMP_MODELS]);

	//
	// set up the submodels (FIXME: this is confusing)
	//
	for (int i = 0; i < mod->numsubmodels; i++)
	{
		bsp29_dmodel_h2_t* bm = &mod->submodels[i];

#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_H2; j++)
#else
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_Q1; j++)
#endif
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
		}

		VectorCopy(bm->maxs, mod->maxs);
		VectorCopy(bm->mins, mod->mins);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{
			// duplicate the basic information
			char	name[10];

			sprintf(name, "*%i", i + 1);
			loadcmodel = &cm_map_models[i + 1];
			*loadcmodel = *mod;
			QStr::Cpy(loadcmodel->name, name);
			mod = loadcmodel;
		}
	}
}

//==========================================================================
//
//	CM_LoadSubmodelsNonMap
//
//==========================================================================

static void CM_LoadSubmodelsNonMap(bsp29_lump_t* l)
{
#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
	bsp29_dmodel_h2_t* in = (bsp29_dmodel_h2_t*)(mod_base + l->fileofs);
#else
	bsp29_dmodel_q1_t* in = (bsp29_dmodel_q1_t*)(mod_base + l->fileofs);
#endif
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	if (l->filelen != sizeof(*in))
	{
		GLog.WriteLine("Non-map BSP models are not supposed to have submodels");
	}

	for (int j = 0; j < 3; j++)
	{
		// spread the mins / maxs by a pixel
		loadcmodel->mins[j] = LittleFloat(in->mins[j]) - 1;
		loadcmodel->maxs[j] = LittleFloat(in->maxs[j]) + 1;
	}
#if defined HEXEN2_HULLS || defined HEXENWORLD_HULLS
	for (int j = 0; j < BSP29_MAX_MAP_HULLS_H2; j++)
#else
	for (int j = 0; j < BSP29_MAX_MAP_HULLS_Q1; j++)
#endif
	{
		loadcmodel->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
	}
	loadcmodel->numleafs = LittleLong(in->visleafs);
}

//==========================================================================
//
//	CM_LoadBrushModelNonMap
//
//==========================================================================

static void CM_LoadBrushModelNonMap(cmodel_t* mod, void* buffer)
{
	loadcmodel->type = mod_brush;

	bsp29_dheader_t* header = (bsp29_dheader_t*)buffer;

	int version = LittleLong(header->version);
	if (version != BSP29_VERSION)
	{
		Sys_Error("CM_LoadBrushModel: %s has wrong version number (%i should be %i)",
			mod->name, version, BSP29_VERSION);
	}

	// swap all the lumps
	mod_base = (byte *)header;

	for (int i = 0; i < sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// load into heap
	CM_LoadPlanes(&header->lumps[BSP29LUMP_PLANES]);
	CM_LoadVisibility(&header->lumps[BSP29LUMP_VISIBILITY]);
	CM_LoadLeafs(&header->lumps[BSP29LUMP_LEAFS]);
	CM_LoadNodes(&header->lumps[BSP29LUMP_NODES]);
	CM_LoadClipnodes(&header->lumps[BSP29LUMP_CLIPNODES]);
	CM_LoadEntities(&header->lumps[BSP29LUMP_ENTITIES]);

	CM_MakeHull0();
	CM_MakeHulls();

	CM_LoadSubmodelsNonMap(&header->lumps[BSP29LUMP_MODELS]);
}

//**************************************************************************
//
//	ALIAS MODELS
//
//**************************************************************************

#if defined HEXEN2_HULLS
static vec3_t	mins, maxs;

static float	aliastransform[3][4];

//==========================================================================
//
//	R_AliasTransformVector
//
//==========================================================================

static void R_AliasTransformVector(vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
	out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
	out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}

//==========================================================================
//
//	CM_LoadAliasFrame
//
//==========================================================================

static void* CM_LoadAliasFrame(void* pin)
{
	daliasframe_t* pdaliasframe = (daliasframe_t*)pin;
	trivertx_t* pinframe = (trivertx_t*)(pdaliasframe + 1);

	aliastransform[0][0] = pheader->scale[0];
	aliastransform[1][1] = pheader->scale[1];
	aliastransform[2][2] = pheader->scale[2];
	aliastransform[0][3] = pheader->scale_origin[0];
	aliastransform[1][3] = pheader->scale_origin[1];
	aliastransform[2][3] = pheader->scale_origin[2];

	for (int j = 0; j < pheader->numverts; j++)
	{
		vec3_t in,out;
		in[0] = pinframe[j].v[0];
		in[1] = pinframe[j].v[1];
		in[2] = pinframe[j].v[2];
		R_AliasTransformVector(in, out);
		for (int i = 0; i < 3; i++)
		{
			if (mins[i] > out[i])
				mins[i] = out[i];
			if (maxs[i] < out[i])
				maxs[i] = out[i];
		}
	}
	pinframe += pheader->numverts;
	return (void*)pinframe;
}

//==========================================================================
//
//	CM_LoadAliasGroup
//
//==========================================================================

static void* CM_LoadAliasGroup(void* pin)
{
	daliasgroup_t* pingroup = (daliasgroup_t*)pin;
	int numframes = LittleLong (pingroup->numframes);
	daliasinterval_t* pin_intervals = (daliasinterval_t *)(pingroup + 1);
	pin_intervals += numframes;
	void* ptemp = (void*)pin_intervals;

	aliastransform[0][0] = pheader->scale[0];
	aliastransform[1][1] = pheader->scale[1];
	aliastransform[2][2] = pheader->scale[2];
	aliastransform[0][3] = pheader->scale_origin[0];
	aliastransform[1][3] = pheader->scale_origin[1];
	aliastransform[2][3] = pheader->scale_origin[2];

	for (int i = 0; i < numframes; i++)
	{
		trivertx_t* poseverts = (trivertx_t*)((daliasframe_t*)ptemp + 1);
		for (int j = 0; j < pheader->numverts; j++)
		{
			vec3_t in, out;
			in[0] = poseverts[j].v[0];
			in[1] = poseverts[j].v[1];
			in[2] = poseverts[j].v[2];
			R_AliasTransformVector(in, out);
			for (int k = 0; k < 3; k++)
			{
				if (mins[k] > out[k])
					mins[k] = out[k];
				if (maxs[k] < out[k])
					maxs[k] = out[k];
			}
		}
		ptemp = (trivertx_t *)((daliasframe_t *)ptemp + 1) + pheader->numverts;
	}
	return ptemp;
}

//==========================================================================
//
//	CM_LoadAllSkins
//
//==========================================================================

static void* CM_LoadAllSkins(int numskins, daliasskintype_t* pskintype)
{
	for (int i = 0; i < numskins; i++)
	{
		int s = pheader->skinwidth * pheader->skinheight;
		pskintype = (daliasskintype_t*)((byte*)(pskintype + 1) + s);
	}
	return (void *)pskintype;
}

//==========================================================================
//
//	CM_LoadAliasModelNew
//
//	Reads extra field for num ST verts, and extra index list of them
//
//==========================================================================

static void CM_LoadAliasModelNew(cmodel_t* mod, void* buffer)
{
	aliashdr_t			tmp_hdr;
	pheader = &tmp_hdr;

	newmdl_t* pinmodel = (newmdl_t *)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != ALIAS_NEWVERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_NEWVERSION);

	pheader->numskins = LittleLong(pinmodel->numskins);
	pheader->skinwidth = LittleLong(pinmodel->skinwidth);
	pheader->skinheight = LittleLong(pinmodel->skinheight);
	pheader->numverts = LittleLong(pinmodel->numverts);
	pheader->version = LittleLong(pinmodel->num_st_verts);	//hide num_st in version
	pheader->numtris = LittleLong(pinmodel->numtris);
	pheader->numframes = LittleLong (pinmodel->numframes);

	for (int i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
	}

	daliasskintype_t* pskintype = (daliasskintype_t*)&pinmodel[1];
	pskintype = (daliasskintype_t*)CM_LoadAllSkins(pheader->numskins, pskintype);
	stvert_t* pinstverts = (stvert_t*)pskintype;
	dnewtriangle_t* pintriangles = (dnewtriangle_t*)&pinstverts[pheader->version];
	daliasframetype_t* pframetype = (daliasframetype_t*)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;
	for (int i = 0; i < pheader->numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong (pframetype->type);
		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)CM_LoadAliasFrame(pframetype + 1);
		}
		else
		{
			pframetype = (daliasframetype_t*)CM_LoadAliasGroup(pframetype + 1);
		}
	}

	mod->type = mod_alias;

	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;
}

//==========================================================================
//
//	CM_LoadAliasModel
//
//==========================================================================

static void CM_LoadAliasModel(cmodel_t* mod, void* buffer)
{
	aliashdr_t			tmp_hdr;
	pheader = &tmp_hdr;

	mdl_t* pinmodel = (mdl_t *)buffer;

	int version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)",
			 mod->name, version, ALIAS_VERSION);

	pheader->numskins = LittleLong(pinmodel->numskins);
	pheader->skinwidth = LittleLong(pinmodel->skinwidth);
	pheader->skinheight = LittleLong(pinmodel->skinheight);
	pheader->numverts = LittleLong(pinmodel->numverts);
	pheader->numtris = LittleLong(pinmodel->numtris);
	pheader->numframes = LittleLong (pinmodel->numframes);
	int numframes = pheader->numframes;

	for (int i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
	}

	daliasskintype_t* pskintype = (daliasskintype_t*)&pinmodel[1];
	pskintype = (daliasskintype_t*)CM_LoadAllSkins(pheader->numskins, pskintype);
	stvert_t* pinstverts = (stvert_t*)pskintype;
	dtriangle_t* pintriangles = (dtriangle_t*)&pinstverts[pheader->numverts];
	daliasframetype_t* pframetype = (daliasframetype_t*)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;
	for (int i = 0; i < numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong(pframetype->type);
		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)CM_LoadAliasFrame(pframetype + 1);
		}
		else
		{
			pframetype = (daliasframetype_t*)CM_LoadAliasGroup(pframetype + 1);
		}
	}

	mod->type = mod_alias;

	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;
}
#else
//==========================================================================
//
//	CM_LoadAliasModelNew
//
//==========================================================================

static void CM_LoadAliasModelNew(cmodel_t* mod, void* buffer)
{
	mod->type = mod_alias;
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
}

//==========================================================================
//
//	CM_LoadAliasModel
//
//==========================================================================

static void CM_LoadAliasModel(cmodel_t* mod, void* buffer)
{
	mod->type = mod_alias;
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
}
#endif

//**************************************************************************

//==========================================================================
//
//	CM_LoadSpriteModel
//
//==========================================================================

static void CM_LoadSpriteModel(cmodel_t* mod, void* buffer)
{
	dsprite_t* pin = (dsprite_t*)buffer;

	int version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
	{
		Sys_Error("%s has wrong version number (%i should be %i)", mod->name, version, SPRITE_VERSION);
	}

	mod->type = mod_sprite;

	mod->mins[0] = mod->mins[1] = -LittleLong(pin->width) / 2;
	mod->maxs[0] = mod->maxs[1] = LittleLong(pin->width) / 2;
	mod->mins[2] = -LittleLong(pin->height) / 2;
	mod->maxs[2] = LittleLong(pin->height) / 2;
}
