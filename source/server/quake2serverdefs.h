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

#define LATENCY_COUNTS  16
#define RATE_MESSAGES   10

struct q2client_frame_t
{
	int areabytes;
	byte areabits[BSP38MAX_MAP_AREAS / 8];	// portalarea visibility bits
	q2player_state_t ps;
	int num_entities;
	int first_entity;						// into the circular sv_packet_entities[]
	int senttime;							// for ping calculations
};
