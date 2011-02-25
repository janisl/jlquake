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

typedef int clipHandle_t;

clipHandle_t CM_InlineModel(int Index);		// 0 = world, 1 + are bmodels

int CM_NumClusters();
int CM_NumInlineModels();
const char* CM_EntityString();
void CM_MapChecksums(int& CheckSum1, int& CheckSum2);

void CM_ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs);

int CM_LeafCluster(int LeafNum);
int CM_LeafArea(int LeafNum);
const byte* CM_LeafAmbientSoundLevel(int LeafNum);

int CM_GetNumTextures();
const char* CM_GetTextureName(int Index);

// creates a clipping hull for an arbitrary box
clipHandle_t CM_TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule = false);

clipHandle_t CM_ModelHull(clipHandle_t Model, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs);
clipHandle_t CM_ModelHull(clipHandle_t Model, int HullNum);

int CM_PointLeafnum(const vec3_t Point);

// only returns non-solid leafs
// returns with TopNode set to the first node that splits the box
// overflow if return ListSize and if *LastLeaf != List[ListSize - 1]
int CM_BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode = NULL, int* LastLeaf = NULL);

// returns an ORed contents mask
int CM_PointContentsQ1(const vec3_t Point, clipHandle_t Model);
int CM_PointContentsQ2(const vec3_t Point, clipHandle_t Model);
int CM_PointContentsQ3(const vec3_t Point, clipHandle_t Model);
int CM_TransformedPointContentsQ1(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
int CM_TransformedPointContentsQ2(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
int CM_TransformedPointContentsQ3(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);

byte* CM_ClusterPVS(int Cluster);
byte* CM_ClusterPHS(int Cluster);

extern	int			c_pointcontents;
