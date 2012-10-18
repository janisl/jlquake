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
#include "connection.h"
#include "demo.h"
#include "../../../server/public.h"
#include "../quake/local.h"
#include "../hexen2/local.h"

double clqhw_connect_time = -1;				// for connection retransmits

//	When the client is taking a long time to load stuff, send keepalive messages
// so the server doesn't disconnect.
void CLQH_KeepaliveMessage()
{
	static float lastmsg;

	if (SV_IsServerActive())
	{
		return;		// no need if server is local
	}
	if (clc.demoplaying)
	{
		return;
	}

	// read messages from server, should just be nops
	QMsg message;
	byte message_data[MAX_MSGLEN];
	message.InitOOB(message_data, sizeof(message_data));

	int ret;
	do
	{
		ret = CLQH_GetMessage(message);
		switch (ret)
		{
		default:
			common->Error("CLQH_KeepaliveMessage: CLQH_GetMessage failed");
		case 0:
			break;	// nothing waiting
		case 1:
			common->Error("CLQH_KeepaliveMessage: received a message");
			break;
		case 2:
			if (message.ReadByte() != (GGameType & GAME_Hexen2 ? h2svc_nop : q1svc_nop))
			{
				common->Error("CLQH_KeepaliveMessage: datagram wasn't a nop");
			}
			break;
		}
	}
	while (ret);

	// check time
	float time = Sys_DoubleTime();
	if (time - lastmsg < 5)
	{
		return;
	}
	lastmsg = time;

	// write out a nop
	common->Printf("--> client to server keepalive\n");

	clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? h2clc_nop : q1clc_nop);
	NET_SendMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
	clc.netchan.message.Clear();
}

static void CLQH_SendMove(in_usercmd_t* cmd)
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
	if (GGameType & GAME_Hexen2)
	{
		buf.WriteByte(h2clc_frame);
		buf.WriteByte(cl.h2_reference_frame);
		buf.WriteByte(cl.h2_current_sequence);

		buf.WriteByte(h2clc_move);
	}
	else
	{
		buf.WriteByte(q1clc_move);
	}
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
	if (GGameType & GAME_Hexen2)
	{
		buf.WriteByte(cmd->lightlevel);
	}

	if (NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &buf) == -1)
	{
		common->Printf("CL_SendMove: lost server connection\n");
		CL_Disconnect(true);
	}
}

void CLQH_SendCmd()
{
	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (clc.qh_signon == SIGNONS)
	{
		// grab frame time
		com_frameTime = Sys_Milliseconds();

		in_usercmd_t cmd = CL_CreateCmd();

		// send the unreliable message
		CLQH_SendMove(&cmd);

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

void CLQH_Disconnect()
{
	//jfm: need to clear parts because some now check world
	CL_ClearParticles();
	// stop sounds (especially looping!)
	S_StopAllSounds();
	clh2_loading_stage = 0;

	// if running a local server, shut it down
	if (clc.demoplaying)
	{
		CLQH_StopPlayback();
	}
	else if (cls.state != CA_DISCONNECTED)
	{
		if (clc.demorecording)
		{
			CLQH_Stop_f();
		}

		common->DPrintf("Sending clc_disconnect\n");
		clc.netchan.message.Clear();
		clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? h2clc_disconnect : q1clc_disconnect);
		NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
		clc.netchan.message.Clear();
		NET_Close(cls.qh_netcon, &clc.netchan);

		cls.state = CA_DISCONNECTED;
		SV_Shutdown("");

		if (GGameType & GAME_Hexen2)
		{
			SVH2_RemoveGIPFiles(NULL);
		}
	}

	clc.demoplaying = cls.qh_timedemo = false;
	clc.qh_signon = 0;
}

void CLQHW_Disconnect()
{
	clqhw_connect_time = -1;

	// stop sounds (especially looping!)
	S_StopAllSounds();

	if (GGameType & GAME_Hexen2)
	{
		clhw_siege = false;	//no more siege display, etc.
	}

	// if running a local server, shut it down
	if (clc.demoplaying)
	{
		CLQH_StopPlayback();
	}
	else if (cls.state != CA_DISCONNECTED)
	{
		if (clc.demorecording)
		{
			CLQH_Stop_f();
		}

		byte final[10];
		final[0] = GGameType & GAME_Hexen2 ? h2clc_stringcmd : q1clc_stringcmd;
		String::Cpy((char*)final + 1, "drop");
		Netchan_Transmit(&clc.netchan, 6, final);
		Netchan_Transmit(&clc.netchan, 6, final);
		Netchan_Transmit(&clc.netchan, 6, final);

		cls.state = CA_DISCONNECTED;

		clc.demoplaying = clc.demorecording = cls.qh_timedemo = false;
	}
	Cam_Reset();

	if (GGameType & GAME_QuakeWorld)
	{
		if (clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}

		CLQW_StopUpload();
	}
}
