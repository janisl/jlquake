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

#ifndef _BSP46FILE_H
#define _BSP46FILE_H

// little-endian "IBSP"
#define BSP38_HEADER	(('P' << 24) + ('S' << 16) + ('B' << 8) + 'I')

#define BSP38_VERSION	38

#define BSP38LUMP_ENTITIES		0
#define BSP38LUMP_PLANES		1
#define BSP38LUMP_VERTEXES		2
#define BSP38LUMP_VISIBILITY	3
#define BSP38LUMP_NODES			4
#define BSP38LUMP_TEXINFO		5
#define BSP38LUMP_FACES			6
#define BSP38LUMP_LIGHTING		7
#define BSP38LUMP_LEAFS			8
#define BSP38LUMP_LEAFFACES		9
#define BSP38LUMP_LEAFBRUSHES	10
#define BSP38LUMP_EDGES			11
#define BSP38LUMP_SURFEDGES		12
#define BSP38LUMP_MODELS		13
#define BSP38LUMP_BRUSHES		14
#define BSP38LUMP_BRUSHSIDES	15
#define BSP38LUMP_POP			16
#define BSP38LUMP_AREAS			17
#define BSP38LUMP_AREAPORTALS	18
#define BSP38HEADER_LUMPS		19

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define BSP38MAX_MAP_MODELS		1024
#define BSP38MAX_MAP_BRUSHES		8192
#define BSP38MAX_MAP_ENTSTRING	0x40000
#define BSP38MAX_MAP_TEXINFO		8192

#define BSP38MAX_MAP_AREAS		256
#define BSP38MAX_MAP_AREAPORTALS	1024
#define BSP38MAX_MAP_PLANES		65536
#define BSP38MAX_MAP_NODES		65536
#define BSP38MAX_MAP_BRUSHSIDES	65536
#define BSP38MAX_MAP_LEAFS		65536
#define BSP38MAX_MAP_LEAFBRUSHES 65536
#define BSP38MAX_MAP_SURFEDGES	256000
#define BSP38MAX_MAP_VISIBILITY	0x100000

#define BSP38_MAXLIGHTMAPS		4

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely
#define BSP38CONTENTS_SOLID			1		// an eye is never valid in a solid
#define BSP38CONTENTS_WINDOW		2		// translucent, but not watery
#define BSP38CONTENTS_AUX			4
#define BSP38CONTENTS_LAVA			8
#define BSP38CONTENTS_SLIME			16
#define BSP38CONTENTS_WATER			32
#define BSP38CONTENTS_MIST			64
#define BSP38_LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define BSP38CONTENTS_AREAPORTAL	0x8000

#define BSP38CONTENTS_PLAYERCLIP	0x10000
#define BSP38CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define BSP38CONTENTS_CURRENT_0		0x40000
#define BSP38CONTENTS_CURRENT_90	0x80000
#define BSP38CONTENTS_CURRENT_180	0x100000
#define BSP38CONTENTS_CURRENT_270	0x200000
#define BSP38CONTENTS_CURRENT_UP	0x400000
#define BSP38CONTENTS_CURRENT_DOWN	0x800000

#define BSP38CONTENTS_ORIGIN		0x1000000	// removed before bsping an entity

#define BSP38CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define BSP38CONTENTS_DEADMONSTER	0x4000000
#define BSP38CONTENTS_DETAIL		0x8000000	// brushes to be added after vis leafs
#define BSP38CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define BSP38CONTENTS_LADDER		0x20000000

#define BSP38SURF_LIGHT		0x1		// value will hold the light strength
#define BSP38SURF_SLICK		0x2		// effects game physics
#define BSP38SURF_SKY		0x4		// don't draw, but add to skybox
#define BSP38SURF_WARP		0x8		// turbulent water warp
#define BSP38SURF_TRANS33	0x10
#define BSP38SURF_TRANS66	0x20
#define BSP38SURF_FLOWING	0x40	// scroll towards angle
#define BSP38SURF_NODRAW	0x80	// don't bother referencing the texture

#define BSP38DVIS_PVS	0
#define BSP38DVIS_PHS	1

struct bsp38_lump_t
{
	qint32		fileofs;
	qint32		filelen;
};

struct bsp38_dheader_t
{
	qint32			ident;
	qint32			version;	
	bsp38_lump_t	lumps[BSP38HEADER_LUMPS];
};

struct bsp38_dmodel_t
{
	float		mins[3];
	float		maxs[3];
	float		origin[3];		// for sounds or lights
	qint32		headnode;
	qint32		firstface;		// submodels just draw faces
	qint32		numfaces;		// without walking the bsp tree
};

struct bsp38_dvertex_t
{
	float		point[3];
};

// planes (x&~1) and (x&~1)+1 are always opposites
struct bsp38_dplane_t
{
	float		normal[3];
	float		dist;
	qint32		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};

struct bsp38_dnode_t
{
	qint32		planenum;
	qint32		children[2];	// negative numbers are -(leafs+1), not nodes
	qint16		mins[3];		// for frustom culling
	qint16		maxs[3];
	quint16		firstface;
	quint16		numfaces;	// counting both sides
};

struct bsp38_texinfo_t
{
	float		vecs[2][4];		// [s/t][xyz offset]
	qint32		flags;			// miptex flags + overrides
	qint32		value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.wal)
	qint32		nexttexinfo;	// for animations, -1 = end of chain
};

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct bsp38_dedge_t
{
	quint16		v[2];		// vertex numbers
};

struct bsp38_dface_t
{
	quint16		planenum;
	qint16		side;

	qint32		firstedge;		// we must support > 64k edges
	qint16		numedges;	
	qint16		texinfo;

	// lighting info
	quint8		styles[BSP38_MAXLIGHTMAPS];
	qint32		lightofs;		// start of [numstyles*surfsize] samples
};

struct bsp38_dleaf_t
{
	qint32		contents;			// OR of all brushes (not needed?)

	qint16		cluster;
	qint16		area;

	qint16		mins[3];			// for frustum culling
	qint16		maxs[3];

	quint16		firstleafface;
	quint16		numleaffaces;

	quint16		firstleafbrush;
	quint16		numleafbrushes;
};

struct bsp38_dbrushside_t
{
	quint16		planenum;		// facing out of the leaf
	qint16		texinfo;
};

struct bsp38_dbrush_t
{
	qint32		firstside;
	qint32		numsides;
	qint32		contents;
};

// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
struct bsp38_dvis_t
{
	qint32		numclusters;
	qint32		bitofs[8][2];	// bitofs[numclusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct bsp38_dareaportal_t
{
	qint32		portalnum;
	qint32		otherarea;
};

struct bsp38_darea_t
{
	qint32		numareaportals;
	qint32		firstareaportal;
};

#endif
