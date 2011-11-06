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

#define MAX_EDICTS_H2		768			// FIXME: ouch! ouch! ouch!

// Player Classes
#define NUM_CLASSES_H2				4
#define NUM_CLASSES_H2MP			5
#define MAX_PLAYER_CLASS			6

#define CLASS_PALADIN				1
#define CLASS_CLERIC 				2
#define CLASS_NECROMANCER			3
#define CLASS_THEIF   				4
#define CLASS_DEMON					5
#define CLASS_DWARF					6

// Timing macros
#define HX_FRAME_TIME		0.05

// h2entity_state_t is the information conveyed from the server
// in an update message
struct h2entity_state_t
{
	int number;			// edict index

	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int frame;
	int colormap;
	int skinnum;
	int effects;
	int scale;			// for Alias models
	int drawflags;		// for Alias models
	int abslight;		// for Alias models
	int wpn_sound;		// for cheap playing of sounds
	int flags;			// nolerp, etc
	byte ClearCount[32];
};
