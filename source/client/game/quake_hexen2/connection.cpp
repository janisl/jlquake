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

#include "connection.h"
#include "../../client_main.h"
#include "../../public.h"
#include "../../system.h"
#include "../particles.h"
#include "../camera.h"
#include "demo.h"
#include "network_channel.h"
#include "../../../server/public.h"
#include "../quake/local.h"
#include "../hexen2/local.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"
#include "../../../common/system.h"

static int clqhw_connect_time = -1;				// for connection retransmits

static bool clqw_allowremotecmd = true;
Cvar* clqw_localid;

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
	NET_SendMessage(&clc.netchan, &clc.netchan.message);
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

	if (NET_SendUnreliableMessage(&clc.netchan, &buf) == -1)
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
	if (!NET_CanSendMessage(&clc.netchan))
	{
		common->DPrintf("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage(&clc.netchan, &clc.netchan.message) == -1)
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
		NET_SendUnreliableMessage(&clc.netchan, &clc.netchan.message);
		clc.netchan.message.Clear();
		NET_Close(&clc.netchan);

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

//	Host should be either "local" or a net address to be passed on
void CLQH_EstablishConnection(const char* host)
{
	if (com_dedicated->integer)
	{
		return;
	}

	if (clc.demoplaying)
	{
		return;
	}

	CL_Disconnect(true);

	netadr_t addr = {};
	Netchan_Setup(NS_CLIENT, &clc.netchan, addr, 0);
	if (!NET_Connect(host, &clc.netchan))
	{
		common->Error("CL_Connect: connect failed\n");
	}
	clc.netchan.lastReceived = net_time * 1000;
	common->DPrintf("CLQH_EstablishConnection: connected to %s\n", host);

	cls.qh_demonum = -1;			// not in the demo loop now
	cls.state = CA_ACTIVE;
	clc.qh_signon = 0;				// need all the signon messages before playing
}

//	User command to connect to server
void CLQH_Connect_f()
{
	cls.qh_demonum = -1;		// stop demo loop in case this fails
	if (clc.demoplaying)
	{
		CLQH_StopPlayback();
		CL_Disconnect(true);
	}

	char name[MAX_QPATH];
	String::Cpy(name, Cmd_Argv(1));
	CLQH_EstablishConnection(name);
}

//	This command causes the client to wait for the signon messages again.
// This is sent just before a server changes levels
void CLQH_Reconnect_f()
{
	if (GGameType & GAME_Hexen2)
	{
		CL_ClearParticles();	//jfm: for restarts which didn't use to clear parts.
	}

	SCRQH_BeginLoadingPlaque();
	clc.qh_signon = 0;		// need new connection messages
}

//	called by CLQHW_Connect_f and CL_CheckResend
void CLQHW_SendConnectPacket()
{
	// JACK: Fixed bug where DNS lookups would cause two connects real fast
	//       Now, adds lookup time to the connect time.
	//		 Should I add it to realtime instead?!?!

	if (cls.state != CA_DISCONNECTED)
	{
		return;
	}

	int t1 = Sys_Milliseconds();

	netadr_t adr;
	if (!SOCK_StringToAdr(cls.servername, &adr, GGameType & GAME_HexenWorld ? HWPORT_SERVER : QWPORT_SERVER))
	{
		common->Printf("Bad server address\n");
		clqhw_connect_time = -1;
		return;
	}

	int t2 = Sys_Milliseconds();

	clqhw_connect_time = cls.realtime + t2 - t1;	// for retransmit requests

	char data[2048];
	if (GGameType & GAME_QuakeWorld)
	{
		cls.quakePort = Cvar_VariableValue("qport");

		Info_SetValueForKey(cls.qh_userinfo, "*ip", SOCK_AdrToString(adr), MAX_INFO_STRING_QW, 64, 64, true, false);

		sprintf(data, "%c%c%c%cconnect %i %i %i \"%s\"\n",
			255, 255, 255, 255, QWPROTOCOL_VERSION, cls.quakePort, cls.challenge, cls.qh_userinfo);
	}
	else
	{
		common->Printf("Connecting to %s...\n", cls.servername);
		sprintf(data, "%c%c%c%cconnect %d \"%s\"\n",
			255, 255, 255, 255, com_portals, cls.qh_userinfo);
	}

	NET_SendPacket(NS_CLIENT, String::Length(data), data, adr);
}

//	Resend a connect message if the last one has timed out
void CLQHW_CheckForResend()
{
	if (clqhw_connect_time == -1)
	{
		return;
	}
	if (cls.state != CA_DISCONNECTED)
	{
		return;
	}
	if (clqhw_connect_time && cls.realtime - clqhw_connect_time < 5000)
	{
		return;
	}

	if (GGameType & GAME_HexenWorld)
	{
		CLQHW_SendConnectPacket();
		return;
	}

	int t1 = Sys_Milliseconds();
	netadr_t adr;
	if (!SOCK_StringToAdr(cls.servername, &adr, QWPORT_SERVER))
	{
		common->Printf("Bad server address\n");
		clqhw_connect_time = -1;
		return;
	}

	int t2 = Sys_Milliseconds();

	clqhw_connect_time = cls.realtime + t2 - t1;	// for retransmit requests

	common->Printf("Connecting to %s...\n", cls.servername);
	char data[2048];
	sprintf(data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);
	NET_SendPacket(NS_CLIENT, String::Length(data), data, adr);
}

void CLQHW_BeginServerConnect()
{
	clqhw_connect_time = 0;
	CLQHW_CheckForResend();
}

void CLQHW_Connect_f()
{
	char* server;

	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: connect <server>\n");
		return;
	}

	server = Cmd_Argv(1);

	CL_Disconnect(true);

	String::NCpy(cls.servername, server, sizeof(cls.servername) - 1);
	CLQHW_BeginServerConnect();
}

//	The server is changing levels
void CLQHW_Reconnect_f()
{
	if (GGameType & GAME_QuakeWorld && clc.download)	// don't change when downloading
	{
		return;
	}

	S_StopAllSounds();

	if (cls.state == CA_CONNECTED)
	{
		common->Printf("reconnecting...\n");
		CL_AddReliableCommand("new");
		return;
	}

	if (!*cls.servername)
	{
		common->Printf("No server to reconnect to...\n");
		return;
	}

	CL_Disconnect(true);
	CLQHW_BeginServerConnect();
}

//	Responses to broadcasts, etc
void CLQHW_ConnectionlessPacket(QMsg& message, netadr_t& net_from)
{
	message.BeginReadingOOB();
	message.ReadLong();			// skip the -1

	int c = message.ReadByte();
	if (!clc.demoplaying)
	{
		common->Printf("%s: ", SOCK_AdrToString(net_from));
	}
	if (c == S2C_CONNECTION)
	{
		common->Printf("connection\n");
		if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
		{
			if (!clc.demoplaying)
			{
				common->Printf("Dup connect received.  Ignored.\n");
			}
			return;
		}
		Netchan_Setup(NS_CLIENT, &clc.netchan, net_from, GGameType & GAME_HexenWorld ? 0 : cls.quakePort);
		clc.netchan.lastReceived = cls.realtime;
		CL_AddReliableCommand("new");
		cls.state = CA_CONNECTED;
		common->Printf("Connected.\n");
		if (GGameType & GAME_QuakeWorld)
		{
			clqw_allowremotecmd = false;	// localid required now for remote cmds
		}
		return;
	}
	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)
	{
		common->Printf("client command\n");

		if (!SOCK_IsLocalIP(net_from))
		{
			common->Printf("Command packet from remote host.  Ignored.\n");
			return;
		}
		Sys_AppActivate();

		const char* s = message.ReadString2();
		char cmdtext[2048];
		String::NCpyZ(cmdtext, s, sizeof(cmdtext));

		if (GGameType & GAME_QuakeWorld)
		{
			char* s2 = const_cast<char*>(message.ReadString2());

			while (*s2 && String::IsSpace(*s2))
			{
				s2++;
			}
			while (*s2 && String::IsSpace(s2[String::Length(s2) - 1]))
			{
				s2[String::Length(s2) - 1] = 0;
			}

			if (!clqw_allowremotecmd && (!*clqw_localid->string || String::Cmp(clqw_localid->string, s2)))
			{
				if (!*clqw_localid->string)
				{
					common->Printf("===========================\n");
					common->Printf("Command packet received from local host, but no "
						"localid has been set.  You may need to upgrade your server "
						"browser.\n");
					common->Printf("===========================\n");
					return;
				}
				common->Printf("===========================\n");
				common->Printf("Invalid localid on command packet received from local host. "
					"\n|%s| != |%s|\n"
					"You may need to reload your server browser and QuakeWorld.\n",
					s2, clqw_localid->string);
				common->Printf("===========================\n");
				Cvar_Set("localid", "");
				return;
			}
		}

		Cbuf_AddText(cmdtext);
		if (GGameType & GAME_QuakeWorld)
		{
			clqw_allowremotecmd = false;
		}
		return;
	}
	// print command from somewhere
	if (c == A2C_PRINT)
	{
		common->Printf("print\n");

		const char* s = message.ReadString2();
		Con_ConsolePrint(s);
		return;
	}

	// ping from somewhere
	if (c == A2A_PING)
	{
		common->Printf("ping\n");

		char data[6];
		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;

		NET_SendPacket(NS_CLIENT, 6, &data, net_from);
		return;
	}

	if (GGameType & GAME_QuakeWorld && c == S2C_CHALLENGE)
	{
		common->Printf("challenge\n");

		const char* s = message.ReadString2();
		cls.challenge = String::Atoi(s);
		CLQHW_SendConnectPacket();
		return;
	}

	if (GGameType & GAME_HexenWorld && c == h2svc_disconnect)
	{
		common->Error("Server disconnected\n");
		return;
	}

	common->Printf("unknown:  %c\n", c);
}

