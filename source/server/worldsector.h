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

#ifndef __WORLDSECTOR_H__
#define __WORLDSECTOR_H__

#include "../common/qcommon.h"
#include "link.h"
#include "quake3serverdefs.h"

#define AREA_DEPTH  4
#define AREA_NODES  32

struct worldSector_t {
	int axis;			// -1 = leaf node
	float dist;
	worldSector_t* children[ 2 ];
	link_t trigger_edicts;
	link_t solid_edicts;
	q3svEntity_t* entities;
};

extern worldSector_t sv_worldSectors[ AREA_NODES ];

// called after the world model has been loaded, before linking any entities
void SV_ClearWorld();

#endif
