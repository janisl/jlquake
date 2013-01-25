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

#include "worldsector.h"

worldSector_t sv_worldSectors[ AREA_NODES ];
static int sv_numworldSectors;

//	Builds a uniformly subdivided tree for the given world size
static worldSector_t* SV_CreateworldSector( int depth, vec3_t mins, vec3_t maxs ) {
	worldSector_t* anode = &sv_worldSectors[ sv_numworldSectors ];
	sv_numworldSectors++;

	ClearLink( &anode->trigger_edicts );
	ClearLink( &anode->solid_edicts );

	if ( depth == AREA_DEPTH ) {
		anode->axis = -1;
		anode->children[ 0 ] = anode->children[ 1 ] = NULL;
		return anode;
	}

	vec3_t size;
	VectorSubtract( maxs, mins, size );
	if ( size[ 0 ] > size[ 1 ] ) {
		anode->axis = 0;
	} else   {
		anode->axis = 1;
	}

	anode->dist = 0.5 * ( maxs[ anode->axis ] + mins[ anode->axis ] );
	vec3_t mins1, maxs1, mins2, maxs2;
	VectorCopy( mins, mins1 );
	VectorCopy( mins, mins2 );
	VectorCopy( maxs, maxs1 );
	VectorCopy( maxs, maxs2 );

	maxs1[ anode->axis ] = mins2[ anode->axis ] = anode->dist;

	anode->children[ 0 ] = SV_CreateworldSector( depth + 1, mins2, maxs2 );
	anode->children[ 1 ] = SV_CreateworldSector( depth + 1, mins1, maxs1 );

	return anode;
}

void SV_ClearWorld() {
	Com_Memset( sv_worldSectors, 0, sizeof ( sv_worldSectors ) );
	sv_numworldSectors = 0;

	// get world map bounds
	vec3_t mins;
	vec3_t maxs;
	CM_ModelBounds( 0, mins, maxs );
	SV_CreateworldSector( 0, mins, maxs );
}
