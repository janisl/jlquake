/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_main.c  -- client main loop

#include "client.h"
#include "../../server/public.h"
#include <limits.h>

Cvar* cl_motd;

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeout;
Cvar* cl_timeNudge;
Cvar* cl_freezeDemo;

Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_motdString;

Cvar* cl_trn;

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

void CL_CheckForResend(void);
void CL_ShowIP_f(void);


/*
==============
CL_EndgameMenu

Called by Com_Error when a game has ended and is dropping out to main menu in the "endgame" menu ('credits' right now)
==============
*/
void CL_EndgameMenu(void)
{
	cls.ws_endgamemenu = true;		// start it next frame
}

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future wsusercmd_t executed before it is executed
======================
*/
/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand(void)
{
	int r, index, l;

	r = clc.q3_reliableSequence - (random() * 5);
	index = clc.q3_reliableSequence & (MAX_RELIABLE_COMMANDS_WOLF - 1);
	l = String::Length(clc.q3_reliableCommands[index]);
	if (l >= MAX_STRING_CHARS - 1)
	{
		l = MAX_STRING_CHARS - 2;
	}
	clc.q3_reliableCommands[index][l] = '\n';
	clc.q3_reliableCommands[index][l + 1] = '\0';
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage(void)
{
	int r;
	QMsg buf;
	byte bufData[MAX_MSGLEN_WOLF];
	int s;

	if (!clc.demofile)
	{
		CLT3_DemoCompleted();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, clc.demofile);
	if (r != 4)
	{
		CLT3_DemoCompleted();
		return;
	}
	clc.q3_serverMessageSequence = LittleLong(s);

	// init the message
	buf.Init(bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demofile);
	if (r != 4)
	{
		CLT3_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1)
	{
		CLT3_DemoCompleted();
		return;
	}
	if (buf.cursize > buf.maxsize)
	{
		common->Error("CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN_WOLF");
	}
	r = FS_Read(buf._data, buf.cursize, clc.demofile);
	if (r != buf.cursize)
	{
		common->Printf("Demo file was truncated.\n");
		CLT3_DemoCompleted();
		return;
	}

	clc.q3_lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CLT3_ParseServerMessage(&buf);
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH], extension[32];
	char* arg;

	if (Cmd_Argc() != 2)
	{
		common->Printf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(true);

	// open the demo file
	arg = Cmd_Argv(1);
	String::Sprintf(extension, sizeof(extension), ".dm_%d", WSPROTOCOL_VERSION);
	if (!String::ICmp(arg + String::Length(arg) - String::Length(extension), extension))
	{
		String::Sprintf(name, sizeof(name), "demos/%s", arg);
	}
	else
	{
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", arg, WSPROTOCOL_VERSION);
	}

	FS_FOpenFileRead(name, &clc.demofile, true);
	if (!clc.demofile)
	{
		common->Error("couldn't open %s", name);
		return;
	}
	String::NCpyZ(clc.q3_demoName, Cmd_Argv(1), sizeof(clc.q3_demoName));

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = true;
	String::NCpyZ(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

	// read demo messages until connected
	while (cls.state >= CA_CONNECTED && cls.state < CA_PRIMED)
	{
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.q3_firstDemoFrameSkipped = false;
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	String::NCpyZ(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	common->DPrintf("CL_NextDemo: %s\n", v);
	if (!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo","");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}


//======================================================================

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}

	Con_Close();
	in_keyCatchers = 0;

// this was for multi-threaded music
//	S_StartBackgroundTrack( "sound/music/l_briefing_1.wav", "", -2);	// '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)

	// if we are already connected to the local host, stay connected
	if (cls.state >= CA_CONNECTED && !String::ICmp(cls.servername, "localhost"))
	{
		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		memset(cls.q3_updateInfoString, 0, sizeof(cls.q3_updateInfoString));
		memset(clc.q3_serverMessage, 0, sizeof(clc.q3_serverMessage));
		memset(&cl.ws_gameState, 0, sizeof(cl.ws_gameState));
		clc.q3_lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	}
	else
	{
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set("nextmap", "");
		CL_Disconnect(true);
		String::NCpyZ(cls.servername, "localhost", sizeof(cls.servername));
		cls.state = CA_CHALLENGING;		// so the connect screen is drawn
		in_keyCatchers = 0;
		SCR_UpdateScreen();
		clc.q3_connectTime = -RETRANSMIT_TIMEOUT;
		SOCK_StringToAdr(cls.servername, &clc.q3_serverAddress, Q3PORT_SERVER);
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}

	// make sure sound is quiet
	S_FadeAllSounds(0, 0, false);
}

/*
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd(void)
{
	char info[MAX_INFO_STRING_Q3];

	if (!cl_motd->integer)
	{
		return;
	}
	common->Printf("Resolving %s\n", UPDATE_SERVER_NAME);
	if (!SOCK_StringToAdr(UPDATE_SERVER_NAME, &cls.q3_updateServer, PORT_UPDATE))
	{
		common->Printf("Couldn't resolve address\n");
		return;
	}
	common->Printf("%s resolved to %i.%i.%i.%i:%i\n", UPDATE_SERVER_NAME,
		cls.q3_updateServer.ip[0], cls.q3_updateServer.ip[1],
		cls.q3_updateServer.ip[2], cls.q3_updateServer.ip[3],
		BigShort(cls.q3_updateServer.port));

	info[0] = 0;
	String::Sprintf(cls.q3_updateChallenge, sizeof(cls.q3_updateChallenge), "%i", rand());

	Info_SetValueForKey(info, "challenge", cls.q3_updateChallenge, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(info, "renderer", cls.glconfig.renderer_string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(info, "version", com_version->string, MAX_INFO_STRING_Q3);

	NET_OutOfBandPrint(NS_CLIENT, cls.q3_updateServer, "getmotd \"%s\"\n", info);
}

/*
===================
CL_RequestAuthorization

Authorization server protocol
-----------------------------

All commands are text in Q3 out of band packets (leading 0xff 0xff 0xff 0xff).

Whenever the client tries to get a challenge from the server it wants to
connect to, it also blindly fires off a packet to the authorize server:

getKeyAuthorize <challenge> <cdkey>

cdkey may be "demo"


#OLD The authorize server returns a:
#OLD
#OLD keyAthorize <challenge> <accept | deny>
#OLD
#OLD A client will be accepted if the cdkey is valid and it has not been used by any other IP
#OLD address in the last 15 minutes.


The server sends a:

getIpAuthorize <challenge> <ip>

The authorize server returns a:

ipAuthorize <challenge> <accept | deny | demo | unknown >

A client will be accepted if a valid cdkey was sent by that ip (only) in the last 15 minutes.
If no response is received from the authorize server after two tries, the client will be let
in anyway.
===================
*/
void CL_RequestAuthorization(void)
{
	char nums[64];
	Cvar* fs;

	if (!cls.q3_authorizeServer.port)
	{
		common->Printf("Resolving %s\n", WSAUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(WSAUTHORIZE_SERVER_NAME, &cls.q3_authorizeServer, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}

		common->Printf("%s resolved to %i.%i.%i.%i:%i\n", WSAUTHORIZE_SERVER_NAME,
			cls.q3_authorizeServer.ip[0], cls.q3_authorizeServer.ip[1],
			cls.q3_authorizeServer.ip[2], cls.q3_authorizeServer.ip[3],
			BigShort(cls.q3_authorizeServer.port));
	}
	if (cls.q3_authorizeServer.type == NA_BAD)
	{
		return;
	}

	CLT3_CDKeyForAuthorize(nums);

	fs = Cvar_Get("cl_anonymous", "0", CVAR_INIT | CVAR_SYSTEMINFO);
	NET_OutOfBandPrint(NS_CLIENT, cls.q3_authorizeServer, va("getKeyAuthorize %i %s", fs->integer, nums));
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 2)
	{
		char buffer[1024];
		int i;

		String::Cpy(buffer, Cmd_Argv(1));
		strcat(buffer, "=");

		for (i = 2; i < argc; i++)
		{
			strcat(buffer, Cmd_Argv(i));
			strcat(buffer, " ");
		}

		Q_putenv(buffer);
	}
	else if (argc == 2)
	{
		char* env = getenv(Cmd_Argv(1));

		if (env)
		{
			common->Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			common->Printf("%s undefined\n", Cmd_Argv(1), env);
		}
	}
}


/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void)
{
	SCR_StopCinematic();
	// RF, make sure loading variables are turned off
	Cvar_Set("savegame_loading", "0");
	Cvar_Set("g_reloading", "0");
	if (cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC)
	{
		Com_Error(ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f(void)
{
	if (!String::Length(cls.servername) || !String::Cmp(cls.servername, "localhost"))
	{
		common->Printf("Can't reconnect to localhost.\n");
		return;
	}
	Cbuf_AddText(va("connect %s\n", cls.servername));
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
		common->Printf("usage: connect [server]\n");
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// fire a message off to the motd server
	CL_RequestMotd();

	// clear any previous "server full" type messages
	clc.q3_serverMessage[0] = 0;

	server = Cmd_Argv(1);

	if (com_sv_running->integer && !String::Cmp(server, "localhost"))
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(true);
	Con_Close();

	String::NCpyZ(cls.servername, server, sizeof(cls.servername));

	if (!SOCK_StringToAdr(cls.servername, &clc.q3_serverAddress, Q3PORT_SERVER))
	{
		common->Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}
	if (clc.q3_serverAddress.port == 0)
	{
		clc.q3_serverAddress.port = BigShort(Q3PORT_SERVER);
	}
	common->Printf("%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
		clc.q3_serverAddress.ip[0], clc.q3_serverAddress.ip[1],
		clc.q3_serverAddress.ip[2], clc.q3_serverAddress.ip[3],
		BigShort(clc.q3_serverAddress.port));

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if (SOCK_IsLocalAddress(clc.q3_serverAddress))
	{
		cls.state = CA_CHALLENGING;
	}
	else
	{
		cls.state = CA_CONNECTING;
	}

	in_keyCatchers = 0;
	clc.q3_connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.q3_connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", server);
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
		common->Printf("You must set 'rcon_password' before\n"
				   "issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	strcat(message, "rcon ");

	strcat(message, rcon_client_password->string);
	strcat(message, " ");

	for (i = 1; i < Cmd_Argc(); i++)
	{
		strcat(message, Cmd_Argv(i));
		strcat(message, " ");
	}

	if (cls.state >= CA_CONNECTED)
	{
		to = clc.netchan.remoteAddress;
	}
	else
	{
		if (!String::Length(rconAddress->string))
		{
			common->Printf("You must either be connected,\n"
					   "or set the 'rconAddress' cvar\n"
					   "to issue rcon commands\n");

			return;
		}
		SOCK_StringToAdr(rconAddress->string, &to, Q3PORT_SERVER);
		if (to.port == 0)
		{
			to.port = BigShort(Q3PORT_SERVER);
		}
	}

	NET_SendPacket(NS_CLIENT, String::Length(message) + 1, message, to);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer(void)
{
	CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void CL_Vid_Restart_f(void)
{

	vmCvar_t musicCvar;

	// RF, don't show percent bar, since the memory usage will just sit at the same level anyway
	Cvar_Set("com_expectedhunkusage", "-1");

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CLT3_ShutdownUI();
	// shutdown the CGame
	CLT3_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart(clc.q3_checksumFeed);

	S_BeginRegistration();	// all sound handles are now invalid

	cls.q3_rendererStarted = false;
	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_soundRegistered = false;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	CIN_CloseAllVideos();

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CLT3_StartHunkUsers();

	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.q3_cgameStarted = true;
		CLT3_InitCGame();
		// send pure checksums
		CLT3_SendPureChecksums();
	}

	// start music if there was any

	Cvar_Register(&musicCvar, "s_currentMusic", "", CVAR_ROM);
	if (String::Length(musicCvar.string))
	{
		S_StartBackgroundTrack(musicCvar.string, musicCvar.string, 1000);
	}

	// fade up volume
	S_FadeAllSounds(1, 0, false);
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();

	CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f(void)
{
	common->Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f(void)
{
	common->Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f(void)
{
	int i;
	int ofs;

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Not connected to a server.\n");
		return;
	}

	for (i = 0; i < MAX_CONFIGSTRINGS_WS; i++)
	{
		ofs = cl.ws_gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		common->Printf("%4i: %s\n", i, cl.ws_gameState.stringData + ofs);
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f(void)
{
	common->Printf("--------- Client Information ---------\n");
	common->Printf("state: %i\n", cls.state);
	common->Printf("Server: %s\n", cls.servername);
	common->Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3));
	common->Printf("--------------------------------------\n");
}


//====================================================================

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend(void)
{
	int port;
	char info[MAX_INFO_STRING_Q3];

	// don't send anything if playing back a demo
	if (clc.demoplaying)
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING)
	{
		return;
	}

	if (cls.realtime - clc.q3_connectTime < RETRANSMIT_TIMEOUT)
	{
		return;
	}

	clc.q3_connectTime = cls.realtime;	// for retransmit requests
	clc.q3_connectPacketCount++;


	switch (cls.state)
	{
	case CA_CONNECTING:
		// requesting a challenge
		if (!SOCK_IsLANAddress(clc.q3_serverAddress))
		{
			CL_RequestAuthorization();
		}
		NET_OutOfBandPrint(NS_CLIENT, clc.q3_serverAddress, "getchallenge");
		break;

	case CA_CHALLENGING:
		// sending back the challenge
		port = Cvar_VariableValue("net_qport");

		String::NCpyZ(info, Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3), sizeof(info));
		Info_SetValueForKey(info, "protocol", va("%i", WSPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "qport", va("%i", port), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "challenge", va("%i", clc.q3_challenge), MAX_INFO_STRING_Q3);
		NET_OutOfBandPrint(NS_CLIENT, clc.q3_serverAddress, "connect \"%s\"", info);
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		common->FatalError("CL_CHeckForResend: bad cls.state");
	}
}


/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket(netadr_t from)
{
	if (cls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore it
	// (it might be a malicious spoof)
	if (cls.realtime - clc.q3_lastPacketTime < 3000)
	{
		return;
	}

	// drop the connection (FIXME: connection dropped dialog)
	common->Printf("Server disconnected for unknown reason\n");
	CL_Disconnect(true);
}


/*
===================
CL_MotdPacket

===================
*/
void CL_MotdPacket(netadr_t from)
{
	const char* challenge;
	char* info;

	// if not from our server, ignore it
	if (!SOCK_CompareAdr(from, cls.q3_updateServer))
	{
		return;
	}

	info = Cmd_Argv(1);

	// check challenge
	challenge = Info_ValueForKey(info, "challenge");
	if (String::Cmp(challenge, cls.q3_updateChallenge))
	{
		return;
	}

	challenge = Info_ValueForKey(info, "motd");

	String::NCpyZ(cls.q3_updateInfoString, info, sizeof(cls.q3_updateInfoString));
	Cvar_Set("cl_motdString", challenge);
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket(netadr_t from, QMsg* msg)
{
	const char* s;
	char* c;

	msg->BeginReadingOOB();
	msg->ReadLong();	// skip the -1

	s = msg->ReadStringLine();

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	common->DPrintf("CL packet %s: %s\n", SOCK_AdrToString(from), c);

	// challenge from the server we are connecting to
	if (!String::ICmp(c, "challengeResponse"))
	{
		if (cls.state != CA_CONNECTING)
		{
			common->Printf("Unwanted challenge response received.  Ignored.\n");
		}
		else
		{
			// start sending challenge repsonse instead of challenge request packets
			clc.q3_challenge = String::Atoi(Cmd_Argv(1));
			cls.state = CA_CHALLENGING;
			clc.q3_connectPacketCount = 0;
			clc.q3_connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.q3_serverAddress = from;
		}
		return;
	}

	// server connection
	if (!String::ICmp(c, "connectResponse"))
	{
		if (cls.state >= CA_CONNECTED)
		{
			common->Printf("Dup connect received.  Ignored.\n");
			return;
		}
		if (cls.state != CA_CHALLENGING)
		{
			common->Printf("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if (!SOCK_CompareBaseAdr(from, clc.q3_serverAddress))
		{
			common->Printf("connectResponse from a different address.  Ignored.\n");
			common->Printf("%s should have been %s\n", SOCK_AdrToString(from),
				SOCK_AdrToString(clc.q3_serverAddress));
			return;
		}
		Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"));
		cls.state = CA_CONNECTED;
		clc.q3_lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if (!String::ICmp(c, "infoResponse"))
	{
		CLT3_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!String::ICmp(c, "statusResponse"))
	{
		CLT3_ServerStatusResponse(from, msg);
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!String::ICmp(c, "disconnect"))
	{
		CL_DisconnectPacket(from);
		return;
	}

	// echo request from server
	if (!String::ICmp(c, "echo"))
	{
		NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
		return;
	}

	// cd check
	if (!String::ICmp(c, "keyAuthorize"))
	{
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD from id
	if (!String::ICmp(c, "motd"))
	{
		CL_MotdPacket(from);
		return;
	}

	// echo request from server
	if (!String::ICmp(c, "print"))
	{
		s = msg->ReadString();
		String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
		common->Printf("%s", s);
		return;
	}

	// echo request from server
	if (!String::ICmp(c, "getserversResponse\\"))
	{
		CLT3_ServersResponsePacket(from, msg);
		return;
	}

	common->DPrintf("Unknown connectionless packet command.\n");
}


/*
=================
CLT3_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CLT3_PacketEvent(netadr_t from, QMsg* msg)
{
	int headerBytes;

	clc.q3_lastPacketTime = cls.realtime;

	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		CL_ConnectionlessPacket(from, msg);
		return;
	}

	if (cls.state < CA_CONNECTED)
	{
		return;		// can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		common->Printf("%s: Runt packet\n",SOCK_AdrToString(from));
		return;
	}

	//
	// packet from server
	//
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		common->DPrintf("%s:sequenced packet without connection\n",
			SOCK_AdrToString(from));
		// FIXME: send a client disconnect?
		return;
	}

	if (!CLT3_Netchan_Process(&clc.netchan, msg))
	{
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.q3_serverMessageSequence = LittleLong(*(int*)msg->_data);

	clc.q3_lastPacketTime = cls.realtime;
	CLT3_ParseServerMessage(msg);

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (clc.demorecording && !clc.q3_demowaiting)
	{
		CLT3_WriteDemoMessage(msg, headerBytes);
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout(void)
{
	//
	// check timeout
	//
	if ((!cl_paused->integer || !sv_paused->integer) &&
		cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC &&
		cls.realtime - clc.q3_lastPacketTime > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)			// timeoutcount saves debugger
		{
			common->Printf("\nServer connection timed out.\n");
			CL_Disconnect(true);
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo(void)
{
	// don't add reliable commands when not yet connected
	if (cls.state < CA_CHALLENGING)
	{
		return;
	}
	// don't overflow the reliable command buffer when paused
	if (cl_paused->integer)
	{
		return;
	}
	// send a reliable userinfo update if needed
	if (cvar_modifiedFlags & CVAR_USERINFO)
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3)));
	}

}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{

	if (!com_cl_running->integer)
	{
		return;
	}

	if (cls.ws_endgamemenu)
	{
		cls.ws_endgamemenu = false;
		UIWS_SetEndGameMenu();
	}
	else if (cls.state == CA_DISCONNECTED && !(in_keyCatchers & KEYCATCH_UI) &&
			 !com_sv_running->integer)
	{
		// if disconnected, bring up the menu
		S_StopAllSounds();
		UI_SetMainMenu();
	}

	// if recording an avi, lock to a fixed fps
	if (cl_avidemo->integer && msec)
	{
		// save the current screen
		if (cls.state == CA_ACTIVE || cl_forceavidemo->integer)
		{
			Cbuf_ExecuteText(EXEC_NOW, "screenshot silent\n");
		}
		// fixed time for next frame
		msec = (1000 / cl_avidemo->integer) * com_timescale->value;
		if (msec == 0)
		{
			msec = 1;
		}
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if (cl_timegraph->integer)
	{
		SCR_DebugGraph(cls.realFrametime * 0.25, 0);
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CLT3_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// Ridah, don't update if we're doing a quick reload
//	if (Cvar_VariableIntegerValue("savegame_loading") != 2) {
//		// if waiting at intermission, don't update sound
//		char buf[MAX_QPATH];
//		Cvar_VariableStringBuffer( "g_missionStats", buf, sizeof(buf) );
//		if (String::Length(buf) <= 1 ) {
//			// update audio
	S_Update();
//		}
//	}

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================
// Ridah, startup-caching system
typedef struct
{
	char name[MAX_QPATH];
	int hits;
	int lastSetIndex;
} cacheItem_t;
typedef enum {
	CACHE_SOUNDS,
	CACHE_MODELS,
	CACHE_IMAGES,

	CACHE_NUMGROUPS
} cacheGroup_t;
static cacheItem_t cacheGroups[CACHE_NUMGROUPS] = {
	{{'s','o','u','n','d',0}, CACHE_SOUNDS},
	{{'m','o','d','e','l',0}, CACHE_MODELS},
	{{'i','m','a','g','e',0}, CACHE_IMAGES},
};
#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75		// if hit on this percentage of maps, it'll get cached

static int cacheIndex;
static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];

static void CL_Cache_StartGather_f(void)
{
	cacheIndex = 0;
	memset(cacheItems, 0, sizeof(cacheItems));

	Cvar_Set("cl_cacheGathering", "1");
}

static void CL_Cache_UsedFile_f(void)
{
	char groupStr[MAX_QPATH];
	char itemStr[MAX_QPATH];
	int i,group;
	cacheItem_t* item;

	if (Cmd_Argc() < 2)
	{
		common->Error("usedfile without enough parameters\n");
		return;
	}

	String::Cpy(groupStr, Cmd_Argv(1));

	String::Cpy(itemStr, Cmd_Argv(2));
	for (i = 3; i < Cmd_Argc(); i++)
	{
		strcat(itemStr, " ");
		strcat(itemStr, Cmd_Argv(i));
	}
	String::ToLower(itemStr);

	// find the cache group
	for (i = 0; i < CACHE_NUMGROUPS; i++)
	{
		if (!String::NCmp(groupStr, cacheGroups[i].name, MAX_QPATH))
		{
			break;
		}
	}
	if (i == CACHE_NUMGROUPS)
	{
		common->Error("usedfile without a valid cache group\n");
		return;
	}

	// see if it's already there
	group = i;
	for (i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++)
	{
		if (!item->name[0])
		{
			// didn't find it, so add it here
			String::NCpyZ(item->name, itemStr, MAX_QPATH);
			if (cacheIndex > 9999)		// hack, but yeh
			{
				item->hits = cacheIndex;
			}
			else
			{
				item->hits++;
			}
			item->lastSetIndex = cacheIndex;
			break;
		}
		if (item->name[0] == itemStr[0] && !String::NCmp(item->name, itemStr, MAX_QPATH))
		{
			if (item->lastSetIndex != cacheIndex)
			{
				item->hits++;
				item->lastSetIndex = cacheIndex;
			}
			break;
		}
	}
}

static void CL_Cache_SetIndex_f(void)
{
	if (Cmd_Argc() < 2)
	{
		common->Error("setindex needs an index\n");
		return;
	}

	cacheIndex = String::Atoi(Cmd_Argv(1));
}

static void CL_Cache_MapChange_f(void)
{
	cacheIndex++;
}

static void CL_Cache_EndGather_f(void)
{
	// save the frequently used files to the cache list file
	int i, j, handle, cachePass;
	char filename[MAX_QPATH];

	cachePass = (int)floor((float)cacheIndex * CACHE_HIT_RATIO);

	for (i = 0; i < CACHE_NUMGROUPS; i++)
	{
		String::NCpyZ(filename, cacheGroups[i].name, MAX_QPATH);
		String::Cat(filename, MAX_QPATH, ".cache");

#ifdef __MACOS__	//DAJ MacOS file typing
		{
			extern _MSL_IMP_EXP_C long _fcreator, _ftype;
			_ftype = 'WlfB';
			_fcreator = 'WlfS';
		}
#endif
		handle = FS_FOpenFileWrite(filename);

		for (j = 0; j < MAX_CACHE_ITEMS; j++)
		{
			// if it's a valid filename, and it's been hit enough times, cache it
			if (cacheItems[i][j].hits >= cachePass && strstr(cacheItems[i][j].name, "/"))
			{
				FS_Write(cacheItems[i][j].name, String::Length(cacheItems[i][j].name), handle);
				FS_Write("\n", 1, handle);
			}
		}

		FS_FCloseFile(handle);
	}

	Cvar_Set("cl_cacheGathering", "0");
}

// done.
//============================================================================

/*
================
CL_MapRestart_f
================
*/
void CL_MapRestart_f(void)
{
	if (!com_cl_running)
	{
		return;
	}
	if (!com_cl_running->integer)
	{
		return;
	}
	common->Printf("This command is no longer functional.\nUse \"loadgame current\" to load the current map.");
}

/*
================
CL_SetRecommended_f
================
*/
void CL_SetRecommended_f(void)
{
	if (Cmd_Argc() > 1)
	{
		Com_SetRecommended(true);
	}
	else
	{
		Com_SetRecommended(false);
	}

}

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef(void)
{
	R_Shutdown(true);
}

/*
============
CL_InitRef
============
*/
void CL_InitRef(void)
{
	common->Printf("----- Initializing Renderer ----\n");

	BotDrawDebugPolygonsFunc = BotDrawDebugPolygons;

	common->Printf("-------------------------------\n");

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");
}

// RF, trap manual client damage commands so users can't issue them manually
void CL_ClientDamageCommand(void)
{
	// do nothing
}

// NERVE - SMF
void CL_startMultiplayer_f(void)
{
#ifdef __MACOS__	//DAJ
	Sys_StartProcess("Wolfenstein MP", true);
#elif defined(__linux__)
	Sys_StartProcess("./wolf.x86", true);
#else
	Sys_StartProcess("WolfMP.exe", true);
#endif
}
// -NERVE - SMF

//----(SA) added
/*
==============
CL_ShellExecute_URL_f
Format:
  shellExecute "open" <url> <doExit>

TTimo
  show_bug.cgi?id=447
  only supporting "open" syntax for URL openings, others are not portable or need to be added on a case-by-case basis
  the shellExecute syntax as been kept to remain compatible with win32 SP demo pk3, but this thing only does open URL

==============
*/

void CL_ShellExecute_URL_f(void)
{
	qboolean doexit;

	common->DPrintf("CL_ShellExecute_URL_f\n");

	if (String::ICmp(Cmd_Argv(1),"open"))
	{
		common->DPrintf("invalid CL_ShellExecute_URL_f syntax (shellExecute \"open\" <url> <doExit>)\n");
		return;
	}

	if (Cmd_Argc() < 4)
	{
		doexit = true;
	}
	else
	{
		doexit = (qboolean)(String::Atoi(Cmd_Argv(3)));
	}

	Sys_OpenURL(Cmd_Argv(2),doexit);
}
//----(SA) end
//===========================================================================================

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	common->Printf("----- Client Initialization -----\n");

	CL_SharedInit();

	Con_Init();

	CL_ClearState();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	UI_Init();

	CL_InitInput();

	CLT3_InitServerLists();

	//
	// register our variables
	//
	cl_motd = Cvar_Get("cl_motd", "1", 0);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	clt3_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	clt3_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	clt3_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
	clt3_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);

	cl_timedemo = Cvar_Get("timedemo", "0", 0);
	cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	clt3_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	clt3_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	clt3_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get("cg_autoswitch", "2", CVAR_ARCHIVE);

	// Rafael - particle switch
	Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE);
	// done

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

	// userinfo
	Cvar_Get("name", "Player", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("model", "bj2", CVAR_USERINFO | CVAR_ARCHIVE);		// temp until we have an skeletal american model
	Cvar_Get("head", "default", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("color", "4", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);

//----(SA) added
	Cvar_Get("cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cg_emptyswitch", "0", CVAR_USERINFO | CVAR_ARCHIVE);
//----(SA) end

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	//
	// register our commands
	//
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CLT3_Record_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CLT3_StopRecord_f);
	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("setenv", CL_Setenv_f);
	Cmd_AddCommand("showip", CL_ShowIP_f);
	Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
	Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);

	// Ridah, startup-caching system
	Cmd_AddCommand("cache_startgather", CL_Cache_StartGather_f);
	Cmd_AddCommand("cache_usedfile", CL_Cache_UsedFile_f);
	Cmd_AddCommand("cache_setindex", CL_Cache_SetIndex_f);
	Cmd_AddCommand("cache_mapchange", CL_Cache_MapChange_f);
	Cmd_AddCommand("cache_endgather", CL_Cache_EndGather_f);

	Cmd_AddCommand("updatescreen", SCR_UpdateScreen);
	// done.

	// RF, add this command so clients can't bind a key to send client damage commands to the server
	Cmd_AddCommand("cld", CL_ClientDamageCommand);

	Cmd_AddCommand("startMultiplayer", CL_startMultiplayer_f);			// NERVE - SMF

	// TTimo
	// show_bug.cgi?id=447
	Cmd_AddCommand("shellExecute", CL_ShellExecute_URL_f);
	//Cmd_AddCommand ( "shellExecute", CL_ShellExecute_f );	//----(SA) added (mainly for opening web pages from the menu)

	// RF, prevent users from issuing a map_restart manually
	Cmd_AddCommand("map_restart", CL_MapRestart_f);

	Cmd_AddCommand("setRecommended", CL_SetRecommended_f);

	CL_InitTranslation();

	CL_InitRef();

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	common->Printf("----- Client Initialization Complete -----\n");
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown(void)
{
	static qboolean recursive = false;

	common->Printf("----- CL_Shutdown -----\n");

	if (recursive)
	{
		printf("recursive shutdown\n");
		return;
	}
	recursive = true;

	CL_Disconnect(true);

	S_Shutdown();
	CL_ShutdownRef();

	CLT3_ShutdownUI();

	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("snd_restart");
	Cmd_RemoveCommand("vid_restart");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("demo");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("globalservers");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("setenv");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("serverstatus");
	Cmd_RemoveCommand("showip");
	Cmd_RemoveCommand("model");

	// Ridah, startup-caching system
	Cmd_RemoveCommand("cache_startgather");
	Cmd_RemoveCommand("cache_usedfile");
	Cmd_RemoveCommand("cache_setindex");
	Cmd_RemoveCommand("cache_mapchange");
	Cmd_RemoveCommand("cache_endgather");

	Cmd_RemoveCommand("updatehunkusage");
	// done.

	Cvar_Set("cl_running", "0");

	recursive = false;

	memset(&cls, 0, sizeof(cls));

	common->Printf("-----------------------\n");
}


/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void)
{
	SOCK_ShowIP();
}

float* CL_GetSimOrg()
{
	return NULL;
}
