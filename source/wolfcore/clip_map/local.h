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

#ifndef _CM_LOCAL_H
#define _CM_LOCAL_H

#define CMH_NON_MAP_MASK	0xffff0000
#define CMH_NON_MAP_SHIFT	16
#define CMH_MODEL_MASK		0x0000ffff

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
	String			Name;

	quint32			CheckSum;
	quint32			CheckSum2;
	
	QClipMap();
	virtual ~QClipMap();
	virtual void LoadMap(const char* name, const Array<quint8>& Buffer) = 0;
#if 0
	virtual void ReloadMap(bool ClientLoad) = 0;
#endif
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
	virtual void SetTempBoxModelContents(int contents) = 0;
	virtual clipHandle_t ModelHull(clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs) = 0;
	virtual int PointLeafnum(const vec3_t p) const = 0;
	virtual int BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf) const = 0;
	virtual int PointContentsQ1(const vec3_t P, clipHandle_t Model) = 0;
	virtual int PointContentsQ2(const vec3_t p, clipHandle_t Model) = 0;
	virtual int PointContentsQ3(const vec3_t P, clipHandle_t Model) = 0;
	virtual int TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual int TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual int TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles) = 0;
	virtual bool HeadnodeVisible(int NodeNum, byte *VisBits) = 0;
	virtual byte* ClusterPVS(int Cluster) = 0;
	virtual byte* ClusterPHS(int Cluster) = 0;
	virtual void SetAreaPortalState(int PortalNum, qboolean Open) = 0;
	virtual void AdjustAreaPortalState(int Area1, int Area2, bool Open) = 0;
	virtual qboolean AreasConnected(int Area1, int Area2) = 0;
	virtual int WriteAreaBits(byte* Buffer, int Area) = 0;
	virtual void WritePortalState(fileHandle_t f) = 0;
	virtual void ReadPortalState(fileHandle_t f) = 0;
	virtual bool HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace) = 0;
	virtual q2trace_t BoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask) = 0;
	virtual q2trace_t TransformedBoxTraceQ2(vec3_t Start, vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model,
		int BrushMask, vec3_t Origin, vec3_t Angles) = 0;
	virtual void BoxTraceQ3(q3trace_t* Results, const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask, int Capsule) = 0;
	virtual void TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule) = 0;
	virtual void DrawDebugSurface(void (*drawPoly)(int color, int numPoints, float *points)) = 0;
};

#if 0
QClipMap* CM_CreateQClipMap29();
QClipMap* CM_CreateQClipMap38();
#endif
QClipMap* CM_CreateQClipMap46();

extern QClipMap*			CMapShared;

#if 0
extern Array<QClipMap*>	CMNonMapModels;

extern Cvar*				cm_flushmap;
#endif

#endif
