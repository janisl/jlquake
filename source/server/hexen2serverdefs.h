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
