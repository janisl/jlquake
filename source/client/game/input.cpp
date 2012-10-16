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

#include "../client.h"
#include "../game/wolfmp/local.h"

// builds an intended movement command to send to the server
/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

struct kbutton_t
{
	int down[2];		// key nums holding it down
	unsigned downtime;	// msec timestamp
	unsigned msec;		// msec down this frame if both a down and up happened
	bool active;		// current state
	bool wasPressed;	// set when down, not cleared when up
};

static unsigned frame_msec;
static int old_com_frameTime;

static kbutton_t in_left;
static kbutton_t in_right;
static kbutton_t in_forward;
static kbutton_t in_back;
static kbutton_t in_lookup;
static kbutton_t in_lookdown;
static kbutton_t in_moveleft;
static kbutton_t in_moveright;
static kbutton_t in_strafe;
static kbutton_t in_speed;
static kbutton_t in_up;
static kbutton_t in_down;
static kbutton_t in_buttons[16];
static kbutton_t in_kick;

static bool in_mlooking;

static Cvar* cl_yawspeed;
static Cvar* cl_pitchspeed;
static Cvar* cl_anglespeedkey;
Cvar* cl_forwardspeed;
static Cvar* cl_backspeed;
static Cvar* cl_sidespeed;
static Cvar* cl_upspeed;
static Cvar* cl_movespeedkey;
Cvar* cl_run;
Cvar* cl_freelook;
Cvar* cl_sensitivity;
static Cvar* cl_mouseAccel;
static Cvar* cl_showMouseRate;
static Cvar* m_filter;
Cvar* m_pitch;
static Cvar* m_yaw;
static Cvar* m_forward;
static Cvar* m_side;
Cvar* v_centerspeed;
Cvar* lookspring;
Cvar* cl_bypassMouseInput;
static Cvar* cl_doubletapdelay;
static Cvar* cl_recoilPitch;
static Cvar* cl_lightlevel;
Cvar* cl_debugMove;
Cvar* cl_nodelta;

int in_impulse;

// Arnout: doubleTap button mapping
static kbutton_t* dtmapping[] =
{
	NULL,				// ETDT_NONE
	&in_moveleft,		// ETDT_MOVELEFT
	&in_moveright,		// ETDT_MOVERIGHT
	&in_forward,		// ETDT_FORWARD
	&in_back,			// ETDT_BACK
	&in_buttons[12],	// ETDT_LEANLEFT
	&in_buttons[13],	// ETDT_LEANRIGHT
	&in_up				// ETDT_UP
};

static void IN_KeyDown(kbutton_t* b)
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

static void IN_KeyUp(kbutton_t* b)
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

