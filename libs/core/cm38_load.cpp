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

#include "core.h"
#include "cm38_local.h"

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
//	QClipMap38::LoadMap
//
//==========================================================================

void QClipMap38::LoadMap(const char* name)
{
	if (!name || !name[0])
	{
		numleafs = 1;
		leafs = new cleaf_t[1];
		Com_Memset(leafs, 0, sizeof(cleaf_t));
		numclusters = 1;
		numareas = 1;
		areas = new carea_t[1];
		Com_Memset(areas, 0, sizeof(carea_t));
		cmodels = new cmodel_t[1];
		Com_Memset(cmodels, 0, sizeof(cmodel_t));
		checksum = 0;
		return;			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	void* buf;
	int length = FS_ReadFile(name, &buf);
	if (!buf)
	{
		throw QDropException(va("Couldn't load %s", name));
	}

	checksum = LittleLong(Com_BlockChecksum(buf, length));

	bsp38_dheader_t header = *(bsp38_dheader_t*)buf;
	for (int i = 0; i < sizeof(bsp38_dheader_t) / 4 ; i++)
	{
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);
	}

	if (header.version != BSP38_VERSION)
	{
		throw QDropException(va("CMod_LoadBrushModel: %s has wrong version number (%i should be %i)",
			name, header.version, BSP38_VERSION));
	}

	byte* cmod_base = (byte*)buf;

	// load into heap
	LoadSurfaces(cmod_base, &header.lumps[BSP38LUMP_TEXINFO]);
	LoadLeafs(cmod_base, &header.lumps[BSP38LUMP_LEAFS]);
	LoadLeafBrushes(cmod_base, &header.lumps[BSP38LUMP_LEAFBRUSHES]);
	LoadPlanes(cmod_base, &header.lumps[BSP38LUMP_PLANES]);
	LoadBrushes(cmod_base, &header.lumps[BSP38LUMP_BRUSHES]);
	LoadBrushSides(cmod_base, &header.lumps[BSP38LUMP_BRUSHSIDES]);
	LoadNodes(cmod_base, &header.lumps[BSP38LUMP_NODES]);
	LoadAreas(cmod_base, &header.lumps[BSP38LUMP_AREAS]);
	LoadAreaPortals(cmod_base, &header.lumps[BSP38LUMP_AREAPORTALS]);
	LoadVisibility(cmod_base, &header.lumps[BSP38LUMP_VISIBILITY]);
	LoadEntityString(cmod_base, &header.lumps[BSP38LUMP_ENTITIES]);
	LoadSubmodels(cmod_base, &header.lumps[BSP38LUMP_MODELS]);

	FS_FreeFile(buf);

	InitBoxHull();

	ClearPortalOpen();

	QStr::Cpy(this->name, name);
}

//==========================================================================
//
//	QClipMap38::LoadSurfaces
//
//==========================================================================

void QClipMap38::LoadSurfaces(const quint8* base, const bsp38_lump_t* l)
{
	const bsp38_texinfo_t* in = (const bsp38_texinfo_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map with no surfaces");
	}

	numtexinfo = count;
	surfaces = new mapsurface_t[count];

	mapsurface_t* out = surfaces;
	for (int i = 0; i < count; i++, in++, out++)
	{
		QStr::NCpy(out->c.name, in->texture, sizeof(out->c.name) - 1);
		QStr::NCpy(out->rname, in->texture, sizeof(out->rname) - 1);
		out->c.flags = LittleLong(in->flags);
		out->c.value = LittleLong(in->value);
	}
}

//==========================================================================
//
//	QClipMap38::LoadLeafs
//
//==========================================================================

