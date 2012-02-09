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

//
//	!!! Used in refdef which is used in Quake 3 VMs, Do not change!
//
#define MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

// sound attenuation values
#define ATTN_NONE				0	// full volume the entire level
#define ATTN_NORM				1
#define ATTN_IDLE				2
#define ATTN_STATIC				3	// diminish very rapidly with distance

#define BIGGEST_MAX_MODELS		512
#define BIGGEST_MAX_SOUNDS		512

#define MAX_CL_STATS		32

#define MAX_SERVERINFO_STRING	512

// edict->movetype values
#define QHMOVETYPE_NONE				0	// never moves
#define QHMOVETYPE_ANGLENOCLIP		1
#define QHMOVETYPE_ANGLECLIP		2
#define QHMOVETYPE_WALK				3	// gravity
#define QHMOVETYPE_STEP				4	// gravity, special edge handling
#define QHMOVETYPE_FLY				5
#define QHMOVETYPE_TOSS				6	// gravity
#define QHMOVETYPE_PUSH				7	// no clip to world, push and crush
#define QHMOVETYPE_NOCLIP			8
#define QHMOVETYPE_FLYMISSILE		9	// extra size to monsters
#define QHMOVETYPE_BOUNCE			10
#define H2MOVETYPE_BOUNCEMISSILE	11	// bounce w/o gravity
#define H2MOVETYPE_FOLLOW			12	// track movement of aiment
#define H2MOVETYPE_PUSHPULL			13	// pushable/pullable object
#define H2MOVETYPE_SWIM				14	// should keep the object in water

#define QHBUTTON_ATTACK	1
#define QHBUTTON_JUMP	2
