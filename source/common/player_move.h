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

#define STEPSIZE		18

#define MAX_FLY_MOVE_CLIP_PLANES	5

#define QHMAX_PHYSENTS	64

struct movevars_t
{
	float gravity;
	float stopspeed;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float wateraccelerate;
	float friction;
	float waterfriction;
	float entgravity;
};

struct qhphysent_t
{
	vec3_t origin;
	clipHandle_t model;		// only for bsp models
	vec3_t mins, maxs;	// only for non-bsp models
	vec3_t angles;
	int info;		// for client or server to identify
};

struct qhpmove_usercmd_t
{
	byte msec;
	vec3_t angles;
	short forwardmove, sidemove, upmove;
	byte buttons;
	byte impulse;

	void Set(const qwusercmd_t& cmd)
	{
		msec = cmd.msec;
		VectorCopy(cmd.angles, angles);
		forwardmove = cmd.forwardmove;
		sidemove = cmd.sidemove;
		upmove = cmd.upmove;
		buttons = cmd.buttons;
		impulse = cmd.impulse;
	}

	void Set(const hwusercmd_t& cmd)
	{
		msec = cmd.msec;
		VectorCopy(cmd.angles, angles);
		forwardmove = cmd.forwardmove;
		sidemove = cmd.sidemove;
		upmove = cmd.upmove;
		buttons = cmd.buttons;
		impulse = cmd.impulse;
	}
};

struct qhplayermove_t
{
	int sequence;	// just for debugging prints

	// player state
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int oldbuttons;
	float waterjumptime;
	float teleport_time;
	bool dead;
	int spectator;
	float hasted;
	int movetype;
	bool crouched;

	// world state
	int numphysent;
	qhphysent_t	physents[QHMAX_PHYSENTS];	// 0 should be the world

	// input
	qhpmove_usercmd_t cmd;

	// results
	int numtouch;
	int touchindex[QHMAX_PHYSENTS];

	int onground;
	int waterlevel;
	int watertype;
};

extern movevars_t movevars;
extern qhplayermove_t qh_pmove;

extern vec3_t pmqh_player_mins;
extern vec3_t pmqh_player_maxs;

extern vec3_t pmqh_player_maxs_crouch;

void PM_ClipVelocity(const vec3_t in, const vec3_t normal, vec3_t out, float overbounce);

void PMQH_Init();
q1trace_t PMQH_TestPlayerMove(const vec3_t start, const vec3_t stop);
bool PMQH_TestPlayerPosition(const vec3_t point);
void PMQH_PlayerMove();
