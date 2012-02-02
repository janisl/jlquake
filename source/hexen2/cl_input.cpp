// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


int					in_impulse;
#ifdef MISSIONPACK
extern qboolean		info_up;
#endif

static Cvar*	m_filter;

static int	mouse_move_x;
static int	mouse_move_y;
static int	old_mouse_x, old_mouse_y;

void IN_KLookDown (void) {IN_KeyDown(&in_klook);}
void IN_KLookUp (void) {IN_KeyUp(&in_klook);}
void IN_MLookDown (void) {IN_KeyDown(&in_mlook);}
void IN_MLookUp (void) {
IN_KeyUp(&in_mlook);
if ( !(in_mlook.active) &&  lookspring->value)
	V_StartPitchDrift();
}

void IN_UseDown (void) {IN_KeyDown(&in_use);}
void IN_UseUp (void) {IN_KeyUp(&in_use);}

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

//==========================================================================

Cvar*	cl_upspeed;
Cvar*	cl_forwardspeed;
Cvar*	cl_backspeed;
Cvar*	cl_sidespeed;

Cvar*	cl_movespeedkey;

Cvar*	cl_yawspeed;
Cvar*	cl_pitchspeed;

Cvar*	cl_anglespeedkey;

Cvar*	cl_prettylights;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;
	
	if (in_speed.active)
		speed = host_frametime * cl_anglespeedkey->value;
	else
		speed = host_frametime;

	if (!in_strafe.active)
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
		cl.viewangles[YAW] = AngleMod(cl.viewangles[YAW]);
	}
	if (in_klook.active)
	{
		V_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}

	// FIXME: This is a cheap way of doing this, it belongs in V_CalcViewRoll
	// but I don't see where I can get the yaw velocity, I have to get on to other things so here it is

	if ((CL_KeyState (&in_left)!=0) && (cl.h2_v.movetype==MOVETYPE_FLY))
		cl.h2_idealroll=-10;
	else if ((CL_KeyState (&in_right)!=0) && (cl.h2_v.movetype==MOVETYPE_FLY))
		cl.h2_idealroll=10;
	else
		cl.h2_idealroll=0;

	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;

	if (up || down)
		V_StopPitchDrift ();
		
	if (cl.viewangles[PITCH] > 80)
		cl.viewangles[PITCH] = 80;
	if (cl.viewangles[PITCH] < -70)
		cl.viewangles[PITCH] = -70;

	if (cl.viewangles[ROLL] > 50)
		cl.viewangles[ROLL] = 50;
	if (cl.viewangles[ROLL] < -50)
		cl.viewangles[ROLL] = -50;
		
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (h2usercmd_t *cmd)
{	
	if (clc.qh_signon != SIGNONS)
		return;
			
	if (cl.h2_v.cameramode)	// Stuck in a different camera so don't move
	{
		Com_Memset(cmd, 0, sizeof(*cmd));
		return;
	}

	// grab frame time 
	com_frameTime = Sys_Milliseconds();

	frame_msec = (unsigned)(host_frametime * 1000);

	CL_AdjustAngles ();
	
	Com_Memset(cmd, 0, sizeof(*cmd));
	
	if (in_strafe.active)
	{
//		cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
//		cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
		cmd->sidemove += 225 * CL_KeyState (&in_right);
		cmd->sidemove -= 225 * CL_KeyState (&in_left);
	}

//	cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
//	cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);
	cmd->sidemove += 225 * CL_KeyState (&in_moveright);
	cmd->sidemove -= 225 * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

	if (!in_klook.active)
	{	
//		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
		cmd->forwardmove += 200 * CL_KeyState (&in_forward);
//		cmd->forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
		cmd->forwardmove -= 200 * CL_KeyState (&in_back);
	}	

//
// adjust for speed key (but not if always runs has been chosen)
//
	if ((cl_forwardspeed->value > 200 || in_speed.active) && cl.h2_v.hasted <= 1)
	{
		cmd->forwardmove *= cl_movespeedkey->value;
		cmd->sidemove *= cl_movespeedkey->value;
		cmd->upmove *= cl_movespeedkey->value;
	}

	// Hasted player?
	if (cl.h2_v.hasted)
	{
		cmd->forwardmove = cmd->forwardmove * cl.h2_v.hasted;
		cmd->sidemove = cmd->sidemove * cl.h2_v.hasted;
		cmd->upmove = cmd->upmove * cl.h2_v.hasted;
	}

	// light level at player's position including dlights
	// this is sent back to the server each frame
	// architectually ugly but it works
	cmd->lightlevel = (byte)cl_lightlevel->value;
}


void CL_MouseEvent(int mx, int my)
{
	mouse_move_x += mx;
	mouse_move_y += my;
}

