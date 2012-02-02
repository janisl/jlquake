/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

Cvar	*cl_nodelta;

unsigned	old_sys_frame_time;

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


Key_Event (int key, qboolean down, unsigned time);

  +mlook src time

===============================================================================
*/


int			in_impulse;

static Cvar	*m_filter;

static int	mouse_move_x;
static int	mouse_move_y;
static int	old_mouse_x, old_mouse_y;

static qboolean	mlooking;

void IN_KLookDown (void) {IN_KeyDown(&in_klook);}
void IN_KLookUp (void) {IN_KeyUp(&in_klook);}

void IN_UseDown (void) {IN_KeyDown(&in_use);}
void IN_UseUp (void) {IN_KeyUp(&in_use);}

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

//==========================================================================

Cvar	*cl_upspeed;
Cvar	*cl_forwardspeed;
Cvar	*cl_sidespeed;

Cvar	*cl_yawspeed;
Cvar	*cl_pitchspeed;

Cvar	*cl_run;

Cvar	*cl_anglespeedkey;


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
		speed = cls.q2_frametimeFloat * cl_anglespeedkey->value;
	else
		speed = cls.q2_frametimeFloat;

	if (!in_strafe.active)
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
	}
	if (in_klook.active)
	{
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (q2usercmd_t *cmd)
{	
	CL_AdjustAngles ();
	
	Com_Memset(cmd, 0, sizeof(*cmd));
	
	VectorCopy (cl.viewangles, cmd->angles);
	if (in_strafe.active)
	{
		cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_right);
		cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_left);
	}

	cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

	if (! in_klook.active )
	{	
		cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward);
		cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState (&in_back);
	}	

//
// adjust for speed key / running
//
	if ( (in_speed.active) ^ (int)(cl_run->value) )
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}	
}

void CL_MouseEvent(int mx, int my)
{
	mouse_move_x += mx;
	mouse_move_y += my;
}

void CL_MouseMove(q2usercmd_t *cmd)
{
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
	if ( in_strafe.active || (lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if ( (mlooking || freelook->value) && !in_strafe.active)
	{
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * mouse_y;
	}
	mouse_move_x = 0;
	mouse_move_y = 0;
}

void CL_ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.q2_frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;
	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove (q2usercmd_t *cmd)
{
	int		ms;
	int		i;

//
// figure button bits
//	
	if (in_buttons[0].active || in_buttons[0].wasPressed)
		cmd->buttons |= BUTTON_ATTACK;
	in_buttons[0].wasPressed = false;
	
	if (in_use.active || in_use.wasPressed)
		cmd->buttons |= BUTTON_USE;
	in_use.wasPressed = false;

	if (anykeydown && in_keyCatchers == 0)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = cls.q2_frametimeFloat * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch ();
	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse = in_impulse;
	in_impulse = 0;

// send the ambient light level at the player's current position
	cmd->lightlevel = (byte)cl_lightlevel->value;
}

/*
=================
CL_CreateCmd
=================
*/
q2usercmd_t CL_CreateCmd (void)
{
	q2usercmd_t	cmd;

	// grab frame time 
	com_frameTime = Sys_Milliseconds_();

	frame_msec = com_frameTime - old_sys_frame_time;
	if (frame_msec < 1)
		frame_msec = 1;
	if (frame_msec > 200)
		frame_msec = 200;
	
	// get basic movement from keyboard
	CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
	CL_MouseMove(&cmd);

	CL_FinishMove (&cmd);

	old_sys_frame_time = com_frameTime;

//cmd.impulse = cls.framecount;

	return cmd;
}


void IN_CenterView (void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.q2_frame.playerstate.pmove.delta_angles[PITCH]);
}

static void IN_MLookDown()
{
	mlooking = true;
}

static void IN_MLookUp()
{
	mlooking = false;
	if (!freelook->value && lookspring->value)
		IN_CenterView ();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	CL_InitInputCommon();
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	m_filter = Cvar_Get ("m_filter", "0", 0);
}



/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	QMsg		buf;
	byte		data[128];
	int			i;
	q2usercmd_t	*cmd, *oldcmd;
	q2usercmd_t	nullcmd;
	int			checksumIndex;

	// build a command even if not connected

	// save this command off for prediction
	i = clc.netchan.outgoingSequence & (CMD_BACKUP_Q2-1);
	cmd = &cl.q2_cmds[i];
	cl.q2_cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd ();

	if (cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING)
		return;

	if ( cls.state == CA_CONNECTED)
	{
		if (clc.netchan.message.cursize	|| curtime - clc.netchan.lastSent > 1000 )
			Netchan_Transmit (&clc.netchan, 0, buf._data);	
		return;
	}

	// send a userinfo update if needed
	if (cvar_modifiedFlags & CVAR_USERINFO)
	{
		CL_FixUpGender();
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		clc.netchan.message.WriteByte(q2clc_userinfo);
		clc.netchan.message.WriteString2(Cvar_InfoString(
			CVAR_USERINFO, MAX_INFO_STRING, MAX_INFO_KEY, MAX_INFO_VALUE, true, false));
	}

	buf.InitOOB(data, sizeof(data));

	if (cmd->buttons)
	{
		// skip the rest of the cinematic
		CIN_SkipCinematic();
	}

	// begin a client move command
	buf.WriteByte(q2clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	buf.WriteByte(0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->value || !cl.q2_frame.valid || cls.q2_demowaiting)
		buf.WriteLong(-1);	// no compression
	else
		buf.WriteLong(cl.q2_frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (clc.netchan.outgoingSequence-2) & (CMD_BACKUP_Q2-1);
	cmd = &cl.q2_cmds[i];
	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence-1) & (CMD_BACKUP_Q2-1);
	cmd = &cl.q2_cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence) & (CMD_BACKUP_Q2-1);
	cmd = &cl.q2_cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf._data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf._data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		clc.netchan.outgoingSequence);

	//
	// deliver the message
	//
	Netchan_Transmit (&clc.netchan, buf.cursize, buf._data);	
}


