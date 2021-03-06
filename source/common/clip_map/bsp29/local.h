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

#ifndef _CM29_LOCAL_H
#define _CM29_LOCAL_H

#include "../local.h"
#include "../../file_formats/bsp29.h"

#define MAX_MAP_HULLS       8

#define MAX_MAP_MODELS      256

#define HULL_NUMBER_MASK    ( MAX_MAP_HULLS - 1 )
#define MODEL_NUMBER_MASK   ( ~HULL_NUMBER_MASK )
#define BOX_HULL_HANDLE     ( MAX_MAP_MODELS * MAX_MAP_HULLS )

struct cnode_t {
	cplane_t* plane;
	int children[ 2 ];
};

struct cleaf_t {
	int contents;
	byte* compressed_vis;
	byte ambient_sound_level[ BSP29_NUM_AMBIENTS ];
};

struct cclipnode_t {
	int planenum;
	int children[ 2 ];					// negative numbers are contents
};

struct chullshared_t {
	vec3_t clip_mins;
	vec3_t clip_maxs;
};

struct chull_t {
	int firstclipnode;
	int lastclipnode;
};

struct cmodel_t {
	//
	// volume occupied by the model graphics
	//
	vec3_t mins;
	vec3_t maxs;

	chull_t hulls[ MAX_MAP_HULLS ];
};

class QClipMap29 : public QClipMap {
private:
	//	Main
	void InitBoxHull();
	cmodel_t* ClipHandleToModel( clipHandle_t Handle );
	chull_t* ClipHandleToHull( clipHandle_t Handle );
	int ContentsToQ2( int Contents ) const;
	int ContentsToQ3( int Contents ) const;

	//	Load
	void LoadVisibility( const quint8* base, const bsp_lump_t* l );
	void LoadEntities( const quint8* base, const bsp_lump_t* l );
	void LoadPlanes( const quint8* base, const bsp_lump_t* l );
	void LoadNodes( const quint8* base, const bsp_lump_t* l );
	void LoadLeafs( const quint8* base, const bsp_lump_t* l );
	void LoadClipnodes( const quint8* base, const bsp_lump_t* l );
	void MakeHull0();
	void MakeHulls();
	void LoadSubmodelsQ1( const quint8* base, const bsp_lump_t* l );
	void LoadSubmodelsH2( const quint8* base, const bsp_lump_t* l );

	//	Test
	void BoxLeafnums_r( leafList_t* ll, int NodeNum ) const;
	int HullPointContents( const chull_t* Hull, int NodeNum, const vec3_t P ) const;
	byte* DecompressVis( byte* in );
	void CalcPHS();

	//	Trace
	bool RecursiveHullCheck( chull_t* hull, int num, float p1f, float p2f,
		vec3_t p1, vec3_t p2, q1trace_t* trace );

public:
	static byte mod_novis[ BSP29_MAX_MAP_LEAFS / 8 ];

	int numplanes;
	cplane_t* planes;

	int numnodes;
	cnode_t* nodes;

	int numleafs;
	cleaf_t* leafs;

	int numclusters;

	int numclipnodes;
	cclipnode_t* clipnodes;

	byte* visdata;

	byte* phs;

	int entitychars;
	char* entitystring;

	int numsubmodels;
	cmodel_t* map_models;

	chullshared_t hullsshared[ MAX_MAP_HULLS ];

	cmodel_t box_model;

	QClipMap29();
	~QClipMap29();

	void LoadMap( const char* name, const idList<quint8>& Buffer );
	void ReloadMap( bool ClientLoad );
	clipHandle_t InlineModel( int Index ) const;
	int GetNumClusters() const;
	int GetNumInlineModels() const;
	const char* GetEntityString() const;
	void MapChecksums( int& CheckSum1, int& CheckSum2 ) const;
	int LeafCluster( int LeafNum ) const;
	int LeafArea( int LeafNum ) const;
	const byte* LeafAmbientSoundLevel( int LeafNum ) const;
	void ModelBounds( clipHandle_t Model, vec3_t Mins, vec3_t Maxs );
	int GetNumTextures() const;
	const char* GetTextureName( int Index ) const;
	clipHandle_t TempBoxModel( const vec3_t Mins, const vec3_t Maxs, bool Capsule );
	void SetTempBoxModelContents( clipHandle_t handle, int contents );
	clipHandle_t ModelHull( clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs );
	int PointLeafnum( const vec3_t P ) const;
	int BoxLeafnums( const vec3_t Mins, const vec3_t Maxs, int* List, int ListSize, int* TopNode, int* LastLeaf ) const;
	int PointContentsQ1( const vec3_t P, clipHandle_t Model );
	int PointContentsQ2( const vec3_t p, clipHandle_t Model );
	int PointContentsQ3( const vec3_t P, clipHandle_t Model );
	int TransformedPointContentsQ1( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles );
	int TransformedPointContentsQ2( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles );
	int TransformedPointContentsQ3( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles );
	bool HeadnodeVisible( int NodeNum, byte* VisBits );
	byte* ClusterPVS( int Cluster );
	byte* ClusterPHS( int Cluster );
	void SetAreaPortalState( int PortalNum, qboolean Open );
	void AdjustAreaPortalState( int Area1, int Area2, bool Open );
	qboolean AreasConnected( int Area1, int Area2 );
	int WriteAreaBits( byte* Buffer, int Area );
	void WritePortalState( fileHandle_t f );
	void ReadPortalState( fileHandle_t f );
	bool HullCheckQ1( clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace );
	q2trace_t BoxTraceQ2( const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask );
	q2trace_t TransformedBoxTraceQ2( const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles );
	void BoxTraceQ3( q3trace_t* Results, const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask, int Capsule );
	void TransformedBoxTraceQ3( q3trace_t* Results, const vec3_t Start, const vec3_t End, const vec3_t Mins, const vec3_t Maxs,
		clipHandle_t Model, int BrushMask, const vec3_t Origin, const vec3_t Angles, int Capsule );
	void DrawDebugSurface( void ( * drawPoly )( int color, int numPoints, float* points ) );
};

#endif
