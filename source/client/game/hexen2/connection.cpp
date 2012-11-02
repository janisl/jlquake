//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"
#include "../quake_hexen2/demo.h"

//	An h2svc_signonnum has been received, perform a client side setup
void CLH2_SignonReply()
{
	char str[8192];

	common->DPrintf("CLH2_SignonReply: %i\n", clc.qh_signon);

	switch (clc.qh_signon)
	{
	case 1:
		CL_AddReliableCommand("prespawn");
		break;

	case 2:
		CL_AddReliableCommand(va("name \"%s\"\n", clqh_name->string));
		CL_AddReliableCommand(va("playerclass %i\n", (int)clh2_playerclass->value));
		CL_AddReliableCommand(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));
		sprintf(str, "spawn %s", cls.qh_spawnparms);
		CL_AddReliableCommand(str);
		break;

	case 3:
		CL_AddReliableCommand("begin");
		break;

	case 4:
		SCR_EndLoadingPlaque();			// allow normal screen updates
		break;
	}
}

void CLHW_SendCmd()
{
	if (cls.state == CA_DISCONNECTED)
	{
		return;
	}
	if (clc.demoplaying)
	{
		// sendcmds come from the demo
		return;
	}

	// save this command off for prediction
	int i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
	hwusercmd_t* cmd = &cl.hw_frames[i].cmd;
	cl.hw_frames[i].senttime = cls.realtime * 0.001;
	cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	Com_Memset(cmd, 0, sizeof(*cmd));
	in_usercmd_t inCmd = CL_CreateCmd();

	cmd->forwardmove = inCmd.forwardmove;
	cmd->sidemove = inCmd.sidemove;
	cmd->upmove = inCmd.upmove;
	cmd->buttons = inCmd.buttons;
	cmd->impulse = inCmd.impulse;
	cmd->msec = inCmd.msec;
	cmd->light_level = inCmd.lightlevel;
	VectorCopy(inCmd.fAngles, cmd->angles);

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
	QMsg buf;
	byte data[128];
	buf.InitOOB(data, 128);

	buf.WriteByte(h2clc_move);
	i = (clc.netchan.outgoingSequence - 2) & UPDATE_MASK_HW;
	MSGHW_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence - 1) & UPDATE_MASK_HW;
	MSGHW_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_HW;
	MSGHW_WriteUsercmd(&buf, &cl.hw_frames[i].cmd, true);

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
		CLHW_WriteDemoCmd(cmd);
	}

	//
	// deliver the message
	//
	Netchan_Transmit(&clc.netchan, buf.cursize, buf._data);
}
