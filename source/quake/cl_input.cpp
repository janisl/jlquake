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


int			in_impulse;

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

//==========================================================================

void CL_MouseEvent(int mx, int my)
{
	cl.mouseDx[cl.mouseIndex] += mx;
	cl.mouseDy[cl.mouseIndex] += my;
}

/*
==============
CL_SendMove
==============
*/
static void CL_SendMove (q1usercmd_t *cmd, in_usercmd_t* inCmd)
{
	int		i;
	QMsg	buf;
	byte	data[128];
	
	buf.InitOOB(data, 128);
	
	cl.q1_cmd = *cmd;

//
// send the movement message
//
    buf.WriteByte(q1clc_move);

	buf.WriteFloat(cl.qh_mtime[0]);	// so server can get ping times

	for (i=0 ; i<3 ; i++)
		buf.WriteAngle(cl.viewangles[i]);
	
    buf.WriteShort(cmd->forwardmove);
    buf.WriteShort(cmd->sidemove);
    buf.WriteShort(cmd->upmove);

    buf.WriteByte(inCmd->buttons);

    buf.WriteByte(in_impulse);
	in_impulse = 0;

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

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	CL_InitInputCommon();
	Cmd_AddCommand ("impulse", IN_Impulse);
}


/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	q1usercmd_t		cmd;

	if (cls.state != CA_CONNECTED)
		return;

	if (clc.qh_signon == SIGNONS)
	{
		// get basic movement from keyboard
		// grab frame time 
		com_frameTime = Sys_Milliseconds();

		frame_msec = (unsigned)(host_frametime * 1000);

		CL_AdjustAngles();
		cl.viewangles[YAW] = AngleMod(cl.viewangles[YAW]);

		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;

		if (cl.viewangles[ROLL] > 50)
			cl.viewangles[ROLL] = 50;
		if (cl.viewangles[ROLL] < -50)
			cl.viewangles[ROLL] = -50;
		
		Com_Memset(&cmd, 0, sizeof(cmd));
		
		in_usercmd_t inCmd;
		inCmd.forwardmove = cmd.forwardmove;
		inCmd.sidemove = cmd.sidemove;
		inCmd.upmove = cmd.upmove;
		inCmd.buttons = 0;
		CL_KeyMove(&inCmd);
	
		// allow mice or other external controllers to add to the move
		CL_MouseMove(&inCmd);

		// get basic movement from joystick
		CL_JoystickMove(&inCmd);

		CL_CmdButtons(&inCmd);
		cmd.forwardmove = inCmd.forwardmove;
		cmd.sidemove = inCmd.sidemove;
		cmd.upmove = inCmd.upmove;

		if (cl.viewangles[PITCH] > 80)
		{
			cl.viewangles[PITCH] = 80;
		}
		if (cl.viewangles[PITCH] < -70)
		{
			cl.viewangles[PITCH] = -70;
		}
	
	// send the unreliable message
		CL_SendMove (&cmd, &inCmd);
	
	}

	if (clc.demoplaying)
	{
		clc.netchan.message.Clear();
		return;
	}
	
// send the reliable message
	if (!clc.netchan.message.cursize)
		return;		// no message at all
	
	if (!NET_CanSendMessage (cls.qh_netcon, &clc.netchan))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.qh_netcon, &clc.netchan, &clc.netchan.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	clc.netchan.message.Clear();
}
