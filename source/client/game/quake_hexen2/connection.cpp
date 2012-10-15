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
