//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../core.h"
#include "local.h"
#include "../file_formats/bsp29.h"
#include "../file_formats/bsp38.h"
#include "../file_formats/bsp46.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QClipMap*			CMapShared;

int					c_pointcontents;
int					c_traces;
int					c_brush_traces;
int					c_patch_traces;

Cvar*				cm_flushmap;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap::QClipMap
//
//==========================================================================

static QClipMap* GetModel(clipHandle_t Handle)
{
	if (!(Handle & CMH_NON_MAP_MASK))
	{
		return CMapShared;
	}
	
	int Index = ((Handle & CMH_NON_MAP_MASK) >> CMH_NON_MAP_SHIFT) - 1;
	if (Index >= CMNonMapModels.Num())
	{
		throw DropException("Invalid handle");
	}
	return CMNonMapModels[Index];
}

//==========================================================================
//
//	QClipMap::QClipMap
//
//==========================================================================

QClipMap::QClipMap()
: CheckSum(0)
, CheckSum2(0)
{
}

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
//	CM_LoadMap
//
//	Loads in the map and all submodels
//
//==========================================================================

void CM_LoadMap(const char* name, bool clientload, int* checksum)
{
	if (!name || (!(GGameType & GAME_Quake2) && !name[0]))
	{
		throw DropException("CM_LoadMap: NULL name");
	}

	Log::develWrite("CM_LoadMap(%s, %i)\n", name, clientload);

	if (!cm_flushmap)
	{
		cm_flushmap = Cvar_Get("cm_flushmap", "0", 0);
	}

	if (CMapShared && CMapShared->Name == name && (clientload || !cm_flushmap->integer))
	{
		// still have the right version
		CMapShared->ReloadMap(clientload);
		if (checksum)
		{
			*checksum = CMapShared->CheckSum;
		}
		return;
	}

	// free old stuff
	CM_ClearMap();

	if (!name[0])
	{
		Array<quint8> Buffer;
		CMapShared = CM_CreateQClipMap38();
		CMapShared->LoadMap(name, Buffer);
		if (checksum)
		{
			*checksum = CMapShared->CheckSum;
		}
		return;
	}
	
	//
	// load the file
	//
	Array<quint8> Buffer;
	if (FS_ReadFile(name, Buffer) <= 0)
	{
		throw DropException(va("Couldn't load %s", name));
	}

	struct TTestHeader
	{
		int		Id;
		int		Version;
	} TestHeader;
	TestHeader = *(TTestHeader*)Buffer.Ptr();
	TestHeader.Id = LittleLong(TestHeader.Id);
	TestHeader.Version = LittleLong(TestHeader.Version);

	switch (TestHeader.Id)
	{
	case BSP29_VERSION:
		CMapShared = CM_CreateQClipMap29();
		break;

	case BSP46_IDENT:
		switch (TestHeader.Version)
		{
		case BSP38_VERSION:
			CMapShared = CM_CreateQClipMap38();
			break;

		case BSP46_VERSION:
			CMapShared = CM_CreateQClipMap46();
			break;

		default:
			throw DropException("Unsupported BSP model version");
		}
		break;

	default:
		throw DropException("Unsupported map file format");
	}

	CMapShared->LoadMap(name, Buffer);
	if (checksum)
	{
		*checksum = CMapShared->CheckSum;
	}
}

//==========================================================================
//
//	CM_ClearMap
//
//==========================================================================

void CM_ClearMap()
{
	if (CMapShared)
	{
		delete CMapShared;
		CMapShared = NULL;
	}
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
	GetModel(Model)->ModelBounds(Model & CMH_MODEL_MASK, Mins, Maxs);
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
	return GetModel(Handle)->ModelHull(Handle & CMH_MODEL_MASK, HullNum, ClipMins, ClipMaxs) | (Handle & CMH_NON_MAP_MASK);
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
	return CM_ModelHull(Handle, HullNum, ClipMins, ClipMaxs);
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
	return GetModel(Model)->PointContentsQ1(P, Model & CMH_MODEL_MASK);
}

//==========================================================================
//
//	CM_PointContentsQ2
//
//==========================================================================

int CM_PointContentsQ2(const vec3_t P, clipHandle_t Model)
{
	return GetModel(Model)->PointContentsQ2(P, Model & CMH_MODEL_MASK);
}

//==========================================================================
//
//	CM_PointContentsQ3
//
//==========================================================================

int CM_PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	return GetModel(Model)->PointContentsQ3(P, Model & CMH_MODEL_MASK);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ1
//
//==========================================================================

int CM_TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return GetModel(Model)->TransformedPointContentsQ1(P, Model & CMH_MODEL_MASK, Origin, Angles);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ2
//
//==========================================================================

int CM_TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return GetModel(Model)->TransformedPointContentsQ2(P, Model & CMH_MODEL_MASK, Origin, Angles);
}

//==========================================================================
//
//	CM_TransformedPointContentsQ3
//
//==========================================================================

int CM_TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return GetModel(Model)->TransformedPointContentsQ3(P, Model & CMH_MODEL_MASK, Origin, Angles);
}

//==========================================================================
//
//	CM_HeadnodeVisible
//
//==========================================================================

bool CM_HeadnodeVisible(int NodeNum, byte* VisBits)
{
	return CMapShared->HeadnodeVisible(NodeNum, VisBits);
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

//==========================================================================
//
//	CM_HullCheckQ1
//
//==========================================================================

bool CM_HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace)
{
	return GetModel(Handle)->HullCheckQ1(Handle & CMH_MODEL_MASK, p1, p2, trace);
}

//==========================================================================
//
//	CM_BoxTraceQ2
//
//==========================================================================

q2trace_t CM_BoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask)
{
	return GetModel(Model)->BoxTraceQ2(Start, End, Mins, Maxs, Model & CMH_MODEL_MASK, BrushMask);
}

//==========================================================================
//
//	CM_TransformedBoxTraceQ2
//
//==========================================================================

q2trace_t CM_TransformedBoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, vec3_t Origin, vec3_t Angles)
{
	return GetModel(Model)->TransformedBoxTraceQ2(Start, End, Mins, Maxs, Model & CMH_MODEL_MASK, BrushMask, Origin, Angles);
}

//==========================================================================
//
//	CM_BoxTraceQ3
//
//==========================================================================

void CM_BoxTraceQ3( q3trace_t *Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
	clipHandle_t Model, int BrushMask, int Capsule)
{
	GetModel(Model)->BoxTraceQ3(Results, Start, End, Mins, Maxs, Model & CMH_MODEL_MASK, BrushMask, Capsule);
}

//==========================================================================
//
//	CM_TransformedBoxTraceQ3
//
//==========================================================================

void CM_TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
	clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule)
{
	GetModel(Model)->TransformedBoxTraceQ3(Results, Start, End, Mins, Maxs, Model & CMH_MODEL_MASK, BrushMask, Origin, Angles, Capsule);
}

//==========================================================================
//
//	CM_DrawDebugSurface
//
//==========================================================================

void CM_DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points))
{
	CMapShared->DrawDebugSurface(drawPoly);
}
