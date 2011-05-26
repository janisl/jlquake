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

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Mod_LoadSpriteFrame
//
//==========================================================================

static void* Mod_LoadSpriteFrame(void* pin, msprite1frame_t** ppframe, int framenum)
{
	dsprite1frame_t* pinframe = (dsprite1frame_t*)pin;

	int width = LittleLong(pinframe->width);
	int height = LittleLong(pinframe->height);
	int size = width * height;

	msprite1frame_t* pspriteframe = new msprite1frame_t;

	Com_Memset(pspriteframe, 0, sizeof (msprite1frame_t));

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	int origin[2];
	origin[0] = LittleLong(pinframe->origin[0]);
	origin[1] = LittleLong(pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	char name[64];
	sprintf(name, "%s_%i", loadmodel->name, framenum);
	byte* pic32 = R_ConvertImage8To32((byte*)(pinframe + 1), width, height, IMG8MODE_Normal);
	pspriteframe->gl_texture = R_CreateImage(name, pic32, width, height, true, true, GL_CLAMP, false);
	delete[] pic32;

	return (void*)((byte*)pinframe + sizeof(dsprite1frame_t) + size);
}

//==========================================================================
//
//	Mod_LoadSpriteGroup
//
//==========================================================================

static void* Mod_LoadSpriteGroup(void* pin, msprite1frame_t** ppframe, int framenum)
{
	dsprite1group_t* pingroup = (dsprite1group_t*)pin;

	int numframes = LittleLong(pingroup->numframes);

	msprite1group_t* pspritegroup = (msprite1group_t*)Mem_Alloc(sizeof(msprite1group_t) +
				(numframes - 1) * sizeof(pspritegroup->frames[0]));

	pspritegroup->numframes = numframes;

	*ppframe = (msprite1frame_t*)pspritegroup;

	dsprite1interval_t* pin_intervals = (dsprite1interval_t*)(pingroup + 1);

	float* poutintervals = new float[numframes];

	pspritegroup->intervals = poutintervals;

	for (int i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0)
		{
			throw QException("Mod_LoadSpriteGroup: interval<=0");
		}

		poutintervals++;
		pin_intervals++;
	}

	void* ptemp = (void*)pin_intervals;

	for (int i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i], framenum * 100 + i);
	}

	return ptemp;
}

//==========================================================================
//
//	Mod_LoadSpriteModel
//
//==========================================================================

void Mod_LoadSpriteModel(model_t* mod, void* buffer)
{
	dsprite1_t* pin = (dsprite1_t*)buffer;

	int version = LittleLong(pin->version);
	if (version != SPRITE1_VERSION)
	{
		throw QException(va("%s has wrong version number "
			"(%i should be %i)", mod->name, version, SPRITE1_VERSION));
	}

	int numframes = LittleLong(pin->numframes);

	int size = sizeof(msprite1_t) +	(numframes - 1) * sizeof(msprite1framedesc_t);

	msprite1_t* psprite = (msprite1_t*)Mem_Alloc(size);

	mod->q1_cache = psprite;

	psprite->type = LittleLong(pin->type);
	psprite->maxwidth = LittleLong(pin->width);
	psprite->maxheight = LittleLong(pin->height);
	psprite->beamlength = LittleFloat(pin->beamlength);
	mod->q1_synctype = (synctype_t)LittleLong(pin->synctype);
	psprite->numframes = numframes;

	mod->q1_mins[0] = mod->q1_mins[1] = -psprite->maxwidth / 2;
	mod->q1_maxs[0] = mod->q1_maxs[1] = psprite->maxwidth / 2;
	mod->q1_mins[2] = -psprite->maxheight / 2;
	mod->q1_maxs[2] = psprite->maxheight / 2;

	//
	// load the frames
	//
	if (numframes < 1)
	{
		throw QException(va("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes));
	}

	mod->q1_numframes = numframes;

	dsprite1frametype_t* pframetype = (dsprite1frametype_t*)(pin + 1);

	for (int i = 0; i < numframes; i++)
	{
		sprite1frametype_t frametype = (sprite1frametype_t)LittleLong(pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dsprite1frametype_t*)Mod_LoadSpriteFrame(pframetype + 1,
				&psprite->frames[i].frameptr, i);
		}
		else
		{
			pframetype = (dsprite1frametype_t*)Mod_LoadSpriteGroup(pframetype + 1,
				&psprite->frames[i].frameptr, i);
		}
	}

	mod->type = MOD_SPRITE;
}

//==========================================================================
//
//	Mod_LoadSpriteModel
//
//==========================================================================

void Mod_FreeSpriteModel(model_t* mod)
{
	msprite1_t* psprite = (msprite1_t*)mod->q1_cache;
	for (int i = 0; i < psprite->numframes; i++)
	{
		if (psprite->frames[i].type == SPR_SINGLE)
		{
			delete psprite->frames[i].frameptr;
		}
		else
		{
			msprite1group_t* pspritegroup = (msprite1group_t*)psprite->frames[i].frameptr;
			for (int j = 0; j < pspritegroup->numframes; j++)
			{
				delete pspritegroup->frames[j];
			}
			delete[] pspritegroup->intervals;
			Mem_Free(pspritegroup);
		}
	}
}
