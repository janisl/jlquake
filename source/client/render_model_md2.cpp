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
//	Mod_LoadMd2Model
//
//==========================================================================

void Mod_LoadMd2Model(model_t* mod, const void* buffer)
{
	const dmd2_t* pinmodel = (const dmd2_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MESH2_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			mod->name, version, MESH2_VERSION));
	}

	dmd2_t* pheader = (dmd2_t*)Mem_Alloc(LittleLong(pinmodel->ofs_end));
	
	// byte swap the header fields and sanity check
	for (int i = 0; i < (int)sizeof(dmd2_t) / 4; i++)
	{
		((int*)pheader)[i] = LittleLong(((int*)buffer)[i]);
	}

	if (pheader->num_xyz <= 0)
	{
		throw QDropException(va("model %s has no vertices", mod->name));
	}

	if (pheader->num_xyz > MAX_MD2_VERTS)
	{
		throw QDropException(va("model %s has too many vertices", mod->name));
	}

	if (pheader->num_st <= 0)
	{
		throw QDropException(va("model %s has no st vertices", mod->name));
	}

	if (pheader->num_tris <= 0)
	{
		throw QDropException(va("model %s has no triangles", mod->name));
	}

	if (pheader->num_frames <= 0)
	{
		throw QDropException(va("model %s has no frames", mod->name));
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	const dmd2_stvert_t* pinst = (const dmd2_stvert_t*)((byte*)pinmodel + pheader->ofs_st);
	dmd2_stvert_t* poutst = (dmd2_stvert_t*)((byte*)pheader + pheader->ofs_st);

	for (int i = 0; i < pheader->num_st; i++)
	{
		poutst[i].s = LittleShort(pinst[i].s);
		poutst[i].t = LittleShort(pinst[i].t);
	}

	//
	// load triangle lists
	//
	const dmd2_triangle_t* pintri = (const dmd2_triangle_t*)((byte*)pinmodel + pheader->ofs_tris);
	dmd2_triangle_t* pouttri = (dmd2_triangle_t*)((byte*)pheader + pheader->ofs_tris);

	for (int i = 0; i < pheader->num_tris; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}

	//
	// load the frames
	//
	for (int i = 0; i < pheader->num_frames; i++)
	{
		const dmd2_frame_t* pinframe = (const dmd2_frame_t*)((byte*)pinmodel +
			pheader->ofs_frames + i * pheader->framesize);
		dmd2_frame_t* poutframe = (dmd2_frame_t*)((byte*)pheader +
			pheader->ofs_frames + i * pheader->framesize);

		Com_Memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (int j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		Com_Memcpy(poutframe->verts, pinframe->verts, pheader->num_xyz * sizeof(dmd2_trivertx_t));
	}

	mod->type = MOD_MESH2;
	mod->q2_extradata = pheader;
	mod->q2_extradatasize = pheader->ofs_end;
	mod->q2_numframes = pheader->num_frames;

	//
	// load the glcmds
	//
	const int* pincmd = (const int*)((byte*)pinmodel + pheader->ofs_glcmds);
	int* poutcmd = (int*)((byte*)pheader + pheader->ofs_glcmds);
	for (int i = 0; i < pheader->num_glcmds; i++)
	{
		poutcmd[i] = LittleLong(pincmd[i]);
	}

	// register all skins
	Com_Memcpy((char*)pheader + pheader->ofs_skins, (char*)pinmodel + pheader->ofs_skins,
		pheader->num_skins*MAX_MD2_SKINNAME);
	for (int i = 0; i < pheader->num_skins; i++)
	{
		mod->q2_skins[i] = R_FindImageFile((char*)pheader + pheader->ofs_skins + i * MAX_MD2_SKINNAME,
			true, true, GL_CLAMP, false, IMG8MODE_Skin);
	}

	mod->q2_mins[0] = -32;
	mod->q2_mins[1] = -32;
	mod->q2_mins[2] = -32;
	mod->q2_maxs[0] = 32;
	mod->q2_maxs[1] = 32;
	mod->q2_maxs[2] = 32;
}

//==========================================================================
//
//	Mod_FreeMd2Model
//
//==========================================================================

void Mod_FreeMd2Model(model_t* mod)
{
	Mem_Free(mod->q2_extradata);
}