void CL_MouseMove(h2usercmd_t *cmd)
{
	if (cl.h2_v.cameramode)	// Stuck in a different camera so don't move
	{
		Com_Memset(cmd, 0, sizeof(*cmd));
		return;
	}

	int mouse_x = mouse_move_x;
	int mouse_y = mouse_move_y;
	if (m_filter->value)
	{
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}

	old_mouse_x = mouse_move_x;
	old_mouse_y = mouse_move_y;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
	if (in_strafe.active || (lookstrafe->value && in_mlook.active))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if (in_mlook.active)
		V_StopPitchDrift ();
		
	if (in_mlook.active && !in_strafe.active)
	{
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if (in_strafe.active && noclip_anglehack)
			cmd->upmove -= m_forward->value * mouse_y;
		else
			cmd->forwardmove -= m_forward->value * mouse_y;
	}

	if (cl.h2_idealroll == 0) // Did keyboard set it already??
	{
		if ((mouse_x <0) && (cl.h2_v.movetype==MOVETYPE_FLY))
			cl.h2_idealroll=-10;
		else if ((mouse_x >0) && (cl.h2_v.movetype==MOVETYPE_FLY))
			cl.h2_idealroll=10;
	}
	mouse_move_x = 0;
	mouse_move_y = 0;
}

/*
==============
CL_SendMove
==============
*/
void CL_SendMove (h2usercmd_t *cmd)
{
	int		i;
	int		bits;
	QMsg	buf;
	byte	data[128];
	
	buf.InitOOB(data, 128);
	
	cl.h2_cmd = *cmd;

//
// send the movement message
//
	buf.WriteByte(h2clc_frame);
	buf.WriteByte(cl.h2_reference_frame);
	buf.WriteByte(cl.h2_current_sequence);

	buf.WriteByte(h2clc_move);

	buf.WriteFloat(cl.qh_mtime[0]);	// so server can get ping times

	for (i=0 ; i<3 ; i++)
		buf.WriteAngle(cl.viewangles[i]);
	
    buf.WriteShort(cmd->forwardmove);
    buf.WriteShort(cmd->sidemove);
    buf.WriteShort(cmd->upmove);

//
// send button bits
//
	bits = 0;
	
	if (in_buttons[0].active || in_buttons[0].wasPressed)
		bits |= 1;
	in_buttons[0].wasPressed = false;
	
	if (in_buttons[1].active || in_buttons[1].wasPressed)
		bits |= 2;
	in_buttons[1].wasPressed = false;
	
	if (in_crouch.active)
		bits |= 4;

    buf.WriteByte(bits);

    buf.WriteByte(in_impulse);
	in_impulse = 0;

//
// light level
//
	buf.WriteByte(cmd->lightlevel);

//
// deliver the message
//
	if (clc.demoplaying)
		return;

//
// allways dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (++cl.qh_movemessages <= 2)
		return;
	
	if (NET_SendUnreliableMessage (cls.qh_netcon, &clc.netchan, &buf) == -1)
	{
		Con_Printf ("CL_SendMove: lost server connection\n");
		CL_Disconnect ();
	}
}

void IN_CrouchDown (void)
{
	if (in_keyCatchers == 0)
	{
		IN_KeyDown(&in_crouch);
	}
}

void IN_CrouchUp (void)
{
	if (in_keyCatchers == 0)
	{
		IN_KeyUp(&in_crouch);
	}
}

#ifdef MISSIONPACK
void IN_infoPlaqueUp(void)
{
	if (in_keyCatchers == 0)
	{
		//They want to lower the plaque
		info_up = 0;
	}
}

void IN_infoPlaqueDown(void)
{
	if (in_keyCatchers == 0)
	{
		//They want to see the plaque
		info_up = 1;
	}
}
#endif

/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	CL_InitInputCommon();
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	Cmd_AddCommand ("+crouch", IN_CrouchDown);
	Cmd_AddCommand ("-crouch", IN_CrouchUp);

#ifdef MISSIONPACK
	Cmd_AddCommand ("+infoplaque", IN_infoPlaqueDown);
	Cmd_AddCommand ("-infoplaque", IN_infoPlaqueUp);
#endif

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

    m_filter = Cvar_Get("m_filter", "0", 0);
}

static void ClearState(kbutton_t& button)
{
	button.active = false;
	button.wasPressed = false;
}

/*
============
CL_ClearStates
============
*/
void CL_ClearStates (void)
{
	ClearState(in_mlook);
	ClearState(in_klook);
	ClearState(in_left);
	ClearState(in_right);
	ClearState(in_forward);
	ClearState(in_back);
	ClearState(in_lookup);
	ClearState(in_lookdown);
	ClearState(in_moveleft);
	ClearState(in_moveright);
	ClearState(in_strafe);
	ClearState(in_speed);
	ClearState(in_use);
	ClearState(in_buttons[1]);
	ClearState(in_buttons[0]);
	ClearState(in_up);
	ClearState(in_down);
	ClearState(in_crouch);
}
