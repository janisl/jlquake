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

#ifndef __QUAKECLIENTDEFS_H__
#define __QUAKECLIENTDEFS_H__

//	Model effects
enum
{
	Q1MDLEF_ROCKET  = 1,	// leave a trail
	Q1MDLEF_GRENADE = 2,	// leave a trail
	Q1MDLEF_GIB     = 4,	// leave a trail
	Q1MDLEF_ROTATE  = 8,	// rotate (bonus items)
	Q1MDLEF_TRACER  = 16,	// green split trail
	Q1MDLEF_ZOMGIB  = 32,	// small blood trail
	Q1MDLEF_TRACER2 = 64,	// orange split trail + rotate
	Q1MDLEF_TRACER3 = 128,	// purple trail
};

#define MAX_SCOREBOARDNAME_Q1   32

struct q1entity_t
{
	q1entity_state_t state;
	float syncbase;			// for client-side animations

	double msgtime;			// time of last update
	vec3_t msg_origins[2];	// last two updates (0 is newest)
	vec3_t msg_angles[2];	// last two updates (0 is newest)
};

// qwplayer_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
struct qwplayer_state_t
{
	int messagenum;		// all player's won't be updated each frame

	double state_time;		// not the same as the packet time,
	// because player commands come asyncronously
	qwusercmd_t command;		// last command for prediction

	vec3_t origin;
	vec3_t viewangles;		// only for demos, not from server
	vec3_t velocity;
	int weaponframe;

	int modelindex;
	int frame;
	int skinnum;
	int effects;

	int flags;			// dead, gib, etc

	float waterjumptime;
	int onground;		// -1 = in air, else pmove entity number
	int oldbuttons;
};

struct qwframe_t
{
	// generated on client side
	qwusercmd_t cmd;		// cmd that generated the frame
	double senttime;	// time cmd was sent off
	int delta_sequence;		// sequence number to delta from, -1 = full update

	// received from server
	double receivedtime;	// time message was received, or -1
	qwplayer_state_t playerstate[MAX_CLIENTS_QHW];	// message received that reflects performing
	// the usercmd
	qwpacket_entities_t packet_entities;
	qboolean invalid;		// true if the packet_entities delta was invalid
};

struct qw_skin_t
{
	char name[16];
	bool failedload;		// the name isn't a valid skin
	byte* data;
};

struct q1player_info_t
{
	char name[MAX_SCOREBOARDNAME_Q1];
	float entertime;
	int frags;

	// skin information
	int topcolor;
	int bottomcolor;

	// New to QuakeWorld
	int userid;
	char userinfo[MAX_INFO_STRING_QW];

	int ping;
	byte pl;

	int _topcolor;
	int _bottomcolor;

	int spectator;
	qw_skin_t* skin;
};

#endif
