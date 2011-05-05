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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct wadinfo_t
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
};

struct lumpinfo_t
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int			wad_numlumps;
static lumpinfo_t*	wad_lumps;
static QArray<byte>	wad_base;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CleanupName
//
//	Lowercases name and pads with terminating 0 to the length of lumpinfo_t->name.
//	Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
//	Can safely be performed in place.
//
//==========================================================================

static void CleanupName(const char* in, char* out)
{
	int i;
	for (i = 0; i < 16; i++)
	{
		int c = in[i];
		if (!c)
		{
			break;
		}

		if (c >= 'A' && c <= 'Z')
		{
			c += ('a' - 'A');
		}
		out[i] = c;
	}
	
	for (; i < 16; i++)
	{
		out[i] = 0;
	}
}

//==========================================================================
//
//	R_LoadWadFile
//
//==========================================================================

void R_LoadWadFile()
{
	if (FS_ReadFile("gfx.wad", wad_base) <= 0)
	{
		throw QException("W_LoadWadFile: couldn't load gfx.wad");
	}

	wadinfo_t* header = (wadinfo_t*)wad_base.Ptr();
	
	if (header->identification[0] != 'W' ||
		header->identification[1] != 'A' ||
		header->identification[2] != 'D' ||
		header->identification[3] != '2')
	{
		throw QException("Wad file gfx.wad doesn't have WAD2 id\n");
	}

	wad_numlumps = LittleLong(header->numlumps);
	int infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t*)&wad_base[infotableofs];

	lumpinfo_t* lump_p = wad_lumps;
	for (int i = 0; i < wad_numlumps; i++, lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		CleanupName(lump_p->name, lump_p->name);
	}
}

//==========================================================================
//
//	R_GetWadLumpByName
//
//==========================================================================

void* R_GetWadLumpByName(const char* name)
{
	char clean[16];
	CleanupName(name, clean);

	lumpinfo_t* lump_p = wad_lumps;
	for (int i = 0; i < wad_numlumps; i++, lump_p++)
	{
		if (!QStr::Cmp(clean, lump_p->name))
		{
			return &wad_base[lump_p->filepos];
		}
	}

	throw QDropException(va("R_GetWadLumpByName: %s not found", name));
}

//==========================================================================
//
//	R_PicFromWad
//
//==========================================================================

image_t* R_PicFromWad(const char* name)
{
	byte* qpic = (byte*)R_GetWadLumpByName(name);

	int width;
	int height;
	byte* pic;
	R_LoadPICMem(qpic, &pic, &width, &height);
	
	image_t* image = R_CreateImage(va("gfx.wad:%s", name), pic, width, height, false, false, GL_CLAMP, true);
	
	delete[] pic;

	return image;
}
