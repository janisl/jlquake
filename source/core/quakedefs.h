//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define MAX_EDICTS_Q1		768			// FIXME: ouch! ouch! ouch!

//
// temp entity events
//
#define Q1TE_SPIKE			0
#define Q1TE_SUPERSPIKE		1
#define Q1TE_GUNSHOT		2
#define Q1TE_EXPLOSION		3
#define Q1TE_TAREXPLOSION	4
#define Q1TE_LIGHTNING1		5
#define Q1TE_LIGHTNING2		6
#define Q1TE_WIZSPIKE		7
#define Q1TE_KNIGHTSPIKE	8
#define Q1TE_LIGHTNING3		9
#define Q1TE_LAVASPLASH		10
#define Q1TE_TELEPORT		11
//	Quake only
#define Q1TE_EXPLOSION2		12
#define Q1TE_BEAM			13
//	QuakeWorld only
#define QWTE_BLOOD			12
#define QWTE_LIGHTNINGBLOOD	13

// q1entity_state_t is the information conveyed from the server
// in an update message
struct q1entity_state_t
{
	int number;			// edict index

	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int frame;
	int colormap;
	int skinnum;
	int effects;
	int flags;			// nolerp, etc
};

struct q1usercmd_t
{
	vec3_t viewangles;

	// intended velocities
	float forwardmove;
	float sidemove;
	float upmove;
};

struct qwusercmd_t
{
	byte msec;
	vec3_t angles;
	short forwardmove, sidemove, upmove;
	byte buttons;
	byte impulse;
};

#define QWMAX_PACKET_ENTITIES	64	// doesn't count nails

struct qwpacket_entities_t
{
	int num_entities;
	q1entity_state_t entities[QWMAX_PACKET_ENTITIES];
};

#define QWMAX_CLIENTS		32
