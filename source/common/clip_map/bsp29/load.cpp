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
#include "../../endian.h"
#include "../../md4.h"
#include "../../Common.h"
#include "local.h"

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

void QClipMap29::LoadMap(const char* AName, const Array<quint8>& Buffer)
{
	bsp29_dheader_t header = *(bsp29_dheader_t*)Buffer.Ptr();

	int version = LittleLong(header.version);
	if (version != BSP29_VERSION)
	{
		common->Error("CM_LoadModel: %s has wrong version number (%i should be %i)",
				AName, version, BSP29_VERSION);
	}

	// swap all the lumps
	for (int i = 0; i < (int)sizeof(bsp29_dheader_t) / 4; i++)
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
	LoadNodes(mod_base, &header.lumps[BSP29LUMP_NODES]);
	LoadClipnodes(mod_base, &header.lumps[BSP29LUMP_CLIPNODES]);
	LoadEntities(mod_base, &header.lumps[BSP29LUMP_ENTITIES]);

	MakeHull0();
	MakeHulls();

	if (GGameType & GAME_Hexen2)
	{
		LoadSubmodelsH2(mod_base, &header.lumps[BSP29LUMP_MODELS]);
	}
	else
	{
		LoadSubmodelsQ1(mod_base, &header.lumps[BSP29LUMP_MODELS]);
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
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	//	Extra planes for box.
	planes = new cplane_t[count + 6];
	Com_Memset(planes, 0, sizeof(cplane_t) * (count + 6));
	numplanes = count;

	cplane_t* out = planes;
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

void QClipMap29::LoadNodes(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dnode_t* in = (const bsp29_dnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	cnode_t* out = new cnode_t[count];

	nodes = out;
	numnodes = count;

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
		common->Error("MOD_LoadBmodel: funny lump size");
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

void QClipMap29::LoadClipnodes(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dclipnode_t* in = (const bsp29_dclipnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	//	Extra space for hull 0 and box.
	cclipnode_t* out = new cclipnode_t[count + numnodes + 6];

	clipnodes = out;
	numclipnodes = count;

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

void QClipMap29::MakeHull0()
{
	cnode_t* in = nodes;
	int count = numnodes;

	cclipnode_t* out = clipnodes + numclipnodes;
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
				out->children[j] = numclipnodes + in->children[j];
			}
		}
	}
}

//==========================================================================
//
//	QClipMap29::MakeHulls
//
//==========================================================================

void QClipMap29::MakeHulls()
{
	chullshared_t* hull = &hullsshared[1];
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	if (GGameType & GAME_Hexen2)
	{
		hull = &hullsshared[2];
		hull->clip_mins[0] = -24;
		hull->clip_mins[1] = -24;
		hull->clip_mins[2] = -20;
		hull->clip_maxs[0] = 24;
		hull->clip_maxs[1] = 24;
		hull->clip_maxs[2] = 20;

		hull = &hullsshared[3];
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -12;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 16;

		hull = &hullsshared[4];
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

		hull = &hullsshared[5];
		hull->clip_mins[0] = -48;
		hull->clip_mins[1] = -48;
		hull->clip_mins[2] = -50;
		hull->clip_maxs[0] = 48;
		hull->clip_maxs[1] = 48;
		hull->clip_maxs[2] = 50;
	}
	else
	{
		hull = &hullsshared[2];
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

void QClipMap29::LoadSubmodelsQ1(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_q1_t* in = (const bsp29_dmodel_q1_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	//	It's always from model 0.
	numclusters = LittleLong(in->visleafs);

	numsubmodels = count;
	map_models = new cmodel_t[count];
	Com_Memset(map_models, 0, sizeof(cmodel_t) * count);

	for (int i = 0; i < count; i++, in++)
	{
		cmodel_t* out = &map_models[i];

		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}

		//	Clip nodes for hull 0 are appended after clip nodes of
		// other hulls. They are build from regular nodes.
		out->hulls[0].firstclipnode = numclipnodes + LittleLong(in->headnode[0]);
		out->hulls[0].lastclipnode = numclipnodes + numnodes - 1;

		for (int j = 1; j < BSP29_MAX_MAP_HULLS_Q1; j++)
		{
			out->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
			out->hulls[j].lastclipnode = numclipnodes - 1;
		}
	}
}

//==========================================================================
//
//	QClipMap29::LoadSubmodelsH2
//
//==========================================================================

void QClipMap29::LoadSubmodelsH2(const quint8* base, const bsp29_lump_t* l)
{
	const bsp29_dmodel_h2_t* in = (const bsp29_dmodel_h2_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		common->Error("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	//	It's always from model 0.
	numclusters = LittleLong(in->visleafs);

	numsubmodels = count;
	map_models = new cmodel_t[count];
	Com_Memset(map_models, 0, sizeof(cmodel_t) * count);

	for (int i = 0; i < count; i++, in++)
	{
		cmodel_t* out = &map_models[i];

		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}

		//	Clip nodes for hull 0 are appended after clip nodes of
		// other hulls. They are build from regular nodes.
		out->hulls[0].firstclipnode = numclipnodes + LittleLong(in->headnode[0]);
		out->hulls[0].lastclipnode = numclipnodes + numnodes - 1;

		for (int j = 1; j < BSP29_MAX_MAP_HULLS_H2; j++)
		{
			out->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
			out->hulls[j].lastclipnode = numclipnodes - 1;
		}
	}
}
