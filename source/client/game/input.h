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

struct kbutton_t
{
	int down[2];		// key nums holding it down
	unsigned downtime;	// msec timestamp
	unsigned msec;		// msec down this frame if both a down and up happened
	bool active;		// current state
	bool wasPressed;	// set when down, not cleared when up
};

struct in_usercmd_t
{
	int forwardmove;
	int sidemove;
	int upmove;
};

extern unsigned frame_msec;

extern kbutton_t in_left;
extern kbutton_t in_right;
extern kbutton_t in_lookup;
extern kbutton_t in_lookdown;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;

extern kbutton_t in_buttons[16];

extern Cvar* cl_yawspeed;
extern Cvar* cl_pitchspeed;
extern Cvar* cl_anglespeedkey;
extern Cvar* cl_forwardspeed;
extern Cvar* cl_run;
extern Cvar* cl_freelook;
extern Cvar* cl_sensitivity;
extern Cvar* m_pitch;
extern Cvar* v_centerspeed;
extern Cvar* lookspring;

void CL_JoystickEvent(int axis, int value, int time);
float CL_KeyState(kbutton_t* key);
void CLQH_StartPitchDrift();
void CLQH_StopPitchDrift();
void CL_InitInputCommon();
void CL_KeyMove(in_usercmd_t* cmd);
void CL_MouseMove(in_usercmd_t* cmd);
void CL_JoystickMove(in_usercmd_t* cmd);
