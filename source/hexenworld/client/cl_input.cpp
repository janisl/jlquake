// cl.input.c  -- builds an intended movement command to send to the server

#include "quakedef.h"

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

===============================================================================
*/

//==========================================================================

void CL_MouseEvent(int mx, int my)
{
	cl.mouseDx[cl.mouseIndex] += mx;
	cl.mouseDy[cl.mouseIndex] += my;
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
	hwusercmd_t* cmd;

	if (clc.demoplaying)
	{
		return;	// sendcmds come from the demo

	}
	// save this command off for prediction
	i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
	cmd = &cl.hw_frames[i].cmd;
	cl.hw_frames[i].senttime = realtime;
	cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	// grab frame time
	com_frameTime = Sys_Milliseconds();

	frame_msec = (unsigned)(host_frametime * 1000);

	Com_Memset(cmd, 0, sizeof(*cmd));
	in_usercmd_t inCmd = CL_CreateCmdCommon();

	cmd->forwardmove = inCmd.forwardmove;
	cmd->sidemove = inCmd.sidemove;
	cmd->upmove = inCmd.upmove;
	cmd->buttons = inCmd.buttons;
	cmd->impulse = inCmd.impulse;
	cmd->msec = inCmd.msec;
	cmd->light_level = inCmd.lightlevel;

	Cam_FinishMove(&inCmd);

	//
	// allways dump the first two message, because it may contain leftover inputs
	// from the last level
	//
	if (++cl.qh_movemessages <= 2)
	{
		return;
	}

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	buf.InitOOB(data, 128);

	buf.WriteByte(h2clc_move);
	i = (clc.netchan.outgoingSequence - 2) & UPDATE_MASK_HW;
	MSG_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence - 1) & UPDATE_MASK_HW;
	MSG_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_HW;
	MSG_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, true);

//	Con_Printf("I  %hd %hd %hd\n",cmd->forwardmove, cmd->sidemove, cmd->upmove);

	// request delta compression of entities
	if (clc.netchan.outgoingSequence - cl.qh_validsequence >= UPDATE_BACKUP_HW - 1)
	{
		cl.qh_validsequence = 0;
	}

	if (cl.qh_validsequence && !cl_nodelta->value && cls.state == CA_ACTIVE &&
		!clc.demorecording)
	{
		cl.hw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_HW].delta_sequence = cl.qh_validsequence;
		buf.WriteByte(hwclc_delta);
		buf.WriteByte(cl.qh_validsequence & 255);
	}
	else
	{
		cl.hw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_HW].delta_sequence = -1;
	}

	if (clc.demorecording)
	{
		CL_WriteDemoCmd(cmd);
	}

//
// deliver the message
//
	Netchan_Transmit(&clc.netchan, buf.cursize, buf._data);
}

/*
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	CL_InitInputCommon();

	cl_nodelta = Cvar_Get("cl_nodelta","0", 0);
}

void IN_CenterViewWMP()
{
}
