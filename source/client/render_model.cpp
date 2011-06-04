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
