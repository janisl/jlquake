//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include "clipmap38_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

mapsurface_t	QClipMap38::nullsurface;

QCvar*			map_noareas;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_CreateQClipMap38
//
//==========================================================================

QClipMap* CM_CreateQClipMap38()
{
	return new QClipMap38();
}

//==========================================================================
//
//	QClipMap38::QClipMap38
//
//==========================================================================

QClipMap38::QClipMap38()
: numtexinfo(0)
, surfaces(NULL)
, numleafs(0)
, leafs(NULL)
, emptyleaf(0)
, solidleaf(0)
, numclusters(0)
, numleafbrushes(0)
, leafbrushes(NULL)
, numplanes(0)
, planes(NULL)
, numbrushes(0)
, brushes(NULL)
, numbrushsides(0)
, brushsides(NULL)
, numnodes(0)
, nodes(NULL)
, numareas(0)
, areas(NULL)
, numareaportals(0)
, areaportals(NULL)
, numvisibility(0)
, visibility(NULL)
, vis(NULL)
, numentitychars(0)
, entitystring(NULL)
, numcmodels(0)
, cmodels(NULL)
, box_planes(NULL)
, box_brush(NULL)
, box_leaf(NULL)
, floodvalid(0)
, checkcount(0)
{
	Com_Memset(&box_model, 0, sizeof(box_model));
	Com_Memset(portalopen, 0, sizeof(portalopen));
	Com_Memset(pvsrow, 0, sizeof(pvsrow));
	Com_Memset(phsrow, 0, sizeof(phsrow));
}

//==========================================================================
//
//	QClipMap38::~QClipMap38
//
//==========================================================================

QClipMap38::~QClipMap38()
{
	delete[] surfaces;
	delete[] leafs;
	delete[] leafbrushes;
	delete[] planes;
	delete[] brushes;
	delete[] brushsides;
	delete[] nodes;
	delete[] areas;
	delete[] areaportals;
	delete[] visibility;
	delete[] entitystring;
	delete[] cmodels;
}

//==========================================================================
//
//	QClipMap38::InlineModel
//
//==========================================================================

clipHandle_t QClipMap38::InlineModel(int Index) const
{
	if (Index < 1 || Index >= numcmodels)
	{
		throw QDropException("CM_InlineModel: bad number");
	}
	return Index;
}

//==========================================================================
//
//	QClipMap38::GetNumClusters
//
//==========================================================================

int QClipMap38::GetNumClusters() const
{
	return numclusters;
}

//==========================================================================
//
//	QClipMap38::GetNumInlineModels
//
//==========================================================================

int QClipMap38::GetNumInlineModels() const
{
	return numcmodels;
}

//==========================================================================
//
//	QClipMap38::GetEntityString
//
//==========================================================================

const char* QClipMap38::GetEntityString() const
{
	return entitystring;
}

//==========================================================================
//
//	QClipMap38::MapChecksums
//
//==========================================================================

void QClipMap38::MapChecksums(int& ACheckSum1, int& ACheckSum2) const
{
	ACheckSum1 = CheckSum;
	ACheckSum2 = CheckSum2;
}

//==========================================================================
//
//	QClipMap38::LeafCluster
//
//==========================================================================

int QClipMap38::LeafCluster(int LeafNum) const
{
	if (LeafNum < 0 || LeafNum >= numleafs)
	{
		throw QDropException("CM_LeafCluster: bad number");
	}
	return leafs[LeafNum].cluster;
}

//==========================================================================
//
//	QClipMap38::LeafArea
//
//==========================================================================

int QClipMap38::LeafArea(int LeafNum) const
{
	if (LeafNum < 0 || LeafNum >= numleafs)
	{
		throw QDropException("CM_LeafArea: bad number");
	}
	return leafs[LeafNum].area;
}

//==========================================================================
//
//	QClipMap38::LeafAmbientSoundLevel
//
//==========================================================================

const byte* QClipMap38::LeafAmbientSoundLevel(int LeafNum) const
{
	return NULL;
}

//==========================================================================
//
//	QClipMap38::ModelBounds
//
//==========================================================================

void QClipMap38::ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	cmodel_t* cmod = ClipHandleToModel(Model);
	VectorCopy(cmod->mins, Mins);
	VectorCopy(cmod->maxs, Maxs);
}

//==========================================================================
//
//	QClipMap38::GetNumTextures
//
//==========================================================================

int QClipMap38::GetNumTextures() const
{
	return numtexinfo;
}

//==========================================================================
//
//	QClipMap38::GetTextureName
//
//==========================================================================

const char* QClipMap38::GetTextureName(int Index) const
{
	return surfaces[Index].rname;
}

//==========================================================================
//
//	QClipMap38::ModelHull
//
//==========================================================================

clipHandle_t QClipMap38::ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
{
	VectorClear(ClipMins);
	VectorClear(ClipMaxs);
	return Handle;
}

//==========================================================================
//
//	QClipMap38::ClipHandleToModel
//
//==========================================================================

cmodel_t* QClipMap38::ClipHandleToModel(clipHandle_t Handle)
{
	if (Handle == BOX_HANDLE)
	{
		return &box_model;
	}
	return &cmodels[Handle];
}

//==========================================================================
//
//	QClipMap38::InitBoxHull
//
//	Set up the planes and nodes so that the six floats of a bounding box
// can just be stored out and get a proper clipping hull structure.
//
//==========================================================================

void QClipMap38::InitBoxHull()
{
	Com_Memset(&box_model, 0, sizeof(box_model));
	box_model.headnode = numnodes;

	box_planes = &planes[numplanes];

	box_brush = &brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = BSP38CONTENTS_MONSTER;

	box_leaf = &leafs[numleafs];
	box_leaf->contents = BSP38CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	leafbrushes[numleafbrushes] = numbrushes;

	for (int i = 0; i < 6; i++)
	{
		int side = i & 1;

		// brush sides
		cbrushside_t* s = &brushsides[numbrushsides + i];
		s->plane = 	planes + (numplanes + i * 2 + side);
		s->surface = &nullsurface;

		// nodes
		cnode_t* c = &nodes[numnodes + i];
		c->plane = planes + (numplanes + i * 2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
		{
			c->children[side ^ 1] = numnodes + i + 1;
		}
		else
		{
			c->children[side ^ 1] = -1 - numleafs;
		}

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
	}	
}

//==========================================================================
//
//	QClipMap38::TempBoxModel
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
//
//==========================================================================

clipHandle_t QClipMap38::TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
{
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

	return BOX_HANDLE;
}

//==========================================================================
//
//	QClipMap38::ContentsToQ1
//
//==========================================================================

int QClipMap38::ContentsToQ1(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMap38::ContentsToQ3
//
//==========================================================================

int QClipMap38::ContentsToQ3(int Contents) const
{
	throw QException("Not implemented");
}

//==========================================================================
//
//	QClipMap38::DrawDebugSurface
//
//==========================================================================

void QClipMap38::DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points))
{
}
