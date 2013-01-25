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

#ifndef __QUAKE2SERVERDEFS_H__
#define __QUAKE2SERVERDEFS_H__

#include "../common/quake2defs.h"
#include "../common/file_formats/bsp38.h"

#define LATENCY_COUNTS  16
#define RATE_MESSAGES   10

struct q2client_frame_t {
	int areabytes;
	byte areabits[ BSP38MAX_MAP_AREAS / 8 ];	// portalarea visibility bits
	q2player_state_t ps;
	int num_entities;
	int first_entity;						// into the circular sv_packet_entities[]
	int senttime;							// for ping calculations
};

// edict->svflags

#define Q2SVF_NOCLIENT          0x00000001	// don't send entity to clients, even if it has effects
#define Q2SVF_DEADMONSTER       0x00000002	// treat as CONTENTS_DEADMONSTER for collision
#define Q2SVF_MONSTER           0x00000004	// treat as CONTENTS_MONSTER for collision

// edict->solid values

enum q2solid_t
{
	Q2SOLID_NOT,		// no interaction with other objects
	Q2SOLID_TRIGGER,	// only touch when inside, after moving
	Q2SOLID_BBOX,		// touch on edge
	Q2SOLID_BSP			// bsp clip, touch on edge
};

//===============================================================

#define MAX_ENT_CLUSTERS_Q2    16

struct q2gclient_t {
	q2player_state_t ps;		// communicated by server to clients
	int ping;
	// the game dll can add anything it wants after
	// this point in the structure
};


struct q2edict_t {
	q2entity_state_t s;
	q2gclient_t* client;
	qboolean inuse;
	int linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t area;					// linked to a division node or leaf

	int num_clusters;				// if -1, use headnode instead
	int clusternums[ MAX_ENT_CLUSTERS_Q2 ];
	int headnode;					// unused if num_clusters != -1
	int areanum, areanum2;

	//================================

	int svflags;					// Q2SVF_NOCLIENT, Q2SVF_DEADMONSTER, Q2SVF_MONSTER, etc
	vec3_t mins, maxs;
	vec3_t absmin, absmax, size;
	q2solid_t solid;
	int clipmask;
	q2edict_t* owner;

	// the game dll can add anything it wants after
	// this point in the structure
};

// destination class for gi.multicast()
enum q2multicast_t
{
	Q2MULTICAST_ALL,
	Q2MULTICAST_PHS,
	Q2MULTICAST_PVS,
	Q2MULTICAST_ALL_R,
	Q2MULTICAST_PHS_R,
	Q2MULTICAST_PVS_R
};

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define Q2AREA_SOLID    1
#define Q2AREA_TRIGGERS 2

#endif
