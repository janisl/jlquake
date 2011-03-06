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
//
//	Handle loading of non-map models, used by Quake and Hexen 2.
//
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include "cm29_local.h"
#include "mdlfile.h"
#include "sprfile.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

class QClipMapNonMap : public QClipMap
{
private:
	//	Main
	void InitBoxHull();
	cmodel_t* ClipHandleToModel(clipHandle_t Handle);
	chull_t* ClipHandleToHull(clipHandle_t Handle);
	int ContentsToQ2(int Contents) const;
	int ContentsToQ3(int Contents) const;

	//	Test
	void BoxLeafnums_r(leafList_t* ll, const cnode_t *node) const;
	int HullPointContents(const chull_t* Hull, int NodeNum, const vec3_t P) const;
	bool HeadnodeVisible(cnode_t* Node, byte* VisBits);
	byte* DecompressVis(byte* in);
	void CalcPHS();

	//	Trace
	bool RecursiveHullCheck(chull_t* hull, int num, float p1f, float p2f,
		vec3_t p1, vec3_t p2, q1trace_t* trace);

public:
	static byte			mod_novis[BSP29_MAX_MAP_LEAFS / 8];

	QClipModel29		Map;

	cmodel_t			box_model;
	bsp29_dclipnode_t	box_clipnodes[6];
	cplane_t			box_planes[6];

	QClipMapNonMap();
	~QClipMapNonMap();

	void LoadMap(const char* name, const QArray<quint8>& Buffer);
	void ReloadMap(bool ClientLoad);
	clipHandle_t InlineModel(int Index) const;
	int GetNumClusters() const;
	int GetNumInlineModels() const;
	const char* GetEntityString() const;
	void MapChecksums(int& CheckSum1, int& CheckSum2) const;
	int LeafCluster(int LeafNum) const;
	int LeafArea(int LeafNum) const;
	const byte* LeafAmbientSoundLevel(int LeafNum) const;
	void ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs);
	int GetNumTextures() const;
	const char* GetTextureName(int Index) const;
	clipHandle_t TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule);
	clipHandle_t ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs);
	int PointLeafnum(const vec3_t P) const;
	int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf) const;
	int PointContentsQ1(const vec3_t P, clipHandle_t Model);
	int PointContentsQ2(const vec3_t p, clipHandle_t Model);
	int PointContentsQ3(const vec3_t P, clipHandle_t Model);
	int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	bool HeadnodeVisible(int NodeNum, byte *VisBits);
	byte* ClusterPVS(int Cluster);
	byte* ClusterPHS(int Cluster);
	void SetAreaPortalState(int PortalNum, qboolean Open);
	void AdjustAreaPortalState(int Area1, int Area2, bool Open);
	qboolean AreasConnected(int Area1, int Area2);
	int WriteAreaBits(byte* Buffer, int Area);
	void WritePortalState(fileHandle_t f);
	void ReadPortalState(fileHandle_t f);
	bool HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace);
	q2trace_t BoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask);
	q2trace_t TransformedBoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model,
		int BrushMask, vec3_t Origin, vec3_t Angles);
	void BoxTraceQ3(q3trace_t* Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, int Capsule);
	void TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule);
	void DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points));

	void LoadNonMap(const char* name, const QArray<quint8>& Buffer);
	void LoadAliasModel(cmodel_t* mod, const void* buffer);
	void LoadAliasModelNew(cmodel_t* mod, const void* buffer);
	void LoadSpriteModel(cmodel_t* mod, const void* buffer);
};

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

	void LoadAliasModel(QClipMapNonMap* mod, const void* buffer);
	void LoadAliasModelNew(QClipMapNonMap* mod, const void* buffer);
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

QArray<QClipMap*>		CMNonMapModels;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_PrecacheModel
//
//==========================================================================

