/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "quakedef.h"

Cvar*	cl_nodelta;

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


int			in_impulse;

static Cvar*	m_filter;

static int	old_mouse_x, old_mouse_y;
static int	mouse_move_x;
static int	mouse_move_y;

void IN_KLookDown (void) {IN_KeyDown(&in_klook);}
void IN_KLookUp (void) {IN_KeyUp(&in_klook);}

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
		CLQH_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;

	if (up || down)
		CLQH_StopPitchDrift ();
		
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
void CL_BaseMove (qwusercmd_t *cmd)
{	
	// grab frame time 
	com_frameTime = Sys_Milliseconds();

	frame_msec = (unsigned)(host_frametime * 1000);

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
		cmd->forwardmove -= cl_backspeed->value * CL_KeyState (&in_back);
	}	

//
// adjust for speed key
//
	if (in_speed.active)
	{
		cmd->forwardmove *= cl_movespeedkey->value;
		cmd->sidemove *= cl_movespeedkey->value;
		cmd->upmove *= cl_movespeedkey->value;
	}	
}

void CL_MouseEvent(int mx, int my)
{
	mouse_move_x += mx;
	mouse_move_y += my;
}

void CL_MouseMove(qwusercmd_t *cmd)
{
	int mouse_x = mouse_move_x;
	int mouse_y = mouse_move_y;
	if (m_filter->value)
	{
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_x + old_mouse_y) * 0.5;
	}

	old_mouse_x = mouse_move_x;
	old_mouse_y = mouse_move_y;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( in_strafe.active || (lookstrafe->value && in_mlooking))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if (in_mlooking)
		CLQH_StopPitchDrift ();
		
	if (in_mlooking && !in_strafe.active)
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
	mouse_move_x = 0;
	mouse_move_y = 0;
}

int MakeChar (int i)
{
	i &= ~3;
	if (i < -127*4)
		i = -127*4;
	if (i > 127*4)
		i = 127*4;
	return i;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove (qwusercmd_t *cmd)
{
	int		i;
	int		ms;

//
// allways dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (++cl.qh_movemessages <= 2)
		return;
//
// figure button bits
//	
	if (in_buttons[0].active || in_buttons[0].wasPressed)
		cmd->buttons |= 1;
	in_buttons[0].wasPressed = false;
	
	if (in_buttons[1].active || in_buttons[1].wasPressed)
		cmd->buttons |= 2;
	in_buttons[1].wasPressed = false;

	// send milliseconds of time to apply the move
	ms = host_frametime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	VectorCopy (cl.viewangles, cmd->angles);

	cmd->impulse = in_impulse;
	in_impulse = 0;


//
// chop down so no extra bits are kept that the server wouldn't get
//
	cmd->forwardmove = MakeChar (cmd->forwardmove);
	cmd->sidemove = MakeChar (cmd->sidemove);
	cmd->upmove = MakeChar (cmd->upmove);

	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ((int)(cmd->angles[i]*65536.0/360)&65535) * (360.0/65536.0);
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
	qwusercmd_t	*cmd, *oldcmd;
	int			checksumIndex;
	int			lost;
	int			seq_hash;

	if (clc.demoplaying)
		return; // sendcmds come from the demo

	// save this command off for prediction
	i = clc.netchan.outgoingSequence & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	cl.qw_frames[i].senttime = realtime;
	cl.qw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

//	seq_hash = (clc.netchan.outgoing_sequence & 0xffff) ; // ^ QW_CHECK_HASH;
	seq_hash = clc.netchan.outgoingSequence;

	// get basic movement from keyboard
	CL_BaseMove (cmd);

	// allow mice or other external controllers to add to the move
	CL_MouseMove(cmd);

	// if we are spectator, try autocam
	if (cl.qh_spectator)
		Cam_Track(cmd);

	CL_FinishMove(cmd);

	Cam_FinishMove(cmd);

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	buf.InitOOB(data, 128);

	buf.WriteByte(q1clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	buf.WriteByte(0);

	// write our lossage percentage
	lost = CL_CalcNet();
	buf.WriteByte((byte)lost);

	i = (clc.netchan.outgoingSequence-2) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	MSGQW_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence-1) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	MSGQW_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	MSGQW_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf._data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf._data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		seq_hash);

	// request delta compression of entities
	if (clc.netchan.outgoingSequence - cl.qh_validsequence >= UPDATE_BACKUP_QW-1)
		cl.qh_validsequence = 0;

	if (cl.qh_validsequence && !cl_nodelta->value && cls.state == CA_ACTIVE &&
		!clc.demorecording)
	{
		cl.qw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_QW].delta_sequence = cl.qh_validsequence;
		buf.WriteByte(qwclc_delta);
		buf.WriteByte(cl.qh_validsequence&255);
	}
	else
		cl.qw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_QW].delta_sequence = -1;

	if (clc.demorecording)
		CL_WriteDemoCmd(cmd);

//
// deliver the message
//
	Netchan_Transmit (&clc.netchan, buf.cursize, buf._data);	
}



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
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	cl_nodelta = Cvar_Get("cl_nodelta","0", 0);
    m_filter = Cvar_Get("m_filter", "0", 0);
}
