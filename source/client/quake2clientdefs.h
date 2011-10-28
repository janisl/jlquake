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

struct q2centity_t
{
	q2entity_state_t baseline;	// delta from this if not from a previous frame
	q2entity_state_t current;
	q2entity_state_t prev;		// will always be valid, but might just be a copy of current

	int serverframe;			// if not current, this ent isn't in the frame

	int trailcount;				// for diminishing grenade trails
	vec3_t lerp_origin;			// for trails (variable hz)

	int fly_stoptime;
};

struct q2frame_t
{
	qboolean valid;		// cleared if delta parsing was invalid
	int serverframe;
	int servertime;		// server time the message is valid for (in msec)
	int deltaframe;
	byte areabits[BSP38MAX_MAP_AREAS / 8];		// portalarea visibility bits
	q2player_state_t playerstate;
	int num_entities;
	int parse_entities;	// non-masked index into cl_parse_entities array
};