clipHandle_t CM_PrecacheModel(const char* Name)
{
	if (!Name[0])
	{
		throw QDropException("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (int i = 0; i < CMNonMapModels.Num(); i++)
	{
		if (CMNonMapModels[i]->Name == Name)
		{
			return (i + 1) << CMH_NON_MAP_SHIFT;
		}
	}

	//
	// load the file
	//
	QArray<quint8> Buffer;
	if (FS_ReadFile(Name, Buffer) <= 0)
	{
		throw QDropException(va("CM_PrecacheModel: %s not found", Name));
	}

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)Buffer.Ptr()))
	{
	case BSP29_VERSION:
		{
			QClipMap29* LoadCMap = new QClipMap29;
			CMNonMapModels.Append(LoadCMap);
			LoadCMap->LoadMap(Name, Buffer);
			if (LoadCMap->Map.map_models[0].numsubmodels > 1)
			{
				GLog.WriteLine("Non-map BSP models are not supposed to have submodels");
			}
			break;
		}

	default:
		{
			QClipMapNonMap* LoadCMap = new QClipMapNonMap;
			CMNonMapModels.Append(LoadCMap);

			LoadCMap->LoadNonMap(Name, Buffer);
		}
	}

	return CMNonMapModels.Num() << CMH_NON_MAP_SHIFT;
}

//==========================================================================
//
//	QClipMapNonMap::LoadNonMap
//
//==========================================================================

void QClipMapNonMap::LoadNonMap(const char* name, const QArray<quint8>& Buffer)
{
	Com_Memset(&Map.map_models, 0, sizeof(Map.map_models));
	cmodel_t* mod = &Map.map_models[0];

	this->Name = name;

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
		mod->type = cmod_alias;
		mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
		mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
		break;
	}
}

//==========================================================================
//
//	QClipMapNonMap::LoadAliasModel
//
//==========================================================================

void QClipMapNonMap::LoadAliasModel(cmodel_t* mod, const void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModel(this, buffer);
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
//	QClipMapNonMap::LoadAliasModelNew
//
//==========================================================================

void QClipMapNonMap::LoadAliasModelNew(cmodel_t* mod, const void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModelNew(this, buffer);
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

void QMdlBoundsLoader::LoadAliasModel(QClipMapNonMap* mod, const void* buffer)
{
	mdl_t* pinmodel = (mdl_t *)buffer;

	int version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			 *mod->Name, version, ALIAS_VERSION));
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

	mod->Map.map_models[0].type = cmod_alias;

	mod->Map.map_models[0].mins[0] = mins[0] - 10;
	mod->Map.map_models[0].mins[1] = mins[1] - 10;
	mod->Map.map_models[0].mins[2] = mins[2] - 10;
	mod->Map.map_models[0].maxs[0] = maxs[0] + 10;
	mod->Map.map_models[0].maxs[1] = maxs[1] + 10;
	mod->Map.map_models[0].maxs[2] = maxs[2] + 10;
}

//==========================================================================
//
//	QMdlBoundsLoader::LoadAliasModelNew
//
//	Reads extra field for num ST verts, and extra index list of them
//
//==========================================================================

void QMdlBoundsLoader::LoadAliasModelNew(QClipMapNonMap* mod, const void* buffer)
{
	newmdl_t* pinmodel = (newmdl_t *)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != ALIAS_NEWVERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			*mod->Name, version, ALIAS_NEWVERSION));
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

	mod->Map.map_models[0].type = cmod_alias;

	mod->Map.map_models[0].mins[0] = mins[0] - 10;
	mod->Map.map_models[0].mins[1] = mins[1] - 10;
	mod->Map.map_models[0].mins[2] = mins[2] - 10;
	mod->Map.map_models[0].maxs[0] = maxs[0] + 10;
	mod->Map.map_models[0].maxs[1] = maxs[1] + 10;
	mod->Map.map_models[0].maxs[2] = maxs[2] + 10;
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
//	QClipMapNonMap::LoadSpriteModel
//
//==========================================================================

void QClipMapNonMap::LoadSpriteModel(cmodel_t* mod, const void* buffer)
{
	dsprite1_t* pin = (dsprite1_t*)buffer;

	int version = LittleLong (pin->version);
	if (version != SPRITE1_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)", *Name, version, SPRITE1_VERSION));
	}

	mod->type = cmod_sprite;

	mod->mins[0] = mod->mins[1] = -LittleLong(pin->width) / 2;
	mod->maxs[0] = mod->maxs[1] = LittleLong(pin->width) / 2;
	mod->mins[2] = -LittleLong(pin->height) / 2;
	mod->maxs[2] = LittleLong(pin->height) / 2;
}




























// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON	(0.03125)

//==========================================================================
//
//	QClipMapNonMap::LoadMap
//
//==========================================================================

void QClipMapNonMap::LoadMap(const char* AName, const QArray<quint8>& Buffer)
{
	Com_Memset(Map.map_models, 0, sizeof(Map.map_models));
	cmodel_t* mod = &Map.map_models[0];

	mod->type = cmod_brush;

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

	Name = AName;
}

//==========================================================================
//
//	QClipMapNonMap::ReloadMap
//
//==========================================================================

void QClipMapNonMap::ReloadMap(bool ClientLoad)
{
}

//==========================================================================
//
//	CM_CreateQClipMapNonMap
//
//==========================================================================

QClipMap* CM_CreateQClipMapNonMap()
{
	return new QClipMapNonMap();
}

//==========================================================================
//
//	QClipMapNonMap::QClipMapNonMap
//
//==========================================================================

QClipMapNonMap::QClipMapNonMap()
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));
}

//==========================================================================
//
//	QClipMapNonMap::~QClipMapNonMap
//
//==========================================================================

QClipMapNonMap::~QClipMapNonMap()
{
}

//==========================================================================
//
//	QClipMapNonMap::InlineModel
//
//==========================================================================

clipHandle_t QClipMapNonMap::InlineModel(int Index) const
{
	if (Index < 1 || Index > Map.map_models[0].numsubmodels)
	{
		throw QDropException("Bad submodel index");
	}
	return Index * MAX_MAP_HULLS;
}

//==========================================================================
//
//	QClipMapNonMap::GetNumClusters
//
//==========================================================================

int QClipMapNonMap::GetNumClusters() const
{
	return Map.map_models[0].numleafs;
}

//==========================================================================
//
//	QClipMapNonMap::GetNumInlineModels
//
//==========================================================================

int QClipMapNonMap::GetNumInlineModels() const
{
	return Map.map_models[0].numsubmodels;
}

//==========================================================================
//
//	QClipMapNonMap::GetEntityString
//
//==========================================================================

const char* QClipMapNonMap::GetEntityString() const
{
	return Map.map_models[0].entities;
}

//==========================================================================
//
//	QClipMapNonMap::MapChecksums
//
//==========================================================================

void QClipMapNonMap::MapChecksums(int& ACheckSum1, int& ACheckSum2) const
{
	ACheckSum1 = CheckSum;
	ACheckSum2 = CheckSum2;
}

//==========================================================================
//
//	QClipMapNonMap::LeafCluster
//
//==========================================================================

int QClipMapNonMap::LeafCluster(int LeafNum) const
{
	//	-1 is because pvs rows are 1 based, not 0 based like leafs
	return LeafNum - 1;
}

//==========================================================================
//
//	QClipMapNonMap::LeafArea
//
//==========================================================================

int QClipMapNonMap::LeafArea(int LeafNum) const
{
	return 0;
}

//==========================================================================
//
//	QClipMapNonMap::LeafAmbientSoundLevel
//
//==========================================================================

const byte* QClipMapNonMap::LeafAmbientSoundLevel(int LeafNum) const
{
	return Map.map_models[0].leafs[LeafNum].ambient_sound_level;
}

//==========================================================================
//
//	QClipMapNonMap::ModelBounds
//
//==========================================================================

void QClipMapNonMap::ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	cmodel_t* cmod = ClipHandleToModel(Model);
	VectorCopy(cmod->mins, Mins);
	VectorCopy(cmod->maxs, Maxs);
}

//==========================================================================
//
//	QClipMapNonMap::GetNumTextures
//
//==========================================================================

int QClipMapNonMap::GetNumTextures() const
{
	return 0;
}

//==========================================================================
//
//	QClipMapNonMap::GetTextureName
//
//==========================================================================

const char* QClipMapNonMap::GetTextureName(int Index) const
{
	return "";
}

//==========================================================================
//
//	QClipMapNonMap::ModelHull
//
//==========================================================================

