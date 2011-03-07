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
#include "cm29_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

byte			QClipMap29::mod_novis[BSP29_MAX_MAP_LEAFS / 8];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_CreateQClipMap29
//
//==========================================================================

QClipMap* CM_CreateQClipMap29()
{
	return new QClipMap29();
}

//==========================================================================
//
//	QClipMap29::QClipMap29
//
//==========================================================================

QClipMap29::QClipMap29()
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));
}

//==========================================================================
//
//	QClipMap29::~QClipMap29
//
//==========================================================================

QClipMap29::~QClipMap29()
{
	map_models[0].Free();
}

//==========================================================================
//
//	cmodel_t::Free
//
//==========================================================================

void cmodel_t::Free()
{
	delete[] visdata;
	visdata = NULL;
	delete[] entities;
	entities = NULL;
	delete[] planes;
	planes = NULL;
	delete[] nodes;
	nodes = NULL;
	delete[] leafs;
	leafs = NULL;
	delete[] clipnodes;
	clipnodes = NULL;
	delete[] hulls[0].clipnodes;
	hulls[0].clipnodes = NULL;
	delete[] phs;
	phs = NULL;
}

//==========================================================================
//
//	QClipMap29::InlineModel
//
//==========================================================================

clipHandle_t QClipMap29::InlineModel(int Index) const
{
	if (Index < 1 || Index > map_models[0].numsubmodels)
	{
		throw QDropException("Bad submodel index");
	}
	return Index * MAX_MAP_HULLS;
}

//==========================================================================
//
//	QClipMap29::GetNumClusters
//
//==========================================================================

int QClipMap29::GetNumClusters() const
{
	return map_models[0].numleafs;
}

//==========================================================================
//
//	QClipMap29::GetNumInlineModels
//
//==========================================================================

int QClipMap29::GetNumInlineModels() const
{
	return map_models[0].numsubmodels;
}

//==========================================================================
//
//	QClipMap29::GetEntityString
//
//==========================================================================

const char* QClipMap29::GetEntityString() const
{
	return map_models[0].entities;
}

//==========================================================================
//
//	QClipMap29::MapChecksums
//
//==========================================================================

void QClipMap29::MapChecksums(int& ACheckSum1, int& ACheckSum2) const
{
	ACheckSum1 = CheckSum;
	ACheckSum2 = CheckSum2;
}

//==========================================================================
//
//	QClipMap29::LeafCluster
//
//==========================================================================

int QClipMap29::LeafCluster(int LeafNum) const
{
	//	-1 is because pvs rows are 1 based, not 0 based like leafs
	return LeafNum - 1;
}

//==========================================================================
//
//	QClipMap29::LeafArea
//
//==========================================================================

int QClipMap29::LeafArea(int LeafNum) const
{
	return 0;
}

//==========================================================================
//
//	QClipMap29::LeafAmbientSoundLevel
//
//==========================================================================

const byte* QClipMap29::LeafAmbientSoundLevel(int LeafNum) const
{
	return map_models[0].leafs[LeafNum].ambient_sound_level;
}

//==========================================================================
//
//	QClipMap29::ModelBounds
//
//==========================================================================

void QClipMap29::ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	cmodel_t* cmod = ClipHandleToModel(Model);
	VectorCopy(cmod->mins, Mins);
	VectorCopy(cmod->maxs, Maxs);
}

//==========================================================================
//
//	QClipMap29::GetNumTextures
//
//==========================================================================

int QClipMap29::GetNumTextures() const
{
	return 0;
}

//==========================================================================
//
//	QClipMap29::GetTextureName
//
//==========================================================================

const char* QClipMap29::GetTextureName(int Index) const
{
	return "";
}

//==========================================================================
//
//	QClipMap29::ModelHull
//
//==========================================================================

clipHandle_t QClipMap29::ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
{
	cmodel_t* model = ClipHandleToModel(Handle);
	if (HullNum < 0 || HullNum >= MAX_MAP_HULLS)
	{
		throw QException("Invalid hull number");
	}
	VectorCopy(model->hulls[HullNum].clip_mins, ClipMins);
	VectorCopy(model->hulls[HullNum].clip_maxs, ClipMaxs);
	return (Handle & MODEL_NUMBER_MASK) | HullNum;
}

//==========================================================================
//
//	QClipMap29::ClipHandleToModel
//
//==========================================================================

cmodel_t* QClipMap29::ClipHandleToModel(clipHandle_t Handle)
{
	Handle /= MAX_MAP_HULLS;
	if (Handle < MAX_MAP_MODELS)
	{
		return &map_models[Handle];
	}
	if (Handle == BOX_HULL_HANDLE / MAX_MAP_HULLS)
	{
		return &box_model;
	}
	throw QException("Invalid handle");
}

//==========================================================================
//
//	QClipMap29::ClipHandleToHull
//
//==========================================================================

chull_t* QClipMap29::ClipHandleToHull(clipHandle_t Handle)
{
	cmodel_t* Model = ClipHandleToModel(Handle);
	return &Model->hulls[Handle & HULL_NUMBER_MASK];
}

//**************************************************************************
//
//	BOX HULL
//
//**************************************************************************

//==========================================================================
//
//	QClipMap29::InitBoxHull
//
//	Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper chull_t structure.
//
//==========================================================================

void QClipMap29::InitBoxHull()
{
	Com_Memset(&box_model, 0, sizeof(box_model));
	box_model.hulls[0].clipnodes = box_clipnodes;
	box_model.hulls[0].planes = box_planes;
	box_model.hulls[0].firstclipnode = 0;
	box_model.hulls[0].lastclipnode = 5;

	Com_Memset(box_planes, 0, sizeof(box_planes));

	for (int i = 0; i < 6; i++)
	{
		box_clipnodes[i].planenum = i;

		int side = i & 1;

		box_clipnodes[i].children[side] = BSP29CONTENTS_EMPTY;
		if (i != 5)
		{
			box_clipnodes[i].children[side ^ 1] = i + 1;
		}
		else
		{
			box_clipnodes[i].children[side ^ 1] = BSP29CONTENTS_SOLID;
		}

		box_planes[i].type = i >> 1;
		box_planes[i].normal[i >> 1] = 1;
	}
}

//==========================================================================
//
//	QClipMap29::TempBoxModel
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
//
//==========================================================================

clipHandle_t QClipMap29::TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
{
	box_planes[0].dist = Maxs[0];
	box_planes[1].dist = Mins[0];
	box_planes[2].dist = Maxs[1];
	box_planes[3].dist = Mins[1];
	box_planes[4].dist = Maxs[2];
	box_planes[5].dist = Mins[2];

	return BOX_HULL_HANDLE;
}

//==========================================================================
//
//	QClipMap29::ContentsToQ2
//
//==========================================================================

int QClipMap29::ContentsToQ2(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMap29::ContentsToQ3
//
//==========================================================================

int QClipMap29::ContentsToQ3(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMap29::DrawDebugSurface
//
//==========================================================================

void QClipMap29::DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points))
{
}
