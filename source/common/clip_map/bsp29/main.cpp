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

// HEADER FILES ------------------------------------------------------------

#include "../../qcommon.h"
#include "../../Common.h"
#include "../../content_types.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

byte QClipMap29::mod_novis[ BSP29_MAX_MAP_LEAFS / 8 ];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_CreateQClipMap29
//
//==========================================================================

QClipMap* CM_CreateQClipMap29() {
	return new QClipMap29();
}

//==========================================================================
//
//	QClipMap29::QClipMap29
//
//==========================================================================

QClipMap29::QClipMap29()
	: numplanes( 0 ),
	planes( NULL ),
	numnodes( 0 ),
	nodes( NULL ),
	numleafs( 0 ),
	leafs( NULL ),
	numclusters( 0 ),
	numclipnodes( 0 ),
	clipnodes( NULL ),
	visdata( NULL ),
	phs( NULL ),
	entitychars( 0 ),
	entitystring( NULL ),
	numsubmodels( 0 ),
	map_models( NULL ) {
	Com_Memset( mod_novis, 0xff, sizeof ( mod_novis ) );
	Com_Memset( hullsshared, 0, sizeof ( hullsshared ) );
}

//==========================================================================
//
//	QClipMap29::~QClipMap29
//
//==========================================================================

QClipMap29::~QClipMap29() {
	delete[] planes;
	delete[] nodes;
	delete[] leafs;
	delete[] clipnodes;
	delete[] visdata;
	delete[] entitystring;
	delete[] phs;
	delete[] map_models;
}

//==========================================================================
//
//	QClipMap29::InlineModel
//
//==========================================================================

clipHandle_t QClipMap29::InlineModel( int Index ) const {
	if ( Index < 1 || Index > numsubmodels ) {
		common->Error( "Bad submodel index" );
	}
	return Index * MAX_MAP_HULLS;
}

//==========================================================================
//
//	QClipMap29::GetNumClusters
//
//==========================================================================

int QClipMap29::GetNumClusters() const {
	return numclusters;
}

//==========================================================================
//
//	QClipMap29::GetNumInlineModels
//
//==========================================================================

int QClipMap29::GetNumInlineModels() const {
	return numsubmodels;
}

//==========================================================================
//
//	QClipMap29::GetEntityString
//
//==========================================================================

const char* QClipMap29::GetEntityString() const {
	return entitystring;
}

//==========================================================================
//
//	QClipMap29::MapChecksums
//
//==========================================================================

void QClipMap29::MapChecksums( int& ACheckSum1, int& ACheckSum2 ) const {
	ACheckSum1 = CheckSum;
	ACheckSum2 = CheckSum2;
}

//==========================================================================
//
//	QClipMap29::LeafCluster
//
//==========================================================================

int QClipMap29::LeafCluster( int LeafNum ) const {
	//	-1 is because pvs rows are 1 based, not 0 based like leafs
	return LeafNum - 1;
}

//==========================================================================
//
//	QClipMap29::LeafArea
//
//==========================================================================

int QClipMap29::LeafArea( int LeafNum ) const {
	return 0;
}

//==========================================================================
//
//	QClipMap29::LeafAmbientSoundLevel
//
//==========================================================================

const byte* QClipMap29::LeafAmbientSoundLevel( int LeafNum ) const {
	return leafs[ LeafNum ].ambient_sound_level;
}

//==========================================================================
//
//	QClipMap29::ModelBounds
//
//==========================================================================

void QClipMap29::ModelBounds( clipHandle_t Model, vec3_t Mins, vec3_t Maxs ) {
	cmodel_t* cmod = ClipHandleToModel( Model );
	VectorCopy( cmod->mins, Mins );
	VectorCopy( cmod->maxs, Maxs );
}

//==========================================================================
//
//	QClipMap29::GetNumTextures
//
//==========================================================================

int QClipMap29::GetNumTextures() const {
	return 0;
}

//==========================================================================
//
//	QClipMap29::GetTextureName
//
//==========================================================================

const char* QClipMap29::GetTextureName( int Index ) const {
	return "";
}

//==========================================================================
//
//	QClipMap29::ModelHull
//
//==========================================================================

