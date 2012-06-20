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

#define NUM_SPAWN_PARMS     16
#define NUM_PING_TIMES      16

#define MAX_BACK_BUFFERS 4

#define MAX_SIGNON_BUFFERS  8

struct qwclient_frame_t
{
	double senttime;
	float ping_time;
	qwpacket_entities_t entities;
};

// edict->solid values
#define QHSOLID_NOT             0		// no interaction with other objects
#define QHSOLID_TRIGGER         1		// touch on edge, but not blocking
#define QHSOLID_BBOX            2		// touch on edge, block
#define QHSOLID_SLIDEBOX        3		// touch on edge, but not an onground
#define QHSOLID_BSP             4		// bsp clip, touch on edge, block
#define H2SOLID_PHASE           5		// won't slow down when hitting entities flagged as QHFL_MONSTER

#define QHMOVE_NORMAL       0
#define QHMOVE_NOMONSTERS   1
#define QHMOVE_MISSILE      2
#define H2MOVE_WATER        3
#define H2MOVE_PHASE        4

#define QHPHS_OVERRIDE_R    8

#define Q1SPAWNFLAG_NOT_EASY          256
#define Q1SPAWNFLAG_NOT_MEDIUM        512
#define Q1SPAWNFLAG_NOT_HARD          1024
#define Q1SPAWNFLAG_NOT_DEATHMATCH    2048
