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

#ifndef __CGAME_INPUT_H__
#define __CGAME_INPUT_H__

#include "../../common/qcommon.h"

struct in_usercmd_t
{
	int forwardmove;
	int sidemove;
	int upmove;
	int buttons;
	int doubleTap;
	int kick;
	vec3_t fAngles;
	int angles[3];
	int impulse;
	int weapon;
	float mtime;
	int msec;
	int serverTime;
	int lightlevel;
	int holdable;
	int mpSetup;
	int identClient;
	int flags;
	int cld;
};

extern int in_impulse;

extern Cvar* cl_forwardspeed;
extern Cvar* cl_run;
extern Cvar* cl_freelook;
extern Cvar* cl_sensitivity;
extern Cvar* m_pitch;
extern Cvar* v_centerspeed;
extern Cvar* lookspring;
extern Cvar* cl_bypassMouseInput;
extern Cvar* cl_debugMove;
extern Cvar* cl_nodelta;

void CLQH_StartPitchDrift();
void CL_InitInput();
in_usercmd_t CL_CreateCmd();
void CL_ClearKeys();

#endif
