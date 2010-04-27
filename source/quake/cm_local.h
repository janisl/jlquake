//**************************************************************************
//**
//**	$Id: endian.cpp 101 2010-04-03 23:06:31Z dj_jl $
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

#define MAX_MAP_HULLS_			8

struct cnode_t
{
	// common with leaf
	int			contents;		// 0, to differentiate from leafs

	// node specific
	cplane_t*	plane;
	cnode_t*	children[2];	
};

struct cleaf_t
{
	// common with node
	int			contents;		// wil be a negative contents number

	// leaf specific
	byte*		compressed_vis;
	byte		ambient_sound_level[BSP29_NUM_AMBIENTS];
};

// !!! if this is changed, it must be changed in asm_i386.h too !!!
struct chull_t
{
	bsp29_dclipnode_t*	clipnodes;
	cplane_t*		planes;
	int				firstclipnode;
	int				lastclipnode;
	vec3_t			clip_mins;
	vec3_t			clip_maxs;
};

struct cmodel_t
{
	char			name[MAX_QPATH];
	bool			needload;		// bmodels and sprites don't cache normally

	modtype_t		type;

	//
	// volume occupied by the model graphics
	//
	vec3_t			mins;
	vec3_t			maxs;

	//
	// brush model
	//
	int				numsubmodels;
	//	Use Hexen 2 type as it has biggest number of hulls.
	bsp29_dmodel_h2_t*	submodels;

	int				numplanes;
	cplane_t*		planes;

	int				numleafs;		// number of visible leafs, not counting 0
	cleaf_t*		leafs;

	int				numnodes;
	cnode_t*		nodes;

	int				numclipnodes;
	bsp29_dclipnode_t*	clipnodes;

	chull_t			hulls[MAX_MAP_HULLS_];

	byte*			visdata;
	char*			entities;

	//QW only
	unsigned		checksum;
	unsigned		checksum2;

	void Free();
};
