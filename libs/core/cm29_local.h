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

#ifndef _CM29_LOCAL_H
#define _CM29_LOCAL_H

#define MAX_MAP_HULLS		8

#define	MAX_MAP_MODELS		256

#define MAX_CMOD_KNOWN		2048

enum cmodtype_t
{
	cmod_brush,
	cmod_sprite,
	cmod_alias
};

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

	cmodtype_t		type;

	//
	// volume occupied by the model graphics
	//
	vec3_t			mins;
	vec3_t			maxs;

	//
	// brush model
	//
	int				numsubmodels;

	int				numplanes;
	cplane_t*		planes;

	int				numleafs;		// number of visible leafs, not counting 0
	cleaf_t*		leafs;

	int				numnodes;
	cnode_t*		nodes;

	int				numclipnodes;
	bsp29_dclipnode_t*	clipnodes;

	chull_t			hulls[MAX_MAP_HULLS];

	byte*			visdata;
	byte*			phs;
	char*			entities;

	//QW only
	unsigned		checksum;
	unsigned		checksum2;

	void Free();
};

class QClipModel29
{
public:
	void LoadVisibility(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadEntities(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadPlanes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadNodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadLeafs(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadClipnodes(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void MakeHull0(cmodel_t* loadcmodel);
	void MakeHulls(cmodel_t* loadcmodel);
};

class QClipModelMap29 : public QClipModel29
{
public:
	cmodel_t	map_models[MAX_MAP_MODELS];

	~QClipModelMap29()
	{
		map_models[0].Free();
	}

	void LoadSubmodelsQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadSubmodelsH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
};

class QClipModelNonMap29 : public QClipModel29
{
public:
	cmodel_t	model;

	~QClipModelNonMap29()
	{
		model.Free();
	}

	void LoadSubmodelsNonMapQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadSubmodelsNonMapH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadBrushModelNonMap(cmodel_t* mod, void* buffer);
};

class QClipMap29
{
public:
	QClipModelMap29	Map;

	QClipModelNonMap29*	known[MAX_CMOD_KNOWN];
	int				numknown;

	chull_t			box_hull;
	bsp29_dclipnode_t	box_clipnodes[6];
	cplane_t		box_planes[6];

	QClipMap29()
	: numknown(0)
	{}
	~QClipMap29();

	void LoadModel(const char* name);
	void InitBoxHull();
};

#endif
