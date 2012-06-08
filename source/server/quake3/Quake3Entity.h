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

class idQuake3Entity : public idEntity3
{
	/*
	int loopSound;		// constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;		// 0 to (MAX_CLIENTS_Q3 - 1), for players and corpses
	int frame;

	int solid;			// for client side prediction, trap_linkentity sets this properly

	int event;			// impulse events -- muzzle flashes, footsteps, etc
	int eventParm;

	// for players
	int powerups;		// bit flags
	int weapon;			// determines weapon and flash model, etc
	int legsAnim;		// mask off ANIM_TOGGLEBIT
	int torsoAnim;		// mask off ANIM_TOGGLEBIT

	int generic1;
	 */
	/*
	q3entityState_t s;				// communicated by server to clients

	qboolean linked;				// qfalse if not in any good cluster
	int linkcount;

	int svFlags;					// Q3SVF_NOCLIENT, Q3SVF_BROADCAST, etc

	// only send to this client when SVF_SINGLECLIENT is set
	// if Q3SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
	int singleClient;

	qboolean bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;					// CONTENTS_TRIGGER, BSP46CONTENTS_SOLID, BSP46CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;			// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != Q3ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	 */
};
