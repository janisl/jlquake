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

#include "cm_local.h"
#include "bsp29file.h"

#define MAX_MAP_HULLS		8

#define	MAX_MAP_MODELS		256

#define MAX_CMOD_KNOWN		2048

#define HULL_NUMBER_MASK	(MAX_MAP_HULLS - 1)
#define MODEL_NUMBER_MASK	(~HULL_NUMBER_MASK)
#define BOX_HULL_HANDLE		((MAX_MAP_MODELS + MAX_CMOD_KNOWN) * MAX_MAP_HULLS)

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
private:
	void LoadBrushModelNonMap(cmodel_t* mod, void* buffer);
	void LoadSubmodelsNonMapQ1(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadSubmodelsNonMapH2(cmodel_t* loadcmodel, const quint8* base, const bsp29_lump_t* l);
	void LoadAliasModel(cmodel_t* mod, void* buffer);
	void LoadAliasModelNew(cmodel_t* mod, void* buffer);
	void LoadSpriteModel(cmodel_t* mod, void* buffer);

public:
	cmodel_t	model;

	~QClipModelNonMap29()
	{
		model.Free();
	}

	void Load(const char* name);
};

class QClipMap29 : public QClipMap
{
private:
	void InitBoxHull();
	int ContentsToQ2(int Contents) const;
	int ContentsToQ3(int Contents) const;

	byte* DecompressVis(byte* in);

public:
	static byte			mod_novis[BSP29_MAX_MAP_LEAFS / 8];

	QClipModelMap29		Map;

	QClipModelNonMap29*	known[MAX_CMOD_KNOWN];
	int					numknown;

	cmodel_t			box_model;
	bsp29_dclipnode_t	box_clipnodes[6];
	cplane_t			box_planes[6];

	QClipMap29();
	~QClipMap29();

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
	byte* ClusterPVS(int Cluster);
	byte* ClusterPHS(int Cluster);
	void SetAreaPortalState(int PortalNum, qboolean Open);
	void AdjustAreaPortalState(int Area1, int Area2, bool Open);
	qboolean AreasConnected(int Area1, int Area2);
	int WriteAreaBits(byte* Buffer, int Area);
	void WritePortalState(fileHandle_t f);
	void ReadPortalState(fileHandle_t f);

	void LoadModel(const char* name);
	cmodel_t* ClipHandleToModel(clipHandle_t Handle);
	chull_t* ClipHandleToHull(clipHandle_t Handle);
	void BoxLeafnums_r(leafList_t* ll, const cnode_t *node) const;
	int HullPointContents(const chull_t* Hull, int NodeNum, const vec3_t P) const;
	void CalcPHS();
};

#endif