void QClipMap38::LoadLeafs(const quint8* base, const bsp38_lump_t* l)
{
	const bsp38_dleaf_t* in = (const bsp38_dleaf_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map with no leafs");
	}

	// need to save space for box planes
	leafs = new cleaf_t[count + 1];
	numleafs = count;
	numclusters = 0;

	cleaf_t* out = leafs;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);
		out->firstleafbrush = LittleShort(in->firstleafbrush);
		out->numleafbrushes = LittleShort(in->numleafbrushes);

		if (out->cluster >= numclusters)
		{
			numclusters = out->cluster + 1;
		}
	}

	if (leafs[0].contents != BSP38CONTENTS_SOLID)
	{
		throw QDropException("Map leaf 0 is not CONTENTS_SOLID");
	}
	solidleaf = 0;
	emptyleaf = -1;
	for (int i = 1; i < numleafs; i++)
	{
		if (!leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
	{
		throw QDropException("Map does not have an empty leaf");
	}
}

//==========================================================================
//
//	QClipMap38::LoadLeafBrushes
//
//==========================================================================

void QClipMap38::LoadLeafBrushes(const quint8* base, const bsp38_lump_t* l)
{
	const quint16* in = (const quint16*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map with no leaf brushes");
	}

	// need to save space for box planes
	leafbrushes = new quint16[count + 1];
	numleafbrushes = count;

	quint16* out = leafbrushes;
	for (int i = 0; i < count; i++, in++, out++)
	{
		*out = LittleShort(*in);
	}
}

//==========================================================================
//
//	QClipMap38::LoadPlanes
//
//==========================================================================

void QClipMap38::LoadPlanes(const quint8* base, const bsp38_lump_t *l)
{
	const bsp38_dplane_t* in = (const bsp38_dplane_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map with no planes");
	}

	// need to save space for box planes
	planes = new cplane_t[count + 12];
	Com_Memset(planes, 0, sizeof(cplane_t) * (count + 12));
	numplanes = count;

	cplane_t* out = planes;	
	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}
		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	QClipMap38::LoadBrushes
//
//==========================================================================

void QClipMap38::LoadBrushes(const quint8* base, const bsp38_lump_t *l)
{
	const bsp38_dbrush_t* in = (const bsp38_dbrush_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	brushes = new cbrush_t[count + 1];
	Com_Memset(brushes, 0, sizeof(cbrush_t) * (count + 1));
	numbrushes = count;

	cbrush_t* out = brushes;
	for (int i = 0; i < count; i++, out++, in++)
	{
		out->firstbrushside = LittleLong(in->firstside);
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
	}
}

//==========================================================================
//
//	QClipMap38::LoadBrushSides
//
//==========================================================================

void QClipMap38::LoadBrushSides(const quint8* base, const bsp38_lump_t *l)
{
	const bsp38_dbrushside_t* in = (const bsp38_dbrushside_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	// need to save space for box planes
	brushsides = new cbrushside_t[count + 6];
	numbrushsides = count;

	cbrushside_t* out = brushsides;
	for (int i = 0; i < count; i++, in++, out++)
	{
		int num = LittleShort(in->planenum);
		out->plane = &planes[num];
		int j = LittleShort(in->texinfo);
		if (j >= numtexinfo)
		{
			throw QDropException("Bad brushside texinfo");
		}
		out->surface = &surfaces[j];
	}
}

//==========================================================================
//
//	QClipMap38::LoadNodes
//
//==========================================================================

void QClipMap38::LoadNodes(const quint8* base, const bsp38_lump_t* l)
{
	const bsp38_dnode_t* in = (const bsp38_dnode_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map has no nodes");
	}

	nodes = new cnode_t[count + 6];
	numnodes = count;

	cnode_t* out = nodes;
	for (int i = 0; i < count; i++, out++, in++)
	{
		out->plane = planes + LittleLong(in->planenum);
		for (int j = 0; j < 2; j++)
		{
			int child = LittleLong(in->children[j]);
			out->children[j] = child;
		}
	}
}

//==========================================================================
//
//	QClipMap38::LoadAreas
//
//==========================================================================

void QClipMap38::LoadAreas(const quint8* base, const bsp38_lump_t *l)
{
	const bsp38_darea_t* in = (const bsp38_darea_t *)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	areas = new carea_t[count];
	numareas = count;

	carea_t* out = areas;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->numareaportals = LittleLong(in->numareaportals);
		out->firstareaportal = LittleLong(in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

//==========================================================================
//
//	QClipMap38::LoadAreaPortals
//
//==========================================================================

void QClipMap38::LoadAreaPortals(const quint8* base, const bsp38_lump_t* l)
{
	const bsp38_dareaportal_t* in = (bsp38_dareaportal_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);

	areaportals = new bsp38_dareaportal_t[count];
	numareaportals = count;

	bsp38_dareaportal_t* out = areaportals;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->portalnum = LittleLong(in->portalnum);
		out->otherarea = LittleLong(in->otherarea);
	}
}

//==========================================================================
//
//	QClipMap38::LoadVisibility
//
//==========================================================================

void QClipMap38::LoadVisibility(const quint8* base, const bsp38_lump_t* l)
{
	numvisibility = l->filelen;
	visibility = new quint8[l->filelen];
	Com_Memcpy(visibility, base + l->fileofs, l->filelen);

	vis = (bsp38_dvis_t*)visibility;
	vis->numclusters = LittleLong(vis->numclusters);
	for (int i = 0; i < vis->numclusters; i++)
	{
		vis->bitofs[i][0] = LittleLong(vis->bitofs[i][0]);
		vis->bitofs[i][1] = LittleLong(vis->bitofs[i][1]);
	}
}

//==========================================================================
//
//	QClipMap38::LoadEntityString
//
//==========================================================================

void QClipMap38::LoadEntityString(const quint8* base, const bsp38_lump_t *l)
{
	numentitychars = l->filelen;
	entitystring = new char[l->filelen];
	Com_Memcpy(entitystring, base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	QClipMap38::LoadSubmodels
//
//==========================================================================

void QClipMap38::LoadSubmodels(const quint8* base, const bsp38_lump_t *l)
{
	const bsp38_dmodel_t* in = (const bsp38_dmodel_t*)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException("MOD_LoadBmodel: funny lump size");
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		throw QDropException("Map with no models");
	}

	cmodels = new cmodel_t[count];
	numcmodels = count;

	cmodel_t* out = cmodels;
	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->headnode = LittleLong(in->headnode);
	}
}

//==========================================================================
//
//	QClipMap38::PrecacheModel
//
//==========================================================================

clipHandle_t QClipMap38::PrecacheModel(const char* Name)
{
	return 0;
}
