//**************************************************************************
//**
//**	$Id$
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

#include "core.h"
#include "bsp29file.h"
#include "cm29_local.h"

// MACROS ------------------------------------------------------------------

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
//	QClipMap29::~QClipMap29
//
//==========================================================================

QClipMap29::~QClipMap29()
{
	for (int i = 0; i < numknown; i++)
	{
		delete known[i];
	}
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
	delete[] phs;
	phs = NULL;
}

//**************************************************************************
//
//	BRUSHMODEL LOADING
//
//**************************************************************************

//==========================================================================
//
//	QClipMap29::LoadModel
//
//==========================================================================

void QClipMap29::LoadModel(const char* name)
{
	Com_Memset(Map.map_models, 0, sizeof(Map.map_models));
	cmodel_t* mod = &Map.map_models[0];
	QStr::Cpy(mod->name, name);

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(name, Buffer) <= 0)
	{
		throw QDropException(va("Couldn't load %s", name));
	}

	mod->type = cmod_brush;

	bsp29_dheader_t header = *(bsp29_dheader_t*)Buffer.Ptr();

	int version = LittleLong(header.version);
	if (version != BSP29_VERSION)
	{
		throw QDropException(va("CM_LoadModel: %s has wrong version number (%i should be %i)",
			mod->name, version, BSP29_VERSION));
	}

	// swap all the lumps
	for (int i = 0; i < sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);
	}

	mod->checksum = 0;
	mod->checksum2 = 0;

	const quint8* mod_base = (quint8*)Buffer.Ptr();

	// checksum all of the map, except for entities
	for (int i = 0; i < BSP29_HEADER_LUMPS; i++)
	{
		if (i == BSP29LUMP_ENTITIES)
		{
			continue;
		}
		mod->checksum ^= LittleLong(Com_BlockChecksum(mod_base + header.lumps[i].fileofs, 
			header.lumps[i].filelen));

		if (i == BSP29LUMP_VISIBILITY || i == BSP29LUMP_LEAFS || i == BSP29LUMP_NODES)
		{
			continue;
		}
		mod->checksum2 ^= LittleLong(Com_BlockChecksum(mod_base + header.lumps[i].fileofs, 
			header.lumps[i].filelen));
	}

	// load into heap
	Map.LoadPlanes(mod, mod_base, &header.lumps[BSP29LUMP_PLANES]);
	Map.LoadVisibility(mod, mod_base, &header.lumps[BSP29LUMP_VISIBILITY]);
	Map.LoadLeafs(mod, mod_base, &header.lumps[BSP29LUMP_LEAFS]);
	Map.LoadNodes(mod, mod_base, &header.lumps[BSP29LUMP_NODES]);
	Map.LoadClipnodes(mod, mod_base, &header.lumps[BSP29LUMP_CLIPNODES]);
	Map.LoadEntities(mod, mod_base, &header.lumps[BSP29LUMP_ENTITIES]);

	Map.MakeHull0(mod);
	Map.MakeHulls(mod);

	if (GGameType & GAME_Hexen2)
	{
		Map.LoadSubmodelsH2(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}
	else
	{
		Map.LoadSubmodelsQ1(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}

	InitBoxHull();
}

//==========================================================================
//
//	QClipModel29::LoadVisibility
//
//==========================================================================

void QClipModel29::LoadVisibility(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadcmodel->visdata = NULL;
		return;
	}
	loadcmodel->visdata = new byte[l->filelen];
	Com_Memcpy(loadcmodel->visdata, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipModel29::LoadEntities
//
//==========================================================================

void QClipModel29::LoadEntities(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadcmodel->entities = NULL;
		return;
	}
	loadcmodel->entities = new char[l->filelen];
	Com_Memcpy(loadcmodel->entities, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipModel29::LoadPlanes
//
//==========================================================================

void QClipModel29::LoadPlanes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dplane_t* in = (const bsp29_dplane_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	cplane_t* out = new cplane_t[count];

	loadcmodel->planes = out;
	loadcmodel->numplanes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}
		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	QClipModel29::LoadNodes
//
//==========================================================================

void QClipModel29::LoadNodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dnode_t* in = (const bsp29_dnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	cnode_t* out = new cnode_t[count];

	loadcmodel->nodes = out;
	loadcmodel->numnodes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->contents = 0;
		int p = LittleLong(in->planenum);
		out->plane = loadcmodel->planes + p;

		for (int j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
			{
				out->children[j] = loadcmodel->nodes + p;
			}
			else
			{
				out->children[j] = (cnode_t *)(loadcmodel->leafs + (-1 - p));
			}
		}
	}
}

//==========================================================================
//
//	QClipModel29::LoadLeafs
//
//==========================================================================

void QClipModel29::LoadLeafs(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dleaf_t* in = (const bsp29_dleaf_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	cleaf_t* out = new cleaf_t[count];

	loadcmodel->leafs = out;
	loadcmodel->numleafs = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		int p = LittleLong(in->contents);
		out->contents = p;

		p = LittleLong(in->visofs);
		if (p == -1)
		{
			out->compressed_vis = NULL;
		}
		else
		{
			out->compressed_vis = loadcmodel->visdata + p;
		}

		for (int j = 0; j < 4; j++)
		{
			out->ambient_sound_level[j] = in->ambient_level[j];
		}
	}
}

//==========================================================================
//
//	QClipModel29::LoadClipnodes
//
//==========================================================================

void QClipModel29::LoadClipnodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dclipnode_t* in = (const bsp29_dclipnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
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
//	QClipModel29::MakeHull0
//
//	Deplicate the drawing hull structure as a clipping hull
//
//==========================================================================

void QClipModel29::MakeHull0(cmodel_t* loadcmodel)
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
//	QClipModel29::MakeHulls
//
//==========================================================================

void QClipModel29::MakeHulls(cmodel_t* loadcmodel)
{
	for (int j = 1; j < MAX_MAP_HULLS; j++)
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

	if (GGameType & GAME_Hexen2)
	{
		hull = &loadcmodel->hulls[2];
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
		if ((GGameType & GAME_HexenWorld) || !(GGameType & GAME_H2Portals))
		{
			hull->clip_mins[0] = -40;
			hull->clip_mins[1] = -40;
			hull->clip_mins[2] = -42;
			hull->clip_maxs[0] = 40;
			hull->clip_maxs[1] = 40;
			hull->clip_maxs[2] = 42;
		}
		else
		{
			hull->clip_mins[0] = -8;
			hull->clip_mins[1] = -8;
			hull->clip_mins[2] = -8;
			hull->clip_maxs[0] = 8;
			hull->clip_maxs[1] = 8;
			hull->clip_maxs[2] = 8;
		}

		hull = &loadcmodel->hulls[5];
		hull->clip_mins[0] = -48;
		hull->clip_mins[1] = -48;
		hull->clip_mins[2] = -50;
		hull->clip_maxs[0] = 48;
		hull->clip_maxs[1] = 48;
		hull->clip_maxs[2] = 50;
	}
	else
	{
		hull = &loadcmodel->hulls[2];
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 64;
	}
}

//==========================================================================
//
//	QClipModelMap29::LoadSubmodelsQ1
//
//==========================================================================

void QClipModelMap29::LoadSubmodelsQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_q1_t* in = (const bsp29_dmodel_q1_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	int count = l->filelen / sizeof(*in);

	loadcmodel->numsubmodels = count;

	for (int i = 0; i < count; i++, in++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			loadcmodel->mins[j] = LittleFloat(in->mins[j]) - 1;
			loadcmodel->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_Q1; j++)
		{
			loadcmodel->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
		}
		loadcmodel->numleafs = LittleLong(in->visleafs);

		if (i < loadcmodel->numsubmodels - 1)
		{
			// duplicate the basic information
			char	name[10];

			sprintf(name, "*%i", i + 1);
			cmodel_t* nextmodel = &map_models[i + 1];
			*nextmodel = *loadcmodel;
			QStr::Cpy(nextmodel->name, name);
			loadcmodel = nextmodel;
		}
	}
}

//==========================================================================
//
//	QClipModelMap29::LoadSubmodelsH2
//
//==========================================================================

void QClipModelMap29::LoadSubmodelsH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_h2_t* in = (const bsp29_dmodel_h2_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
	}
	int count = l->filelen / sizeof(*in);

	loadcmodel->numsubmodels = count;

	for (int i = 0; i < count; i++, in++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			loadcmodel->mins[j] = LittleFloat(in->mins[j]) - 1;
			loadcmodel->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_H2; j++)
		{
			loadcmodel->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
		}
		loadcmodel->numleafs = LittleLong(in->visleafs);

		if (i < loadcmodel->numsubmodels - 1)
		{
			// duplicate the basic information
			char	name[10];

			sprintf(name, "*%i", i + 1);
			cmodel_t* nextmodel = &map_models[i + 1];
			*nextmodel = *loadcmodel;
			QStr::Cpy(nextmodel->name, name);
			loadcmodel = nextmodel;
		}
	}
}

