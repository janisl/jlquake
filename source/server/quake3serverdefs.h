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

enum
{
	Q3GT_FFA,				// free for all
	Q3GT_SINGLE_PLAYER = 2,	// single player ffa
	//-- team games go after this --
	Q3GT_TEAM,				// team deathmatch
};

enum
{
	Q3ET_PLAYER = 1,
	Q3ET_ITEM = 2,
	Q3ET_MOVER = 4,

	WSET_PROP = 32,
};

#define MAX_DOWNLOAD_WINDOW         8		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 byte block chunks

struct q3clientSnapshot_t
{
	int areabytes;
	byte areabits[MAX_MAP_AREA_BYTES];					// portalarea visibility bits
	q3playerState_t q3_ps;
	wsplayerState_t ws_ps;
	wmplayerState_t wm_ps;
	etplayerState_t et_ps;
	int num_entities;
	int first_entity;					// into the circular sv_packet_entities[]
										// the entities MUST be in increasing state number
										// order, otherwise the delta compression will fail
	int messageSent;					// time the message was transmitted
	int messageAcked;					// time the message was acked
	int messageSize;					// used to rate drop packets
};

struct netchan_buffer_t
{
	QMsg msg;
	byte msgBuffer[MAX_MSGLEN];
	char lastClientCommandString[MAX_STRING_CHARS];
	netchan_buffer_t* next;
};
