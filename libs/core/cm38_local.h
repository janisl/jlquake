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

#ifndef _CM38_LOCAL_H
#define _CM38_LOCAL_H

#include "cm_local.h"
#include "bsp38file.h"

#define BOX_HANDLE		BSP38MAX_MAP_MODELS

struct mapsurface_t	// used internally due to name len probs //ZOID
{
	q2csurface_t	c;
	char			rname[32];
};

struct cleaf_t
{
	int			contents;
	int			cluster;
	int			area;
	quint16		firstleafbrush;
	quint16		numleafbrushes;
};

struct cbrush_t
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
};

struct cbrushside_t
{
	cplane_t*		plane;
	mapsurface_t*	surface;
};

struct cnode_t
{
	cplane_t*	plane;
	int			children[2];		// negative numbers are leafs
};

struct carea_t
{
	int			numareaportals;
	int			firstareaportal;
	int			floodnum;			// if two areas have equal floodnums, they are connected
	int			floodvalid;
};

struct cmodel_t
{
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		origin;		// for sounds or lights
	int			headnode;
};

class QClipMap38 : public QClipMap
{
private:
	//	Main
	void InitBoxHull();
	int ContentsToQ1(int Contents) const;
	int ContentsToQ3(int Contents) const;

	//	Load
	void LoadSurfaces(const quint8* base, const bsp38_lump_t* l);
	void LoadLeafs(const quint8* base, const bsp38_lump_t* l);
	void LoadLeafBrushes(const quint8* base, const bsp38_lump_t* l);
	void LoadPlanes(const quint8* base, const bsp38_lump_t *l);
	void LoadBrushes(const quint8* base, const bsp38_lump_t *l);
	void LoadBrushSides(const quint8* base, const bsp38_lump_t *l);
	void LoadNodes(const quint8* base, const bsp38_lump_t* l);
	void LoadAreas(const quint8* base, const bsp38_lump_t *l);
	void LoadAreaPortals(const quint8* base, const bsp38_lump_t* l);
	void LoadVisibility(const quint8* base, const bsp38_lump_t* l);
	void LoadEntityString(const quint8* base, const bsp38_lump_t *l);
	void LoadSubmodels(const quint8* base, const bsp38_lump_t *l);

	//	Test
	void DecompressVis(const byte* in, byte* out);
	void FloodArea(carea_t* area, int floodnum);

	//	Trace
	void RecursiveHullCheck(int num, float p1f, float p2f, vec3_t p1, vec3_t p2);
	void TraceToLeaf(int leafnum);
	void ClipBoxToBrush(vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, q2trace_t* trace, cbrush_t* brush);
	void TestInLeaf(int leafnum);
	void TestBoxInBrush(vec3_t mins, vec3_t maxs, vec3_t p1, q2trace_t* trace, cbrush_t* brush);

public:
	int				numtexinfo;
	mapsurface_t*	surfaces;

	int				numleafs;
	cleaf_t*		leafs;
	int				emptyleaf;
	int				solidleaf;

	int				numclusters;

	int				numleafbrushes;
	quint16*		leafbrushes;

	int				numplanes;
	cplane_t*		planes;

	int				numbrushes;
	cbrush_t*		brushes;

	int				numbrushsides;
	cbrushside_t*	brushsides;

	int				numnodes;
	cnode_t*		nodes;

	int				numareas;
	carea_t*		areas;

	int				numareaportals;
	bsp38_dareaportal_t*	areaportals;

	int				numvisibility;
	quint8*			visibility;
	bsp38_dvis_t*	vis;

	int				numentitychars;
	char*			entitystring;

	int				numcmodels;
	cmodel_t*		cmodels;

	cplane_t*		box_planes;
	cbrush_t*		box_brush;
	cleaf_t*		box_leaf;
	cmodel_t		box_model;

	static mapsurface_t	nullsurface;

	qboolean		portalopen[BSP38MAX_MAP_AREAPORTALS];

	int				floodvalid;

	byte			pvsrow[BSP38MAX_MAP_LEAFS / 8];
	byte			phsrow[BSP38MAX_MAP_LEAFS / 8];

	int				checkcount;

	vec3_t			trace_start;
	vec3_t			trace_end;
	vec3_t			trace_mins;
	vec3_t			trace_maxs;
	vec3_t			trace_extents;
	q2trace_t		trace_trace;
	int				trace_contents;
	bool			trace_ispoint;		// optimized case

	QClipMap38();
	~QClipMap38();

	void LoadMap(const char* name, const QArray<quint8>& Buffer);
	void ReloadMap(bool ClientLoad);
	clipHandle_t PrecacheModel(const char* Name);
	clipHandle_t InlineModel(int Index) const;
	int GetNumClusters() const;
	int GetNumInlineModels() const;
	const char* GetEntityString() const;
	void MapChecksums(int& CheckSum1, int& CheckSum2) const;
	int LeafCluster(int LeafNum) const;
	int LeafArea(int LeafNum) const;
	const byte* LeafAmbientSoundLevel(int LeafNum) const;
	void ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs);
	int GetNumTextures() const;
	const char* GetTextureName(int Index) const;
	clipHandle_t TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule);
	clipHandle_t ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs);
	int PointLeafnum(const vec3_t P) const;
	int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf) const;
	int PointContentsQ1(const vec3_t P, clipHandle_t Model);
	int PointContentsQ2(const vec3_t p, clipHandle_t Model);
	int PointContentsQ3(const vec3_t P, clipHandle_t Model);
	int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	bool HeadnodeVisible(int NodeNum, byte *VisBits);
	byte* ClusterPVS(int Cluster);
	byte* ClusterPHS(int Cluster);
	void SetAreaPortalState(int PortalNum, qboolean Open);
	void AdjustAreaPortalState(int Area1, int Area2, bool Open);
	qboolean AreasConnected(int Area1, int Area2);
	int WriteAreaBits(byte* Buffer, int Area);
	void WritePortalState(fileHandle_t f);
	void ReadPortalState(fileHandle_t f);
	bool HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace);
	q2trace_t BoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask);
	q2trace_t TransformedBoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model,
		int BrushMask, vec3_t Origin, vec3_t Angles);
	void BoxTraceQ3(q3trace_t* Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, int Capsule);
	void TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End, vec3_t Mins, vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule);
	void DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points));

	cmodel_t* ClipHandleToModel(clipHandle_t Handle);
	void FloodAreaConnections();
	void ClearPortalOpen();
	int PointLeafnum_r(const vec3_t P, int Num) const;
	void BoxLeafnums_r(leafList_t* ll, int NodeNum) const;
};

extern QCvar*		map_noareas;

#endif
