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

#ifndef _CM46_LOCAL_H
#define _CM46_LOCAL_H

#include "cm_local.h"
#include "bsp46file.h"

#define MAX_FACETS			1024
#define MAX_PATCH_PLANES	2048
#define PLANE_TRI_EPSILON	0.1

#define SIDE_FRONT	0
#define SIDE_BACK	1
#define SIDE_ON		2
#define SIDE_CROSS	3

#define MAX_MAP_BOUNDS			65535

#define	MAX_SUBMODELS			256

#define	BOX_MODEL_HANDLE		255
#define CAPSULE_MODEL_HANDLE	254

// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

struct cLeaf_t
{
	int			cluster;
	int			area;

	int			firstLeafBrush;
	int			numLeafBrushes;

	int			firstLeafSurface;
	int			numLeafSurfaces;
};

struct cArea_t
{
	int			floodnum;
	int			floodvalid;
};

struct cbrushside_t
{
	cplane_t*	plane;
	int			surfaceFlags;
	int			shaderNum;
};

struct cbrush_t
{
	int			shaderNum;		// the shader that determined the contents
	int			contents;
	vec3_t		bounds[2];
	int			numsides;
	cbrushside_t*	sides;
	int			checkcount;		// to avoid repeated testings
};

struct cNode_t
{
	cplane_t*	plane;
	int			children[2];		// negative numbers are leafs
};

// Used for oriented capsule collision detection
struct sphere_t
{
	bool		use;
	float		radius;
	float		halfheight;
	vec3_t		offset;
};

struct traceWork_t
{
	vec3_t		start;
	vec3_t		end;
	vec3_t		size[2];	// size of the box being swept through the model
	vec3_t		offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float		maxOffset;	// longest corner length from origin
	vec3_t		extents;	// greatest of abs(size[0]) and abs(size[1])
	vec3_t		bounds[2];	// enclosing box of start and end surrounding by size
	vec3_t		modelOrigin;// origin of the model tracing through
	int			contents;	// ored contents of the model tracing through
	qboolean	isPoint;	// optimized case
	q3trace_t	trace;		// returned from trace call
	sphere_t	sphere;		// sphere for oriendted capsule collision
};

struct patchPlane_t
{
	float		plane[4];
	int			signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
};

struct facet_t
{
	int			surfacePlane;
	int			numBorders;		// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
	int			borderPlanes[4 + 6 + 16];
	int			borderInward[4 + 6 + 16];
	qboolean	borderNoAdjust[4 + 6 + 16];
};

#define	MAX_GRID_SIZE	129

struct cGrid_t
{
	int			width;
	int			height;
	bool		wrapWidth;
	bool		wrapHeight;
	vec3_t		points[MAX_GRID_SIZE][MAX_GRID_SIZE];	// [width][height]

	void SetWrapWidth();
	void SubdivideColumns();
	void RemoveDegenerateColumns();
	void Transpose();
private:
	static bool NeedsSubdivision(vec3_t a, vec3_t b, vec3_t c);
	static void Subdivide(vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3);
	static bool ComparePoints(const float* a, const float* b);
};

struct patchCollide_t
{
	vec3_t		bounds[2];
	int			numPlanes;			// surface planes plus edge planes
	patchPlane_t*	planes;
	int			numFacets;
	facet_t*	facets;

	void FromGrid(cGrid_t* grid);
	void TraceThrough(traceWork_t* tw) const;
	bool PositionTest(traceWork_t* tw) const;
private:
	static int FindPlane(float* p1, float* p2, float* p3);
	static int SignbitsForNormal(vec3_t normal);
	static int EdgePlaneNum(cGrid_t* grid, int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int k);
	static int GridPlane(int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int tri);
	static void SetBorderInward(facet_t* facet, cGrid_t* grid,
		int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int which);
	static int PointOnPlaneSide(const float* p, int planeNum);
	static bool ValidateFacet(facet_t* facet);
	static void AddFacetBevels(facet_t* facet);
	static bool PlaneEqual(patchPlane_t* p, float plane[4], int* flipped);
	static int FindPlane2(float plane[4], int* flipped);
	static void CM_SnapVector(vec3_t normal);
	void TracePointThrough(traceWork_t* tw) const;
	static int CheckFacetPlane(float* plane, vec3_t start, vec3_t end, float* enterFrac, float* leaveFrac, int* hit);
};

struct cPatch_t
{
	int			checkcount;				// to avoid repeated testings
	int			surfaceFlags;
	int			contents;
	patchCollide_t*	pc;
};

struct winding_t
{
	int			numpoints;
	vec3_t		p[4];		// variable sized
};

struct cmodel_t
{
	vec3_t		mins;
	vec3_t		maxs;
	cLeaf_t		leaf;			// submodels don't reference the main tree
};

class QClipMap46 : public QClipMap
{
private:
	//	Main
	void InitBoxHull();
	int ContentsToQ1(int Contents) const;
	int ContentsToQ2(int Contents) const;

