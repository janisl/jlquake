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

static void CM_LoadAliasModel(cmodel_t* mod, void* buffer);
static void CM_LoadAliasModelNew(cmodel_t* mod, void* buffer);
static void CM_LoadSpriteModel(cmodel_t* mod, void* buffer);
static void CM_InitBoxHull();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte			mod_novis[BSP29_MAX_MAP_LEAFS/8];

static QClipMap29*	CMap;

#define MAX_MOD_KNOWN	2048
static QClipMap29*	cm_known[MAX_MOD_KNOWN];
static int			mod_numknown;

chull_t		box_hull;
bsp29_dclipnode_t	box_clipnodes[6];
cplane_t	box_planes[6];

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

	cnode_t* node = CMap->map_models[0].nodes;
	while (1)
	{
		if (node->contents < 0)
		{
			return (cleaf_t*)node - CMap->map_models[0].leafs;
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
	return CM_DecompressVis(CMap->map_models[0].leafs[Cluster + 1].compressed_vis,
		&CMap->map_models[0]);
}

//==========================================================================
//
//	CM_ClusterPHS
//
//==========================================================================

byte* CM_ClusterPHS(int Cluster)
{
	return CMap->map_models[0].phs + (Cluster + 1) * 4 * ((CM_NumClusters() + 31) >> 5);
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
		if (!QStr::Cmp(CMap->map_models[0].name, name))
		{
			//	Already got it.
			return &CMap->map_models[0];
		}

		CMap->map_models[0].Free();
		delete CMap;
	}

	CMap = new QClipMap29;
	Com_Memset(CMap->map_models, 0, sizeof(CMap->map_models));
	cmodel_t* mod = &CMap->map_models[0];
	QStr::Cpy(mod->name, name);

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(mod->name, Buffer) <= 0)
	{
		return NULL;
	}

	CMap->LoadBrushModel(mod, Buffer.Ptr());

	CM_InitBoxHull();

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
	if (i < 1 || i > CMap->map_models[0].numsubmodels)
	{
		throw QDropException("Bad submodel index");
	}
	return &CMap->map_models[i];
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
	for (int i = 0; i < mod_numknown; i++)
	{
		if (!QStr::Cmp(cm_known[i]->map_models[0].name, name))
		{
			return &cm_known[i]->map_models[0];
		}
	}

	if (mod_numknown == MAX_MOD_KNOWN)
	{
		Sys_Error("mod_numknown == MAX_MOD_KNOWN");
	}
	cm_known[mod_numknown] = new QClipMap29;
	Com_Memset(cm_known[mod_numknown]->map_models, 0, sizeof(cm_known[mod_numknown]->map_models));
	cmodel_t* mod = &cm_known[mod_numknown]->map_models[0];
	QStr::Cpy(mod->name, name);
	QClipMap29* LoadCMap = cm_known[mod_numknown];
	mod_numknown++;

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
		CM_LoadAliasModelNew(mod, Buffer.Ptr());
		break;

	case IDPOLYHEADER:
		CM_LoadAliasModel(mod, Buffer.Ptr());
		break;

	case IDSPRITEHEADER:
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
	checksum = CMap->map_models[0].checksum;
	checksum2 = CMap->map_models[0].checksum2;
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
	return CMap->map_models[0].numleafs;
}

//==========================================================================
//
//	CM_NumInlineModels
//
//==========================================================================

int CM_NumInlineModels()
{
	return CMap->map_models[0].numsubmodels;
}

//==========================================================================
//
//	CM_EntityString
//
//==========================================================================

const char* CM_EntityString()
{
	return CMap->map_models[0].entities;
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
	return CMap->map_models[0].leafs[LeafNum].ambient_sound_level;
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

	mod->type = cmod_alias;

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
	mod->type = cmod_alias;
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
	mod->type = cmod_alias;
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
	chull_t* hull = &CMap->map_models[0].hulls[HullNum];
	return CM_HullPointContents(hull, hull->firstclipnode, p);
}

//==========================================================================
//
//	CM_InitBoxHull
//
//	Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper chull_t structure.
//
//==========================================================================

static void CM_InitBoxHull()
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
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
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
	return CM_RecursiveHullCheck(&CMap->map_models[0].hulls[0], 0, 0, 1, start, end, trace);
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
		int leafnum = leaf - CMap->map_models[0].leafs;

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
	CM_BoxLeafnums_r(mins, maxs, CMap->map_models[0].nodes);
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

	num = CMap->map_models[0].numleafs;
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


	CMap->map_models[0].phs = (byte*)Hunk_Alloc (rowbytes*num);
	count = 0;
	scan = pvs;
	dest = (unsigned *)CMap->map_models[0].phs;
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
