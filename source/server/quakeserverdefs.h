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
