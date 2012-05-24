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

//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!

//entity info
struct aas_entityinfo_t
{
	int valid;				// true if updated this frame
	int type;				// entity type
	int flags;				// entity flags
	float ltime;			// local time
	float update_time;		// time between last and current update
	int number;				// number of the entity
	vec3_t origin;			// origin of the entity
	vec3_t angles;			// angles of the model
	vec3_t old_origin;		// for lerping
	vec3_t lastvisorigin;	// last visible origin
	vec3_t mins;			// bounding box minimums
	vec3_t maxs;			// bounding box maximums
	int groundent;			// ground entity
	int solid;				// solid type
	int modelindex;			// model used
	int modelindex2;		// weapons, CTF flags, etc
	int frame;				// model frame number
	int event;				// impulse events -- muzzle flashes, footsteps, etc
	int eventParm;			// even parameter
	int powerups;			// bit flags
	int weapon;				// determines weapon and flash model, etc
	int legsAnim;			// mask off ANIM_TOGGLEBIT
	int torsoAnim;			// mask off ANIM_TOGGLEBIT
};

//handle to the next bsp entity
int AAS_NextBSPEntity(int ent);
//return the value of the BSP epair key
bool AAS_ValueForBSPEpairKey(int ent, const char* key, char* value, int size);
//get a vector for the BSP epair key
bool AAS_VectorForBSPEpairKey(int ent, const char* key, vec3_t v);
//get a float for the BSP epair key
bool AAS_FloatForBSPEpairKey(int ent, const char* key, float* value);
//get an integer for the BSP epair key
bool AAS_IntForBSPEpairKey(int ent, const char* key, int* value);

//returns true if AAS is initialized
bool AAS_Initialized();
//returns the current time
float AAS_Time();
void AAS_SetCurrentWorld(int index);
