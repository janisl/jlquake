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

	void LoadAliasModel(QClipMap29* mod, void* buffer);
	void LoadAliasModelNew(QClipMap29* mod, void* buffer);
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

	QClipMap29* LoadCMap = new QClipMap29;
	CMNonMapModels.Append(LoadCMap);

	LoadCMap->LoadNonMap(Name);

	return CMNonMapModels.Num() << CMH_NON_MAP_SHIFT;
}

//==========================================================================
//
//	QClipMap29::LoadNonMap
//
//==========================================================================

void QClipMap29::LoadNonMap(const char* name)
{
	Com_Memset(&Map.map_models, 0, sizeof(Map.map_models));
	cmodel_t* mod = &Map.map_models[0];

	//
	// load the file
	//
	QArray<byte> Buffer;
	if (FS_ReadFile(name, Buffer) <= 0)
	{
		throw QDropException(va("CM_PrecacheModel: %s not found", name));
	}

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
		LoadMap(name, Buffer);
		if (mod->numsubmodels > 1)
		{
			GLog.WriteLine("Non-map BSP models are not supposed to have submodels");
		}
		break;
	}
}

//==========================================================================
//
//	QClipMap29::LoadAliasModel
//
//==========================================================================

void QClipMap29::LoadAliasModel(cmodel_t* mod, void* buffer)
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
//	QClipMap29::LoadAliasModelNew
//
//==========================================================================

void QClipMap29::LoadAliasModelNew(cmodel_t* mod, void* buffer)
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

void QMdlBoundsLoader::LoadAliasModel(QClipMap29* mod, void* buffer)
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

void QMdlBoundsLoader::LoadAliasModelNew(QClipMap29* mod, void* buffer)
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
//	QClipMap29::LoadSpriteModel
//
//==========================================================================

void QClipMap29::LoadSpriteModel(cmodel_t* mod, void* buffer)
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
