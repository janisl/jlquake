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

#ifndef _CM_LOCAL_H
#define _CM_LOCAL_H

struct leafList_t
{
	int		count;
	int		maxcount;
	int		*list;
	vec3_t	bounds[2];
	int		lastLeaf;		// for overflows where each leaf can't be stored individually
	int		topnode;
};

class QClipMap
{
public:
	virtual ~QClipMap();
	virtual clipHandle_t InlineModel(int Index) const = 0;
	virtual int GetNumClusters() const = 0;
	virtual int GetNumInlineModels() const = 0;
	virtual const char* GetEntityString() const = 0;
	virtual void MapChecksums(int& CheckSum1, int& CheckSum2) const = 0;
	virtual int LeafCluster(int LeafNum) const = 0;
	virtual int LeafArea(int LeafNum) const = 0;
	virtual const byte* LeafAmbientSoundLevel(int LeafNum) const = 0;
	virtual void ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs) = 0;
	virtual int GetNumTextures() const = 0;
	virtual const char* GetTextureName(int Index) const = 0;
	virtual clipHandle_t TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule) = 0;
	virtual clipHandle_t ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs) = 0;
	virtual int PointLeafnum(const vec3_t p) const = 0;
	virtual int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf) const = 0;
	virtual int PointContentsQ1(const vec3_t P, clipHandle_t Model) = 0;
	virtual int PointContentsQ2(const vec3_t p, clipHandle_t Model) = 0;
	virtual int PointContentsQ3(const vec3_t P, clipHandle_t Model) = 0;
	virtual int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual byte* ClusterPVS(int Cluster) = 0;
	virtual byte* ClusterPHS(int Cluster) = 0;
};

extern QClipMap*			CMapShared;

#endif