clipHandle_t QClipMapNonMap::ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
{
	cmodel_t* model = ClipHandleToModel(Handle);
	if (HullNum < 0 || HullNum >= MAX_MAP_HULLS)
	{
		throw QException("Invalid hull number");
	}
	if (!model || model->type != cmod_brush)
	{
		throw QException("Non bsp model");
	}
	VectorCopy(model->hulls[HullNum].clip_mins, ClipMins);
	VectorCopy(model->hulls[HullNum].clip_maxs, ClipMaxs);
	return (Handle & MODEL_NUMBER_MASK) | HullNum;
}

//==========================================================================
//
//	QClipMapNonMap::ClipHandleToModel
//
//==========================================================================

cmodel_t* QClipMapNonMap::ClipHandleToModel(clipHandle_t Handle)
{
	Handle /= MAX_MAP_HULLS;
	if (Handle < MAX_MAP_MODELS)
	{
		return &Map.map_models[Handle];
	}
	if (Handle == BOX_HULL_HANDLE / MAX_MAP_HULLS)
	{
		return &box_model;
	}
	throw QException("Invalid handle");
}

//==========================================================================
//
//	QClipMapNonMap::ClipHandleToHull
//
//==========================================================================

chull_t* QClipMapNonMap::ClipHandleToHull(clipHandle_t Handle)
{
	cmodel_t* Model = ClipHandleToModel(Handle);
	return &Model->hulls[Handle & HULL_NUMBER_MASK];
}

//**************************************************************************
//
//	BOX HULL
//
//**************************************************************************

//==========================================================================
//
//	QClipMapNonMap::InitBoxHull
//
//	Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper chull_t structure.
//
//==========================================================================

void QClipMapNonMap::InitBoxHull()
{
	Com_Memset(&box_model, 0, sizeof(box_model));
	box_model.hulls[0].clipnodes = box_clipnodes;
	box_model.hulls[0].planes = box_planes;
	box_model.hulls[0].firstclipnode = 0;
	box_model.hulls[0].lastclipnode = 5;

	Com_Memset(box_planes, 0, sizeof(box_planes));

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
//	QClipMapNonMap::TempBoxModel
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
//
//==========================================================================

clipHandle_t QClipMapNonMap::TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
{
	box_planes[0].dist = Maxs[0];
	box_planes[1].dist = Mins[0];
	box_planes[2].dist = Maxs[1];
	box_planes[3].dist = Mins[1];
	box_planes[4].dist = Maxs[2];
	box_planes[5].dist = Mins[2];

	return BOX_HULL_HANDLE;
}

//==========================================================================
//
//	QClipMapNonMap::ContentsToQ2
//
//==========================================================================

int QClipMapNonMap::ContentsToQ2(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMapNonMap::ContentsToQ3
//
//==========================================================================

int QClipMapNonMap::ContentsToQ3(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMapNonMap::DrawDebugSurface
//
//==========================================================================

void QClipMapNonMap::DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points))
{
}

//==========================================================================
//
//	QClipMapNonMap::PointLeafnum
//
//==========================================================================

int QClipMapNonMap::PointLeafnum(const vec3_t P) const
{
	cnode_t* Node = Map.map_models[0].nodes;
	while (1)
	{
		if (Node->contents < 0)
		{
			return (cleaf_t*)Node - Map.map_models[0].leafs;
		}
		cplane_t* Plane = Node->plane;
		float d = DotProduct(P, Plane->normal) - Plane->dist;
		if (d > 0)
		{
			Node = Node->children[0];
		}
		else
		{
			Node = Node->children[1];
		}
	}

	return 0;	// never reached
}

//==========================================================================
//
//	QClipMapNonMap::BoxLeafnums_r
//
//==========================================================================

void QClipMapNonMap::BoxLeafnums_r(leafList_t* ll, const cnode_t *node) const
{
	if (node->contents == BSP29CONTENTS_SOLID)
	{
		return;
	}

	if (node->contents < 0)
	{
		const cleaf_t* leaf = (const cleaf_t*)node;
		int LeafNum = leaf - Map.map_models[0].leafs;

		// store the lastLeaf even if the list is overflowed
		if (LeafNum != 0)
		{
			ll->lastLeaf = LeafNum;
		}

		if (ll->count == ll->maxcount)
		{
			return;
		}

		ll->list[ll->count++] = LeafNum;
		return;
	}

	// NODE_MIXED

	const cplane_t* splitplane = node->plane;
	int sides = BOX_ON_PLANE_SIDE(ll->bounds[0], ll->bounds[1], splitplane);

	if (sides == 3 && ll->topnode == -1)
	{
		ll->topnode = node - Map.map_models[0].nodes;
	}

	// recurse down the contacted sides
	if (sides & 1)
	{
		BoxLeafnums_r(ll, node->children[0]);
	}

	if (sides & 2)
	{
		BoxLeafnums_r(ll, node->children[1]);
	}
}

//==========================================================================
//
//	QClipMapNonMap::BoxLeafnums
//
//==========================================================================

int QClipMapNonMap::BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode, int* LastLeaf) const
{
	leafList_t ll;

	VectorCopy(Mins, ll.bounds[0]);
	VectorCopy(Maxs, ll.bounds[1]);
	ll.count = 0;
	ll.list = List;
	ll.maxcount = ListSize;
	ll.topnode = -1;
	ll.lastLeaf = 0;

	BoxLeafnums_r(&ll, Map.map_models[0].nodes);

	if (TopNode)
	{
		*TopNode = ll.topnode;
	}
	if (LastLeaf)
	{
		*LastLeaf = ll.lastLeaf;
	}
	return ll.count;
}

