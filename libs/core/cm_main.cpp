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
#include "cm_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QClipMap*			CMapShared;

int					c_pointcontents;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap::~QClipMap
//
//==========================================================================

QClipMap::~QClipMap()
{
}

//==========================================================================
//
//	CM_InlineModel
//
//==========================================================================

clipHandle_t CM_InlineModel(int Index)
{
	return CMapShared->InlineModel(Index);
}

//==========================================================================
//
//	CM_NumClusters
//
//==========================================================================

int CM_NumClusters()
{
	return CMapShared->GetNumClusters();
}

//==========================================================================
//
//	CM_NumInlineModels
//
//==========================================================================

int CM_NumInlineModels()
{
	return CMapShared->GetNumInlineModels();
}

//==========================================================================
//
//	CM_EntityString
//
//==========================================================================

const char* CM_EntityString()
{
	return CMapShared->GetEntityString();
}

//==========================================================================
//
//	CM_MapChecksums
//
//==========================================================================

void CM_MapChecksums(int& CheckSum1, int& CheckSum2)
{
	CMapShared->MapChecksums(CheckSum1, CheckSum2);
}

//==========================================================================
//
//	CM_LeafCluster
//
//==========================================================================

int CM_LeafCluster(int LeafNum)
{
	return CMapShared->LeafCluster(LeafNum);
}

//==========================================================================
//
//	CM_LeafArea
//
//==========================================================================

int CM_LeafArea(int LeafNum)
{
	return CMapShared->LeafArea(LeafNum);
}

//==========================================================================
//
//	CM_LeafAmbientSoundLevel
//
//==========================================================================

const byte* CM_LeafAmbientSoundLevel(int LeafNum)
{
	if (!CMapShared)
	{
		return NULL;		// sound may call this without map loaded
	}
	return CMapShared->LeafAmbientSoundLevel(LeafNum);
}

//==========================================================================
//
//	CM_ModelBounds
//
//==========================================================================

void CM_ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs)
{
	CMapShared->ModelBounds(Model, Mins, Maxs);
}

//==========================================================================
//
//	CM_GetNumTextures
//
//==========================================================================

int CM_GetNumTextures()
{
	return CMapShared->GetNumTextures();
}

//==========================================================================
//
//	CM_GetTextureName
//
//==========================================================================

const char* CM_GetTextureName(int Index)
{
	return CMapShared->GetTextureName(Index);
}

//==========================================================================
//
//	CM_TempBoxModel
//
//==========================================================================

clipHandle_t CM_TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule)
{
	return CMapShared->TempBoxModel(Mins, Maxs, Capsule);
}

//==========================================================================
//
//	CM_ModelHull
//
//==========================================================================

clipHandle_t CM_ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs)
{
	return CMapShared->ModelHull(Handle, HullNum, ClipMins, ClipMaxs);
}

//==========================================================================
//
//	CM_ModelHull
//
//==========================================================================

clipHandle_t CM_ModelHull(clipHandle_t Handle, int HullNum)
{
	vec3_t ClipMins;
	vec3_t ClipMaxs;
	return CMapShared->ModelHull(Handle, HullNum, ClipMins, ClipMaxs);
}

//==========================================================================
//
//	CM_PointLeafnum
//
//==========================================================================

int CM_PointLeafnum(const vec3_t Point)
{
	if (!CMapShared)
	{
		// sound may call this without map loaded
		return 0;
	}
	return CMapShared->PointLeafnum(Point);
}

//==========================================================================
//
//	CM_BoxLeafnums
//
//==========================================================================

int CM_BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int *List, int ListSize, int *TopNode, int *LastLeaf)
{
	return CMapShared->BoxLeafnums(Mins, Maxs, List, ListSize, TopNode, LastLeaf);
}

//==========================================================================
//
//	CM_PointContentsQ1
//
//==========================================================================

int CM_PointContentsQ1(const vec3_t P, clipHandle_t Model)
{
	return CMapShared->PointContentsQ1(P, Model);
}

//==========================================================================
//
//	CM_PointContentsQ2
//
//==========================================================================

int CM_PointContentsQ2(const vec3_t P, clipHandle_t Model)
{
	return CMapShared->PointContentsQ2(P, Model);
}

//==========================================================================
//
//	CM_PointContentsQ3
//
//==========================================================================

int CM_PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	return CMapShared->PointContentsQ3(P, Model);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ1
//
//==========================================================================

int CM_TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return CMapShared->TransformedPointContentsQ1(P, Model, Origin, Angles);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ2
//
//==========================================================================

int CM_TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return CMapShared->TransformedPointContentsQ2(P, Model, Origin, Angles);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ3
//
//==========================================================================

int CM_TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return CMapShared->TransformedPointContentsQ3(P, Model, Origin, Angles);
}

//==========================================================================
//
//	CM_ClusterPVS
//
//==========================================================================

byte* CM_ClusterPVS(int Cluster)
{
	return CMapShared->ClusterPVS(Cluster);
}

//==========================================================================
//
//	CM_ClusterPHS
//
//==========================================================================

byte* CM_ClusterPHS(int Cluster)
{
	return CMapShared->ClusterPHS(Cluster);
}

//==========================================================================
//
//	CM_SetAreaPortalState
//
//==========================================================================

void CM_SetAreaPortalState(int PortalNum, qboolean Open)
{
	CMapShared->SetAreaPortalState(PortalNum, Open);
}

//==========================================================================
//
//	CM_AdjustAreaPortalState
//
//==========================================================================

void CM_AdjustAreaPortalState(int Area1, int Area2, bool Open)
{
	CMapShared->AdjustAreaPortalState(Area1, Area2, Open);
}

//==========================================================================
//
//	CM_AreasConnected
//
//==========================================================================

qboolean CM_AreasConnected(int Area1, int Area2)
{
	return CMapShared->AreasConnected(Area1, Area2);
}

//==========================================================================
//
//	CM_WriteAreaBits
//
//==========================================================================

int CM_WriteAreaBits(byte* Buffer, int Area)
{
	return CMapShared->WriteAreaBits(Buffer, Area);
}

//==========================================================================
//
//	CM_WritePortalState
//
//==========================================================================

void CM_WritePortalState(fileHandle_t f)
{
	CMapShared->WritePortalState(f);
}

//==========================================================================
//
//	CM_ReadPortalState
//
//==========================================================================

void CM_ReadPortalState(fileHandle_t f)
{
	CMapShared->ReadPortalState(f);
}
