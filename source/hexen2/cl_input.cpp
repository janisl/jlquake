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

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

//==========================================================================

Cvar*	cl_prettylights;

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
	
	if (in_buttons[2].active)
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
void CL_InitInput (void)
{
	CL_InitInputCommon();
	Cmd_AddCommand ("impulse", IN_Impulse);

#ifdef MISSIONPACK
	Cmd_AddCommand ("+infoplaque", IN_infoPlaqueDown);
	Cmd_AddCommand ("-infoplaque", IN_infoPlaqueUp);
#endif
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	h2usercmd_t		cmd;

	if (cls.state != CA_CONNECTED)
		return;

	if (clc.qh_signon == SIGNONS)
	{
	// get basic movement from keyboard
		Com_Memset(&cmd, 0, sizeof(cmd));
			
		if (!cl.h2_v.cameramode)	// Stuck in a different camera so don't move
		{
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
			
			in_usercmd_t inCmd;
			inCmd.forwardmove = cmd.forwardmove;
			inCmd.sidemove = cmd.sidemove;
			inCmd.upmove = cmd.upmove;
			CL_KeyMove(&inCmd);

			// light level at player's position including dlights
			// this is sent back to the server each frame
			// architectually ugly but it works
			cmd.lightlevel = (byte)cl_lightlevel->value;
	
			// allow mice or other external controllers to add to the move
			CL_MouseMove(&inCmd);

			// get basic movement from joystick
			CL_JoystickMove(&inCmd);
			cmd.forwardmove = inCmd.forwardmove;
			cmd.sidemove = inCmd.sidemove;
			cmd.upmove = inCmd.upmove;

			if (cl.viewangles[PITCH] > 80)
				cl.viewangles[PITCH] = 80;
			if (cl.viewangles[PITCH] < -70)
				cl.viewangles[PITCH] = -70;
		}
	
	// send the unreliable message
		CL_SendMove (&cmd);
	
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
