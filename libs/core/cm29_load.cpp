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
#include "mdlfile.h"
#include "sprfile.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

class QMdlBoundsLoader
{
public:
	int			skinwidth;
	int			skinheight;
	int			numverts;
	vec3_t		scale;
	vec3_t		scale_origin;

	vec3_t		mins;
	vec3_t		maxs;

	float		aliastransform[3][4];

	void LoadAliasModel(cmodel_t* mod, void* buffer);
	void LoadAliasModelNew(cmodel_t* mod, void* buffer);
	void* LoadAllSkins(int numskins, daliasskintype_t* pskintype);
	void* LoadAliasFrame(void* pin);
	void* LoadAliasGroup(void* pin);
	void AliasTransformVector(vec3_t in, vec3_t out);
};

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
//	QClipMap29::LoadModel
//
//==========================================================================

void QClipMap29::LoadModel(const char* name, const QArray<quint8>& Buffer)
{
	Com_Memset(Map.map_models, 0, sizeof(Map.map_models));
	cmodel_t* mod = &Map.map_models[0];
	QStr::Cpy(mod->name, name);

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

	const quint8* mod_base = Buffer.Ptr();

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

	CalcPHS();
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
//	QClipMap29::PrecacheModel
//
//==========================================================================

clipHandle_t QClipMap29::PrecacheModel(const char* Name)
{
	if (!Name[0])
	{
		throw QDropException("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (int i = 0; i < numknown; i++)
	{
		if (!QStr::Cmp(known[i]->model.name, Name))
		{
			return (MAX_MAP_MODELS + i) * MAX_MAP_HULLS;
		}
	}

	if (numknown == MAX_CMOD_KNOWN)
	{
		throw QDropException("mod_numknown == MAX_CMOD_KNOWN");
	}
	known[numknown] = new QClipModelNonMap29;
	QClipModelNonMap29* LoadCMap = known[numknown];
	numknown++;

	LoadCMap->Load(Name);

	return (MAX_MAP_MODELS + numknown - 1) * MAX_MAP_HULLS;
}

//==========================================================================
//
//	QClipModelNonMap29::Load
//
//==========================================================================

void QClipModelNonMap29::Load(const char* name)
{
	Com_Memset(&model, 0, sizeof(model));
	cmodel_t* mod = &model;

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(name, Buffer) <= 0)
	{
		throw QDropException(va("CM_PrecacheModel: %s not found", name));
	}

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)Buffer.Ptr()))
	{
	case RAPOLYHEADER:
		LoadAliasModelNew(mod, Buffer.Ptr());
		break;

	case IDPOLYHEADER:
		LoadAliasModel(mod, Buffer.Ptr());
		break;

	case IDSPRITE1HEADER:
		LoadSpriteModel(mod, Buffer.Ptr());
		break;

	default:
		LoadBrushModelNonMap(mod, Buffer.Ptr());
		break;
	}

	QStr::Cpy(mod->name, name);
}

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
//	LOADING OF NON-BSP MODELS
//
//**************************************************************************

//==========================================================================
//
//	QClipModelNonMap29::LoadAliasModel
//
//==========================================================================

void QClipModelNonMap29::LoadAliasModel(cmodel_t* mod, void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModel(mod, buffer);
	}
	else
	{
		mod->type = cmod_alias;
		mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
		mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
	}
}

//==========================================================================
//
//	QClipModelNonMap29::LoadAliasModelNew
//
//==========================================================================

void QClipModelNonMap29::LoadAliasModelNew(cmodel_t* mod, void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModelNew(mod, buffer);
	}
	else
	{
		mod->type = cmod_alias;
		mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
		mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
	}
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAliasModel
//
//==========================================================================

void QMdlBoundsLoader::LoadAliasModel(cmodel_t* mod, void* buffer)
{
	mdl_t* pinmodel = (mdl_t *)buffer;

	int version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			 mod->name, version, ALIAS_VERSION));
	}

	int numskins = LittleLong(pinmodel->numskins);
	skinwidth = LittleLong(pinmodel->skinwidth);
	skinheight = LittleLong(pinmodel->skinheight);
	numverts = LittleLong(pinmodel->numverts);
	int numtris = LittleLong(pinmodel->numtris);
	int numframes = LittleLong (pinmodel->numframes);

	for (int i = 0; i < 3; i++)
	{
		scale[i] = LittleFloat (pinmodel->scale[i]);
		scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
	}

	daliasskintype_t* pskintype = (daliasskintype_t*)&pinmodel[1];
	pskintype = (daliasskintype_t*)LoadAllSkins(numskins, pskintype);
	stvert_t* pinstverts = (stvert_t*)pskintype;
	dtriangle_t* pintriangles = (dtriangle_t*)&pinstverts[numverts];
	daliasframetype_t* pframetype = (daliasframetype_t*)&pintriangles[numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;
	for (int i = 0; i < numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong(pframetype->type);
		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)LoadAliasFrame(pframetype + 1);
		}
		else
		{
			pframetype = (daliasframetype_t*)LoadAliasGroup(pframetype + 1);
		}
	}

	mod->type = cmod_alias;

	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAliasModelNew
//
//	Reads extra field for num ST verts, and extra index list of them
//
//==========================================================================

