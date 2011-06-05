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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

model_t*		loadmodel;
model_t*		currentmodel;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_AllocModel
//
//==========================================================================

model_t* R_AllocModel()
{
	if (tr.numModels == MAX_MOD_KNOWN)
	{
		return NULL;
	}

	model_t* mod = new model_t;
	Com_Memset(mod, 0, sizeof(model_t));
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

//==========================================================================
//
//	R_ModelInit
//
//==========================================================================

void R_ModelInit()
{
	// leave a space for NULL model
	tr.numModels = 0;

	model_t* mod = R_AllocModel();
	mod->type = MOD_BAD;
}

//==========================================================================
//
//	R_FreeModel
//
//==========================================================================

void R_FreeModel(model_t* mod)
{
	if (mod->type == MOD_SPRITE)
	{
		Mod_FreeSpriteModel(mod);
	}
	else if (mod->type == MOD_SPRITE2)
	{
		Mod_FreeSprite2Model(mod);
	}
	else if (mod->type == MOD_MESH1)
	{
		Mod_FreeMdlModel(mod);
	}
	else if (mod->type == MOD_MESH2)
	{
		Mod_FreeMd2Model(mod);
	}
	else if (mod->type == MOD_MESH3)
	{
		R_FreeMd3(mod);
	}
	else if (mod->type == MOD_MD4)
	{
		R_FreeMd4(mod);
	}
	else if (mod->type == MOD_BRUSH29)
	{
		Mod_FreeBsp29(mod);
	}
	else if (mod->type == MOD_BRUSH38)
	{
		Mod_FreeBsp38(mod);
	}
	else if (mod->type == MOD_BRUSH46)
	{
		R_FreeBsp46Model(mod);
	}
	delete mod;
}

//==========================================================================
//
//	R_FreeModels
//
//==========================================================================

void R_FreeModels()
{
	for (int i = 0; i < tr.numModels; i++)
	{
		R_FreeModel(tr.models[i]);
		tr.models[i] = NULL;
	}
	tr.numModels = 0;

	if (tr.world)
	{
		R_FreeBsp46(tr.world);
	}
}

//==========================================================================
//
//	R_GetModelByHandle
//
//==========================================================================

model_t* R_GetModelByHandle(qhandle_t index)
{
	// out of range gets the defualt model
	if (index < 1 || index >= tr.numModels)
	{
		return tr.models[0];
	}

	return tr.models[index];
}

//==========================================================================
//
//	R_RegisterModel
//
//	Loads in a model for the given name
//	Zero will be returned if the model fails to load. An entry will be
// retained for failed models as an optimization to prevent disk rescanning
// if they are asked for again.
//
//==========================================================================

int R_RegisterModel(const char* name)
{
	if (!name || !name[0])
	{
		GLog.Write("R_RegisterModel: NULL name\n");
		return 0;
	}

	if (QStr::Length(name) >= MAX_QPATH)
	{
		GLog.Write("Model name exceeds MAX_QPATH\n");
		return 0;
	}

	//
	// search the currently loaded models
	//
	for (int hModel = 1; hModel < tr.numModels; hModel++)
	{
		model_t* mod = tr.models[hModel];
		if (!QStr::Cmp(mod->name, name))
		{
			if (mod->type == MOD_BAD)
			{
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t
	model_t* mod = R_AllocModel();
	if (mod == NULL)
	{
		GLog.Write(S_COLOR_YELLOW "R_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	QStr::NCpyZ(mod->name, name, sizeof(mod->name));

	// make sure the render thread is stopped
	R_SyncRenderThread();

	void* buf;
	int modfilelen = FS_ReadFile(name, &buf);
	if (!buf)
	{
		GLog.Write(S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name);
		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		mod->type = MOD_BAD;
		return 0;
	}

	loadmodel = mod;

	//	call the apropriate loader
	bool loaded;
	switch (LittleLong(*(qint32*)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadMdlModel(mod, buf);
		loaded = true;
		break;

	case RAPOLYHEADER:
		Mod_LoadMdlModelNew(mod, buf);
		loaded = true;
		break;

	case IDSPRITE1HEADER:
		Mod_LoadSpriteModel(mod, buf);
		loaded = true;
		break;

	case BSP29_VERSION:
		Mod_LoadBrush29Model(mod, buf);
		loaded = true;
		break;

	case IDMESH2HEADER:
		Mod_LoadMd2Model(mod, buf);
		loaded = true;
		break;

	case IDSPRITE2HEADER:
		Mod_LoadSprite2Model(mod, buf, modfilelen);
		loaded = true;
		break;

	case MD3_IDENT:
		loaded = R_LoadMd3(mod, buf);
		break;

	case MD4_IDENT:
		loaded = R_LoadMD4(mod, buf, name);
		break;

	default:
		GLog.Write(S_COLOR_YELLOW "R_RegisterModel: unknown fileid for %s\n", name);
		loaded = false;
	}

	FS_FreeFile(buf);

	if (!loaded)
	{
		GLog.Write(S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name);
		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		mod->type = MOD_BAD;
		return 0;
	}

	return mod->index;
}
