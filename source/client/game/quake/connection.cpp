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

#include "../../client_main.h"
#include "../../public.h"
#include "local.h"
#include "../quake_hexen2/demo.h"

static byte* clqw_upload_data;
static int clqw_upload_pos;
static int clqw_upload_size;

//	An q1svc_signonnum has been received, perform a client side setup
void CLQ1_SignonReply()
{
	char str[8192];

	common->DPrintf("CLQ1_SignonReply: %i\n", clc.qh_signon);

	switch (clc.qh_signon)
	{
	case 1:
		CL_AddReliableCommand("prespawn");
		break;

	case 2:
		CL_AddReliableCommand(va("name \"%s\"\n", clqh_name->string));
		CL_AddReliableCommand(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));
		sprintf(str, "spawn %s", cls.qh_spawnparms);
		CL_AddReliableCommand(str);
		break;

	case 3:
		CL_AddReliableCommand("begin");
		break;

	case 4:
		SCR_EndLoadingPlaque();		// allow normal screen updates
		break;
	}
}

int CLQW_CalcNet()
{
	for (int i = clc.netchan.outgoingSequence - UPDATE_BACKUP_QW + 1
		 ; i <= clc.netchan.outgoingSequence
		 ; i++)
	{
		qwframe_t* frame = &cl.qw_frames[i & UPDATE_MASK_QW];
		if (frame->receivedtime == -1)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 9999;		// dropped
		}
		else if (frame->receivedtime == -2)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 10000;	// choked
		}
		else if (frame->invalid)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 9998;		// invalid delta
		}
		else
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = (frame->receivedtime - frame->senttime) * 20;
		}
	}

	int lost = 0;
	for (int a = 0; a < NET_TIMINGS_QH; a++)
	{
		int i = (clc.netchan.outgoingSequence - a) & NET_TIMINGSMASK_QH;
		if (clqh_packet_latency[i] == 9999)
		{
			lost++;
		}
	}
	return lost * 100 / NET_TIMINGS_QH;
}

void CLQW_NextUpload()
{
	if (!clqw_upload_data)
	{
		return;
	}

	int r = clqw_upload_size - clqw_upload_pos;
	if (r > 768)
	{
		r = 768;
	}
	byte buffer[1024];
	Com_Memcpy(buffer, clqw_upload_data + clqw_upload_pos, r);
	clc.netchan.message.WriteByte(qwclc_upload);
	clc.netchan.message.WriteShort(r);

	clqw_upload_pos += r;
	int size = clqw_upload_size;
	if (!size)
	{
		size = 1;
	}
	int percent = clqw_upload_pos * 100 / size;
	clc.netchan.message.WriteByte(percent);
	clc.netchan.message.WriteData(buffer, r);

	common->DPrintf("UPLOAD: %6d: %d written\n", clqw_upload_pos - r, r);

	if (clqw_upload_pos != clqw_upload_size)
	{
		return;
	}

	common->Printf("Upload completed\n");

	Mem_Free(clqw_upload_data);
	clqw_upload_data = 0;
	clqw_upload_pos = clqw_upload_size = 0;
}

void CLQW_StartUpload(const byte* data, int size)
{
	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected

	}
	// override
	if (clqw_upload_data)
	{
		free(clqw_upload_data);
	}

	common->DPrintf("Upload starting of %d...\n", size);

	clqw_upload_data = (byte*)Mem_Alloc(size);
	Com_Memcpy(clqw_upload_data, data, size);
	clqw_upload_size = size;
	clqw_upload_pos = 0;

	CLQW_NextUpload();
}

bool CLQW_IsUploading()
{
	if (clqw_upload_data)
	{
		return true;
	}
	return false;
}

void CLQW_StopUpload()
{
	if (clqw_upload_data)
	{
		Mem_Free(clqw_upload_data);
	}
	clqw_upload_data = NULL;
}

void CLQW_SendCmd()
{
	if (cls.state == CA_DISCONNECTED)
	{
		return;
	}
	if (clc.demoplaying)
	{
		return;	// sendcmds come from the demo

	}
	// save this command off for prediction
	int i = clc.netchan.outgoingSequence & UPDATE_MASK_QW;
	qwusercmd_t* cmd = &cl.qw_frames[i].cmd;
	cl.qw_frames[i].senttime = cls.realtime * 0.001;
	cl.qw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	int seq_hash = clc.netchan.outgoingSequence;

	// get basic movement from keyboard
	Com_Memset(cmd, 0, sizeof(*cmd));

	in_usercmd_t inCmd = CL_CreateCmd();

	cmd->forwardmove = inCmd.forwardmove;
	cmd->sidemove = inCmd.sidemove;
	cmd->upmove = inCmd.upmove;
	cmd->buttons = inCmd.buttons;
	cmd->impulse = inCmd.impulse;
	cmd->msec = inCmd.msec;
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

	buf.WriteByte(q1clc_move);

	// save the position for a checksum byte
	int checksumIndex = buf.cursize;
	buf.WriteByte(0);

	// write our lossage percentage
	int lost = CLQW_CalcNet();
	buf.WriteByte((byte)lost);

	i = (clc.netchan.outgoingSequence - 2) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	qwusercmd_t nullcmd = {};
	MSGQW_WriteDeltaUsercmd(&buf, &nullcmd, cmd);
	qwusercmd_t* oldcmd = cmd;

	i = (clc.netchan.outgoingSequence - 1) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	MSGQW_WriteDeltaUsercmd(&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_QW;
	cmd = &cl.qw_frames[i].cmd;
	MSGQW_WriteDeltaUsercmd(&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf._data[checksumIndex] = COMQW_BlockSequenceCRCByte(
		buf._data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		seq_hash);

	// request delta compression of entities
	if (clc.netchan.outgoingSequence - cl.qh_validsequence >= UPDATE_BACKUP_QW - 1)
	{
		cl.qh_validsequence = 0;
	}

	if (cl.qh_validsequence && !cl_nodelta->value && cls.state == CA_ACTIVE &&
		!clc.demorecording)
	{
		cl.qw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_QW].delta_sequence = cl.qh_validsequence;
		buf.WriteByte(qwclc_delta);
		buf.WriteByte(cl.qh_validsequence & 255);
	}
	else
	{
		cl.qw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_QW].delta_sequence = -1;
	}

	if (clc.demorecording)
	{
		CLQW_WriteDemoCmd(cmd);
	}

	//
	// deliver the message
	//
	Netchan_Transmit(&clc.netchan, buf.cursize, buf._data);
}
