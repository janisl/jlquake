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

struct hwclient_frame_t
{
	double senttime;
	float ping_time;
	hwpacket_entities_t entities;
};

struct h2client_frames_t
{
	h2entity_state_t states[MAX_CLIENT_STATES_H2];
	int count;
};

struct h2client_state2_t
{
	h2client_frames_t frames[H2MAX_FRAMES + 2];	// 0 = base, 1-max = proposed, max+1 = too late
};

// Built-in Spawn Flags
#define H2SPAWNFLAG_NOT_PALADIN       0x00000100
#define H2SPAWNFLAG_NOT_CLERIC        0x00000200
#define H2SPAWNFLAG_NOT_NECROMANCER   0x00000400
#define H2SPAWNFLAG_NOT_THEIF         0x00000800
#define H2SPAWNFLAG_NOT_EASY          0x00001000
#define H2SPAWNFLAG_NOT_MEDIUM        0x00002000
#define H2SPAWNFLAG_NOT_HARD          0x00004000
#define H2SPAWNFLAG_NOT_DEATHMATCH    0x00008000
#define H2SPAWNFLAG_NOT_COOP          0x00010000
#define H2SPAWNFLAG_NOT_SINGLE        0x00020000

// server flags
#define H2SFL_NEW_UNIT      16
#define H2SFL_NEW_EPISODE   32

//Dm Modes
#define HWDM_SIEGE                  3

#define H2FL2_CROUCHED            4096

#define HWHULL_IMPLICIT       0	//Choose the hull based on bounding box- like in Quake
#define HWHULL_POINT          1	//0 0 0, 0 0 0
#define HWHULL_PLAYER         2	//'-16 -16 0', '16 16 56'
#define HWHULL_SCORPION       3	//'-24 -24 -20', '24 24 20'
#define HWHULL_CROUCH         4	//'-16 -16 0', '16 16 28'
#define HWHULL_HYDRA          5	//'-28 -28 -24', '28 28 24'
#define HWHULL_GOLEM          6	//???,???

#define H2_SAVEGAME_VERSION    5
