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
// cl_main.c  -- client main loop

#include "client.h"
#include "../../common/file_formats/md2.h"

Cvar* adr0;
Cvar* adr1;
Cvar* adr2;
Cvar* adr3;
Cvar* adr4;
Cvar* adr5;
Cvar* adr6;
Cvar* adr7;
Cvar* adr8;

Cvar* cl_stereo_separation;

Cvar* rcon_client_password;
Cvar* rcon_address;

Cvar* cl_noskins;
Cvar* cl_autoskins;
Cvar* cl_timeout;
Cvar* cl_predict;
//Cvar	*cl_minfps;
Cvar* cl_maxfps;
Cvar* cl_gun;

Cvar* cl_add_particles;
Cvar* cl_add_entities;
Cvar* cl_add_blend;

Cvar* cl_shownet;
Cvar* cl_showmiss;
Cvar* cl_showclamp;

Cvar* cl_paused;
Cvar* cl_timedemo;

Cvar* cl_lightlevel;

//
// userinfo
//
Cvar* info_password;
Cvar* info_spectator;
Cvar* name;
Cvar* skin;
Cvar* rate;
Cvar* fov;
Cvar* msg;
Cvar* gender;
Cvar* gender_auto;

Cvar* cl_vwep;

q2entity_state_t cl_parse_entities[MAX_PARSE_ENTITIES];

extern Cvar* allow_download;
extern Cvar* allow_download_players;
extern Cvar* allow_download_models;
extern Cvar* allow_download_sounds;
extern Cvar* allow_download_maps;

static bool vid_restart_requested;

//======================================================================


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(void)
{
	int len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = net_message.cursize - 8;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(net_message._data + 8, len, clc.demofile);
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f(void)
{
	int len;

	if (!clc.demorecording)
	{
		Com_Printf("Not recording a demo.\n");
		return;
	}

// finish up
	len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	Com_Printf("Stopped demo.\n");
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f(void)
{
	char name[MAX_OSPATH];
	byte buf_data[MAX_MSGLEN_Q2];
	QMsg buf;
	int i;
	int len;
	q2entity_state_t* ent;
	q2entity_state_t nullstate;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		Com_Printf("Already recording.\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		Com_Printf("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	String::Sprintf(name, sizeof(name), "demos/%s.dm2", Cmd_Argv(1));

	Com_Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.q2_demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	buf.InitOOB(buf_data, sizeof(buf_data));

	// send the serverdata
	buf.WriteByte(q2svc_serverdata);
	buf.WriteLong(PROTOCOL_VERSION);
	buf.WriteLong(0x10000 + cl.servercount);
	buf.WriteByte(1);	// demos are always attract loops
	buf.WriteString2(cl.q2_gamedir);
	buf.WriteShort(cl.playernum);

	buf.WriteString2(cl.q2_configstrings[Q2CS_NAME]);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS_Q2; i++)
	{
		if (cl.q2_configstrings[i][0])
		{
			if (buf.cursize + String::Length(cl.q2_configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleLong(buf.cursize);
				FS_Write(&len, 4, clc.demofile);
				FS_Write(buf._data, buf.cursize, clc.demofile);
				buf.cursize = 0;
			}

			buf.WriteByte(q2svc_configstring);
			buf.WriteShort(i);
			buf.WriteString2(cl.q2_configstrings[i]);
		}

	}

	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_EDICTS_Q2; i++)
	{
		ent = &clq2_entities[i].baseline;
		if (!ent->modelindex)
		{
			continue;
		}

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleLong(buf.cursize);
			FS_Write(&len, 4, clc.demofile);
			FS_Write(buf._data, buf.cursize, clc.demofile);
			buf.cursize = 0;
		}

		buf.WriteByte(q2svc_spawnbaseline);
		MSG_WriteDeltaEntity(&nullstate, &clq2_entities[i].baseline, &buf, true, true);
	}

	buf.WriteByte(q2svc_stufftext);
	buf.WriteString2("precache\n");

	// write it to the demo file

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf._data, buf.cursize, clc.demofile);

	// the rest of the demo file will be individual frames
}

//======================================================================

