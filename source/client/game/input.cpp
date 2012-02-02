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

//	Quake and Hexen 2.
kbutton_t in_mlook;

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
	}
}