//	Read all incoming data from the server
void CLQH_ReadFromServer()
{
	cl.qh_oldtime = cl.qh_serverTimeFloat;
	cl.qh_serverTimeFloat += cls.frametime * 0.001;
	cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);

	// allocate space for network message buffer
	QMsg net_message;
	byte net_message_buf[MAX_MSGLEN];
	net_message.InitOOB(net_message_buf, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_MSGLEN_Q1);

	int ret;
	do
	{
		ret = CLQH_GetMessage(net_message);
		if (ret == -1)
		{
			common->Error("CLQH_ReadFromServer: lost server connection");
		}
		if (!ret)
		{
			break;
		}

		cl.qh_last_received_message = cls.realtime * 0.001;
		if (GGameType & GAME_Hexen2)
		{
			CLH2_ParseServerMessage(net_message);
		}
		else
		{
			CLQ1_ParseServerMessage(net_message);
		}
	}
	while (ret && cls.state == CA_ACTIVE);

	if (cl_shownet->value)
	{
		common->Printf("\n");
	}
}

void CLQHW_ReadPackets()
{
	netadr_t net_from;
	QMsg net_message;
	byte net_message_buffer[MAX_MSGLEN_HW + 9];	// one more than msg + header
	net_message.InitOOB(net_message_buffer, GGameType & GAME_HexenWorld ? MAX_MSGLEN_HW + 9 : MAX_MSGLEN_QW * 2);

	while (CLQHW_GetMessage(net_message, net_from))
	{
		//
		// remote command packet
		//
		if (*(int*)net_message._data == -1)
		{
			CLQHW_ConnectionlessPacket(net_message, net_from);
			continue;
		}

		if (net_message.cursize < 8)
		{
			common->Printf("%s: Runt packet\n",SOCK_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!clc.demoplaying &&
			!SOCK_CompareAdr(net_from, clc.netchan.remoteAddress))
		{
			common->DPrintf("%s:sequenced packet without connection\n",
				SOCK_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&clc.netchan, &net_message))
		{
			continue;		// wasn't accepted for some reason
		}
		clc.netchan.lastReceived = cls.realtime;
		if (GGameType & GAME_HexenWorld)
		{
			CLHW_ParseServerMessage(net_message);
		}
		else
		{
			CLQW_ParseServerMessage(net_message);
		}
	}

	//
	// check timeout
	//
	if ((cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE) &&
		cls.realtime - clc.netchan.lastReceived > cl_timeout->value * 1000)
	{
		common->Printf("\nServer connection timed out.\n");
		CL_Disconnect(true);
		return;
	}
}
