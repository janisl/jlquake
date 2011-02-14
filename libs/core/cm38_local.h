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

struct mapsurface_t	// used internally due to name len probs //ZOID
{
	csurface_t	c;
	char		rname[32];
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

class QClipMap38
{
private:
	//	Map loading
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

	//	Areaportals
	void FloodArea(carea_t* area, int floodnum);

public:
	char			name[MAX_QPATH];

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
	int				box_headnode;
	cbrush_t*		box_brush;
	cleaf_t*		box_leaf;

	unsigned		checksum;

	static mapsurface_t	nullsurface;

	qboolean		portalopen[BSP38MAX_MAP_AREAPORTALS];

	int				floodvalid;

	QClipMap38()
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
	, box_headnode(0)
	, box_brush(NULL)
	, box_leaf(NULL)
	, checksum(0)
	, floodvalid(0)
	{
		name[0] = 0;
	}
	~QClipMap38();

	void InitBoxHull();
	void LoadMap(const char* name);
	void FloodAreaConnections();
	void ClearPortalOpen();
};

#endif
