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

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

//==========================================================================

void CL_MouseEvent(int mx, int my)
{
	cl.mouseDx[cl.mouseIndex] += mx;
	cl.mouseDy[cl.mouseIndex] += my;
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
void CL_FinishMove (hwusercmd_t *cmd)
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

	if (in_buttons[2].active)
		cmd->buttons |= 4;

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
	if ((int)cl.h2_v.artifact_active & ARTFLAG_FROZEN)
	{
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
		cmd->upmove = 0;
	}
	else
	{
		cmd->forwardmove = MakeChar (cmd->forwardmove);
		cmd->sidemove = MakeChar (cmd->sidemove);
		cmd->upmove = MakeChar (cmd->upmove);
	}

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
	hwusercmd_t	*cmd;

	if (clc.demoplaying)
		return; // sendcmds come from the demo

	// save this command off for prediction
	i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
	cmd = &cl.hw_frames[i].cmd;
	cl.hw_frames[i].senttime = realtime;
	cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	Com_Memset(cmd, 0, sizeof(*cmd));

	if (!cl.h2_v.cameramode)	// Stuck in a different camera so don't move
	{
		// grab frame time 
		com_frameTime = Sys_Milliseconds();

		frame_msec = (unsigned)(host_frametime * 1000);

		// get basic movement from keyboard
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
	
		VectorCopy (cl.viewangles, cmd->angles);

		in_usercmd_t inCmd;
		inCmd.forwardmove = cmd->forwardmove;
		inCmd.sidemove = cmd->sidemove;
		inCmd.upmove = cmd->upmove;
		CL_KeyMove(&inCmd);

		cmd->light_level = (byte)cl_lightlevel->value;

		// allow mice or other external controllers to add to the move
		CL_MouseMove(&inCmd);

		// get basic movement from joystick
		CL_JoystickMove(&inCmd);
		cmd->forwardmove = inCmd.forwardmove;
		cmd->sidemove = inCmd.sidemove;
		cmd->upmove = inCmd.upmove;

		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}

	// if we are spectator, try autocam
	if (cl.qh_spectator)
		Cam_Track(cmd);

	CL_FinishMove(cmd);

	Cam_FinishMove(cmd);

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	buf.InitOOB(data, 128);

	buf.WriteByte(h2clc_move);
	i = (clc.netchan.outgoingSequence-2) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence-1) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, true);

//	Con_Printf("I  %hd %hd %hd\n",cmd->forwardmove, cmd->sidemove, cmd->upmove);

	// request delta compression of entities
	if (clc.netchan.outgoingSequence - cl.qh_validsequence >= UPDATE_BACKUP_HW-1)
		cl.qh_validsequence = 0;

	if (cl.qh_validsequence && !cl_nodelta->value && cls.state == CA_ACTIVE &&
		!clc.demorecording)
	{
		cl.hw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_HW].delta_sequence = cl.qh_validsequence;
		buf.WriteByte(hwclc_delta);
		buf.WriteByte(cl.qh_validsequence&255);
	}
	else
		cl.hw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_HW].delta_sequence = -1;

	if (clc.demorecording)
		CL_WriteDemoCmd(cmd);

//
// deliver the message
//
	Netchan_Transmit (&clc.netchan, buf.cursize, buf._data);	
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

	cl_nodelta = Cvar_Get("cl_nodelta","0", 0);
}