void CL_MouseEvent(int dx, int dy)
{
	if (GGameType & GAME_Tech3 && in_keyCatchers & KEYCATCH_UI)
	{
		// NERVE - SMF - if we just want to pass it along to game
		if (GGameType & (GAME_WolfMP | GAME_ET) && cl_bypassMouseInput->integer == 1)
		{
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		}
		else
		{
			UI_MouseEvent(dx, dy);
		}
	}
	else if (GGameType & GAME_Tech3 && in_keyCatchers & KEYCATCH_CGAME)
	{
		if (GGameType & GAME_ET && cl_bypassMouseInput->integer == 1)
		{
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		}
		else
		{
			CLT3_MouseEvent(dx, dy);
		}
	}
	else
	{
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

//	Joystick values stay set until changed
void CL_JoystickEvent(int axis, int value)
{
	if (axis < 0 || axis >= MAX_JOYSTICK_AXIS)
	{
		common->Error("CL_JoystickEvent: bad axis %i", axis);
	}
	cl.joystickAxis[axis] = value;
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

static void IN_Button15Down()
{
	IN_KeyDown(&in_buttons[15]);
}

static void IN_Button15Up()
{
	IN_KeyUp(&in_buttons[15]);
}

static void IN_KickDown()
{
	IN_KeyDown(&in_kick);
}

static void IN_KickUp()
{
	IN_KeyUp(&in_kick);
}

//	Returns the fraction of the frame that the key was down
static float CL_KeyState(kbutton_t* key)
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

static void CLQH_StopPitchDrift()
{
	cl.qh_laststop = cl.qh_serverTimeFloat;
	cl.qh_nodrift = true;
	cl.qh_pitchvel = 0;
}

static void IN_CenterView()
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
	if (GGameType & GAME_WolfSP)
	{
		cl.viewangles[PITCH] = -SHORT2ANGLE(cl.ws_snap.ps.delta_angles[PITCH]);
	}
	if (GGameType & GAME_WolfMP)
	{
		if (CLWM_CheckCenterView())
		{
			cl.viewangles[PITCH] = -SHORT2ANGLE(cl.wm_snap.ps.delta_angles[PITCH]);
		}
	}
	//	Nothing for Enemy-territory
}

static void IN_MLookDown()
{
	in_mlooking = true;
}

static void IN_MLookUp()
{
	in_mlooking = false;
	if (!cl_freelook->value && (!(GGameType & GAME_Tech3) || lookspring->value))
	{
		IN_CenterView();
	}
}

static void Force_CenterView_f()
{
	cl.viewangles[PITCH] = 0;
}

static void IN_Impulse()
{
	in_impulse = String::Atoi(Cmd_Argv(1));
}

static void CLH2_SetIdealRoll(float delta)
{
	// FIXME: This is a cheap way of doing this, it belongs in V_CalcViewRoll
	// but I don't see where I can get the yaw velocity, I have to get on to other things so here it is
	if (cl.h2_idealroll != 0)
	{
		//	Keyboard set it already.
		return;
	}
	if (cl.h2_v.movetype != QHMOVETYPE_FLY)
	{
		return;
	}

	if (delta < 0)
	{
		cl.h2_idealroll = -10;
	}
	else if (delta > 0)
	{
		cl.h2_idealroll = 10;
	}
}

//	Moves the local angle positions
static void CL_AdjustAngles()
{
	if (GGameType & GAME_Hexen2)
	{
		cl.h2_idealroll = 0;
	}

	float speed = 0.001 * cls.frametime;
	if (in_speed.active || ((GGameType & GAME_HexenWorld) && cl.qh_spectator))
	{
		speed *= cl_anglespeedkey->value;
	}

	if (!in_strafe.active)
	{
		float right = CL_KeyState(&in_right);
		float left = CL_KeyState(&in_left);
		cl.viewangles[YAW] -= speed * cl_yawspeed->value * right;
		cl.viewangles[YAW] += speed * cl_yawspeed->value * left;
		if (GGameType & GAME_Hexen2)
		{
			CLH2_SetIdealRoll(right - left);
		}
	}

	float up = CL_KeyState(&in_lookup);
	float down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * down;

	if ((GGameType & GAME_QuakeHexen) && (up || down))
	{
		CLQH_StopPitchDrift();
	}
}

static void CL_KeyMove(in_usercmd_t* cmd)
{
	float forwardspeed;
	float backspeed;
	float sidespeed;
	float upspeed;
	float speedAdjust = 1;
	if (GGameType & GAME_Quake)
	{
		forwardspeed = cl_forwardspeed->value;
		backspeed = cl_backspeed->value;
		sidespeed = cl_sidespeed->value;
		upspeed = cl_upspeed->value;

		// adjust for speed key
		if (in_speed.active)
		{
			speedAdjust = cl_movespeedkey->value;
		}
	}
	else if (GGameType & GAME_Hexen2)
	{
		forwardspeed = 200;
		backspeed = 200;
		sidespeed = 225;
		upspeed = cl_upspeed->value;

		// adjust for speed key (but not if always runs has been chosen)
		if ((((GGameType & GAME_HexenWorld) && cl.qh_spectator) ||
			 cl_forwardspeed->value > 200 || in_speed.active) && cl.h2_v.hasted <= 1)
		{
			speedAdjust *= cl_movespeedkey->value;
		}
		// Hasted player?
		if (cl.h2_v.hasted)
		{
			speedAdjust *= cl.h2_v.hasted;
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		forwardspeed = cl_forwardspeed->value;
		backspeed = forwardspeed;
		sidespeed = cl_sidespeed->value;
		upspeed = cl_upspeed->value;

		// adjust for speed key / running
		if (in_speed.active ^ cl_run->integer)
		{
			speedAdjust = 2;
		}
	}
	else
	{
		// adjust for speed key / running
		if (in_speed.active ^ cl_run->integer)
		{
			forwardspeed = 127;
		}
		else
		{
			forwardspeed = 64;
		}
		backspeed = forwardspeed;
		sidespeed = forwardspeed;
		upspeed = forwardspeed;
	}

	forwardspeed *= speedAdjust;
	backspeed *= speedAdjust;
	sidespeed *= speedAdjust;
	upspeed *= speedAdjust;

	if (in_strafe.active)
	{
		float right = CL_KeyState(&in_right);
		float left = CL_KeyState(&in_left);
		cmd->sidemove += sidespeed * right;
		cmd->sidemove -= sidespeed * left;
		if (GGameType & GAME_Hexen2)
		{
			CLH2_SetIdealRoll(right - left);
		}
	}

	cmd->sidemove += sidespeed * CL_KeyState(&in_moveright);
	cmd->sidemove -= sidespeed * CL_KeyState(&in_moveleft);

	cmd->upmove += upspeed * CL_KeyState(&in_up);
	cmd->upmove -= upspeed * CL_KeyState(&in_down);

	cmd->forwardmove += forwardspeed * CL_KeyState(&in_forward);
	cmd->forwardmove -= backspeed * CL_KeyState(&in_back);
}

static void CL_MouseMove(in_usercmd_t* cmd)
{
	if ((GGameType & GAME_QuakeHexen) && (in_mlooking || cl_freelook->integer))
	{
		CLQH_StopPitchDrift();
	}

	float mx;
	float my;
	// allow mouse smoothing
	if (m_filter->integer)
	{
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5;
	}
	else
	{
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	float rate = sqrt(mx * mx + my * my) / (float)frame_msec;
	float accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	if (GGameType & GAME_Tech3)
	{
		// scale by FOV
		accelSensitivity *= cl.q3_cgameSensitivity;
	}

	if (rate && cl_showMouseRate->integer)
	{
		common->Printf("%f : %f\n", rate, accelSensitivity);
	}

	// Rafael - mg42
	if ((GGameType & GAME_WolfSP && cl.ws_snap.ps.persistant[WSPERS_HWEAPON_USE]) ||
		(GGameType & GAME_WolfMP && cl.wm_snap.ps.persistant[WMPERS_HWEAPON_USE]) ||
		(GGameType & GAME_ET && cl.et_snap.ps.persistant[ETPERS_HWEAPON_USE]))
	{
		mx *= 2.5;
		my *= 2;
	}
	else
	{
		mx *= accelSensitivity;
		my *= accelSensitivity;
	}

	if (!mx && !my)
	{
		return;
	}

	// add mouse X/Y movement to cmd
	if (in_strafe.active)
	{
		cmd->sidemove += m_side->value * mx;
	}
	else
	{
		cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ((in_mlooking || cl_freelook->integer) && !in_strafe.active)
	{
		cl.viewangles[PITCH] += m_pitch->value * my;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * my;
	}

	if (GGameType & GAME_Hexen2)
	{
		CLH2_SetIdealRoll(mx);
	}
}

static void CL_JoystickMove(in_usercmd_t* cmd)
{
	float movespeed = (GGameType & GAME_Tech3) ? 1 : 400.0 / 127.0;
	float anglespeed;
	if (in_speed.active)
	{
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	}
	else
	{
		anglespeed = 0.001 * cls.frametime;
	}

	if (!in_strafe.active)
	{
		cl.viewangles[YAW] += anglespeed * cl_yawspeed->value * cl.joystickAxis[AXIS_SIDE];
	}
	else
	{
		cmd->sidemove += movespeed * cl.joystickAxis[AXIS_SIDE];
	}

	if (in_mlooking)
	{
		cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value * cl.joystickAxis[AXIS_FORWARD];
	}
	else
	{
		cmd->forwardmove += movespeed * cl.joystickAxis[AXIS_FORWARD];
	}

	cmd->upmove += cl.joystickAxis[AXIS_UP];
}

static void CL_ClampAngles(float oldPitch)
{
	if (GGameType & GAME_QuakeHexen)
	{
		cl.viewangles[YAW] = AngleMod(cl.viewangles[YAW]);

		if (cl.viewangles[PITCH] > 80)
		{
			cl.viewangles[PITCH] = 80;
		}
		if (cl.viewangles[PITCH] < -70)
		{
			cl.viewangles[PITCH] = -70;
		}

		if (cl.viewangles[ROLL] > 50)
		{
			cl.viewangles[ROLL] = 50;
		}
		if (cl.viewangles[ROLL] < -50)
		{
			cl.viewangles[ROLL] = -50;
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		oldPitch = SHORT2ANGLE(cl.q2_frame.playerstate.pmove.delta_angles[PITCH]);
		if (oldPitch > 180)
		{
			oldPitch -= 360;
		}
		if (cl.viewangles[PITCH] + oldPitch > 89)
		{
			cl.viewangles[PITCH] = 89 - oldPitch;
		}
		if (cl.viewangles[PITCH] + oldPitch < -89)
		{
			cl.viewangles[PITCH] = -89 - oldPitch;
		}
	}
	else
	{
		// check to make sure the angles haven't wrapped
		if (cl.viewangles[PITCH] - oldPitch > 90)
		{
			cl.viewangles[PITCH] = oldPitch + 90;
		}
		else if (oldPitch - cl.viewangles[PITCH] > 90)
		{
			cl.viewangles[PITCH] = oldPitch - 90;
		}
	}
}

static void CL_CmdButtons(in_usercmd_t* cmd)
{
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	int numButtons = (GGameType & (GAME_Quake | GAME_Quake2)) ? 2 :
					 (GGameType & GAME_Hexen2) ? 3 :
					 (GGameType & GAME_ET) ? 16 : 15;
	for (int i = 0; i < numButtons; i++)
	{
		if (in_buttons[i].active || in_buttons[i].wasPressed)
		{
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = false;
	}

	if (GGameType & GAME_Quake2)
	{
		if (anykeydown && in_keyCatchers == 0)
		{
			cmd->buttons |= Q2BUTTON_ANY;
		}
	}

	if (GGameType & GAME_Tech3)
	{
		if (in_keyCatchers && (!(GGameType & (GAME_WolfMP | GAME_ET)) || !cl_bypassMouseInput->integer))
		{
			cmd->buttons |= Q3BUTTON_TALK;
		}

		// the walking flag is to keep animations consistant
		// even during acceleration and develeration
		if (in_speed.active ^ cl_run->integer)
		{
			cmd->buttons &= ~Q3BUTTON_WALKING;
		}
		else
		{
			cmd->buttons |= Q3BUTTON_WALKING;
		}
	}

	if (GGameType & GAME_Quake3)
	{
		// allow the game to know if any key at all is
		// currently pressed, even if it isn't bound to anything
		if (anykeydown && !in_keyCatchers)
		{
			cmd->buttons |= Q3BUTTON_ANY;
		}
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		// allow the game to know if any key at all is
		// currently pressed, even if it isn't bound to anything
		if (anykeydown && (!in_keyCatchers ||
						   (GGameType & (GAME_WolfMP | GAME_ET) && cl_bypassMouseInput->integer)))
		{
			cmd->buttons |= WOLFBUTTON_ANY;
		}

		if (cmd->buttons & WOLFBUTTON_ACTIVATE)
		{
			if (cmd->sidemove > 0)
			{
				cmd->buttons |= WOLFBUTTON_LEANRIGHT;
			}
			else if (cmd->sidemove < 0)
			{
				cmd->buttons |= WOLFBUTTON_LEANLEFT;
			}
			// disallow the strafe when holding 'activate'
			cmd->sidemove = 0;
		}
	}
}

void CL_DoubleTap(in_usercmd_t* cmd)
{
	if (!(GGameType & GAME_ET))
	{
		return;
	}

	// Arnout: clear 'waspressed' from double tap buttons
	for (int i = 1; i < ETDT_NUM; i++)
	{
		dtmapping[i]->wasPressed = false;
	}

	// Arnout: double tap
	cmd->doubleTap = ETDT_NONE;	// reset
	if (com_frameTime - cl.et_doubleTap.lastdoubleTap > cl_doubletapdelay->integer + 150 + cls.frametime)
	{
		// double tap only once every 500 msecs (add
		// frametime for low(-ish) fps situations)
		for (int i = 1; i < ETDT_NUM; i++)
		{
			bool key_down = dtmapping[i]->active || dtmapping[i]->wasPressed;

			if (key_down && !cl.et_doubleTap.pressedTime[i])
			{
				cl.et_doubleTap.pressedTime[i] = com_frameTime;
			}
			else if (!key_down && !cl.et_doubleTap.releasedTime[i] &&
					 (com_frameTime - cl.et_doubleTap.pressedTime[i]) < (cl_doubletapdelay->integer + cls.frametime))
			{
				cl.et_doubleTap.releasedTime[i] = com_frameTime;
			}
			else if (key_down &&
					 (com_frameTime - cl.et_doubleTap.pressedTime[i]) < (cl_doubletapdelay->integer + cls.frametime) &&
					 (com_frameTime - cl.et_doubleTap.releasedTime[i]) < (cl_doubletapdelay->integer + cls.frametime))
			{
				cl.et_doubleTap.pressedTime[i] = cl.et_doubleTap.releasedTime[i] = 0;
				cmd->doubleTap = i;
				cl.et_doubleTap.lastdoubleTap = com_frameTime;
			}
			else if (!key_down && (cl.et_doubleTap.pressedTime[i] || cl.et_doubleTap.releasedTime[i]))
			{
				if (com_frameTime - cl.et_doubleTap.pressedTime[i] >= (cl_doubletapdelay->integer + cls.frametime))
				{
					cl.et_doubleTap.pressedTime[i] = cl.et_doubleTap.releasedTime[i] = 0;
				}
			}
		}
	}
}

static void CL_Kick(in_usercmd_t* cmd)
{
	if (!(GGameType & (GAME_WolfSP | GAME_WolfMP)))
	{
		return;
	}
	cmd->kick = ClampChar(CL_KeyState(&in_kick));
}

static void CL_RecoilPitch()
{
	if (!(GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET)))
	{
		return;
	}

	// RF, set the kickAngles so aiming is effected
	float recoilAdd = cl_recoilPitch->value;
	if (Q_fabs(cl.viewangles[PITCH] + recoilAdd) < 40)
	{
		cl.viewangles[PITCH] += recoilAdd;
	}
	// the recoilPitch has been used, so clear it out
	cl_recoilPitch->value = 0;
}

static int MakeChar(int i)
{
	i &= ~3;
	if (i < -127 * 4)
	{
		i = -127 * 4;
	}
	if (i > 127 * 4)
	{
		i = 127 * 4;
	}
	return i;
}

static void CL_FinishMove(in_usercmd_t* cmd)
{
	if (GGameType & GAME_HexenWorld && (int)cl.h2_v.artifact_active & HWARTFLAG_FROZEN)
	{
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
		cmd->upmove = 0;
	}
	if ((GGameType & GAME_WolfSP && cl.ws_snap.ps.persistant[WSPERS_HWEAPON_USE]) ||
		(GGameType & GAME_WolfMP && cl.wm_snap.ps.persistant[WMPERS_HWEAPON_USE]))
	{
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
		cmd->upmove = 0;
		cmd->kick = 0;
	}

	VectorCopy(cl.viewangles, cmd->fAngles);
	for (int i = 0; i < 3; i++)
	{
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		//
		// chop down so no extra bits are kept that the server wouldn't get
		//
		cmd->forwardmove = MakeChar(cmd->forwardmove);
		cmd->sidemove = MakeChar(cmd->sidemove);
		cmd->upmove = MakeChar(cmd->upmove);

		for (int i = 0; i < 3; i++)
		{
			cmd->fAngles[i] = cmd->angles[i] * (360.0 / 65536.0);
		}
	}

	if (!(GGameType & GAME_Tech3))
	{
		cmd->impulse = in_impulse;
		in_impulse = 0;
	}

	if (GGameType & GAME_QuakeHexen && !(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		cmd->mtime = cl.qh_mtime[0];	// so server can get ping times
	}
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2))
	{
		// send milliseconds of time to apply the move
		int ms = cls.frametime;
		if (ms > 250)
		{
			ms = 100;		// time was unreasonable
		}
		cmd->msec = ms;
	}

	if (GGameType & (GAME_Hexen2 | GAME_Quake2))
	{
		// send the ambient light level at the player's current position
		cmd->lightlevel = (byte)cl_lightlevel->value;
	}

	if (GGameType & GAME_Tech3)
	{
		cmd->forwardmove = ClampChar(cmd->forwardmove);
		cmd->sidemove = ClampChar(cmd->sidemove);
		cmd->upmove = ClampChar(cmd->upmove);

		// send the current server time so the amount of movement
		// can be determined without allowing cheating
		cmd->serverTime = cl.serverTime;

		// copy the state that the cgame is currently sending
		cmd->weapon = cl.q3_cgameUserCmdValue;
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		cmd->holdable = cl.wb_cgameUserHoldableValue;	//----(SA)	modified
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		cmd->identClient = cl.wm_cgameMpIdentClient;	// NERVE - SMF
	}
	if (GGameType & GAME_WolfSP)
	{
		cmd->cld = cl.ws_cgameCld;			// NERVE - SMF
	}
	if (GGameType & GAME_WolfMP)
	{
		cmd->mpSetup = cl.wm_cgameMpSetup;				// NERVE - SMF
	}
	if (GGameType & GAME_ET)
	{
		cmd->flags = cl.et_cgameFlags;
	}
}

in_usercmd_t CL_CreateCmd()
{
	frame_msec = com_frameTime - old_com_frameTime;
	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if (frame_msec < 1)
	{
		frame_msec = 1;
	}
	if (frame_msec > 200)
	{
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;

	vec3_t oldAngles;
	VectorCopy(cl.viewangles, oldAngles);

	in_usercmd_t cmd;
	Com_Memset(&cmd, 0, sizeof(cmd));

	if (!(GGameType & GAME_Hexen2) || !cl.h2_v.cameramode)	// Stuck in a different camera so don't move
	{
		// keyboard angle adjustment
		CL_AdjustAngles();

		// get basic movement from keyboard
		CL_KeyMove(&cmd);

		// get basic movement from mouse
		CL_MouseMove(&cmd);

		// get basic movement from joystick
		CL_JoystickMove(&cmd);

		CL_ClampAngles(oldAngles[PITCH]);
	}

	CL_CmdButtons(&cmd);

	CL_DoubleTap(&cmd);

	CL_Kick(&cmd);

	CL_RecoilPitch();

	// if we are spectator, try autocam
	Cam_Track(&cmd);

	// store out the final values
	CL_FinishMove(&cmd);

	Cam_FinishMove(&cmd);

	// draw debug graphs of turning for mouse testing
	if (cl_debugMove->integer)
	{
		if (cl_debugMove->integer == 1)
		{
			SCR_DebugGraph(abs(cl.viewangles[YAW] - oldAngles[YAW]), 0);
		}
		if (cl_debugMove->integer == 2)
		{
			SCR_DebugGraph(abs(cl.viewangles[PITCH] - oldAngles[PITCH]), 0);
		}
	}

	return cmd;
}

void CL_InitInput()
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
	Cmd_AddCommand("+mlook", IN_MLookDown);
	Cmd_AddCommand("-mlook", IN_MLookUp);
	if (!(GGameType & GAME_ET))
	{
		Cmd_AddCommand("centerview", IN_CenterView);
	}
	if (!(GGameType & GAME_Tech3))
	{
		Cmd_AddCommand("impulse", IN_Impulse);
	}
	if (GGameType & GAME_QuakeHexen)
	{
		Cmd_AddCommand("+jump", IN_Button1Down);
		Cmd_AddCommand("-jump", IN_Button1Up);
		Cmd_AddCommand("force_centerview", Force_CenterView_f);
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
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		Cmd_AddCommand("+button1", IN_Button1Down);
		Cmd_AddCommand("-button1", IN_Button1Up);
		Cmd_AddCommand("+useitem", IN_Button2Down);
		Cmd_AddCommand("-useitem", IN_Button2Up);
		Cmd_AddCommand("+salute", IN_Button3Down);
		Cmd_AddCommand("-salute", IN_Button3Up);
		Cmd_AddCommand("+button4", IN_Button4Down);
		Cmd_AddCommand("-button4", IN_Button4Up);
		Cmd_AddCommand("+sprint", IN_Button5Down);
		Cmd_AddCommand("-sprint", IN_Button5Up);
		Cmd_AddCommand("+activate", IN_Button6Down);
		Cmd_AddCommand("-activate", IN_Button6Up);
		Cmd_AddCommand("+attack2", IN_Button8Down);
		Cmd_AddCommand("-attack2", IN_Button8Up);
		Cmd_AddCommand("+zoom", IN_Button9Down);
		Cmd_AddCommand("-zoom", IN_Button9Up);
		Cmd_AddCommand("+reload", IN_Button11Down);
		Cmd_AddCommand("-reload", IN_Button11Up);
		Cmd_AddCommand("+leanleft", IN_Button12Down);
		Cmd_AddCommand("-leanleft", IN_Button12Up);
		Cmd_AddCommand("+leanright", IN_Button13Down);
		Cmd_AddCommand("-leanright", IN_Button13Up);
	}
	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		Cmd_AddCommand("+quickgren", IN_Button10Down);
		Cmd_AddCommand("-quickgren", IN_Button10Up);
		Cmd_AddCommand("+wbutton7", IN_Button15Down);
		Cmd_AddCommand("-wbutton7", IN_Button15Up);
		Cmd_AddCommand("+kick", IN_KickDown);
		Cmd_AddCommand("-kick", IN_KickUp);
	}
	if (GGameType & GAME_WolfSP)
	{
		Cmd_AddCommand("+wbutton6", IN_Button14Down);
		Cmd_AddCommand("-wbutton6", IN_Button14Up);
	}
	if (GGameType & GAME_WolfMP)
	{
		Cmd_AddCommand("+dropweapon", IN_Button14Down);
		Cmd_AddCommand("-dropweapon", IN_Button14Up);
	}
	if (GGameType & GAME_ET)
	{
		Cmd_AddCommand("+prone", IN_Button15Down);
		Cmd_AddCommand("-prone", IN_Button15Up);
	}

	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);
	cl_freelook = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE);
	cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0);
#ifdef MACOS_X
	// Input is jittery on OS X w/o this
	m_filter = Cvar_Get("m_filter", "1", CVAR_ARCHIVE);
#else
	m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE);
#endif
	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	cl_debugMove = Cvar_Get("cl_debugMove", "0", 0);
	if (!(GGameType & GAME_QuakeHexen) || GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
	}
	if (!(GGameType & GAME_Tech3))
	{
		m_forward = Cvar_Get("m_forward", "1", CVAR_ARCHIVE);
		m_side = Cvar_Get("m_side", "1", CVAR_ARCHIVE);
		lookspring = Cvar_Get("lookspring", "0", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_QuakeHexen)
	{
		cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", CVAR_ARCHIVE);
		cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
		cl_movespeedkey = Cvar_Get("cl_movespeedkey", "2.0", 0);
		v_centerspeed = Cvar_Get("v_centerspeed","500", 0);
	}
	if (GGameType & GAME_Quake)
	{
		cl_backspeed = Cvar_Get("cl_backspeed", "200", CVAR_ARCHIVE);
		cl_sidespeed = Cvar_Get("cl_sidespeed","350", 0);
	}
	if (GGameType & (GAME_Hexen2 | GAME_Quake2))
	{
		cl_lightlevel = Cvar_Get("r_lightlevel", "0", 0);
	}
	if (GGameType & GAME_Quake2)
	{
		cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", 0);
		cl_sidespeed = Cvar_Get("cl_sidespeed", "200", 0);
		cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
		cl_run = Cvar_Get("cl_run", "0", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_Tech3)
	{
		cl_run = Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
		m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE);
		m_side = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE);
	}
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		cl_recoilPitch = Cvar_Get("cg_recoilPitch", "0", CVAR_ROM);
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		cl_bypassMouseInput = Cvar_Get("cl_bypassMouseInput", "0", 0);
	}
	if (GGameType & GAME_ET)
	{
		cl_doubletapdelay = Cvar_Get("cl_doubletapdelay", "350", CVAR_ARCHIVE);
	}
}

void CL_ClearKeys()
{
	Com_Memset(&in_left, 0, sizeof(in_left));
	Com_Memset(&in_right, 0, sizeof(in_right));
	Com_Memset(&in_forward, 0, sizeof(in_forward));
	Com_Memset(&in_back, 0, sizeof(in_back));
	Com_Memset(&in_lookup, 0, sizeof(in_lookup));
	Com_Memset(&in_lookdown, 0, sizeof(in_lookdown));
	Com_Memset(&in_moveleft, 0, sizeof(in_moveleft));
	Com_Memset(&in_moveright, 0, sizeof(in_moveright));
	Com_Memset(&in_strafe, 0, sizeof(in_strafe));
	Com_Memset(&in_speed, 0, sizeof(in_speed));
	Com_Memset(&in_up, 0, sizeof(in_up));
	Com_Memset(&in_down, 0, sizeof(in_down));
	Com_Memset(in_buttons, 0, sizeof(in_buttons));
	Com_Memset(&in_kick, 0, sizeof(in_kick));
	in_mlooking = false;
}
