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

#include "../../Common.h"
#include "../../common_defs.h"
#include "../../console_variable.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap46::PointLeafnum
//
//==========================================================================

int QClipMap46::PointLeafnum( const vec3_t p ) const {
	if ( !numNodes ) {
		// map not loaded
		return 0;
	}
	return PointLeafnum_r( p, 0 );
}

//==========================================================================
//
//	QClipMap46::PointLeafnum_r
//
//==========================================================================

int QClipMap46::PointLeafnum_r( const vec3_t P, int Num ) const {
	while ( Num >= 0 ) {
		const cNode_t* Node = nodes + Num;
		const cplane_t* Plane = Node->plane;

		float d;
		if ( Plane->type < 3 ) {
			d = P[ Plane->type ] - Plane->dist;
		} else   {
			d = DotProduct( Plane->normal, P ) - Plane->dist;
		}
		if ( d < 0 ) {
			Num = Node->children[ 1 ];
		} else   {
			Num = Node->children[ 0 ];
		}
	}

	c_pointcontents++;		// optimize counter

	return -1 - Num;
}

/*
======================================================================

LEAF LISTING

======================================================================
*/

//==========================================================================
//
//	QClipMap46::StoreLeafs
//
//==========================================================================

void QClipMap46::StoreLeafs( leafList_t* ll, int NodeNum ) const {
	int LeafNum = -1 - NodeNum;

	// store the lastLeaf even if the list is overflowed
	if ( leafs[ LeafNum ].cluster != -1 ) {
		ll->lastLeaf = LeafNum;
	}

	if ( ll->count >= ll->maxcount ) {
		return;
	}
	ll->list[ ll->count++ ] = LeafNum;
}

//==========================================================================
//
//	QClipMap46::BoxLeafnums_r
//
//	Fills in a list of all the leafs touched
//
//==========================================================================

void QClipMap46::BoxLeafnums_r( leafList_t* ll, int nodenum ) const {
	while ( 1 ) {
		if ( nodenum < 0 ) {
			StoreLeafs( ll, nodenum );
			return;
		}

		const cNode_t* node = &nodes[ nodenum ];
		const cplane_t* plane = node->plane;
		int s = BoxOnPlaneSide( ll->bounds[ 0 ], ll->bounds[ 1 ], plane );
		if ( s == 1 ) {
			nodenum = node->children[ 0 ];
		} else if ( s == 2 )     {
			nodenum = node->children[ 1 ];
		} else   {
			// go down both
			if ( ll->topnode == -1 ) {
				ll->topnode = nodenum;
			}
			BoxLeafnums_r( ll, node->children[ 0 ] );
			nodenum = node->children[ 1 ];
		}
	}
}

//==========================================================================
//
//	QClipMap46::BoxLeafnums
//
//==========================================================================

int QClipMap46::BoxLeafnums( const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode, int* LastLeaf ) const {
	leafList_t ll;

	VectorCopy( Mins, ll.bounds[ 0 ] );
	VectorCopy( Maxs, ll.bounds[ 1 ] );
	ll.count = 0;
	ll.maxcount = ListSize;
	ll.list = List;
	ll.topnode = -1;
	ll.lastLeaf = 0;

	BoxLeafnums_r( &ll, 0 );

	if ( TopNode ) {
		*TopNode = ll.topnode;
	}
	if ( LastLeaf ) {
		*LastLeaf = ll.lastLeaf;
	}
	return ll.count;
}

//==========================================================================
//
//	QClipMap46::PointContentsQ1
//
//==========================================================================

int QClipMap46::PointContentsQ1( const vec3_t P, clipHandle_t Model ) {
	return ContentsToQ1( PointContentsQ3( P, Model ) );
}

//==========================================================================
//
//	QClipMap46::PointContentsQ2
//
//==========================================================================

int QClipMap46::PointContentsQ2( const vec3_t P, clipHandle_t Model ) {
	return ContentsToQ2( PointContentsQ3( P, Model ) );
}

//==========================================================================
//
//	QClipMap46::PointContentsQ3
//
//==========================================================================

