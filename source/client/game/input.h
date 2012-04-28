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
};

extern unsigned frame_msec;
extern int in_impulse;

extern Cvar* cl_forwardspeed;
extern Cvar* cl_run;
extern Cvar* cl_freelook;
extern Cvar* cl_sensitivity;
extern Cvar* m_pitch;
extern Cvar* v_centerspeed;
extern Cvar* lookspring;
extern Cvar* cl_bypassMouseInput;

void CL_JoystickEvent(int axis, int value, int time);
void CLQH_StartPitchDrift();
void CL_InitInputCommon();
in_usercmd_t CL_CreateCmdCommon();
void CL_ClearKeys();
