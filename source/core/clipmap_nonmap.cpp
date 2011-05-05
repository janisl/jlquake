//**************************************************************************
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
#include "clipmap_local.h"
#include "bsp29file.h"
#include "mdlfile.h"
#include "sprfile.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

class QNonBspModelException : public QException
{
public:
	QNonBspModelException()
	: QException("Non bsp model")
	{}
};

class QClipMapNonMap : public QClipMap
{
private:
	void LoadAliasModel(const void* buffer);
	void LoadAliasModelNew(const void* buffer);
	void LoadSpriteModel(const void* buffer);

public:
	//
	// volume occupied by the model graphics
	//
	vec3_t			ModelMins;
	vec3_t			ModelMaxs;

	void LoadMap(const char* name, const QArray<quint8>& Buffer);
	void ReloadMap(bool ClientLoad)
	{}
	clipHandle_t InlineModel(int Index) const
	{ return 0; }
	int GetNumClusters() const
	{ return 0; }
	int GetNumInlineModels() const
	{ return 0; }
	const char* GetEntityString() const
	{ return NULL; }
	void MapChecksums(int& CheckSum1, int& CheckSum2) const
	{}
	int LeafCluster(int LeafNum) const
	{ return 0; }
	int LeafArea(int LeafNum) const
	{ return 0; }
	const byte* LeafAmbientSoundLevel(int LeafNum) const
	{ return NULL; }
	void ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs);
	int GetNumTextures() const
	{ return 0; }
	const char* GetTextureName(int Index) const
	{ return NULL; }
	clipHandle_t TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
	{ return 0; }
	clipHandle_t ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
	{ throw QNonBspModelException(); }
	int PointLeafnum(const vec3_t P) const
	{ return 0; }
	int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf) const
	{ return 0; }
	int PointContentsQ1(const vec3_t P, clipHandle_t Model)
	{ throw QNonBspModelException(); }
	int PointContentsQ2(const vec3_t p, clipHandle_t Model)
	{ throw QNonBspModelException(); }
	int PointContentsQ3(const vec3_t P, clipHandle_t Model)
	{ throw QNonBspModelException(); }
	int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
	{ throw QNonBspModelException(); }
	int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
	{ throw QNonBspModelException(); }
	int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
	{ throw QNonBspModelException(); }
	bool HeadnodeVisible(int NodeNum, byte *VisBits)
	{ return false; }
	byte* ClusterPVS(int Cluster)
	{ return NULL; }
	byte* ClusterPHS(int Cluster)
	{ return NULL; }
	void SetAreaPortalState(int PortalNum, qboolean Open)
	{}
	void AdjustAreaPortalState(int Area1, int Area2, bool Open)
	{}
	qboolean AreasConnected(int Area1, int Area2)
	{ return false; }
	int WriteAreaBits(byte* Buffer, int Area)
	{ return 0; }
	void WritePortalState(fileHandle_t f)
	{}
	void ReadPortalState(fileHandle_t f)
	{}
	bool HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace)
	{ throw QNonBspModelException(); }
	q2trace_t BoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask)
	{ throw QNonBspModelException(); }
	q2trace_t TransformedBoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model,
		int BrushMask, vec3_t Origin, vec3_t Angles)
	{ throw QNonBspModelException(); }
	void BoxTraceQ3(q3trace_t* Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, int Capsule)
	{ throw QNonBspModelException(); }
	void TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule)
	{ throw QNonBspModelException(); }
	void DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points))
	{}
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
			QClipMap* LoadCMap = CM_CreateQClipMap29();
			CMNonMapModels.Append(LoadCMap);
			LoadCMap->LoadMap(Name, Buffer);
			if (LoadCMap->GetNumInlineModels() > 1)
			{
				GLog.WriteLine("Non-map BSP models are not supposed to have submodels");
			}
			break;
		}

	default:
		{
			QClipMapNonMap* LoadCMap = new QClipMapNonMap;
			CMNonMapModels.Append(LoadCMap);

			LoadCMap->LoadMap(Name, Buffer);
		}
	}

	return CMNonMapModels.Num() << CMH_NON_MAP_SHIFT;
}

//==========================================================================
//
//	QClipMapNonMap::LoadMap
//
//==========================================================================

void QClipMapNonMap::LoadMap(const char* name, const QArray<quint8>& Buffer)
{
	this->Name = name;

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)Buffer.Ptr()))
	{
	case RAPOLYHEADER:
		LoadAliasModelNew(Buffer.Ptr());
		break;

	case IDPOLYHEADER:
		LoadAliasModel(Buffer.Ptr());
		break;

	case IDSPRITE1HEADER:
		LoadSpriteModel(Buffer.Ptr());
		break;

	default:
		ModelMins[0] = ModelMins[1] = ModelMins[2] = -16;
		ModelMaxs[0] = ModelMaxs[1] = ModelMaxs[2] = 16;
		break;
	}
}

//==========================================================================
//
//	QClipMapNonMap::LoadAliasModel
//
//==========================================================================

void QClipMapNonMap::LoadAliasModel(const void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModel(this, buffer);
	}
	else
	{
		ModelMins[0] = ModelMins[1] = ModelMins[2] = -16;
		ModelMaxs[0] = ModelMaxs[1] = ModelMaxs[2] = 16;
	}
}

//==========================================================================
//
//	QClipMapNonMap::LoadAliasModelNew
//
//==========================================================================

void QClipMapNonMap::LoadAliasModelNew(const void* buffer)
{
	if (GGameType & GAME_Hexen2)
	{
		QMdlBoundsLoader BoundsLoader;
		BoundsLoader.LoadAliasModelNew(this, buffer);
	}
	else
	{
		ModelMins[0] = ModelMins[1] = ModelMins[2] = -16;
		ModelMaxs[0] = ModelMaxs[1] = ModelMaxs[2] = 16;
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

	mod->ModelMins[0] = mins[0] - 10;
	mod->ModelMins[1] = mins[1] - 10;
	mod->ModelMins[2] = mins[2] - 10;
	mod->ModelMaxs[0] = maxs[0] + 10;
	mod->ModelMaxs[1] = maxs[1] + 10;
	mod->ModelMaxs[2] = maxs[2] + 10;
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

	mod->ModelMins[0] = mins[0] - 10;
	mod->ModelMins[1] = mins[1] - 10;
	mod->ModelMins[2] = mins[2] - 10;
	mod->ModelMaxs[0] = maxs[0] + 10;
	mod->ModelMaxs[1] = maxs[1] + 10;
	mod->ModelMaxs[2] = maxs[2] + 10;
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

void QClipMapNonMap::LoadSpriteModel(const void* buffer)
{
	dsprite1_t* pin = (dsprite1_t*)buffer;

	int version = LittleLong (pin->version);
	if (version != SPRITE1_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)", *Name, version, SPRITE1_VERSION));
	}

	ModelMins[0] = ModelMins[1] = -LittleLong(pin->width) / 2;
	ModelMaxs[0] = ModelMaxs[1] = LittleLong(pin->width) / 2;
	ModelMins[2] = -LittleLong(pin->height) / 2;
	ModelMaxs[2] = LittleLong(pin->height) / 2;
}

//==========================================================================
//
//	QClipMapNonMap::ModelBounds
//
//	The only valid thing that can be called on non BSP model.
//
//==========================================================================

void QClipMapNonMap::ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	VectorCopy(ModelMins, Mins);
	VectorCopy(ModelMaxs, Maxs);
}