int QClipMap46::PointContentsQ3( const vec3_t P, clipHandle_t Model ) {
	if ( !numNodes ) {
		// map not loaded
		return 0;
	}

	cLeaf_t* leaf;
	if ( Model ) {
		cmodel_t* clipm = ClipHandleToModel( Model );
		leaf = &clipm->leaf;
	} else   {
		int leafnum = PointLeafnum_r( P, 0 );
		leaf = &leafs[ leafnum ];
	}

	int contents = 0;
	for ( int k = 0; k < leaf->numLeafBrushes; k++ ) {
		int brushnum = leafbrushes[ leaf->firstLeafBrush + k ];
		cbrush_t* b = &brushes[ brushnum ];

		// see if the point is in the brush
		int i;
		for ( i = 0; i < b->numsides; i++ ) {
			float d = DotProduct( P, b->sides[ i ].plane->normal );
// FIXME test for Cash
//			if (d >= b->sides[i].plane->dist ) {
			if ( d > b->sides[ i ].plane->dist ) {
				break;
			}
		}

		if ( i == b->numsides ) {
			contents |= b->contents;
		}
	}

	return contents;
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ1
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ1( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles ) {
	return ContentsToQ1( TransformedPointContentsQ3( P, Model, Origin, Angles ) );
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ2
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ2( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles ) {
	return ContentsToQ2( TransformedPointContentsQ3( P, Model, Origin, Angles ) );
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ3
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ3( const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles ) {
	// subtract origin offset
	vec3_t p_l;
	VectorSubtract( P, Origin, p_l );

	// rotate start and end into the models frame of reference
	if ( Model != BOX_MODEL_HANDLE && ( Angles[ 0 ] || Angles[ 1 ] || Angles[ 2 ] ) ) {
		vec3_t forward, right, up;
		AngleVectors( Angles, forward, right, up );

		vec3_t temp;
		VectorCopy( p_l, temp );
		p_l[ 0 ] = DotProduct( temp, forward );
		p_l[ 1 ] = -DotProduct( temp, right );
		p_l[ 2 ] = DotProduct( temp, up );
	}

	return PointContentsQ3( p_l, Model );
}

//==========================================================================
//
//	QClipMap46::HeadnodeVisible
//
//	Returns true if any leaf under headnode has a cluster that
// is potentially visible
//
//==========================================================================

bool QClipMap46::HeadnodeVisible( int NodeNum, byte* VisBits ) {
	if ( NodeNum < 0 ) {
		int leafnum = -1 - NodeNum;
		int cluster = leafs[ leafnum ].cluster;
		if ( cluster == -1 ) {
			return false;
		}
		if ( VisBits[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) {
			return true;
		}
		return false;
	}

	cNode_t* node = &nodes[ NodeNum ];
	if ( HeadnodeVisible( node->children[ 0 ], VisBits ) ) {
		return true;
	}
	return HeadnodeVisible( node->children[ 1 ], VisBits );
}

/*
===============================================================================

PVS

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::ClusterPVS
//
//==========================================================================

byte* QClipMap46::ClusterPVS( int Cluster ) {
	if ( Cluster < 0 || Cluster >= numClusters || !vised ) {
		return visibility;
	}

	return visibility + Cluster * clusterBytes;
}

//==========================================================================
//
//	QClipMap46::ClusterPHS
//
//==========================================================================

byte* QClipMap46::ClusterPHS( int Cluster ) {
	//	FIXME: Could calculate it like QuakeWorld does.
	return ClusterPVS( Cluster );
}

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::FloodAreaConnections
//
//==========================================================================

void QClipMap46::FloodAreaConnections() {
	// all current floods are now invalid
	floodvalid++;
	int floodnum = 0;

	cArea_t* area = areas;
	for ( int i = 0; i < numAreas; i++, area++ ) {
		if ( area->floodvalid == floodvalid ) {
			continue;		// already flooded into
		}
		floodnum++;
		FloodArea_r( i, floodnum );
	}
}

//==========================================================================
//
//	QClipMap46::FloodArea_r
//
//==========================================================================

void QClipMap46::FloodArea_r( int AreaNum, int FloodNum ) {
	cArea_t* area = &areas[ AreaNum ];

	if ( area->floodvalid == floodvalid ) {
		if ( area->floodnum == FloodNum ) {
			return;
		}
		common->Error( "FloodArea_r: reflooded" );
	}

	area->floodnum = FloodNum;
	area->floodvalid = floodvalid;
	int* con = areaPortals + AreaNum * numAreas;
	for ( int i = 0; i < numAreas; i++ ) {
		if ( con[ i ] > 0 ) {
			FloodArea_r( i, FloodNum );
		}
	}
}

//==========================================================================
//
//	QClipMap46::SetAreaPortalState
//
//==========================================================================

void QClipMap46::SetAreaPortalState( int portalnum, qboolean open ) {
}

//==========================================================================
//
//	QClipMap46::AdjustAreaPortalState
//
//==========================================================================

void QClipMap46::AdjustAreaPortalState( int Area1, int Area2, bool Open ) {
	if ( Area1 < 0 || Area2 < 0 ) {
		return;
	}

	if ( Area1 >= numAreas || Area2 >= numAreas ) {
		common->Error( "CM_ChangeAreaPortalState: bad area number" );
	}

	if ( Open ) {
		areaPortals[ Area1 * numAreas + Area2 ]++;
		areaPortals[ Area2 * numAreas + Area1 ]++;
	} else if ( ( GGameType & GAME_Quake3 ) || areaPortals[ Area2 * numAreas + Area1 ] )         {	// Ridah, fixes loadgame issue
		areaPortals[ Area1 * numAreas + Area2 ]--;
		areaPortals[ Area2 * numAreas + Area1 ]--;
		if ( areaPortals[ Area2 * numAreas + Area1 ] < 0 ) {
			common->Error( "CM_AdjustAreaPortalState: negative reference count" );
		}
	}

	FloodAreaConnections();
}

//==========================================================================
//
//	QClipMap46::AreasConnected
//
//==========================================================================

qboolean QClipMap46::AreasConnected( int Area1, int Area2 ) {
	if ( cm_noAreas->integer ) {
		return true;
	}

	if ( Area1 < 0 || Area2 < 0 ) {
		return false;
	}

	if ( Area1 >= numAreas || Area2 >= numAreas ) {
		common->Error( "area >= numAreas" );
	}

	if ( areas[ Area1 ].floodnum == areas[ Area2 ].floodnum ) {
		return true;
	}
	return false;
}

//==========================================================================
//
//	QClipMap46::WriteAreaBits
//
//	Writes a bit vector of all the areas that are in the same flood as the
// area parameter. Returns the number of bytes needed to hold all the bits.
//
//	The bits are OR'd in, so you can CM_WriteAreaBits from multiple
// viewpoints and get the union of all visible areas.
//
//	This is used to cull non-visible entities from snapshots
//
//==========================================================================

int QClipMap46::WriteAreaBits( byte* Buffer, int Area ) {
	int bytes = ( numAreas + 7 ) >> 3;

	if ( cm_noAreas->integer || Area == -1 ) {
		// for debugging, send everything
		Com_Memset( Buffer, 255, bytes );
	} else   {
		int floodnum = areas[ Area ].floodnum;
		for ( int i = 0; i < numAreas; i++ ) {
			if ( areas[ i ].floodnum == floodnum || Area == -1 ) {
				Buffer[ i >> 3 ] |= 1 << ( i & 7 );
			}
		}
	}

	return bytes;
}

//==========================================================================
//
//	QClipMap46::WritePortalState
//
//	Writes the portal state to a savegame file
//
//==========================================================================

void QClipMap46::WritePortalState( fileHandle_t f ) {
	FS_Write( areaPortals, sizeof ( areaPortals ), f );
}

//==========================================================================
//
//	QClipMap46::ReadPortalState
//
//	Reads the portal state from a savegame file and recalculates the area
// connections
//
//==========================================================================

void QClipMap46::ReadPortalState( fileHandle_t f ) {
	FS_Read( areaPortals, sizeof ( areaPortals ), f );
	FloodAreaConnections();
}
