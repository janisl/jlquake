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


#ifdef MISSIONPACK
extern qboolean info_up;
#endif

//==========================================================================

Cvar* cl_prettylights;

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
	QMsg buf;
	byte data[128];

	buf.InitOOB(data, 128);

	cl.qh_cmd = *cmd;

//
// send the movement message
//
	buf.WriteByte(h2clc_frame);
	buf.WriteByte(cl.h2_reference_frame);
	buf.WriteByte(cl.h2_current_sequence);

	buf.WriteByte(h2clc_move);
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
	buf.WriteByte(cmd->lightlevel);

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

	if (NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &buf) == -1)
	{
		Con_Printf("CL_SendMove: lost server connection\n");
		CL_Disconnect();
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
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	CL_InitInputCommon();
#ifdef MISSIONPACK
	Cmd_AddCommand("+infoplaque", IN_infoPlaqueDown);
	Cmd_AddCommand("-infoplaque", IN_infoPlaqueUp);
#endif
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd(void)
{
	if (cls.state != CA_CONNECTED)
	{
		return;
	}

	if (clc.qh_signon == SIGNONS)
	{
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
		Con_DPrintf("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message) == -1)
	{
		Host_Error("CL_WriteToServer: lost server connection");
	}

	clc.netchan.message.Clear();
}

void IN_CenterViewWMP()
{
}
