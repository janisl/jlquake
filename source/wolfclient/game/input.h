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

void CL_JoystickEvent(int axis, int value, int time);
#if 0
void CLQH_StartPitchDrift();
#endif
void CL_InitInputCommon();
#if 0
in_usercmd_t CL_CreateCmdCommon();
#endif