//==========================================================================
//
//	QClipMapNonMap::HullPointContents
//
//==========================================================================

int QClipMapNonMap::HullPointContents(const chull_t* Hull, int NodeNum, const vec3_t P) const
{
	while (NodeNum >= 0)
	{
		if (NodeNum < Hull->firstclipnode || NodeNum > Hull->lastclipnode)
		{
			throw QException("SV_HullPointContents: bad node number");
		}
	
		bsp29_dclipnode_t* node = Hull->clipnodes + NodeNum;
		cplane_t* plane = Hull->planes + node->planenum;

		float d;
		if (plane->type < 3)
		{
			d = P[plane->type] - plane->dist;
		}
		else
		{
			d = DotProduct(plane->normal, P) - plane->dist;
		}
		if (d < 0)
		{
			NodeNum = node->children[1];
		}
		else
		{
			NodeNum = node->children[0];
		}
	}

	return NodeNum;
}

//==========================================================================
//
//	QClipMapNonMap::PointContentsQ1
//
//==========================================================================

int QClipMapNonMap::PointContentsQ1(const vec3_t P, clipHandle_t Model)
{
	chull_t* hull = ClipHandleToHull(Model);
	return HullPointContents(hull, hull->firstclipnode, P);
}

//==========================================================================
//
//	QClipMapNonMap::PointContentsQ2
//
//==========================================================================

int QClipMapNonMap::PointContentsQ2(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ2(PointContentsQ1(P, Model));
}

//==========================================================================
//
//	QClipMapNonMap::PointContentsQ3
//
//==========================================================================

int QClipMapNonMap::PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ3(PointContentsQ1(P, Model));
}

//==========================================================================
//
//	QClipMapNonMap::TransformedPointContentsQ1
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

int QClipMapNonMap::TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	// subtract origin offset
	vec3_t p_l;
	VectorSubtract(P, Origin, p_l);

	// rotate start and end into the models frame of reference
	if (Model != BOX_HULL_HANDLE && (Angles[0] || Angles[1] || Angles[2]))
	{
		vec3_t forward, right, up;
		AngleVectors(Angles, forward, right, up);

		vec3_t temp;
		VectorCopy(p_l, temp);
		p_l[0] = DotProduct(temp, forward);
		p_l[1] = -DotProduct(temp, right);
		p_l[2] = DotProduct(temp, up);
	}

	return PointContentsQ1(p_l, Model);
}

//==========================================================================
//
//	QClipMapNonMap::TransformedPointContentsQ2
//
//==========================================================================

