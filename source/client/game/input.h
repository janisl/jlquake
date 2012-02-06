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
extern kbutton_t in_forward;
extern kbutton_t in_back;
extern kbutton_t in_lookup;
extern kbutton_t in_lookdown;
extern kbutton_t in_moveleft;
extern kbutton_t in_moveright;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;
extern kbutton_t in_up;
extern kbutton_t in_down;

extern kbutton_t in_buttons[16];

extern bool in_mlooking;

extern Cvar* cl_forwardspeed;
extern Cvar* cl_backspeed;
extern Cvar* cl_movespeedkey;
extern Cvar* cl_freelook;
extern Cvar* cl_run;
extern Cvar* v_centerspeed;
extern Cvar* lookspring;

void IN_KeyDown(kbutton_t* b);
void IN_KeyUp(kbutton_t* b);
float CL_KeyState(kbutton_t* key);
void CLQH_StartPitchDrift();
void CLQH_StopPitchDrift();
void IN_CenterView();
void CL_InitInputCommon();
void CL_KeyMove(in_usercmd_t* cmd);