//**************************************************************************
//
//	LOADING OF NON-MAP MODELS
//
//**************************************************************************

//==========================================================================
//
//	QClipModelNonMap29::LoadBrushModelNonMap
//
//==========================================================================

void QClipModelNonMap29::LoadBrushModelNonMap(cmodel_t* mod, void* buffer)
{
	mod->type = cmod_brush;

	bsp29_dheader_t header = *(bsp29_dheader_t*)buffer;

	int version = LittleLong(header.version);
	if (version != BSP29_VERSION)
	{
		throw QDropException(va("CM_LoadBrushModel: %s has wrong version number (%i should be %i)",
			mod->name, version, BSP29_VERSION));
	}

	// swap all the lumps
	const quint8* mod_base = (quint8*)buffer;

	for (int i = 0; i < sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);
	}

	// load into heap
	LoadPlanes(mod, mod_base, &header.lumps[BSP29LUMP_PLANES]);
	LoadVisibility(mod, mod_base, &header.lumps[BSP29LUMP_VISIBILITY]);
	LoadLeafs(mod, mod_base, &header.lumps[BSP29LUMP_LEAFS]);
	LoadNodes(mod, mod_base, &header.lumps[BSP29LUMP_NODES]);
	LoadClipnodes(mod, mod_base, &header.lumps[BSP29LUMP_CLIPNODES]);
	LoadEntities(mod, mod_base, &header.lumps[BSP29LUMP_ENTITIES]);

	MakeHull0(mod);
	MakeHulls(mod);

	if (GGameType & GAME_Hexen2)
	{
		LoadSubmodelsNonMapH2(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}
	else
	{
		LoadSubmodelsNonMapQ1(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}
}

//==========================================================================
//
//	QClipModelNonMap29::LoadSubmodelsNonMapQ1
//
//==========================================================================

void QClipModelNonMap29::LoadSubmodelsNonMapQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_q1_t* in = (const bsp29_dmodel_q1_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
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
	for (int j = 0; j < BSP29_MAX_MAP_HULLS_Q1; j++)
	{
		loadcmodel->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
	}
	loadcmodel->numleafs = LittleLong(in->visleafs);
}

//==========================================================================
//
//	QClipModelNonMap29::LoadSubmodelsNonMapH2
//
//==========================================================================

void QClipModelNonMap29::LoadSubmodelsNonMapH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_h2_t* in = (const bsp29_dmodel_h2_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadcmodel->name));
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
	for (int j = 0; j < BSP29_MAX_MAP_HULLS_H2; j++)
	{
		loadcmodel->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
	}
	loadcmodel->numleafs = LittleLong(in->visleafs);
}

//**************************************************************************
//
//	BOX HULL
//
//**************************************************************************

//==========================================================================
//
//	QClipMap29::InitBoxHull
//
//	Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper chull_t structure.
//
//==========================================================================

void QClipMap29::InitBoxHull()
{
	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for (int i = 0; i < 6; i++)
	{
		box_clipnodes[i].planenum = i;

		int side = i & 1;

		box_clipnodes[i].children[side] = BSP29CONTENTS_EMPTY;
		if (i != 5)
		{
			box_clipnodes[i].children[side ^ 1] = i + 1;
		}
		else
		{
			box_clipnodes[i].children[side ^ 1] = BSP29CONTENTS_SOLID;
		}

		box_planes[i].type = i >> 1;
		box_planes[i].normal[i >> 1] = 1;
	}
}
