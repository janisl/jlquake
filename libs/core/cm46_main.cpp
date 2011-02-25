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
#include "cm46_local.h"

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
//	QClipMap46::~QClipMap46
//
//==========================================================================

QClipMap46::~QClipMap46()
{
	for (int i = 1; i < numSubModels; i++)
	{
		delete[] (leafbrushes + cmodels[i].leaf.firstLeafBrush);
		delete[] (leafsurfaces + cmodels[i].leaf.firstLeafSurface);
	}
	for (int i = 0; i < numSurfaces; i++)
	{
		if (!surfaces[i])
		{
			continue;
		}
		delete[] surfaces[i]->pc->facets;
		delete[] surfaces[i]->pc->planes;
		delete surfaces[i]->pc;
		delete surfaces[i];
	}
	delete[] shaders;
	delete[] leafs;
	delete[] areas;
	delete[] areaPortals;
	delete[] leafbrushes;
	delete[] leafsurfaces;
	delete[] planes;
	delete[] brushsides;
	delete[] brushes;
	delete[] nodes;
	delete[] entityString;
	delete[] visibility;
	delete[] surfaces;
	delete[] cmodels;
}

//==========================================================================
//
//	QClipMap46::InlineModel
//
//==========================================================================

clipHandle_t QClipMap46::InlineModel(int Index) const
{
	if (Index < 0 || Index >= numSubModels)
	{
		throw QDropException("CM_InlineModel: bad number");
	}
	return Index;
}

//==========================================================================
//
//	QClipMap46::GetNumClusters
//
//==========================================================================

int QClipMap46::GetNumClusters() const
{
	return numClusters;
}

//==========================================================================
//
//	QClipMap46::GetNumInlineModels
//
//==========================================================================

int QClipMap46::GetNumInlineModels() const
{
	return numSubModels;
}

//==========================================================================
//
//	QClipMap46::GetEntityString
//
//==========================================================================

const char* QClipMap46::GetEntityString() const
{
	return entityString;
}

//==========================================================================
//
//	QClipMap46::MapChecksums
//
//==========================================================================

void QClipMap46::MapChecksums(int& CheckSum1, int& CheckSum2) const
{
	CheckSum1 = checksum;
	CheckSum2 = checksum;
}

//==========================================================================
//
//	QClipMap46::LeafCluster
//
//==========================================================================

int QClipMap46::LeafCluster(int LeafNum) const
{
	if (LeafNum < 0 || LeafNum >= numLeafs)
	{
		throw QDropException("CM_LeafCluster: bad number");
	}
	return leafs[LeafNum].cluster;
}

//==========================================================================
//
//	QClipMap46::LeafArea
//
//==========================================================================

int QClipMap46::LeafArea(int LeafNum) const
{
	if (LeafNum < 0 || LeafNum >= numLeafs)
	{
		throw QDropException("CM_LeafArea: bad number");
	}
	return leafs[LeafNum].area;
}

//==========================================================================
//
//	QClipMap46::LeafAmbientSoundLevel
//
//==========================================================================

const byte* QClipMap46::LeafAmbientSoundLevel(int LeafNum) const
{
	return NULL;
}

//==========================================================================
//
//	QClipMap46::ModelBounds
//
//==========================================================================

void QClipMap46::ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	cmodel_t* cmod = ClipHandleToModel(Model);
	VectorCopy(cmod->mins, Mins);
	VectorCopy(cmod->maxs, Maxs);
}

//==========================================================================
//
//	QClipMap46::GetNumTextures
//
//==========================================================================

int QClipMap46::GetNumTextures() const
{
	return 0;
}

//==========================================================================
//
//	QClipMap46::GetTextureName
//
//==========================================================================

const char* QClipMap46::GetTextureName(int Index) const
{
	return "";
}

//==========================================================================
//
//	QClipMap46::ModelHull
//
//==========================================================================

clipHandle_t QClipMap46::ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
{
	VectorClear(ClipMins);
	VectorClear(ClipMaxs);
	return Handle;
}

//==========================================================================
//
//	QClipMap46::ClipHandleToModel
//
//==========================================================================

cmodel_t* QClipMap46::ClipHandleToModel(clipHandle_t Handle)
{
	if (Handle < 0)
	{
		throw QDropException(va("CM_ClipHandleToModel: bad handle %i", Handle));
	}
	if (Handle < numSubModels)
	{
		return &cmodels[Handle];
	}
	if (Handle == BOX_MODEL_HANDLE)
	{
		return &box_model;
	}
	if (Handle < MAX_SUBMODELS)
	{
		throw QDropException(va("CM_ClipHandleToModel: bad handle %i < %i < %i", 
			numSubModels, Handle, MAX_SUBMODELS));
	}
	throw QDropException(va("CM_ClipHandleToModel: bad handle %i", Handle + MAX_SUBMODELS));
}

//==========================================================================
//
//	QClipMap46::InitBoxHull
//
//	Set up the planes and nodes so that the six floats of a bounding box
// can just be stored out and get a proper clipping hull structure.
//
//==========================================================================

void QClipMap46::InitBoxHull()
{
	box_planes = &planes[numPlanes];

	box_brush = &brushes[numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = brushsides + numBrushSides;
	box_brush->contents = BSP46CONTENTS_BODY;

	Com_Memset(&box_model, 0, sizeof(box_model));
	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = numBrushes;
	box_model.leaf.firstLeafBrush = numLeafBrushes;
	leafbrushes[numLeafBrushes] = numBrushes;

	for (int i = 0; i < 6; i++)
	{
		int side = i & 1;

		// brush sides
		cbrushside_t* s = &brushsides[numBrushSides + i];
		s->plane = planes + (numPlanes + i * 2 + side);
		s->surfaceFlags = 0;

		// planes
		cplane_t* p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + (i >> 1);
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = -1;

		SetPlaneSignbits(p);
	}	
}

//==========================================================================
//
//	QClipMap46::TempBoxModel
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
// Capsules are handled differently though.
//
//==========================================================================

clipHandle_t QClipMap46::TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
{
	VectorCopy(Mins, box_model.mins);
	VectorCopy(Maxs, box_model.maxs);

	if (Capsule)
	{
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = Maxs[0];
	box_planes[1].dist = -Maxs[0];
	box_planes[2].dist = Mins[0];
	box_planes[3].dist = -Mins[0];
	box_planes[4].dist = Maxs[1];
	box_planes[5].dist = -Maxs[1];
	box_planes[6].dist = Mins[1];
	box_planes[7].dist = -Mins[1];
	box_planes[8].dist = Maxs[2];
	box_planes[9].dist = -Maxs[2];
	box_planes[10].dist = Mins[2];
	box_planes[11].dist = -Mins[2];

	VectorCopy(Mins, box_brush->bounds[0]);
	VectorCopy(Maxs, box_brush->bounds[1]);

	return BOX_MODEL_HANDLE;
}

//==========================================================================
//
//	QClipMap46::ContentsToQ1
//
//==========================================================================

int QClipMap46::ContentsToQ1(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMap46::ContentsToQ2
//
//==========================================================================

int QClipMap46::ContentsToQ2(int Contents) const
{
	throw QException("Not implemented");
}