clipHandle_t QClipMap29::ModelHull( clipHandle_t Handle, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs ) {
	if ( HullNum < 0 || HullNum >= MAX_MAP_HULLS ) {
		common->FatalError( "Invalid hull number" );
	}
	VectorCopy( hullsshared[ HullNum ].clip_mins, ClipMins );
	VectorCopy( hullsshared[ HullNum ].clip_maxs, ClipMaxs );
	return ( Handle & MODEL_NUMBER_MASK ) | HullNum;
}

//==========================================================================
//
//	QClipMap29::ClipHandleToModel
//
//==========================================================================

cmodel_t* QClipMap29::ClipHandleToModel( clipHandle_t Handle ) {
	Handle /= MAX_MAP_HULLS;
	if ( Handle < MAX_MAP_MODELS ) {
		return &map_models[ Handle ];
	}
	if ( Handle == BOX_HULL_HANDLE / MAX_MAP_HULLS ) {
		return &box_model;
	}
	common->FatalError( "Invalid handle" );
	return NULL;
}

//==========================================================================
//
//	QClipMap29::ClipHandleToHull
//
//==========================================================================

chull_t* QClipMap29::ClipHandleToHull( clipHandle_t Handle ) {
	cmodel_t* Model = ClipHandleToModel( Handle );
	return &Model->hulls[ Handle & HULL_NUMBER_MASK ];
}

//**************************************************************************
//
//	BOX HULL
//
//**************************************************************************

//==========================================================================
//
//	QClipMap29::InitBoxHull
//
//	Set up the planes and clipnodes so that the six floats of a bounding box
// can just be stored out and get a proper chull_t structure.
//
//==========================================================================

void QClipMap29::InitBoxHull() {
	Com_Memset( &box_model, 0, sizeof ( box_model ) );
	box_model.hulls[ 0 ].firstclipnode = numclipnodes + numnodes;
	box_model.hulls[ 0 ].lastclipnode = numclipnodes + numnodes + 5;

	cplane_t* box_planes = planes + numplanes;
	cclipnode_t* box_clipnodes = clipnodes + numclipnodes + numnodes;

	for ( int i = 0; i < 6; i++ ) {
		box_clipnodes[ i ].planenum = numplanes + i;

		int side = i & 1;

		box_clipnodes[ i ].children[ side ] = BSP29CONTENTS_EMPTY;
		if ( i != 5 ) {
			box_clipnodes[ i ].children[ side ^ 1 ] = numclipnodes + numnodes + i + 1;
		} else   {
			box_clipnodes[ i ].children[ side ^ 1 ] = BSP29CONTENTS_SOLID;
		}

		box_planes[ i ].type = i >> 1;
		box_planes[ i ].normal[ i >> 1 ] = 1;
	}
}

//==========================================================================
//
//	QClipMap29::TempBoxModel
//
//	To keep everything totally uniform, bounding boxes are turned into small
// BSP trees instead of being compared directly.
//
//==========================================================================

clipHandle_t QClipMap29::TempBoxModel( const vec3_t Mins, const vec3_t Maxs, bool Capsule ) {
	cplane_t* box_planes = planes + numplanes;
	box_planes[ 0 ].dist = Maxs[ 0 ];
	box_planes[ 1 ].dist = Mins[ 0 ];
	box_planes[ 2 ].dist = Maxs[ 1 ];
	box_planes[ 3 ].dist = Mins[ 1 ];
	box_planes[ 4 ].dist = Maxs[ 2 ];
	box_planes[ 5 ].dist = Mins[ 2 ];

	return BOX_HULL_HANDLE;
}

void QClipMap29::SetTempBoxModelContents( clipHandle_t handle, int contents ) {
}

//==========================================================================
//
//	QClipMap29::ContentsToQ2
//
//==========================================================================

int QClipMap29::ContentsToQ2( int Contents ) const {
	common->FatalError( "Not implemented" );
	return 0;
}

//==========================================================================
//
//	QClipMap29::ContentsToQ3
//
//==========================================================================

int QClipMap29::ContentsToQ3( int Contents ) const {
	common->FatalError( "Not implemented" );
	return 0;
}

//==========================================================================
//
//	QClipMap29::DrawDebugSurface
//
//==========================================================================

void QClipMap29::DrawDebugSurface( void ( * drawPoly )( int color, int numPoints, float* points ) ) {
}
