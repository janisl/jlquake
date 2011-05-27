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

#define	LL(x) x=LittleLong(x)

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
//	R_LoadMd3Lod
//
//==========================================================================

static bool R_LoadMd3Lod(model_t* mod, int lod, const void* buffer, const char* mod_name)
{
	md3Header_t* pinmodel = (md3Header_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MD3_VERSION)
	{
		GLog.Write(S_COLOR_YELLOW "R_LoadMD3: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD3_VERSION);
		return false;
	}

	mod->type = MOD_MESH3;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mod->q3_md3[lod] = (md3Header_t*)Mem_Alloc(size);

	Com_Memcpy(mod->q3_md3[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->q3_md3[lod]->ident);
	LL(mod->q3_md3[lod]->version);
	LL(mod->q3_md3[lod]->numFrames);
	LL(mod->q3_md3[lod]->numTags);
	LL(mod->q3_md3[lod]->numSurfaces);
	LL(mod->q3_md3[lod]->ofsFrames);
	LL(mod->q3_md3[lod]->ofsTags);
	LL(mod->q3_md3[lod]->ofsSurfaces);
	LL(mod->q3_md3[lod]->ofsEnd);

	if (mod->q3_md3[lod]->numFrames < 1)
	{
		GLog.Write(S_COLOR_YELLOW "R_LoadMD3: %s has no frames\n", mod_name);
		return false;
	}

	// swap all the frames
	md3Frame_t* frame = (md3Frame_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsFrames);
	for (int i = 0; i < mod->q3_md3[lod]->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (int j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
	}

	// swap all the tags
	md3Tag_t* tag = (md3Tag_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsTags);
	for (int i = 0; i < mod->q3_md3[lod]->numTags * mod->q3_md3[lod]->numFrames; i++, tag++)
	{
		for (int j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}
	}

	// swap all the surfaces
	md3Surface_t* surf = (md3Surface_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsSurfaces);
	for (int i = 0; i < mod->q3_md3[lod]->numSurfaces; i++)
	{
		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsEnd);
		
		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			throw QDropException(va("R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts));
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			throw QDropException(va("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles));
		}

		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		QStr::ToLower(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		int Len = QStr::Length(surf->name);
		if (Len > 2 && surf->name[Len - 2] == '_')
		{
			surf->name[Len - 2] = 0;
		}

		// register the shaders
		md3Shader_t* shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (int j = 0; j < surf->numShaders; j++, shader++)
		{
			shader_t* sh = R_FindShader(shader->name, LIGHTMAP_NONE, true);
			if (sh->defaultShader)
			{
				shader->shaderIndex = 0;
			}
			else
			{
				shader->shaderIndex = sh->index;
			}
		}

		// swap all the triangles
		md3Triangle_t* tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (int j = 0; j < surf->numTriangles; j++, tri++)
		{
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
		md3St_t* st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (int j = 0; j < surf->numVerts; j++, st++)
		{
			st->st[0] = LittleFloat(st->st[0]);
			st->st[1] = LittleFloat(st->st[1]);
		}

		// swap all the XyzNormals
		md3XyzNormal_t* xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (int j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++) 
		{
			xyz->xyz[0] = LittleShort(xyz->xyz[0]);
			xyz->xyz[1] = LittleShort(xyz->xyz[1]);
			xyz->xyz[2] = LittleShort(xyz->xyz[2]);

			xyz->normal = LittleShort(xyz->normal);
		}

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}

	return true;
}

//==========================================================================
//
//	R_LoadMd3
//
//==========================================================================

bool R_LoadMd3(model_t* mod, void* buffer)
{
	mod->q3_numLods = 0;

	//
	// load the files
	//
	int numLoaded = 0;

	int lod = MD3_MAX_LODS - 1;
	for (; lod >= 0; lod--)
	{
		char filename[1024];

		QStr::Cpy(filename, mod->name);

		void* buf;
		if (lod == 0)
		{
			buf = buffer;
		}
		else
		{
			char namebuf[80];

			if (QStr::RChr(filename, '.'))
			{
				*QStr::RChr(filename, '.') = 0;
			}
			sprintf(namebuf, "_%d.md3", lod);
			QStr::Cat(filename, sizeof(filename), namebuf);

			FS_ReadFile(filename, (void**)&buf);
			if (!buf)
			{
				continue;
			}
		}

		int ident = LittleLong(*(unsigned*)buf);
		if (ident != MD3_IDENT)
		{
			GLog.Write(S_COLOR_YELLOW "RE_RegisterModel: unknown fileid for %s\n", filename);
			return false;
		}

		bool loaded = R_LoadMd3Lod(mod, lod, buf, filename);

		if (lod != 0)
		{
			FS_FreeFile(buf);
		}

		if (!loaded)
		{
			if (lod == 0)
			{
				return false;
			}
			else
			{
				break;
			}
		}
		else
		{
			mod->q3_numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
//			if ( lod <= r_lodbias->integer ) {
//				break;
//			}
		}
	}

	if (!numLoaded)
	{
		return false;
	}

	// duplicate into higher lod spots that weren't
	// loaded, in case the user changes r_lodbias on the fly
	for (lod--; lod >= 0; lod--)
	{
		mod->q3_numLods++;
		mod->q3_md3[lod] = mod->q3_md3[lod + 1];
	}

	return true;
}

//==========================================================================
//
//	R_FreeMd3
//
//==========================================================================

void R_FreeMd3(model_t* mod)
{
	Mem_Free(mod->q3_md3[0]);
	for (int lod = 1; lod < MD3_MAX_LODS; lod++)
	{
		if (mod->q3_md3[lod] != mod->q3_md3[lod - 1])
		{
			Mem_Free(mod->q3_md3[lod]);
		}
	}
}