/*
===================
Cmd_ForwardToServer

adds the current command line as a q2clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer(void)
{
	char* cmd;

	cmd = Cmd_Argv(0);
	if (cls.state <= CA_CONNECTED || *cmd == '-' || *cmd == '+')
	{
		Com_Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	clc.netchan.message.WriteByte(q2clc_stringcmd);
	clc.netchan.message.Print(cmd);
	if (Cmd_Argc() > 1)
	{
		clc.netchan.message.Print(" ");
		clc.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}

void CL_Setenv_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 2)
	{
		char buffer[1000];
		int i;

		String::Cpy(buffer, Cmd_Argv(1));
		String::Cat(buffer, sizeof(buffer), "=");

		for (i = 2; i < argc; i++)
		{
			String::Cat(buffer, sizeof(buffer), Cmd_Argv(i));
			String::Cat(buffer, sizeof(buffer), " ");
		}

		putenv(buffer);
	}
	else if (argc == 2)
	{
		char* env = getenv(Cmd_Argv(1));

		if (env)
		{
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			Com_Printf("%s undefined\n", Cmd_Argv(1), env);
		}
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f(void)
{
	if (cls.state != CA_CONNECTED && cls.state != CA_ACTIVE)
	{
		Com_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		clc.netchan.message.WriteByte(q2clc_stringcmd);
		clc.netchan.message.WriteString2(Cmd_ArgsUnmodified());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f(void)
{
	// never pause in multiplayer
	if (Cvar_VariableValue("maxclients") > 1 || !Com_ServerState())
	{
		Cvar_SetValueLatched("paused", 0);
		return;
	}

	Cvar_SetValueLatched("paused", !cl_paused->value);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f(void)
{
	CL_Disconnect();
	Com_Quit();
}

/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop(void)
{
	if (cls.state == CA_UNINITIALIZED)
	{
		return;
	}
	if (cls.state == CA_DISCONNECTED)
	{
		return;
	}

	CL_Disconnect();

	// drop loading plaque unless this is the initial game start
	if (cls.q2_disable_servercount != -1)
	{
		SCR_EndLoadingPlaque();		// get rid of loading plaque
	}
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket(void)
{
	netadr_t adr;
	int port;

	if (!SOCK_StringToAdr(cls.servername, &adr, PORT_SERVER))
	{
		Com_Printf("Bad server address\n");
		cls.q2_connect_time = 0;
		return;
	}

	port = Cvar_VariableValue("qport");
	cvar_modifiedFlags &= ~CVAR_USERINFO;

	Netchan_OutOfBandPrint(NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge,
		Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING, MAX_INFO_KEY,
			MAX_INFO_VALUE, true, false));
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend(void)
{
	netadr_t adr;

	// if the local server is running and we aren't
	// then connect
	if (cls.state == CA_DISCONNECTED && Com_ServerState())
	{
		cls.state = CA_CONNECTING;
		String::NCpy(cls.servername, "localhost", sizeof(cls.servername) - 1);
		// we don't need a challenge on the localhost
		CL_SendConnectPacket();
		return;
//		cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != CA_CONNECTING)
	{
		return;
	}

	if (cls.realtime - cls.q2_connect_time < 3000)
	{
		return;
	}

	if (!SOCK_StringToAdr(cls.servername, &adr, PORT_SERVER))
	{
		Com_Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}

	cls.q2_connect_time = cls.realtime;	// for retransmit requests

	Com_Printf("Connecting to %s...\n", cls.servername);

	Netchan_OutOfBandPrint(NS_CLIENT, adr, "getchallenge\n");
}


/*
================
CL_Connect_f

================
*/
void CL_Connect_f(void)
{
	char* server;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: connect <server>\n");
		return;
	}

	if (Com_ServerState())
	{	// if running a local server, kill it and reissue
		SV_Shutdown("Server quit\n", false);
	}
	else
	{
		CL_Disconnect();
	}

	server = Cmd_Argv(1);

	NET_Config(true);		// allow remote

	CL_Disconnect();

	cls.state = CA_CONNECTING;
	String::NCpy(cls.servername, server, sizeof(cls.servername) - 1);
	cls.q2_connect_time = -99999;	// CL_CheckForResend() will fire immediately
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f(void)
{
	char message[1024];
	int i;
	netadr_t to;

	if (!rcon_client_password->string)
	{
		Com_Printf("You must set 'rcon_password' before\n"
				   "issuing an rcon command.\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config(true);		// allow remote

	String::Cat(message, sizeof(message), "rcon ");

	String::Cat(message, sizeof(message), rcon_client_password->string);
	String::Cat(message, sizeof(message), " ");

	for (i = 1; i < Cmd_Argc(); i++)
	{
		String::Cat(message, sizeof(message), Cmd_Argv(i));
		String::Cat(message, sizeof(message), " ");
	}

	if (cls.state >= CA_CONNECTED)
	{
		to = clc.netchan.remoteAddress;
	}
	else
	{
		if (!String::Length(rcon_address->string))
		{
			Com_Printf("You must either be connected,\n"
					   "or set the 'rcon_address' cvar\n"
					   "to issue rcon commands\n");

			return;
		}
		SOCK_StringToAdr(rcon_address->string, &to, PORT_SERVER);
	}

	NET_SendPacket(NS_CLIENT, String::Length(message) + 1, message, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState(void)
{
	S_StopAllSounds();
	CLQ2_ClearEffects();
	CLQ2_ClearTEnts();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));
	Com_Memset(&clq2_entities, 0, sizeof(clq2_entities));

	clc.netchan.message.Clear();

}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(void)
{
	byte final[32];

	if (cls.state == CA_DISCONNECTED)
	{
		return;
	}

	if (cl_timedemo && cl_timedemo->value)
	{
		int time;

		time = Sys_Milliseconds_() - cl.q2_timedemo_start;
		if (time > 0)
		{
			Com_Printf("%i frames, %3.1f seconds: %3.1f fps\n", cl.q2_timedemo_frames,
				time / 1000.0, cl.q2_timedemo_frames * 1000.0 / time);
		}
	}

	VectorClear(v_blend);

	M_ForceMenuOff();

	cls.q2_connect_time = 0;

	SCR_StopCinematic();

	if (clc.demorecording)
	{
		CL_Stop_f();
	}

	// send a disconnect message to the server
	final[0] = q2clc_stringcmd;
	String::Cpy((char*)final + 1, "disconnect");
	Netchan_Transmit(&clc.netchan, String::Length((char*)final), final);
	Netchan_Transmit(&clc.netchan, String::Length((char*)final), final);
	Netchan_Transmit(&clc.netchan, String::Length((char*)final), final);

	CL_ClearState();

	// stop download
	if (clc.download)
	{
		FS_FCloseFile(clc.download);
		clc.download = 0;
	}

	cls.state = CA_DISCONNECTED;
}

void CL_Disconnect_f(void)
{
	Com_Error(ERR_DROP, "Disconnected from server");
}


/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f(void)
{
	char send[2048];
	int i, l;
	char* in, * out;
	netadr_t adr;

	if (Cmd_Argc() != 3)
	{
		Com_Printf("packet <destination> <contents>\n");
		return;
	}

	NET_Config(true);		// allow remote

	if (!SOCK_StringToAdr(Cmd_Argv(1), &adr, PORT_SERVER))
	{
		Com_Printf("Bad address\n");
		return;
	}

	in = Cmd_Argv(2);
	out = send + 4;
	send[0] = send[1] = send[2] = send[3] = (char)0xff;

	l = String::Length(in);
	for (i = 0; i < l; i++)
	{
		if (in[i] == '\\' && in[i + 1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
		{
			*out++ = in[i];
		}
	}
	*out = 0;

	NET_SendPacket(NS_CLIENT, out - send, send, adr);
}

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f(void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (clc.download)
	{
		return;
	}

	SCR_BeginLoadingPlaque();
	cls.state = CA_CONNECTED;	// not active anymore, but not disconnected
	Com_Printf("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f(void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (clc.download)
	{
		return;
	}

	S_StopAllSounds();
	if (cls.state == CA_CONNECTED)
	{
		Com_Printf("reconnecting...\n");
		cls.state = CA_CONNECTED;
		clc.netchan.message.WriteChar(q2clc_stringcmd);
		clc.netchan.message.WriteString2("new");
		return;
	}

	if (*cls.servername)
	{
		if (cls.state >= CA_CONNECTED)
		{
			CL_Disconnect();
			cls.q2_connect_time = cls.realtime - 1500;
		}
		else
		{
			cls.q2_connect_time = -99999;	// fire immediately

		}
		cls.state = CA_CONNECTING;
		Com_Printf("reconnecting...\n");
	}
}

/*
=================
CL_ParseStatusMessage

Handle a reply from a ping
=================
*/
void CL_ParseStatusMessage(void)
{
	const char* s;

	s = net_message.ReadString2();

	Com_Printf("%s\n", s);
	M_AddToServerList(net_from, const_cast<char*>(s));
}


/*
=================
CL_PingServers_f
=================
*/
void CL_PingServers_f(void)
{
	int i;
	netadr_t adr;
	char name[32];
	const char* adrstring;
	Cvar* noudp;

	NET_Config(true);		// allow remote

	// send a broadcast packet
	Com_Printf("pinging broadcast...\n");

	noudp = Cvar_Get("noudp", "0", CVAR_INIT);
	if (!noudp->value)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i = 0; i < 16; i++)
	{
		String::Sprintf(name, sizeof(name), "adr%i", i);
		adrstring = Cvar_VariableString(name);
		if (!adrstring || !adrstring[0])
		{
			continue;
		}

		Com_Printf("pinging %s...\n", adrstring);
		if (!SOCK_StringToAdr(adrstring, &adr, PORT_SERVER))
		{
			Com_Printf("Bad address: %s\n", adrstring);
			continue;
		}
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
		{
			continue;
		}
		Com_Printf("client %i: %s\n", i, cl.q2_configstrings[Q2CS_PLAYERSKINS + i]);
		SCR_UpdateScreen();
		Sys_SendKeyEvents();	// pump message loop
		IN_ProcessEvents();
		CL_ParseClientinfo(i);
	}
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket(void)
{
	char* s;
	char* c;

	net_message.BeginReadingOOB();
	net_message.ReadLong();	// skip the -1

	s = const_cast<char*>(net_message.ReadStringLine2());

	Cmd_TokenizeString(s, false);

	c = Cmd_Argv(0);

	Com_Printf("%s: %s\n", SOCK_AdrToString(net_from), c);

	// server connection
	if (!String::Cmp(c, "client_connect"))
	{
		if (cls.state == CA_CONNECTED)
		{
			Com_Printf("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup(NS_CLIENT, &clc.netchan, net_from, cls.quakePort);
		clc.netchan.message.WriteChar(q2clc_stringcmd);
		clc.netchan.message.WriteString2("new");
		cls.state = CA_CONNECTED;
		return;
	}

	// server responding to a status broadcast
	if (!String::Cmp(c, "info"))
	{
		CL_ParseStatusMessage();
		return;
	}

	// remote command from gui front end
	if (!String::Cmp(c, "cmd"))
	{
		if (!SOCK_IsLocalAddress(net_from))
		{
			Com_Printf("Command packet from remote host.  Ignored.\n");
			return;
		}
		Sys_AppActivate();
		s = const_cast<char*>(net_message.ReadString2());
		Cbuf_AddText(s);
		Cbuf_AddText("\n");
		return;
	}
	// print command from somewhere
	if (!String::Cmp(c, "print"))
	{
		s = const_cast<char*>(net_message.ReadString2());
		Com_Printf("%s", s);
		return;
	}

	// ping from somewhere
	if (!String::Cmp(c, "ping"))
	{
		Netchan_OutOfBandPrint(NS_CLIENT, net_from, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!String::Cmp(c, "challenge"))
	{
		cls.challenge = String::Atoi(Cmd_Argv(1));
		CL_SendConnectPacket();
		return;
	}

	// echo request from server
	if (!String::Cmp(c, "echo"))
	{
		Netchan_OutOfBandPrint(NS_CLIENT, net_from, "%s", Cmd_Argv(1));
		return;
	}

	Com_Printf("Unknown command.\n");
}


/*
=================
CL_DumpPackets

A vain attempt to help bad TCP stacks that cause problems
when they overflow
=================
*/
void CL_DumpPackets(void)
{
	while (NET_GetPacket(NS_CLIENT, &net_from, &net_message))
	{
		Com_Printf("dumnping a packet\n");
	}
}

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets(void)
{
	while (NET_GetPacket(NS_CLIENT, &net_from, &net_message))
	{
//	Com_Printf ("packet\n");
		//
		// remote command packet
		//
		if (*(int*)net_message._data == -1)
		{
			CL_ConnectionlessPacket();
			continue;
		}

		if (cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING)
		{
			continue;		// dump it if not connected

		}
		if (net_message.cursize < 8)
		{
			Com_Printf("%s: Runt packet\n",SOCK_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!SOCK_CompareAdr(net_from, clc.netchan.remoteAddress))
		{
			Com_DPrintf("%s:sequenced packet without connection\n",
				SOCK_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&clc.netchan, &net_message))
		{
			continue;		// wasn't accepted for some reason
		}
		CL_ParseServerMessage();
	}

	//
	// check timeout
	//
	if (cls.state >= CA_CONNECTED &&
		cls.realtime - clc.netchan.lastReceived > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)	// timeoutcount saves debugger
		{
			Com_Printf("\nServer connection timed out.\n");
			CL_Disconnect();
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}

}


//=============================================================================

/*
==============
CL_FixUpGender_f
==============
*/
void CL_FixUpGender(void)
{
	char* p;
	char sk[80];

	if (gender_auto->value)
	{

		if (gender->modified)
		{
			// was set directly, don't override the user
			gender->modified = false;
			return;
		}

		String::NCpy(sk, skin->string, sizeof(sk) - 1);
		if ((p = strchr(sk, '/')) != NULL)
		{
			*p = 0;
		}
		if (String::ICmp(sk, "male") == 0 || String::ICmp(sk, "cyborg") == 0)
		{
			Cvar_SetLatched("gender", "male");
		}
		else if (String::ICmp(sk, "female") == 0 || String::ICmp(sk, "crackhor") == 0)
		{
			Cvar_SetLatched("gender", "female");
		}
		else
		{
			Cvar_SetLatched("gender", "none");
		}
		gender->modified = false;
	}
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f(void)
{
	Com_Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING, MAX_INFO_KEY,
			MAX_INFO_VALUE, true, false));
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();
	CL_RegisterSounds();
}

int precache_check;	// for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte* precache_model;	// used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT + 13)

static const char* env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CL_RequestNextDownload(void)
{
	int map_checksum;		// for detecting cheater maps
	char fn[MAX_OSPATH];
	dmd2_t* pheader;

	if (cls.state != CA_CONNECTED)
	{
		return;
	}

	if (!allow_download->value && precache_check < ENV_CNT)
	{
		precache_check = ENV_CNT;
	}

//ZOID
	if (precache_check == Q2CS_MODELS)	// confirm map
	{
		precache_check = Q2CS_MODELS + 2;	// 0 isn't used
		if (allow_download_maps->value)
		{
			if (!CL_CheckOrDownloadFile(cl.q2_configstrings[Q2CS_MODELS + 1]))
			{
				return;	// started a download
			}
		}
	}
	if (precache_check >= Q2CS_MODELS && precache_check < Q2CS_MODELS + MAX_MODELS_Q2)
	{
		if (allow_download_models->value)
		{
			while (precache_check < Q2CS_MODELS + MAX_MODELS_Q2 &&
				   cl.q2_configstrings[precache_check][0])
			{
				if (cl.q2_configstrings[precache_check][0] == '*' ||
					cl.q2_configstrings[precache_check][0] == '#')
				{
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0)
				{
					if (!CL_CheckOrDownloadFile(cl.q2_configstrings[precache_check]))
					{
						precache_model_skin = 1;
						return;	// started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model)
				{

					FS_ReadFile(cl.q2_configstrings[precache_check], (void**)&precache_model);
					if (!precache_model)
					{
						precache_model_skin = 0;
						precache_check++;
						continue;	// couldn't load it
					}
					if (LittleLong(*(unsigned*)precache_model) != IDMESH2HEADER)
					{
						// not an alias model
						FS_FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmd2_t*)precache_model;
					if (LittleLong(pheader->version) != MESH2_VERSION)
					{
						precache_check++;
						precache_model_skin = 0;
						continue;	// couldn't load it
					}
				}

				pheader = (dmd2_t*)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->num_skins))
				{
					if (!CL_CheckOrDownloadFile((char*)precache_model +
							LittleLong(pheader->ofs_skins) +
							(precache_model_skin - 1) * MAX_MD2_SKINNAME))
					{
						precache_model_skin++;
						return;	// started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = Q2CS_SOUNDS;
	}
	if (precache_check >= Q2CS_SOUNDS && precache_check < Q2CS_SOUNDS + MAX_SOUNDS_Q2)
	{
		if (allow_download_sounds->value)
		{
			if (precache_check == Q2CS_SOUNDS)
			{
				precache_check++;	// zero is blank
			}
			while (precache_check < Q2CS_SOUNDS + MAX_SOUNDS_Q2 &&
				   cl.q2_configstrings[precache_check][0])
			{
				if (cl.q2_configstrings[precache_check][0] == '*')
				{
					precache_check++;
					continue;
				}
				String::Sprintf(fn, sizeof(fn), "sound/%s", cl.q2_configstrings[precache_check++]);
				if (!CL_CheckOrDownloadFile(fn))
				{
					return;	// started a download
				}
			}
		}
		precache_check = Q2CS_IMAGES;
	}
	if (precache_check >= Q2CS_IMAGES && precache_check < Q2CS_IMAGES + MAX_IMAGES_Q2)
	{
		if (precache_check == Q2CS_IMAGES)
		{
			precache_check++;	// zero is blank
		}
		while (precache_check < Q2CS_IMAGES + MAX_IMAGES_Q2 &&
			   cl.q2_configstrings[precache_check][0])
		{
			String::Sprintf(fn, sizeof(fn), "pics/%s.pcx", cl.q2_configstrings[precache_check++]);
			if (!CL_CheckOrDownloadFile(fn))
			{
				return;	// started a download
			}
		}
		precache_check = Q2CS_PLAYERSKINS;
	}
	// skins are special, since a player has three things to download:
	// model, weapon model and skin
	// so precache_check is now *3
	if (precache_check >= Q2CS_PLAYERSKINS && precache_check < Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT)
	{
		if (allow_download_players->value)
		{
			while (precache_check < Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT)
			{
				int i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], * p;

				i = (precache_check - Q2CS_PLAYERSKINS) / PLAYER_MULT;
				n = (precache_check - Q2CS_PLAYERSKINS) % PLAYER_MULT;

				if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
				{
					precache_check = Q2CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.q2_configstrings[Q2CS_PLAYERSKINS + i], '\\')) != NULL)
				{
					p++;
				}
				else
				{
					p = cl.q2_configstrings[Q2CS_PLAYERSKINS + i];
				}
				String::Cpy(model, p);
				p = strchr(model, '/');
				if (!p)
				{
					p = strchr(model, '\\');
				}
				if (p)
				{
					*p++ = 0;
					String::Cpy(skin, p);
				}
				else
				{
					*skin = 0;
				}

				switch (n)
				{
				case 0:	// model
					String::Sprintf(fn, sizeof(fn), "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile(fn))
					{
						precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 1:	// weapon model
					String::Sprintf(fn, sizeof(fn), "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile(fn))
					{
						precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 2:	// weapon skin
					String::Sprintf(fn, sizeof(fn), "players/%s/weapon.pcx", model);
					if (!CL_CheckOrDownloadFile(fn))
					{
						precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 3:	// skin
					String::Sprintf(fn, sizeof(fn), "players/%s/%s.pcx", model, skin);
					if (!CL_CheckOrDownloadFile(fn))
					{
						precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 4:	// skin_i
					String::Sprintf(fn, sizeof(fn), "players/%s/%s_i.pcx", model, skin);
					if (!CL_CheckOrDownloadFile(fn))
					{
						precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return;	// started a download
					}
					// move on to next model
					precache_check = Q2CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;

		CM_LoadMap(cl.q2_configstrings[Q2CS_MODELS + 1], true, &map_checksum);

		if (map_checksum != String::Atoi(cl.q2_configstrings[Q2CS_MAPCHECKSUM]))
		{
			Com_Error(ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.q2_configstrings[Q2CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT)
	{
		if (allow_download->value && allow_download_maps->value)
		{
			while (precache_check < TEXTURE_CNT)
			{
				int n = precache_check++ - ENV_CNT - 1;

				if (n & 1)
				{
					String::Sprintf(fn, sizeof(fn), "env/%s%s.pcx",
						cl.q2_configstrings[Q2CS_SKY], env_suf[n / 2]);
				}
				else
				{
					String::Sprintf(fn, sizeof(fn), "env/%s%s.tga",
						cl.q2_configstrings[Q2CS_SKY], env_suf[n / 2]);
				}
				if (!CL_CheckOrDownloadFile(fn))
				{
					return;	// started a download
				}
			}
		}
		precache_check = TEXTURE_CNT;
	}

	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT + 1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT + 1)
	{
		if (allow_download->value && allow_download_maps->value)
		{
			while (precache_tex < CM_GetNumTextures())
			{
				char fn[MAX_OSPATH];

				sprintf(fn, "textures/%s.wal", CM_GetTextureName(precache_tex++));
				if (!CL_CheckOrDownloadFile(fn))
				{
					return;	// started a download
				}
			}
		}
		precache_check = TEXTURE_CNT + 999;
	}

//ZOID
	CL_RegisterSounds();
	CL_PrepRefresh();

	clc.netchan.message.WriteByte(q2clc_stringcmd);
	clc.netchan.message.WriteString2(va("begin %i\n", precache_spawncount));
}

/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f(void)
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		int map_checksum;		// for detecting cheater maps

		CM_LoadMap(cl.q2_configstrings[Q2CS_MODELS + 1], true, &map_checksum);
		CL_RegisterSounds();
		CL_PrepRefresh();
		return;
	}

	precache_check = Q2CS_MODELS;
	precache_spawncount = String::Atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh.
============
*/
static void VID_Restart_f(void)
{
	vid_restart_requested = true;
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal(void)
{
	cls.state = CA_DISCONNECTED;
	cls.realtime = Sys_Milliseconds_();

	CL_InitInput();

	adr0 = Cvar_Get("adr0", "", CVAR_ARCHIVE);
	adr1 = Cvar_Get("adr1", "", CVAR_ARCHIVE);
	adr2 = Cvar_Get("adr2", "", CVAR_ARCHIVE);
	adr3 = Cvar_Get("adr3", "", CVAR_ARCHIVE);
	adr4 = Cvar_Get("adr4", "", CVAR_ARCHIVE);
	adr5 = Cvar_Get("adr5", "", CVAR_ARCHIVE);
	adr6 = Cvar_Get("adr6", "", CVAR_ARCHIVE);
	adr7 = Cvar_Get("adr7", "", CVAR_ARCHIVE);
	adr8 = Cvar_Get("adr8", "", CVAR_ARCHIVE);

//
// register our variables
//
	cl_stereo_separation = Cvar_Get("cl_stereo_separation", "0.4", CVAR_ARCHIVE);

	cl_add_blend = Cvar_Get("cl_blend", "1", 0);
	cl_add_particles = Cvar_Get("cl_particles", "1", 0);
	cl_add_entities = Cvar_Get("clq2_entities", "1", 0);
	cl_gun = Cvar_Get("cl_gun", "1", 0);
	clq2_footsteps = Cvar_Get("cl_footsteps", "1", 0);
	cl_noskins = Cvar_Get("cl_noskins", "0", 0);
	cl_autoskins = Cvar_Get("cl_autoskins", "0", 0);
	cl_predict = Cvar_Get("cl_predict", "1", 0);
//	cl_minfps = Cvar_Get ("cl_minfps", "5", 0);
	cl_maxfps = Cvar_Get("cl_maxfps", "90", 0);



	cl_shownet = Cvar_Get("cl_shownet", "0", 0);
	cl_showmiss = Cvar_Get("cl_showmiss", "0", 0);
	cl_showclamp = Cvar_Get("showclamp", "0", 0);
	cl_timeout = Cvar_Get("cl_timeout", "120", 0);
	cl_paused = Cvar_Get("paused", "0", 0);
	cl_timedemo = Cvar_Get("timedemo", "0", 0);

	rcon_client_password = Cvar_Get("rcon_password", "", 0);
	rcon_address = Cvar_Get("rcon_address", "", 0);

	cl_lightlevel = Cvar_Get("r_lightlevel", "0", 0);

	//
	// userinfo
	//
	info_password = Cvar_Get("password", "", CVAR_USERINFO);
	info_spectator = Cvar_Get("spectator", "0", CVAR_USERINFO);
	name = Cvar_Get("name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE);
	skin = Cvar_Get("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE);
	rate = Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);		// FIXME
	msg = Cvar_Get("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	q2_hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	fov = Cvar_Get("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE);
	gender = Cvar_Get("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	gender_auto = Cvar_Get("gender_auto", "1", CVAR_ARCHIVE);
	gender->modified = false;	// clear this so we know when user sets it manually

	cl_vwep = Cvar_Get("cl_vwep", "1", CVAR_ARCHIVE);


	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand("pause", CL_Pause_f);
	Cmd_AddCommand("pingservers", CL_PingServers_f);
	Cmd_AddCommand("skins", CL_Skins_f);

	Cmd_AddCommand("userinfo", CL_Userinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand("changing", CL_Changing_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stop", CL_Stop_f);

	Cmd_AddCommand("quit", CL_Quit_f);

	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);

	Cmd_AddCommand("rcon", CL_Rcon_f);

//  Cmd_AddCommand ("packet", CL_Packet_f); // this is dangerous to leave in

	Cmd_AddCommand("setenv", CL_Setenv_f);

	Cmd_AddCommand("precache", CL_Precache_f);

	Cmd_AddCommand("download", CL_Download_f);

	Cmd_AddCommand("vid_restart", VID_Restart_f);

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server
	Cmd_AddCommand("wave", NULL);
	Cmd_AddCommand("inven", NULL);
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("use", NULL);
	Cmd_AddCommand("drop", NULL);
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("info", NULL);
	Cmd_AddCommand("prog", NULL);
	Cmd_AddCommand("give", NULL);
	Cmd_AddCommand("god", NULL);
	Cmd_AddCommand("notarget", NULL);
	Cmd_AddCommand("noclip", NULL);
	Cmd_AddCommand("invuse", NULL);
	Cmd_AddCommand("invprev", NULL);
	Cmd_AddCommand("invnext", NULL);
	Cmd_AddCommand("invdrop", NULL);
	Cmd_AddCommand("weapnext", NULL);
	Cmd_AddCommand("weapprev", NULL);
}



/*
===============
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CL_WriteConfiguration()
{
	if (cls.state == CA_UNINITIALIZED)
	{
		return;
	}

	fileHandle_t f = FS_FOpenFileWrite("config.cfg");
	if (!f)
	{
		Com_Printf("Couldn't write config.cfg.\n");
		return;
	}

	FS_Printf(f, "// generated by quake, do not modify\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}


/*
==================
CL_FixCvarCheats

==================
*/

typedef struct
{
	const char* name;
	const char* value;
	Cvar* var;
} cheatvar_t;

cheatvar_t cheatvars[] = {
	{"timescale", "1"},
	{"timedemo", "0"},
	{"r_drawworld", "1"},
	{"cl_testlights", "0"},
	{"r_fullbright", "0"},
	{"r_drawflat", "0"},
	{"paused", "0"},
	{"fixedtime", "0"},
	{"r_lightmap", "0"},
	{"r_saturatelighting", "0"},
	{NULL, NULL}
};

int numcheatvars;

void CL_FixCvarCheats(void)
{
	int i;
	cheatvar_t* var;

	if (!String::Cmp(cl.q2_configstrings[Q2CS_MAXCLIENTS], "1") ||
		!cl.q2_configstrings[Q2CS_MAXCLIENTS][0])
	{
		return;		// single player can cheat

	}
	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get(cheatvars[numcheatvars].name,
				cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i = 0, var = cheatvars; i < numcheatvars; i++, var++)
	{
		if (String::Cmp(var->var->string, var->value))
		{
			Cvar_SetLatched(var->name, var->value);
		}
	}
}

/*
** VID_NewWindow
*/
static void VID_NewWindow(int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff()
{
	R_BeginRegistration(&cls.glconfig);

	// let the sound and input subsystems know about the new window
	VID_NewWindow(cls.glconfig.vidWidth, cls.glconfig.vidHeight);

	Draw_InitLocal();
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
static void VID_CheckChanges(void)
{
	if (vid_restart_requested)
	{
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_restart_requested = false;
		cl.q2_refresh_prepped = false;
		cls.disable_screen = true;

		R_Shutdown(true);
		CL_InitRenderStuff();
		cls.disable_screen = false;
	}
}

//============================================================================

/*
==================
CL_SendCommand

==================
*/
void CL_SendCommand(void)
{
	// get new key events
	Sys_SendKeyEvents();

	IN_ProcessEvents();

	// process console commands
	Cbuf_Execute();

	// fix any cheating cvars
	CL_FixCvarCheats();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();
}


void CL_UpdateSounds()
{
	if (cl_paused->value)
	{
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (!cl.q2_sound_prepped)
	{
		return;
	}

	for (int i = 0; i < MAX_EDICTS_Q2; i++)
	{
		vec3_t origin;
		CL_GetEntitySoundOrigin(i, origin);
		S_UpdateEntityPosition(i, origin);
	}

	S_ClearLoopingSounds(false);
	for (int i = 0; i < cl.q2_frame.num_entities; i++)
	{
		int num = (cl.q2_frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		q2entity_state_t* ent = &cl_parse_entities[num];
		if (!ent->sound)
		{
			continue;
		}
		S_AddLoopingSound(num, ent->origin, vec3_origin, 0, cl.sound_precache[ent->sound], 0, 0);
	}
}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{
	static int extratime;
	static int lasttimecalled;

	if (com_dedicated->value)
	{
		return;
	}

	extratime += msec;

	if (!cl_timedemo->value)
	{
		if (cls.state == CA_CONNECTED && extratime < 100)
		{
			return;			// don't flood packets out while connecting
		}
		if (extratime < 1000 / cl_maxfps->value)
		{
			return;			// framerate is too high
		}
	}

	// let the mouse activate or deactivate
	IN_Frame();

	// decide the simulation time
	cls.frametime = extratime;
	cls.q2_frametimeFloat = extratime / 1000.0;
	cl.serverTime += extratime;
	cls.realtime = curtime;

	extratime = 0;
	if (cls.q2_frametimeFloat > (1.0 / 5))
	{
		cls.q2_frametimeFloat = (1.0 / 5);
	}
	if (cls.frametime > 200)
	{
		cls.frametime = 200;
	}
	cls.realFrametime = cls.frametime;

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
	{
		clc.netchan.lastReceived = Sys_Milliseconds_();
	}

	// fetch results from server
	CL_ReadPackets();

	// send a new command message to the server
	CL_SendCommand();

	// predict all unacknowledged movements
	CL_PredictMovement();

	// allow rendering DLL change
	VID_CheckChanges();
	if (!cl.q2_refresh_prepped && cls.state == CA_ACTIVE)
	{
		CL_PrepRefresh();
	}

	// update the screen
	if (host_speeds->value)
	{
		time_before_ref = Sys_Milliseconds_();
	}
	SCR_UpdateScreen();
	if (host_speeds->value)
	{
		time_after_ref = Sys_Milliseconds_();
	}

	// update audio
	CL_UpdateSounds();

	S_Respatialize(cl.playernum + 1, cl.refdef.vieworg, cl.refdef.viewaxis, 0);

	S_Update();

	CDAudio_Update();

	// advance local effects for next frame
	CL_RunDLights();
	CL_RunLightStyles();
	SCR_RunCinematic();
	SCR_RunConsole();

	cls.framecount++;

	if (log_stats->value)
	{
		if (cls.state == CA_ACTIVE)
		{
			if (!lasttimecalled)
			{
				lasttimecalled = Sys_Milliseconds_();
				if (log_stats_file)
				{
					FS_Printf(log_stats_file, "0\n");
				}
			}
			else
			{
				int now = Sys_Milliseconds_();

				if (log_stats_file)
				{
					FS_Printf(log_stats_file, "%d\n", now - lasttimecalled);
				}
				lasttimecalled = now;
			}
		}
	}
}


//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	if (com_dedicated->value)
	{
		return;		// nothing running on the client

	}
	CL_SharedInit();

	// all archived variables will now be loaded

	Con_Init();
	IN_Init();
#if defined __linux__ || defined __sgi
	S_Init();
	CL_InitRenderStuff();
#else
	CL_InitRenderStuff();
	S_Init();	// sound must be initialized after window is created
#endif

	V_Init();

	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	M_Init();

	SCR_Init();
	cls.disable_screen = true;	// don't draw yet

	CDAudio_Init();
	CL_InitLocal();

//	Cbuf_AddText ("exec autoexec.cfg\n");
	FS_ExecAutoexec();
	Cbuf_Execute();

}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

	CL_WriteConfiguration();

	CDAudio_Shutdown();
	S_Shutdown();
	IN_Shutdown();
	R_Shutdown(true);
}

float* CL_GetSimOrg()
{
	return NULL;
}

#include "../server/server.h"
bool CL_IsServerActive()
{
	return sv.state == ss_game;
}
