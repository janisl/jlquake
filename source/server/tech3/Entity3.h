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

//	This is initial portion of shared entity struct that is common in all
// Tech3 games.
struct sharedEntityCommon_t
{
	int number;			// entity index
	int eType;			// entityType_t
	int eFlags;

	q3trajectory_t pos;	// for calculating position
	q3trajectory_t apos;	// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;	// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;	// -1 = in air

	int constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
};

class idEntity3 : public Interface
{
public:
	void SetGEntity(void* newGEntity);

	/*
	int number;			// entity index
	int eType;			// entityType_t
	int eFlags;

	q3trajectory_t pos;	// for calculating position
	q3trajectory_t apos;	// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;	// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;	// -1 = in air

	int constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	 */
private:
	sharedEntityCommon_t* gentity;
};

inline void idEntity3::SetGEntity(void* newGEntity)
{
	gentity = reinterpret_cast<sharedEntityCommon_t*>(newGEntity);
}