	//	Load
	void LoadShaders(const quint8* base, const bsp46_lump_t* l);
	void LoadLeafs(const quint8* base, const bsp46_lump_t* l);
	void LoadLeafBrushes(const quint8* base, const bsp46_lump_t* l);
	void LoadLeafSurfaces(const quint8* base, const bsp46_lump_t* l);
	void LoadPlanes(const quint8* base, const bsp46_lump_t* l);
	void LoadBrushSides(const quint8* base, const bsp46_lump_t* l);
	void LoadBrushes(const quint8* base, const bsp46_lump_t* l);
	void LoadNodes(const quint8* base, const bsp46_lump_t* l);
	void LoadVisibility(const quint8* base, const bsp46_lump_t* l);
	void LoadEntityString(const quint8* base, const bsp46_lump_t* l);
	void LoadPatches(const quint8* base, const bsp46_lump_t* surfs, const bsp46_lump_t* verts);
	void LoadSubmodels(const quint8* base, const bsp46_lump_t* l);

	//	Patch
	static patchCollide_t* GeneratePatchCollide(int width, int height, vec3_t* points);

public:
	char		name[MAX_QPATH];
	unsigned	checksum;

	int			numShaders;
	bsp46_dshader_t*	shaders;

	int			numLeafs;
	cLeaf_t*	leafs;

	int			numLeafBrushes;
	int*		leafbrushes;

	int			numLeafSurfaces;
	int*		leafsurfaces;

	int			numPlanes;
	cplane_t	*planes;

	int			numBrushSides;
	cbrushside_t*	brushsides;

	int			numBrushes;
	cbrush_t	*brushes;

	int			numNodes;
	cNode_t*	nodes;

	int			numAreas;
	cArea_t*	areas;
	int*		areaPortals;	// [ numAreas*numAreas ] reference counts

	int			numClusters;
	int			clusterBytes;
	byte*		visibility;
	bool		vised;			// if false, visibility is just a single cluster of ffs

	int			numEntityChars;
	char*		entityString;

	int			numSurfaces;
	cPatch_t**	surfaces;			// non-patches will be NULL

	int			numSubModels;
	cmodel_t	*cmodels;

	cmodel_t	box_model;
	cplane_t*	box_planes;
	cbrush_t*	box_brush;

	int			floodvalid;
	int			checkcount;					// incremented on each trace

	QClipMap46()
	: checksum(0)
	, numShaders(0)
	, shaders(NULL)
	, numLeafs(0)
	, leafs(NULL)
	, numLeafBrushes(0)
	, leafbrushes(NULL)
	, numLeafSurfaces(0)
	, leafsurfaces(NULL)
	, numPlanes(0)
	, planes(NULL)
	, numBrushSides(0)
	, brushsides(NULL)
	, numBrushes(0)
	, brushes(NULL)
	, numNodes(0)
	, nodes(NULL)
	, numAreas(0)
	, areas(NULL)
	, areaPortals(NULL)
	, numClusters(0)
	, clusterBytes(0)
	, visibility(NULL)
	, vised(false)
	, numEntityChars(0)
	, entityString(NULL)
	, numSurfaces(0)
	, surfaces(NULL)
	, numSubModels(0)
	, cmodels(NULL)
	, box_planes(NULL)
	, box_brush(NULL)
	, floodvalid(0)
	, checkcount(0)
	{
		name[0] = 0;
	}
	~QClipMap46();

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
	int PointLeafnum(const vec3_t p) const;
	int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int *List, int ListSize, int *TopNode, int *LastLeaf) const;
	int PointContentsQ1(const vec3_t P, clipHandle_t Model);
	int PointContentsQ2(const vec3_t p, clipHandle_t Model);
	int PointContentsQ3(const vec3_t P, clipHandle_t Model);
	int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
	byte* ClusterPVS(int Cluster);
	byte* ClusterPHS(int Cluster);

	void LoadMap(const char* name);
	cmodel_t* ClipHandleToModel(clipHandle_t Handle);
	int PointLeafnum_r(const vec3_t P, int Num) const;
	void StoreLeafs(leafList_t* ll, int NodeNum) const;
	void BoxLeafnums_r(leafList_t* ll, int nodenum) const;
};

void CM46_FreeWinding(winding_t* w);
winding_t* CM46_BaseWindingForPlane(vec3_t normal, vec_t dist);
void CM46_ChopWindingInPlace(winding_t** inout, vec3_t normal, vec_t dist, vec_t epsilon);
// frees the original if clipped
void CM46_WindingBounds(winding_t* w, vec3_t mins, vec3_t maxs);
winding_t* CM46_CopyWinding(winding_t* w);

extern	QCvar		*cm_noAreas;
extern	QCvar		*cm_noCurves;
extern	QCvar		*cm_playerCurveClip;

#endif
