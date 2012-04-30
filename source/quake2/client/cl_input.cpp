/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "client.h"

Cvar* cl_nodelta;

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

//==========================================================================

void CL_MouseEvent(int mx, int my)
{
	cl.mouseDx[cl.mouseIndex] += mx;
	cl.mouseDy[cl.mouseIndex] += my;
}

/*
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	CL_InitInputCommon();

	cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
}



/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd(void)
{
	QMsg buf;
	byte data[128];
	int i;
	q2usercmd_t* cmd, * oldcmd;
	q2usercmd_t nullcmd;
	int checksumIndex;

	// build a command even if not connected

	// save this command off for prediction
	i = clc.netchan.outgoingSequence & (CMD_BACKUP_Q2 - 1);
	cmd = &cl.q2_cmds[i];
	cl.q2_cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	// grab frame time
	com_frameTime = Sys_Milliseconds_();

	in_usercmd_t inCmd = CL_CreateCmd();
	Com_Memset(cmd, 0, sizeof(*cmd));
	cmd->forwardmove = inCmd.forwardmove;
	cmd->sidemove = inCmd.sidemove;
	cmd->upmove = inCmd.upmove;
	cmd->buttons = inCmd.buttons;
	for (int i = 0; i < 3; i++)
	{
		cmd->angles[i] = inCmd.angles[i];
	}
	cmd->impulse = inCmd.impulse;
	cmd->msec = inCmd.msec;
	cmd->lightlevel = inCmd.lightlevel;

	if (cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING)
	{
		return;
	}

	if (cls.state == CA_CONNECTED)
	{
		if (clc.netchan.message.cursize || curtime - clc.netchan.lastSent > 1000)
		{
			Netchan_Transmit(&clc.netchan, 0, clc.netchan.message._data);
		}
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
	{
		buf.WriteLong(-1);	// no compression
	}
	else
	{
		buf.WriteLong(cl.q2_frame.serverframe);
	}

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (clc.netchan.outgoingSequence - 2) & (CMD_BACKUP_Q2 - 1);
	cmd = &cl.q2_cmds[i];
	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	MSGQ2_WriteDeltaUsercmd(&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence - 1) & (CMD_BACKUP_Q2 - 1);
	cmd = &cl.q2_cmds[i];
	MSGQ2_WriteDeltaUsercmd(&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence) & (CMD_BACKUP_Q2 - 1);
	cmd = &cl.q2_cmds[i];
	MSGQ2_WriteDeltaUsercmd(&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf._data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf._data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		clc.netchan.outgoingSequence);

	//
	// deliver the message
	//
	Netchan_Transmit(&clc.netchan, buf.cursize, buf._data);
}



void IN_CenterViewWMP()
{
}
