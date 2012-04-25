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

#ifndef _BSP46FILE_H
#define _BSP46FILE_H

// little-endian "IBSP"
#define BSP46_IDENT             (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I')

#define BSP46_VERSION           46

#define BSP46LUMP_ENTITIES      0
#define BSP46LUMP_SHADERS       1
#define BSP46LUMP_PLANES        2
#define BSP46LUMP_NODES         3
#define BSP46LUMP_LEAFS         4
#define BSP46LUMP_LEAFSURFACES  5
#define BSP46LUMP_LEAFBRUSHES   6
#define BSP46LUMP_MODELS        7
#define BSP46LUMP_BRUSHES       8
#define BSP46LUMP_BRUSHSIDES    9
#define BSP46LUMP_DRAWVERTS     10
#define BSP46LUMP_DRAWINDEXES   11
#define BSP46LUMP_FOGS          12
#define BSP46LUMP_SURFACES      13
#define BSP46LUMP_LIGHTMAPS     14
#define BSP46LUMP_LIGHTGRID     15
#define BSP46LUMP_VISIBILITY    16

#define BSP46_HEADER_LUMPS      17

#define BSP46SURF_NODAMAGE          0x1		// never give falling damage
#define BSP46SURF_SLICK             0x2		// effects game physics
#define BSP46SURF_SKY               0x4		// lighting from environment map
#define BSP46SURF_LADDER            0x8
#define BSP46SURF_NOIMPACT          0x10	// don't make missile explosions
#define BSP46SURF_NOMARKS           0x20	// don't leave missile marks
#define BSP46SURF_FLESH             0x40	// make flesh sounds and effects
#define BSP46SURF_NODRAW            0x80	// don't generate a drawsurface at all
#define BSP46SURF_HINT              0x100	// make a primary bsp splitter
#define BSP46SURF_SKIP              0x200	// completely ignore, allowing non-closed brushes
#define BSP46SURF_NOLIGHTMAP        0x400	// surface doesn't need a lightmap
#define BSP46SURF_POINTLIGHT        0x800	// generate lighting info at vertexes
#define BSP46SURF_METALSTEPS        0x1000	// clanking footsteps
#define BSP46SURF_NOSTEPS           0x2000	// no footstep sounds
#define BSP46SURF_NONSOLID          0x4000	// don't collide against curves with this set
#define BSP46SURF_LIGHTFILTER       0x8000	// act as a light filter during q3map -light
#define BSP46SURF_ALPHASHADOW       0x10000	// do per-pixel light shadow casting in q3map
#define BSP46SURF_NODLIGHT          0x20000	// don't dlight even if solid (solid lava, skies)
#define BSP46SURF_DUST              0x40000	// leave a dust trail when walking on this surface

enum bsp46_mapSurfaceType_t
{
	BSP46MST_BAD,
	BSP46MST_PLANAR,
	BSP46MST_PATCH,
	BSP46MST_TRIANGLE_SOUP,
	BSP46MST_FLARE
};

struct bsp46_lump_t
{
	qint32 fileofs;
	qint32 filelen;
};

struct bsp46_dheader_t
{
	qint32 ident;
	qint32 version;

	bsp46_lump_t lumps[BSP46_HEADER_LUMPS];
};

struct bsp46_dmodel_t
{
	float mins[3];
	float maxs[3];
	qint32 firstSurface;
	qint32 numSurfaces;
	qint32 firstBrush;
	qint32 numBrushes;
};

struct bsp46_dshader_t
{
	char shader[MAX_QPATH];
	qint32 surfaceFlags;
	qint32 contentFlags;
};

// planes x^1 is allways the opposite of plane x

struct bsp46_dplane_t
{
	float normal[3];
	float dist;
};

struct bsp46_dnode_t
{
	qint32 planeNum;
	qint32 children[2];			// negative numbers are -(leafs+1), not nodes
	qint32 mins[3];				// for frustom culling
	qint32 maxs[3];
};

struct bsp46_dleaf_t
{
	qint32 cluster;					// -1 = opaque cluster (do I still store these?)
	qint32 area;

	qint32 mins[3];					// for frustum culling
	qint32 maxs[3];

	qint32 firstLeafSurface;
	qint32 numLeafSurfaces;

	qint32 firstLeafBrush;
	qint32 numLeafBrushes;
};

struct bsp46_dbrushside_t
{
	qint32 planeNum;				// positive plane side faces out of the leaf
	qint32 shaderNum;
};

struct bsp46_dbrush_t
{
	qint32 firstSide;
	qint32 numSides;
	qint32 shaderNum;			// the shader that determines the contents flags
};

struct bsp46_dfog_t
{
	char shader[MAX_QPATH];
	qint32 brushNum;
	qint32 visibleSide;			// the brush side that ray tests need to clip against (-1 == none)
};

struct bsp46_drawVert_t
{
	vec3_t xyz;
	float st[2];
	float lightmap[2];
	vec3_t normal;
	quint8 color[4];
};

struct bsp46_dsurface_t
{
	qint32 shaderNum;
	qint32 fogNum;
	qint32 surfaceType;

	qint32 firstVert;
	qint32 numVerts;

	qint32 firstIndex;
	qint32 numIndexes;

	qint32 lightmapNum;
	qint32 lightmapX;
	qint32 lightmapY;
	qint32 lightmapWidth;
	qint32 lightmapHeight;

	vec3_t lightmapOrigin;
	vec3_t lightmapVecs[3];			// for patches, [0] and [1] are lodbounds

	qint32 patchWidth;
	qint32 patchHeight;
};

#endif