void QMdlBoundsLoader::LoadAliasModelNew(cmodel_t* mod, void* buffer)
{
	newmdl_t* pinmodel = (newmdl_t *)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != ALIAS_NEWVERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			mod->name, version, ALIAS_NEWVERSION));
	}

	int numskins = LittleLong(pinmodel->numskins);
	skinwidth = LittleLong(pinmodel->skinwidth);
	skinheight = LittleLong(pinmodel->skinheight);
	numverts = LittleLong(pinmodel->numverts);
	int numstverts = LittleLong(pinmodel->num_st_verts);	//hide num_st in version
	int numtris = LittleLong(pinmodel->numtris);
	int numframes = LittleLong (pinmodel->numframes);

	for (int i = 0; i < 3; i++)
	{
		scale[i] = LittleFloat (pinmodel->scale[i]);
		scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
	}

	daliasskintype_t* pskintype = (daliasskintype_t*)&pinmodel[1];
	pskintype = (daliasskintype_t*)LoadAllSkins(numskins, pskintype);
	stvert_t* pinstverts = (stvert_t*)pskintype;
	dnewtriangle_t* pintriangles = (dnewtriangle_t*)&pinstverts[numstverts];
	daliasframetype_t* pframetype = (daliasframetype_t*)&pintriangles[numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;
	for (int i = 0; i < numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong (pframetype->type);
		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)LoadAliasFrame(pframetype + 1);
		}
		else
		{
			pframetype = (daliasframetype_t*)LoadAliasGroup(pframetype + 1);
		}
	}

	mod->type = cmod_alias;

	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAllSkins
//
//==========================================================================

void* QMdlBoundsLoader::LoadAllSkins(int numskins, daliasskintype_t* pskintype)
{
	for (int i = 0; i < numskins; i++)
	{
		int s = skinwidth * skinheight;
		pskintype = (daliasskintype_t*)((byte*)(pskintype + 1) + s);
	}
	return (void *)pskintype;
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAliasFrame
//
//==========================================================================

void* QMdlBoundsLoader::LoadAliasFrame(void* pin)
{
	daliasframe_t* pdaliasframe = (daliasframe_t*)pin;
	trivertx_t* pinframe = (trivertx_t*)(pdaliasframe + 1);

	aliastransform[0][0] = scale[0];
	aliastransform[1][1] = scale[1];
	aliastransform[2][2] = scale[2];
	aliastransform[0][3] = scale_origin[0];
	aliastransform[1][3] = scale_origin[1];
	aliastransform[2][3] = scale_origin[2];

	for (int j = 0; j < numverts; j++)
	{
		vec3_t in,out;
		in[0] = pinframe[j].v[0];
		in[1] = pinframe[j].v[1];
		in[2] = pinframe[j].v[2];
		AliasTransformVector(in, out);
		for (int i = 0; i < 3; i++)
		{
			if (mins[i] > out[i])
			{
				mins[i] = out[i];
			}
			if (maxs[i] < out[i])
			{
				maxs[i] = out[i];
			}
		}
	}
	pinframe += numverts;
	return (void*)pinframe;
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAliasGroup
//
//==========================================================================

void* QMdlBoundsLoader::LoadAliasGroup(void* pin)
{
	daliasgroup_t* pingroup = (daliasgroup_t*)pin;
	int numframes = LittleLong (pingroup->numframes);
	daliasinterval_t* pin_intervals = (daliasinterval_t *)(pingroup + 1);
	pin_intervals += numframes;
	void* ptemp = (void*)pin_intervals;

	aliastransform[0][0] = scale[0];
	aliastransform[1][1] = scale[1];
	aliastransform[2][2] = scale[2];
	aliastransform[0][3] = scale_origin[0];
	aliastransform[1][3] = scale_origin[1];
	aliastransform[2][3] = scale_origin[2];

	for (int i = 0; i < numframes; i++)
	{
		trivertx_t* poseverts = (trivertx_t*)((daliasframe_t*)ptemp + 1);
		for (int j = 0; j < numverts; j++)
		{
			vec3_t in, out;
			in[0] = poseverts[j].v[0];
			in[1] = poseverts[j].v[1];
			in[2] = poseverts[j].v[2];
			AliasTransformVector(in, out);
			for (int k = 0; k < 3; k++)
			{
				if (mins[k] > out[k])
					mins[k] = out[k];
				if (maxs[k] < out[k])
					maxs[k] = out[k];
			}
		}
		ptemp = (trivertx_t *)((daliasframe_t *)ptemp + 1) + numverts;
	}
	return ptemp;
}

//==========================================================================
//
//	QMdlBoundsLoader::AliasTransformVector
//
//==========================================================================

void QMdlBoundsLoader::AliasTransformVector(vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
	out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
	out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}

//==========================================================================
//
//	QClipModelNonMap29::LoadSpriteModel
//
//==========================================================================

void QClipModelNonMap29::LoadSpriteModel(cmodel_t* mod, void* buffer)
{
	dsprite1_t* pin = (dsprite1_t*)buffer;

	int version = LittleLong (pin->version);
	if (version != SPRITE1_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)", mod->name, version, SPRITE1_VERSION));
	}

	mod->type = cmod_sprite;

	mod->mins[0] = mod->mins[1] = -LittleLong(pin->width) / 2;
	mod->maxs[0] = mod->maxs[1] = LittleLong(pin->width) / 2;
	mod->mins[2] = -LittleLong(pin->height) / 2;
	mod->maxs[2] = LittleLong(pin->height) / 2;
}
