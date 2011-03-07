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

//**************************************************************************
//
//	BRUSHMODEL LOADING
//
//**************************************************************************

//==========================================================================
//
//	QClipMap29::LoadMap
//
//==========================================================================

void QClipMap29::LoadMap(const char* AName, const QArray<quint8>& Buffer)
{
	Com_Memset(map_models, 0, sizeof(map_models));
	cmodel_t* mod = &map_models[0];

	bsp29_dheader_t header = *(bsp29_dheader_t*)Buffer.Ptr();

	int version = LittleLong(header.version);
	if (version != BSP29_VERSION)
	{
		throw QDropException(va("CM_LoadModel: %s has wrong version number (%i should be %i)",
			AName, version, BSP29_VERSION));
	}

	// swap all the lumps
	for (int i = 0; i < sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);
	}

	CheckSum = 0;
	CheckSum2 = 0;

	const quint8* mod_base = Buffer.Ptr();

	// checksum all of the map, except for entities
	for (int i = 0; i < BSP29_HEADER_LUMPS; i++)
	{
		if (i == BSP29LUMP_ENTITIES)
		{
			continue;
		}
		CheckSum ^= LittleLong(Com_BlockChecksum(mod_base + header.lumps[i].fileofs, 
			header.lumps[i].filelen));

		if (i == BSP29LUMP_VISIBILITY || i == BSP29LUMP_LEAFS || i == BSP29LUMP_NODES)
		{
			continue;
		}
		CheckSum2 ^= LittleLong(Com_BlockChecksum(mod_base + header.lumps[i].fileofs, 
			header.lumps[i].filelen));
	}

	// load into heap
	LoadPlanes(mod_base, &header.lumps[BSP29LUMP_PLANES]);
	LoadVisibility(mod_base, &header.lumps[BSP29LUMP_VISIBILITY]);
	LoadLeafs(mod_base, &header.lumps[BSP29LUMP_LEAFS]);
	LoadNodes(mod, mod_base, &header.lumps[BSP29LUMP_NODES]);
	LoadClipnodes(mod, mod_base, &header.lumps[BSP29LUMP_CLIPNODES]);
	LoadEntities(mod_base, &header.lumps[BSP29LUMP_ENTITIES]);

	MakeHull0(mod);
	MakeHulls(mod);

	if (GGameType & GAME_Hexen2)
	{
		LoadSubmodelsH2(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}
	else
	{
		LoadSubmodelsQ1(mod, mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}

	InitBoxHull();

	CalcPHS();

	Name = AName;
}

//==========================================================================
//
//	QClipMap29::ReloadMap
//
//==========================================================================

void QClipMap29::ReloadMap(bool ClientLoad)
{
}

//==========================================================================
//
//	QClipMap29::LoadVisibility
//
//==========================================================================

void QClipMap29::LoadVisibility(const quint8* base, const bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		visdata = NULL;
		return;
	}
	visdata = new byte[l->filelen];
	Com_Memcpy(visdata, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipMap29::LoadEntities
//
//==========================================================================

void QClipMap29::LoadEntities(const quint8* base, const bsp29_lump_t* l)
{
	entitychars = l->filelen;
	if (!l->filelen)
	{
		entitystring = NULL;
		return;
	}
	entitystring = new char[l->filelen];
	Com_Memcpy(entitystring, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipMap29::LoadPlanes
//
//==========================================================================

void QClipMap29::LoadPlanes(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dplane_t* in = (const bsp29_dplane_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	cplane_t* out = new cplane_t[count];

	planes = out;
	numplanes = count;

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
//	QClipMap29::LoadNodes
//
//==========================================================================

void QClipMap29::LoadNodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dnode_t* in = (const bsp29_dnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	cnode_t* out = new cnode_t[count];

	loadcmodel->nodes = out;
	loadcmodel->numnodes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		int p = LittleLong(in->planenum);
		out->plane = planes + p;

		for (int j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			out->children[j] = p;
		}
	}
}

//==========================================================================
//
//	QClipMap29::LoadLeafs
//
//==========================================================================

void QClipMap29::LoadLeafs(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dleaf_t* in = (const bsp29_dleaf_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	cleaf_t* out = new cleaf_t[count];

	leafs = out;
	numleafs = count;

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
			out->compressed_vis = visdata + p;
		}

		for (int j = 0; j < 4; j++)
		{
			out->ambient_sound_level[j] = in->ambient_level[j];
		}
	}
}

//==========================================================================
//
//	QClipMap29::LoadClipnodes
//
//==========================================================================

void QClipMap29::LoadClipnodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dclipnode_t* in = (const bsp29_dclipnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
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
//	QClipMap29::MakeHull0
//
//	Deplicate the drawing hull structure as a clipping hull
//
//==========================================================================

void QClipMap29::MakeHull0(cmodel_t* loadcmodel)
{
	chull_t* hull = &loadcmodel->hulls[0];

	cnode_t* in = loadcmodel->nodes;
	int count = loadcmodel->numnodes;
	bsp29_dclipnode_t* out = new bsp29_dclipnode_t[count];

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = planes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - planes;
		for (int j = 0; j < 2; j++)
		{
			if (in->children[j] < 0)
			{
				cleaf_t* child = leafs + (-1 - in->children[j]);
				out->children[j] = child->contents;
			}
			else
			{
				out->children[j] = in->children[j];
			}
		}
	}
}

//==========================================================================
//
//	QClipMap29::MakeHulls
//
//==========================================================================

void QClipMap29::MakeHulls(cmodel_t* loadcmodel)
{
	for (int j = 1; j < MAX_MAP_HULLS; j++)
	{
		loadcmodel->hulls[j].clipnodes = loadcmodel->clipnodes;
		loadcmodel->hulls[j].firstclipnode = 0;
		loadcmodel->hulls[j].lastclipnode = loadcmodel->numclipnodes - 1;
		loadcmodel->hulls[j].planes = planes;
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
//	QClipMap29::LoadSubmodelsQ1
//
//==========================================================================

void QClipMap29::LoadSubmodelsQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_q1_t* in = (const bsp29_dmodel_q1_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
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
		if (i == 0)
		{
			numclusters = LittleLong(in->visleafs);
		}

		if (i < loadcmodel->numsubmodels - 1)
		{
			// duplicate the basic information
			cmodel_t* nextmodel = &map_models[i + 1];
			*nextmodel = *loadcmodel;
			loadcmodel = nextmodel;
		}
	}
}

//==========================================================================
//
//	QClipMap29::LoadSubmodelsH2
//
//==========================================================================

void QClipMap29::LoadSubmodelsH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_h2_t* in = (const bsp29_dmodel_h2_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
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
		if (i == 0)
		{
			numclusters = LittleLong(in->visleafs);
		}

		if (i < loadcmodel->numsubmodels - 1)
		{
			// duplicate the basic information
			cmodel_t* nextmodel = &map_models[i + 1];
			*nextmodel = *loadcmodel;
			loadcmodel = nextmodel;
		}
	}
}
