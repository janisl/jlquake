/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
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
static void CL_SendMove(in_usercmd_t* cmd)
{
	cl.qh_cmd = *cmd;

//
// deliver the message
//
	if (clc.demoplaying)
	{
		return;
	}

//
// allways dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (++cl.qh_movemessages <= 2)
	{
		return;
	}

	QMsg buf;
	byte data[128];

	buf.InitOOB(data, 128);

//
// send the movement message
//
	buf.WriteByte(q1clc_move);
	buf.WriteFloat(cmd->mtime);
	for (int i = 0; i < 3; i++)
	{
		buf.WriteAngle(cmd->fAngles[i]);
	}
	buf.WriteShort(cmd->forwardmove);
	buf.WriteShort(cmd->sidemove);
	buf.WriteShort(cmd->upmove);
	buf.WriteByte(cmd->buttons);
	buf.WriteByte(cmd->impulse);

	if (NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &buf) == -1)
	{
		common->Printf("CL_SendMove: lost server connection\n");
		CL_Disconnect();
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	CL_InitInputCommon();
}


/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd(void)
{
	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (clc.qh_signon == SIGNONS)
	{
		// get basic movement from keyboard
		// grab frame time
		com_frameTime = Sys_Milliseconds();

		in_usercmd_t cmd = CL_CreateCmd();

		// send the unreliable message
		CL_SendMove(&cmd);

	}

	if (clc.demoplaying)
	{
		clc.netchan.message.Clear();
		return;
	}

// send the reliable message
	if (!clc.netchan.message.cursize)
	{
		return;		// no message at all

	}
	if (!NET_CanSendMessage(cls.qh_netcon, &clc.netchan))
	{
		common->DPrintf("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message) == -1)
	{
		common->Error("CL_WriteToServer: lost server connection");
	}

	clc.netchan.message.Clear();
}

void IN_CenterViewWMP()
{
}
