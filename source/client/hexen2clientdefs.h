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

//	.MDL model effect flags.
enum
{
	H2MDLEF_ROCKET = 1,				// leave a trail
	H2MDLEF_GRENADE = 2,			// leave a trail
	H2MDLEF_GIB = 4,				// leave a trail
	H2MDLEF_ROTATE = 8,				// rotate (bonus items)
	H2MDLEF_TRACER = 16,			// green split trail
	H2MDLEF_ZOMGIB = 32,			// small blood trail
	H2MDLEF_TRACER2 = 64,			// orange split trail + rotate
	H2MDLEF_TRACER3 = 128,			// purple trail
	H2MDLEF_FIREBALL = 256,			// Yellow transparent trail in all directions
	H2MDLEF_ICE = 512,				// Blue-white transparent trail, with gravity
	H2MDLEF_MIP_MAP = 1024,			// This model has mip-maps
	H2MDLEF_SPIT = 2048,			// Black transparent trail with negative light
	H2MDLEF_TRANSPARENT = 4096,		// Transparent sprite
	H2MDLEF_SPELL = 8192,			// Vertical spray of particles
	H2MDLEF_HOLEY = 16384,			// Solid model with color 0
	H2MDLEF_SPECIAL_TRANS = 32768,	// Translucency through the particle table
	H2MDLEF_FACE_VIEW = 65536,		// Poly Model always faces you
	H2MDLEF_VORP_MISSILE = 131072,	// leave a trail at top and bottom of model
	H2MDLEF_SET_STAFF = 262144,		// slowly move up and left/right
	H2MDLEF_MAGICMISSILE = 524288,	// a trickle of blue/white particles with gravity
	H2MDLEF_BONESHARD = 1048576,	// a trickle of brown particles with gravity
	H2MDLEF_SCARAB = 2097152,		// white transparent particles with little gravity
	H2MDLEF_ACIDBALL = 4194304,		// Green drippy acid shit
	H2MDLEF_BLOODSHOT = 8388608,	// Blood rain shot trail
};

#define MAX_INVENTORY_H2            15		// Max inventory array size

#define H2MAX_SCOREBOARDNAME    32
struct h2player_info_t
{
	// scoreboard information
	char name[H2MAX_SCOREBOARDNAME];
	float entertime;
	int frags;

	// skin information
	int topColour;
	int bottomColour;
	int playerclass;

	//	New to HexenWorld
	int userid;
	char userinfo[HWMAX_INFO_STRING];

	int ping;

	int level;
	int spectator;
	int modelindex;
	qboolean Translated;
	int siege_team;
	qboolean shownames_off;

	vec3_t origin;
};

// hwplayer_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
struct hwplayer_state_t
{
	int messagenum;		// all player's won't be updated each frame

	double state_time;		// not the same as the packet time,
	// because player commands come asyncronously
	hwusercmd_t command;		// last command for prediction

	vec3_t origin;
	vec3_t viewangles;		// only for demos, not from server
	vec3_t velocity;
	int weaponframe;

	int modelindex;
	int frame;
	int skinnum;
	int effects;
	int drawflags;
	int scale;
	int abslight;

	int flags;			// dead, gib, etc

	float waterjumptime;
	int onground;		// -1 = in air, else pmove entity number
	int oldbuttons;
};

struct hwframe_t
{
	// generated on client side
	hwusercmd_t cmd;		// cmd that generated the frame
	double senttime;	// time cmd was sent off
	int delta_sequence;		// sequence number to delta from, -1 = full update

	// received from server
	double receivedtime;	// time message was received, or -1
	hwplayer_state_t playerstate[MAX_CLIENTS_QHW];	// message received that reflects performing
	// the usercmd
	hwpacket_entities_t packet_entities;
	qboolean invalid;		// true if the packet_entities delta was invalid
};

struct h2entity_t
{
	h2entity_state_t state;
	float syncbase;			// for client-side animations

	double msgtime;			// time of last update
	vec3_t msg_origins[2];	// last two updates (0 is newest)
	vec3_t msg_angles[2];	// last two updates (0 is newest)
};

struct h2client_frames2_t
{
	h2entity_state_t states[MAX_CLIENT_STATES_H2 * 2];
	int count;
};
