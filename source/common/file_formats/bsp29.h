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

#ifndef _BSP29FILE_H
#define _BSP29FILE_H

#define BSP29_VERSION			29

#define BSP29LUMP_ENTITIES		0
#define BSP29LUMP_PLANES		1
#define BSP29LUMP_TEXTURES		2
#define BSP29LUMP_VERTEXES		3
#define BSP29LUMP_VISIBILITY	4
#define BSP29LUMP_NODES			5
#define BSP29LUMP_TEXINFO		6
#define BSP29LUMP_FACES			7
#define BSP29LUMP_LIGHTING		8
#define BSP29LUMP_CLIPNODES		9
#define BSP29LUMP_LEAFS			10
#define BSP29LUMP_MARKSURFACES	11
#define BSP29LUMP_EDGES			12
#define BSP29LUMP_SURFEDGES		13
#define BSP29LUMP_MODELS		14

#define BSP29_HEADER_LUMPS		15

#define BSP29_MIPLEVELS			4

#define BSP29TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

#define BSP29_MAXLIGHTMAPS		4

#define BSP29AMBIENT_WATER		0
#define BSP29AMBIENT_SKY		1
#define BSP29AMBIENT_SLIME		2
#define BSP29AMBIENT_LAVA		3

#define BSP29_NUM_AMBIENTS		4		// automatic ambient sounds

#define BSP29_MAX_MAP_LEAFS		32767

//	This is where Quake and Hexen 2 maps differ.
#define BSP29_MAX_MAP_HULLS_Q1	4
#define BSP29_MAX_MAP_HULLS_H2	8

struct bsp29_lump_t
{
	qint32		fileofs;
	qint32		filelen;
};

struct bsp29_dheader_t
{
	qint32			version;	
	bsp29_lump_t	lumps[BSP29_HEADER_LUMPS];
};

struct bsp29_dplane_t
{
	float		normal[3];
	float		dist;
	qint32		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};

struct bsp29_dmiptexlump_t
{
	qint32		nummiptex;
	qint32		dataofs[4];		// [nummiptex]
};

struct bsp29_miptex_t
{
	char		name[16];
	quint32		width;
	quint32		height;
	quint32		offsets[BSP29_MIPLEVELS];		// four mip maps stored
};

struct bsp29_dvertex_t
{
	float		point[3];
};

struct bsp29_dnode_t
{
	qint32		planenum;
	qint16		children[2];	// negative numbers are -(leafs+1), not nodes
	qint16		mins[3];		// for sphere culling
	qint16		maxs[3];
	quint16		firstface;
	quint16		numfaces;	// counting both sides
};

struct bsp29_texinfo_t
{
	float		vecs[2][4];		// [s/t][xyz offset]
	qint32		miptex;
	qint32		flags;
};

struct bsp29_dface_t
{
	qint16		planenum;
	qint16		side;

	qint32		firstedge;		// we must support > 64k edges
	qint16		numedges;	
	qint16		texinfo;

	// lighting info
	quint8		styles[BSP29_MAXLIGHTMAPS];
	qint32		lightofs;		// start of [numstyles*surfsize] samples
};

struct bsp29_dclipnode_t
{
	qint32		planenum;
	qint16		children[2];	// negative numbers are contents
};

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
struct bsp29_dleaf_t
{
	qint32		contents;
	qint32		visofs;				// -1 = no visibility info

	qint16		mins[3];			// for frustum culling
	qint16		maxs[3];

	quint16		firstmarksurface;
	quint16		nummarksurfaces;

	quint8		ambient_level[BSP29_NUM_AMBIENTS];
};

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct bsp29_dedge_t
{
	quint16		v[2];		// vertex numbers
};

struct bsp29_dmodel_q1_t
{
	float		mins[3];
	float		maxs[3];
	float		origin[3];
	qint32		headnode[BSP29_MAX_MAP_HULLS_Q1];
	qint32		visleafs;		// not including the solid leaf 0
	qint32		firstface;
	qint32		numfaces;
};

struct bsp29_dmodel_h2_t
{
	float		mins[3];
	float		maxs[3];
	float		origin[3];
	qint32		headnode[BSP29_MAX_MAP_HULLS_H2];
	qint32		visleafs;		// not including the solid leaf 0
	qint32		firstface;
	qint32		numfaces;
};

#endif