int QClipMapNonMap::TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ2(TransformedPointContentsQ1(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMapNonMap::TransformedPointContentsQ3
//
//==========================================================================

int QClipMapNonMap::TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ3(TransformedPointContentsQ1(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMapNonMap::HeadnodeVisible
//
//	Returns true if any leaf under headnode has a cluster that
// is potentially visible
//
//==========================================================================

bool QClipMapNonMap::HeadnodeVisible(int NodeNum, byte* VisBits)
{
	return HeadnodeVisible(&Map.map_models[0].nodes[NodeNum], VisBits);
}

//==========================================================================
//
//	QClipMapNonMap::HeadnodeVisible
//
//	Returns true if any leaf under headnode has a cluster that
// is potentially visible
//
//==========================================================================

bool QClipMapNonMap::HeadnodeVisible(cnode_t* Node, byte* VisBits)
{
	if (Node->contents < 0)
	{
		const cleaf_t* leaf = (const cleaf_t*)Node;
		int LeafNum = leaf - Map.map_models[0].leafs;
		int cluster = LeafNum - 1;
		if (cluster == -1)
		{
			return false;
		}
		if (VisBits[cluster >> 3] & (1 << (cluster & 7)))
		{
			return true;
		}
		return false;
	}

	if (HeadnodeVisible(Node->children[0], VisBits))
	{
		return true;
	}
	return HeadnodeVisible(Node->children[1], VisBits);
}

//==========================================================================
//
//	QClipMapNonMap::DecompressVis
//
//==========================================================================

byte* QClipMapNonMap::DecompressVis(byte* in)
{
	static byte		decompressed[BSP29_MAX_MAP_LEAFS / 8];

	if (!in)
	{
		// no vis info
		return mod_novis;
	}

	int row = (Map.map_models[0].numleafs + 7) >> 3;
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
//	QClipMapNonMap::ClusterPVS
//
//==========================================================================

byte* QClipMapNonMap::ClusterPVS(int Cluster)
{
	if (Cluster < 0)
	{
		return mod_novis;
	}
	return DecompressVis(Map.map_models[0].leafs[Cluster + 1].compressed_vis);
}

//==========================================================================
//
//	QClipMapNonMap::CalcPHS
//
//	Calculates the PHS (Potentially Hearable Set)
//
//==========================================================================

void QClipMapNonMap::CalcPHS()
{
	GLog.Write("Building PHS...\n");

	int num = Map.map_models[0].numleafs;
	int rowwords = (num + 31) >> 5;
	int rowbytes = rowwords * 4;

	byte* pvs = new byte[rowbytes * num];
	byte* scan = pvs;
	int vcount = 0;
	for (int i = 0 ; i < num; i++, scan += rowbytes)
	{
		Com_Memcpy(scan, ClusterPVS(LeafCluster(i)), rowbytes);
		if (i == 0)
		{
			continue;
		}
		for (int j = 0; j < num; j++)
		{
			if (scan[j >> 3] & (1 << (j & 7)))
			{
				vcount++;
			}
		}
	}

	Map.map_models[0].phs = new byte[rowbytes * num];
	int count = 0;
	scan = pvs;
	unsigned* dest = (unsigned*)Map.map_models[0].phs;
	for (int i = 0; i < num; i++, dest += rowwords, scan += rowbytes)
	{
		Com_Memcpy(dest, scan, rowbytes);
		for (int j = 0; j < rowbytes; j++)
		{
			int bitbyte = scan[j];
			if (!bitbyte)
			{
				continue;
			}
			for (int k = 0; k < 8; k++)
			{
				if (!(bitbyte & (1 << k)))
				{
					continue;
				}
				// or this pvs row into the phs
				// +1 because pvs is 1 based
				int index = ((j << 3) + k + 1);
				if (index >= num)
				{
					continue;
				}
				unsigned* src = (unsigned*)pvs + index * rowwords;
				for (int l = 0; l < rowwords; l++)
				{
					dest[l] |= src[l];
				}
			}
		}

		if (i == 0)
		{
			continue;
		}
		for (int j = 0; j < num; j++)
		{
			if (((byte*)dest)[j >> 3] & (1 << (j & 7)))
			{
				count++;
			}
		}
	}

	delete[] pvs;

	GLog.Write("Average leafs visible / hearable / total: %i / %i / %i\n",
		vcount / num, count / num, num);
}

//==========================================================================
//
//	QClipMapNonMap::ClusterPHS
//
//==========================================================================

byte* QClipMapNonMap::ClusterPHS(int Cluster)
{
	return Map.map_models[0].phs + (Cluster + 1) * 4 * ((Map.map_models[0].numleafs + 31) >> 5);
}

//==========================================================================
//
//	QClipMapNonMap::SetAreaPortalState
//
//==========================================================================

void QClipMapNonMap::SetAreaPortalState(int portalnum, qboolean open)
{
}

//==========================================================================
//
//	QClipMapNonMap::AdjustAreaPortalState
//
//==========================================================================

void QClipMapNonMap::AdjustAreaPortalState(int Area1, int Area2, bool Open)
{
}

//==========================================================================
//
//	QClipMapNonMap::AreasConnected
//
//==========================================================================

qboolean QClipMapNonMap::AreasConnected(int Area1, int Area2)
{
	return true;
}

//==========================================================================
//
//	QClipMapNonMap::WriteAreaBits
//
//==========================================================================

int QClipMapNonMap::WriteAreaBits(byte* Buffer, int Area)
{
	return 0;
}

//==========================================================================
//
//	QClipMapNonMap::WritePortalState
//
//==========================================================================

void QClipMapNonMap::WritePortalState(fileHandle_t f)
{
}

//==========================================================================
//
//	QClipMapNonMap::ReadPortalState
//
//==========================================================================

void QClipMapNonMap::ReadPortalState(fileHandle_t f)
{
}

//==========================================================================
//
//  QClipMapNonMap::HullCheckQ1
//
//==========================================================================

bool QClipMapNonMap::HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t * trace)
{
	chull_t *hull = ClipHandleToHull(Handle);
	return RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, p1, p2, trace);
}

//==========================================================================
//
//  QClipMapNonMap::RecursiveHullCheck
//
//==========================================================================

bool QClipMapNonMap::RecursiveHullCheck(chull_t * hull, int num, float p1f,
	float p2f, vec3_t p1, vec3_t p2, q1trace_t * trace)
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
		return true;			// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
	{
		throw QException("SV_RecursiveHullCheck: bad node number");
	}

	//
	// find the point distances
	//
	bsp29_dclipnode_t *node = hull->clipnodes + num;
	cplane_t *plane = hull->planes + node->planenum;

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
		return RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
	}
	if (t1 < 0 && t2 < 0)
	{
		return RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	float frac;
	if (t1 < 0)
	{
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		frac = (t1 - DIST_EPSILON) / (t1 - t2);
	}
	if (frac < 0)
	{
		frac = 0;
	}
	if (frac > 1)
	{
		frac = 1;
	}

	float midf = p1f + (p2f - p1f) * frac;
	vec3_t mid;
	for (int i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	int side = (t1 < 0);

	// move up to the node
	if (!RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
	{
		return false;
	}

#ifdef PARANOID
	if (HullPointContents(hull, node->children[side], mid) == BSP29CONTENTS_SOLID)
	{
		GLog.Write("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (HullPointContents(hull, node->children[side ^ 1], mid) != BSP29CONTENTS_SOLID)
	{
		// go past the node
		return RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f,
								  mid, p2, trace);
	}

	if (trace->allsolid)
	{
		return false;			// never got out of the solid area
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

	while (HullPointContents(hull, hull->firstclipnode, mid) == BSP29CONTENTS_SOLID)
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
			mid[i] = p1[i] + frac * (p2[i] - p1[i]);
		}
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return false;
}

//==========================================================================
//
//	QClipMapNonMap::BoxTraceQ2
//
//==========================================================================

q2trace_t QClipMapNonMap::BoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMapNonMap::TransformedBoxTraceQ2
//
//==========================================================================

q2trace_t QClipMapNonMap::TransformedBoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, vec3_t Origin, vec3_t Angles)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMapNonMap::BoxTraceQ3
//
//==========================================================================

void QClipMapNonMap::BoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, int Capsule)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMapNonMap::TransformedBoxTraceQ3
//
//==========================================================================

void QClipMapNonMap::TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start,
	const vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask,
	const vec3_t Origin, const vec3_t Angles, int Capsule)
{
	throw QDropException("Not implemented");
}

byte			QClipMapNonMap::mod_novis[BSP29_MAX_MAP_LEAFS / 8];
