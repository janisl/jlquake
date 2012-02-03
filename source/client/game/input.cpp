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

#include "../client.h"

unsigned frame_msec;

kbutton_t in_left;
kbutton_t in_right;
kbutton_t in_forward;
kbutton_t in_back;
kbutton_t in_lookup;
kbutton_t in_lookdown;
kbutton_t in_moveleft;
kbutton_t in_moveright;
kbutton_t in_strafe;
kbutton_t in_speed;
kbutton_t in_up;
kbutton_t in_down;
kbutton_t in_buttons[16];

//	All except Quake 3
kbutton_t in_klook;

bool in_mlooking;

Cvar* v_centerspeed;

void IN_KeyDown(kbutton_t* b)
{
	const char* c = Cmd_Argv(1);
	int k;
	if (c[0])
	{
		k = String::Atoi(c);
	}
	else
	{
		k = -1;		// typed manually at the console for continuous down
	}

	if (k == b->down[0] || k == b->down[1])
	{
		return;		// repeating key
	}

	if (!b->down[0])
	{
		b->down[0] = k;
	}
	else if (!b->down[1])
	{
		b->down[1] = k;
	}
	else
	{
		common->Printf("Three keys down for a button!\n");
		return;
	}

	if (b->active)
	{
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = String::Atoi(c);

	b->active = true;
	b->wasPressed = true;
}

void IN_KeyUp(kbutton_t* b)
{
	const char* c = Cmd_Argv(1);
	if (!c[0])
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = false;
		return;
	}
	int k = String::Atoi(c);

	if (b->down[0] == k)
	{
		b->down[0] = 0;
	}
	else if (b->down[1] == k)
	{
		b->down[1] = 0;
	}
	else
	{
		return;		// key up without coresponding down (menu pass through)
	}
	if (b->down[0] || b->down[1])
	{
		return;		// some other key is still holding it down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	unsigned uptime = String::Atoi(c);
	if (uptime)
	{
		b->msec += uptime - b->downtime;
	}
	else
	{
		b->msec += frame_msec / 2;
	}

	b->active = false;
}

static void IN_UpDown()
{
	IN_KeyDown(&in_up);
}

static void IN_UpUp()
{
	IN_KeyUp(&in_up);
}

static void IN_DownDown()
{
	IN_KeyDown(&in_down);
}

static void IN_DownUp()
{
	IN_KeyUp(&in_down);
}

static void IN_LeftDown()
{
	IN_KeyDown(&in_left);
}

static void IN_LeftUp()
{
	IN_KeyUp(&in_left);
}

static void IN_RightDown()
{
	IN_KeyDown(&in_right);
}

static void IN_RightUp()
{
	IN_KeyUp(&in_right);
}

static void IN_ForwardDown()
{
	IN_KeyDown(&in_forward);
}

static void IN_ForwardUp()
{
	IN_KeyUp(&in_forward);
}

static void IN_BackDown()
{
	IN_KeyDown(&in_back);
}

static void IN_BackUp()
{
	IN_KeyUp(&in_back);
}

static void IN_LookupDown()
{
	IN_KeyDown(&in_lookup);
}

static void IN_LookupUp()
{
	IN_KeyUp(&in_lookup);
}

static void IN_LookdownDown()
{
	IN_KeyDown(&in_lookdown);
}

static void IN_LookdownUp()
{
	IN_KeyUp(&in_lookdown);
}

static void IN_MoveleftDown()
{
	IN_KeyDown(&in_moveleft);
}

static void IN_MoveleftUp()
{
	IN_KeyUp(&in_moveleft);
}

static void IN_MoverightDown()
{
	IN_KeyDown(&in_moveright);
}

static void IN_MoverightUp()
{
	IN_KeyUp(&in_moveright);
}

static void IN_SpeedDown()
{
	IN_KeyDown(&in_speed);
}

static void IN_SpeedUp()
{
	IN_KeyUp(&in_speed);
}

static void IN_StrafeDown()
{
	IN_KeyDown(&in_strafe);
}

static void IN_StrafeUp()
{
	IN_KeyUp(&in_strafe);
}

static void IN_Button0Down()
{
	IN_KeyDown(&in_buttons[0]);
}

static void IN_Button0Up()
{
	IN_KeyUp(&in_buttons[0]);
}

static void IN_Button1Down()
{
	IN_KeyDown(&in_buttons[1]);
}

static void IN_Button1Up()
{
	IN_KeyUp(&in_buttons[1]);
}

static void IN_Button2Down()
{
	IN_KeyDown(&in_buttons[2]);
}

static void IN_Button2Up()
{
	IN_KeyUp(&in_buttons[2]);
}

static void IN_Button3Down()
{
	IN_KeyDown(&in_buttons[3]);
}

static void IN_Button3Up()
{
	IN_KeyUp(&in_buttons[3]);
}

static void IN_Button4Down()
{
	IN_KeyDown(&in_buttons[4]);
}

static void IN_Button4Up()
{
	IN_KeyUp(&in_buttons[4]);
}

static void IN_Button5Down()
{
	IN_KeyDown(&in_buttons[5]);
}

static void IN_Button5Up()
{
	IN_KeyUp(&in_buttons[5]);
}

static void IN_Button6Down()
{
	IN_KeyDown(&in_buttons[6]);
}

static void IN_Button6Up()
{
	IN_KeyUp(&in_buttons[6]);
}

static void IN_Button7Down()
{
	IN_KeyDown(&in_buttons[7]);
}

static void IN_Button7Up()
{
	IN_KeyUp(&in_buttons[7]);
}

static void IN_Button8Down()
{
	IN_KeyDown(&in_buttons[8]);
}

static void IN_Button8Up()
{
	IN_KeyUp(&in_buttons[8]);
}

static void IN_Button9Down()
{
	IN_KeyDown(&in_buttons[9]);
}

static void IN_Button9Up()
{
	IN_KeyUp(&in_buttons[9]);
}

static void IN_Button10Down()
{
	IN_KeyDown(&in_buttons[10]);
}

static void IN_Button10Up()
{
	IN_KeyUp(&in_buttons[10]);
}

static void IN_Button11Down()
{
	IN_KeyDown(&in_buttons[11]);
}

static void IN_Button11Up()
{
	IN_KeyUp(&in_buttons[11]);
}

static void IN_Button12Down()
{
	IN_KeyDown(&in_buttons[12]);
}

static void IN_Button12Up()
{
	IN_KeyUp(&in_buttons[12]);
}

static void IN_Button13Down()
{
	IN_KeyDown(&in_buttons[13]);
}

static void IN_Button13Up()
{
	IN_KeyUp(&in_buttons[13]);
}

static void IN_Button14Down()
{
	IN_KeyDown(&in_buttons[14]);
}

static void IN_Button14Up()
{
	IN_KeyUp(&in_buttons[14]);
}

//	Returns the fraction of the frame that the key was down
float CL_KeyState(kbutton_t* key)
{
	int msec = key->msec;
	key->msec = 0;

	if (key->active)
	{
		// still down
		if (!key->downtime)
		{
			msec = com_frameTime;
		}
		else
		{
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

	float val = (float)msec / frame_msec;
	if (val < 0)
	{
		val = 0;
	}
	if (val > 1)
	{
		val = 1;
	}

	return val;
}

void CLQH_StartPitchDrift()
{
	if (cl.qh_laststop == cl.qh_serverTimeFloat)
	{
		return;		// something else is keeping it from drifting
	}
	if (cl.qh_nodrift || !cl.qh_pitchvel)
	{
		cl.qh_pitchvel = v_centerspeed->value;
		cl.qh_nodrift = false;
		cl.qh_driftmove = 0;
	}
}

void CLQH_StopPitchDrift()
{
	cl.qh_laststop = cl.qh_serverTimeFloat;
	cl.qh_nodrift = true;
	cl.qh_pitchvel = 0;
}

void IN_CenterView()
{
	if (GGameType & GAME_QuakeHexen)
	{
		CLQH_StartPitchDrift();
	}
	if (GGameType & GAME_Quake2)
	{
		cl.viewangles[PITCH] = -SHORT2ANGLE(cl.q2_frame.playerstate.pmove.delta_angles[PITCH]);
	}
	if (GGameType & GAME_Quake3)
	{
		cl.viewangles[PITCH] = -SHORT2ANGLE(cl.q3_snap.ps.delta_angles[PITCH]);
	}
}

void CL_InitInputCommon()
{
	Cmd_AddCommand("+moveup",IN_UpDown);
	Cmd_AddCommand("-moveup",IN_UpUp);
	Cmd_AddCommand("+movedown",IN_DownDown);
	Cmd_AddCommand("-movedown",IN_DownUp);
	Cmd_AddCommand("+left",IN_LeftDown);
	Cmd_AddCommand("-left",IN_LeftUp);
	Cmd_AddCommand("+right",IN_RightDown);
	Cmd_AddCommand("-right",IN_RightUp);
	Cmd_AddCommand("+forward",IN_ForwardDown);
	Cmd_AddCommand("-forward",IN_ForwardUp);
	Cmd_AddCommand("+back",IN_BackDown);
	Cmd_AddCommand("-back",IN_BackUp);
	Cmd_AddCommand("+lookup", IN_LookupDown);
	Cmd_AddCommand("-lookup", IN_LookupUp);
	Cmd_AddCommand("+lookdown", IN_LookdownDown);
	Cmd_AddCommand("-lookdown", IN_LookdownUp);
	Cmd_AddCommand("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand("+moveright", IN_MoverightDown);
	Cmd_AddCommand("-moveright", IN_MoverightUp);
	Cmd_AddCommand("+strafe", IN_StrafeDown);
	Cmd_AddCommand("-strafe", IN_StrafeUp);
	Cmd_AddCommand("+speed", IN_SpeedDown);
	Cmd_AddCommand("-speed", IN_SpeedUp);
	Cmd_AddCommand("+attack", IN_Button0Down);
	Cmd_AddCommand("-attack", IN_Button0Up);
	Cmd_AddCommand("centerview",IN_CenterView);
	if (GGameType & GAME_QuakeHexen)
	{
		Cmd_AddCommand("+jump", IN_Button1Down);
		Cmd_AddCommand("-jump", IN_Button1Up);
	}
	if (GGameType & GAME_Hexen2)
	{
		Cmd_AddCommand("+crouch", IN_Button2Down);
		Cmd_AddCommand("-crouch", IN_Button2Up);
	}
	if (GGameType & GAME_Quake2)
	{
		Cmd_AddCommand("+use", IN_Button1Down);
		Cmd_AddCommand("-use", IN_Button1Up);
	}
	if (GGameType & GAME_Quake3)
	{
		Cmd_AddCommand("+button0", IN_Button0Down);
		Cmd_AddCommand("-button0", IN_Button0Up);
		Cmd_AddCommand("+button1", IN_Button1Down);
		Cmd_AddCommand("-button1", IN_Button1Up);
		Cmd_AddCommand("+button2", IN_Button2Down);
		Cmd_AddCommand("-button2", IN_Button2Up);
		Cmd_AddCommand("+button3", IN_Button3Down);
		Cmd_AddCommand("-button3", IN_Button3Up);
		Cmd_AddCommand("+button4", IN_Button4Down);
		Cmd_AddCommand("-button4", IN_Button4Up);
		Cmd_AddCommand("+button5", IN_Button5Down);
		Cmd_AddCommand("-button5", IN_Button5Up);
		Cmd_AddCommand("+button6", IN_Button6Down);
		Cmd_AddCommand("-button6", IN_Button6Up);
		Cmd_AddCommand("+button7", IN_Button7Down);
		Cmd_AddCommand("-button7", IN_Button7Up);
		Cmd_AddCommand("+button8", IN_Button8Down);
		Cmd_AddCommand("-button8", IN_Button8Up);
		Cmd_AddCommand("+button9", IN_Button9Down);
		Cmd_AddCommand("-button9", IN_Button9Up);
		Cmd_AddCommand("+button10", IN_Button10Down);
		Cmd_AddCommand("-button10", IN_Button10Up);
		Cmd_AddCommand("+button11", IN_Button11Down);
		Cmd_AddCommand("-button11", IN_Button11Up);
		Cmd_AddCommand("+button12", IN_Button12Down);
		Cmd_AddCommand("-button12", IN_Button12Up);
		Cmd_AddCommand("+button13", IN_Button13Down);
		Cmd_AddCommand("-button13", IN_Button13Up);
		Cmd_AddCommand("+button14", IN_Button14Down);
		Cmd_AddCommand("-button14", IN_Button14Up);
	}

	if (GGameType & GAME_QuakeHexen)
	{
		v_centerspeed = Cvar_Get("v_centerspeed","500", 0);
	}
}
