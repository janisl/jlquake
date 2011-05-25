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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// this table is also present in q3map

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Mod_LoadSprite2Model
//
//==========================================================================

void Mod_LoadSprite2Model(model_t* mod, void* buffer, int modfilelen)
{
	dsprite2_t* sprin = (dsprite2_t*)buffer;
	dsprite2_t* sprout = (dsprite2_t*)Mem_Alloc(modfilelen);

	sprout->ident = LittleLong(sprin->ident);
	sprout->version = LittleLong(sprin->version);
	sprout->numframes = LittleLong(sprin->numframes);

	if (sprout->version != SPRITE2_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
				 mod->name, sprout->version, SPRITE2_VERSION));
	}

	if (sprout->numframes > MAX_MD2_SKINS)
	{
		throw QDropException(va("%s has too many frames (%i > %i)",
			mod->name, sprout->numframes, MAX_MD2_SKINS));
	}

	// byte swap everything
	for (int i = 0; i < sprout->numframes; i++)
	{
		sprout->frames[i].width = LittleLong(sprin->frames[i].width);
		sprout->frames[i].height = LittleLong(sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong(sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong(sprin->frames[i].origin_y);
		Com_Memcpy(sprout->frames[i].name, sprin->frames[i].name, MAX_SP2_SKINNAME);
		mod->q2_skins[i] = R_FindImageFile(sprout->frames[i].name, true, true, GL_CLAMP);
	}

	mod->q2_extradata = sprout;
	mod->q2_extradatasize = modfilelen;
	mod->type = MOD_SPRITE2;
}

//==========================================================================
//
//	Mod_FreeSprite2Model
//
//==========================================================================

void Mod_FreeSprite2Model(model_t* mod)
{
	Mem_Free(mod->q2_extradata);
}
