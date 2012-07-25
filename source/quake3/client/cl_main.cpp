/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_main.c  -- client main loop

#include "client.h"
#include <limits.h>

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

Cvar* cl_nodelta;

Cvar* cl_motd;

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeout;
Cvar* cl_maxpackets;
Cvar* cl_packetdup;
Cvar* cl_timeNudge;
Cvar* cl_showTimeDelta;
Cvar* cl_freezeDemo;

Cvar* cl_showSend;
Cvar* cl_timedemo;
Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_activeAction;

Cvar* cl_motdString;

Cvar* cl_allowDownload;

Cvar* cl_serverStatusResendTime;
Cvar* cl_trn;

vm_t* cgvm;

ping_t cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int serverStatusCount;

void CL_CheckForResend(void);
void CL_ShowIP_f(void);
void CL_ServerStatus_f(void);
void CL_ServerStatusResponse(netadr_t from, QMsg* msg);

/*
===============
CL_CDDialog

Called by Com_Error when a cd is needed
===============
*/
void CL_CDDialog(void)
{
	cls.q3_cddialog = true;	// start it next frame
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
not have future q3usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char* cmd)
{
	int index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if (clc.q3_reliableSequence - clc.q3_reliableAcknowledge > MAX_RELIABLE_COMMANDS_Q3)
	{
		Com_Error(ERR_DROP, "Client command overflow");
	}
	clc.q3_reliableSequence++;
	index = clc.q3_reliableSequence & (MAX_RELIABLE_COMMANDS_Q3 - 1);
	String::NCpyZ(clc.q3_reliableCommands[index], cmd, sizeof(clc.q3_reliableCommands[index]));
}

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand(void)
{
	int r, index, l;

	r = clc.q3_reliableSequence - (random() * 5);
	index = clc.q3_reliableSequence & (MAX_RELIABLE_COMMANDS_Q3 - 1);
	l = String::Length(clc.q3_reliableCommands[index]);
	if (l >= MAX_STRING_CHARS - 1)
	{
		l = MAX_STRING_CHARS - 2;
	}
	clc.q3_reliableCommands[index][l] = '\n';
	clc.q3_reliableCommands[index][l + 1] = '\0';
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(QMsg* msg, int headerBytes)
{
	int len, swlen;

	// write the packet sequence
	len = clc.q3_serverMessageSequence;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(msg->_data + headerBytes, len, clc.demofile);
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f(void)
{
	int len;

	if (!clc.demorecording)
	{
		common->Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	clc.q3_spDemoRecording = false;
	common->Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename(int number, char* fileName)
{
	int a,b,c,d;

	if (number < 0 || number > 9999)
	{
		String::Sprintf(fileName, MAX_OSPATH, "demo9999.tga");
		return;
	}

	a = number / 1000;
	number -= a * 1000;
	b = number / 100;
	number -= b * 100;
	c = number / 10;
	number -= c * 10;
	d = number;

	String::Sprintf(fileName, MAX_OSPATH, "demo%i%i%i%i",
		a, b, c, d);
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static char demoName[MAX_QPATH];		// compiler bug workaround
void CL_Record_f(void)
{
	char name[MAX_OSPATH];
	byte bufData[MAX_MSGLEN_Q3];
	QMsg buf;
	int i;
	int len;
	q3entityState_t* ent;
	q3entityState_t nullstate;
	char* s;

	if (Cmd_Argc() > 2)
	{
		common->Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		if (!clc.q3_spDemoRecording)
		{
			common->Printf("Already recording.\n");
		}
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("You must be in a level to record.\n");
		return;
	}

	// sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	if (!Cvar_VariableValue("g_synchronousClients"))
	{
		common->Printf(S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		String::NCpyZ(demoName, s, sizeof(demoName));
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION);
	}
	else
	{
		int number;

		// scan for a free demo name
		for (number = 0; number <= 9999; number++)
		{
			CL_DemoFilename(number, demoName);
			String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION);

			len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;	// file doesn't exist
			}
		}
	}

	// open the demo file

	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;
	if (Cvar_VariableValue("ui_recordSPDemo"))
	{
		clc.q3_spDemoRecording = true;
	}
	else
	{
		clc.q3_spDemoRecording = false;
	}


	String::NCpyZ(clc.q3_demoName, demoName, sizeof(clc.q3_demoName));

	// don't start saving messages until a non-delta compressed message is received
	clc.q3_demowaiting = true;

	// write out the gamestate message
	buf.Init(bufData, sizeof(bufData));
	buf.Bitstream();

	// NOTE, MRE: all server->client messages now acknowledge
	buf.WriteLong(clc.q3_reliableSequence);

	buf.WriteByte(q3svc_gamestate);
	buf.WriteLong(clc.q3_serverCommandSequence);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
	{
		if (!cl.q3_gameState.stringOffsets[i])
		{
			continue;
		}
		s = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[i];
		buf.WriteByte(q3svc_configstring);
		buf.WriteShort(i);
		buf.WriteBigString(s);
	}

	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		ent = &cl.q3_entityBaselines[i];
		if (!ent->number)
		{
			continue;
		}
		buf.WriteByte(q3svc_baseline);
		MSGQ3_WriteDeltaEntity(&buf, &nullstate, ent, true);
	}

	buf.WriteByte(q3svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	buf.WriteLong(clc.q3_clientNum);
	// write the checksum feed
	buf.WriteLong(clc.q3_checksumFeed);

	// finished writing the client packet
	buf.WriteByte(q3svc_EOF);

	// write it to the demo file
	len = LittleLong(clc.q3_serverMessageSequence - 1);
	FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf._data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted(void)
{
	if (cl_timedemo && cl_timedemo->integer)
	{
		int time;

		time = Sys_Milliseconds() - clc.q3_timeDemoStart;
		if (time > 0)
		{
			common->Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.q3_timeDemoFrames,
				time / 1000.0, clc.q3_timeDemoFrames * 1000.0 / time);
		}
	}

	CL_Disconnect(true);
	CL_NextDemo();
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
	byte bufData[MAX_MSGLEN_Q3];
	int s;

	if (!clc.demofile)
	{
		CL_DemoCompleted();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, clc.demofile);
	if (r != 4)
	{
		CL_DemoCompleted();
		return;
	}
	clc.q3_serverMessageSequence = LittleLong(s);

	// init the message
	buf.Init(bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demofile);
	if (r != 4)
	{
		CL_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1)
	{
		CL_DemoCompleted();
		return;
	}
	if (buf.cursize > buf.maxsize)
	{
		Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN_Q3");
	}
	r = FS_Read(buf._data, buf.cursize, clc.demofile);
	if (r != buf.cursize)
	{
		common->Printf("Demo file was truncated.\n");
		CL_DemoCompleted();
		return;
	}

	clc.q3_lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseServerMessage(&buf);
}

/*
====================
CL_WalkDemoExt
====================
*/
static void CL_WalkDemoExt(char* arg, char* name, int* demofile)
{
	int i = 0;
	*demofile = 0;
	while (demo_protocols[i])
	{
		String::Sprintf(name, MAX_OSPATH, "demos/%s.dm_%d", arg, demo_protocols[i]);
		FS_FOpenFileRead(name, demofile, true);
		if (*demofile)
		{
			common->Printf("Demo file: %s\n", name);
			break;
		}
		else
		{
			common->Printf("Not found: %s\n", name);
		}
		i++;
	}
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH];
	char* arg, * ext_test;
	int protocol, i;
	char retry[MAX_OSPATH];

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

	// check for an extension .dm_?? (?? is protocol)
	ext_test = arg + String::Length(arg) - 6;
	if ((String::Length(arg) > 6) && (ext_test[0] == '.') && ((ext_test[1] == 'd') || (ext_test[1] == 'D')) && ((ext_test[2] == 'm') || (ext_test[2] == 'M')) && (ext_test[3] == '_'))
	{
		protocol = String::Atoi(ext_test + 4);
		i = 0;
		while (demo_protocols[i])
		{
			if (demo_protocols[i] == protocol)
			{
				break;
			}
			i++;
		}
		if (demo_protocols[i])
		{
			String::Sprintf(name, sizeof(name), "demos/%s", arg);
			FS_FOpenFileRead(name, &clc.demofile, true);
		}
		else
		{
			common->Printf("Protocol %d not supported for demos\n", protocol);
			String::NCpyZ(retry, arg, sizeof(retry));
			retry[String::Length(retry) - 6] = 0;
			CL_WalkDemoExt(retry, name, &clc.demofile);
		}
	}
	else
	{
		CL_WalkDemoExt(arg, name, &clc.demofile);
	}

	if (!clc.demofile)
	{
		Com_Error(ERR_DROP, "couldn't open %s", name);
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
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop(void)
{
	// start the demo loop again
	Cbuf_AddText("d1\n");
	in_keyCatchers = 0;
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
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void)
{

	// clear sounds
	S_DisableSounds();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	R_Shutdown(false);		// don't destroy window or context

	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_rendererStarted = false;
	cls.q3_soundRegistered = false;
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{

	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	CL_StartHunkUsers();
}

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

	// if we are already connected to the local host, stay connected
	if (cls.state >= CA_CONNECTED && !String::ICmp(cls.servername, "localhost"))
	{
		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset(cls.q3_updateInfoString, 0, sizeof(cls.q3_updateInfoString));
		Com_Memset(clc.q3_serverMessage, 0, sizeof(clc.q3_serverMessage));
		Com_Memset(&cl.q3_gameState, 0, sizeof(cl.q3_gameState));
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
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState(void)
{

//	S_StopAllSounds();

	Com_Memset(&cl, 0, sizeof(cl));
}


/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(qboolean showMainMenu)
{
	if (!com_cl_running || !com_cl_running->integer)
	{
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if (clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if (clc.download)
	{
		FS_FCloseFile(clc.download);
		clc.download = 0;
	}
	*clc.downloadTempName = *clc.downloadName = 0;
	Cvar_Set("cl_downloadName", "");

	if (clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

	if (uivm && showMainMenu)
	{
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);
	}

	SCR_StopCinematic();
	S_ClearSoundBuffer(true);

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if (cls.state >= CA_CONNECTED)
	{
		CL_AddReliableCommand("disconnect");
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	CL_ClearState();

	// wipe the client connection
	Com_Memset(&clc, 0, sizeof(clc));

	cls.state = CA_DISCONNECTED;

	// allow cheats locally
	Cvar_Set("sv_cheats", "1");

	// not connected to a pure server anymore
	cl_connectedToPureServer = false;
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(const char* string)
{
	char* cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if (cmd[0] == '-')
	{
		return;
	}

	if (clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+')
	{
		common->Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	if (Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(string);
	}
	else
	{
		CL_AddReliableCommand(cmd);
	}
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
	common->Printf("%s resolved to %s\n", UPDATE_SERVER_NAME, SOCK_AdrToString(cls.q3_updateServer));

	info[0] = 0;
	// NOTE TTimo xoring against Com_Milliseconds, otherwise we may not have a true randomization
	// only srand I could catch before here is tr_noise.c l:26 srand(1001)
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=382
	// NOTE: the Com_Milliseconds xoring only affects the lower 16-bit word,
	//   but I decided it was enough randomization
	String::Sprintf(cls.q3_updateChallenge, sizeof(cls.q3_updateChallenge), "%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());

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
	int i, j, l;
	Cvar* fs;

	if (!cls.q3_authorizeServer.port)
	{
		common->Printf("Resolving %s\n", Q3AUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(Q3AUTHORIZE_SERVER_NAME, &cls.q3_authorizeServer, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}

		common->Printf("%s resolved to %s\n", Q3AUTHORIZE_SERVER_NAME, SOCK_AdrToString(cls.q3_authorizeServer));
	}
	if (cls.q3_authorizeServer.type == NA_BAD)
	{
		return;
	}

	// only grab the alphanumeric values from the cdkey, to avoid any dashes or spaces
	j = 0;
	l = String::Length(cl_cdkey);
	if (l > 32)
	{
		l = 32;
	}
	for (i = 0; i < l; i++)
	{
		if ((cl_cdkey[i] >= '0' && cl_cdkey[i] <= '9') ||
			(cl_cdkey[i] >= 'a' && cl_cdkey[i] <= 'z') ||
			(cl_cdkey[i] >= 'A' && cl_cdkey[i] <= 'Z')
			)
		{
			nums[j] = cl_cdkey[i];
			j++;
		}
	}
	nums[j] = 0;

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
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f(void)
{
	if (cls.state != CA_ACTIVE || clc.demoplaying)
	{
		common->Printf("Not connected to a server.\n");
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(Cmd_Args());
	}
}

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
	Cvar_Set("ui_singlePlayerActive", "0");
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
	Cvar_Set("ui_singlePlayerActive", "0");
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

	Cvar_Set("ui_singlePlayerActive", "0");

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

	/* MrE: 2000-09-13: now called in CL_DownloadsComplete
	CL_FlushMemory( );
	*/

	String::NCpyZ(cls.servername, server, sizeof(cls.servername));

	if (!SOCK_StringToAdr(cls.servername, &clc.q3_serverAddress, Q3PORT_SERVER))
	{
		common->Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}
	common->Printf("%s resolved to %s\n", cls.servername, SOCK_AdrToString(clc.q3_serverAddress));

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
	netadr_t to;

	if (!rcon_client_password->string)
	{
		common->Printf("You must set 'rconpassword' before\n"
				   "issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	String::Cat(message, sizeof(message), "rcon ");

	String::Cat(message, sizeof(message), rcon_client_password->string);
	String::Cat(message, sizeof(message), " ");

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
	String::Cat(message, sizeof(message), Cmd_Cmd() + 5);

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
	}

	NET_SendPacket(NS_CLIENT, String::Length(message) + 1, message, to);
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums(void)
{
	const char* pChecksums;
	char cMsg[MAX_INFO_VALUE_Q3];
	int i;

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = FS_ReferencedPakPureChecksums();

	// "cp"
	// "Yf"
	String::Sprintf(cMsg, sizeof(cMsg), "Yf ");
	String::Cat(cMsg, sizeof(cMsg), va("%d ", cl.q3_serverId));
	String::Cat(cMsg, sizeof(cMsg), pChecksums);
	for (i = 0; i < 2; i++)
	{
		cMsg[i] += 10;
	}
	CL_AddReliableCommand(cMsg);
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

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart(clc.q3_checksumFeed);

	cls.q3_rendererStarted = false;
	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_soundRegistered = false;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.q3_cgameStarted = true;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
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

	for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
	{
		ofs = cl.q3_gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		common->Printf("%4i: %s\n", i, cl.q3_gameState.stringData + ofs);
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
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete(void)
{

	// if we downloaded files we need to restart the file system
	if (clc.downloadRestart)
	{
		clc.downloadRestart = false;

		FS_Restart(clc.q3_checksumFeed);// We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand("donedl");

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if (cls.state != CA_LOADING)
	{
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory();

	// initialize the CGame
	cls.q3_cgameStarted = true;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload(const char* localName, const char* remoteName)
{

	common->DPrintf("***** CL_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

	String::NCpyZ(clc.downloadName, localName, sizeof(clc.downloadName));
	String::Sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0;	// Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName));
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void)
{
	char* s;
	char* remoteName, * localName;

	// We are looking to start a download here
	if (*clc.downloadList)
	{
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
		{
			s++;
		}
		remoteName = s;

		if ((s = strchr(s, '@')) == NULL)
		{
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if ((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + String::Length(localName);	// point at the nul byte

		}
		CL_BeginDownload(localName, remoteName);

		clc.downloadRestart = true;

		// move over the rest
		memmove(clc.downloadList, s, String::Length(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void)
{
	char missingfiles[1024];

	if (!cl_allowDownload->integer)
	{
		// autodownload is disabled on the client
		// but it's possible that some referenced files on the server are missing
		if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false))
		{
			// NOTE TTimo I would rather have that printed as a modal message box
			//   but at this point while joining the game we don't know wether we will successfully join or not
			common->Printf("\nWARNING: You are missing some files referenced by the server:\n%s"
					   "You might not be able to join the game\n"
					   "Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n", missingfiles);
		}
	}
	else if (FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), true))
	{

		common->Printf("Need paks: %s\n", clc.downloadList);

		if (*clc.downloadList)
		{
			// if autodownloading is not enabled on the server
			cls.state = CA_CONNECTED;
			CL_NextDownload();
			return;
		}

	}

	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend(void)
{
	int port, i;
	char info[MAX_INFO_STRING_Q3];
	char data[MAX_INFO_STRING_Q3];

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
		Info_SetValueForKey(info, "protocol", va("%i", PROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "qport", va("%i", port), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "challenge", va("%i", clc.q3_challenge), MAX_INFO_STRING_Q3);

		String::Cpy(data, "connect ");
		// TTimo adding " " around the userinfo string to avoid truncated userinfo on the server
		//   (Com_TokenizeString tokenizes around spaces)
		data[8] = '"';

		for (i = 0; i < String::Length(info); i++)
		{
			data[9 + i] = info[i];		// + (clc.q3_challenge)&0x3;
		}
		data[9 + i] = '"';
		data[10 + i] = 0;

		// NOTE TTimo don't forget to set the right data length!
		NET_OutOfBandData(NS_CLIENT, clc.q3_serverAddress, (byte*)&data[0], i + 10);
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error(ERR_FATAL, "CL_CheckForResend: bad cls.state");
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

	// drop the connection
	common->Printf("Server disconnected for unknown reason\n");
	Cvar_Set("com_errorMessage", "Server disconnected for unknown reason\n");
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
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo(q3serverInfo_t* server, q3serverAddress_t* address)
{
	server->adr.type  = NA_IP;
	server->adr.ip[0] = address->ip[0];
	server->adr.ip[1] = address->ip[1];
	server->adr.ip[2] = address->ip[2];
	server->adr.ip[3] = address->ip[3];
	server->adr.port  = address->port;
	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType = 0;
	server->netType = 0;
}

#define MAX_SERVERSPERPACKET    256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket(netadr_t from, QMsg* msg)
{
	int i, count, max, total;
	q3serverAddress_t addresses[MAX_SERVERSPERPACKET];
	int numservers;
	byte* buffptr;
	byte* buffend;

	common->Printf("CL_ServersResponsePacket\n");

	if (cls.q3_numglobalservers == -1)
	{
		// state to detect lack of servers or lack of response
		cls.q3_numglobalservers = 0;
		cls.q3_numGlobalServerAddresses = 0;
	}

	if (cls.q3_nummplayerservers == -1)
	{
		cls.q3_nummplayerservers = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->_data;
	buffend    = buffptr + msg->cursize;
	while (buffptr + 1 < buffend)
	{
		// advance to initial token
		do
		{
			if (*buffptr++ == '\\')
			{
				break;
			}
		}
		while (buffptr < buffend);

		if (buffptr >= buffend - 6)
		{
			break;
		}

		// parse out ip
		addresses[numservers].ip[0] = *buffptr++;
		addresses[numservers].ip[1] = *buffptr++;
		addresses[numservers].ip[2] = *buffptr++;
		addresses[numservers].ip[3] = *buffptr++;

		// parse out port
		addresses[numservers].port = (*buffptr++) << 8;
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port = BigShort(addresses[numservers].port);

		// syntax check
		if (*buffptr != '\\')
		{
			break;
		}

		common->DPrintf("server: %d ip: %d.%d.%d.%d:%d\n",numservers,
			addresses[numservers].ip[0],
			addresses[numservers].ip[1],
			addresses[numservers].ip[2],
			addresses[numservers].ip[3],
			addresses[numservers].port);

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET)
		{
			break;
		}

		// parse out EOT
		if (buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T')
		{
			break;
		}
	}

	if (cls.q3_masterNum == 0)
	{
		count = cls.q3_numglobalservers;
		max = MAX_GLOBAL_SERVERS_Q3;
	}
	else
	{
		count = cls.q3_nummplayerservers;
		max = MAX_OTHER_SERVERS_Q3;
	}

	for (i = 0; i < numservers && count < max; i++)
	{
		// build net address
		q3serverInfo_t* server = (cls.q3_masterNum == 0) ? &cls.q3_globalServers[count] : &cls.q3_mplayerServers[count];

		CL_InitServerInfo(server, &addresses[i]);
		// advance to next slot
		count++;
	}

	// if getting the global list
	if (cls.q3_masterNum == 0)
	{
		if (cls.q3_numGlobalServerAddresses < MAX_GLOBAL_SERVERS_Q3)
		{
			// if we couldn't store the servers in the main list anymore
			for (; i < numservers && count >= max; i++)
			{
				q3serverAddress_t* addr;
				// just store the addresses in an additional list
				addr = &cls.q3_globalServerAddresses[cls.q3_numGlobalServerAddresses++];
				addr->ip[0] = addresses[i].ip[0];
				addr->ip[1] = addresses[i].ip[1];
				addr->ip[2] = addresses[i].ip[2];
				addr->ip[3] = addresses[i].ip[3];
				addr->port  = addresses[i].port;
			}
		}
	}

	if (cls.q3_masterNum == 0)
	{
		cls.q3_numglobalservers = count;
		total = count + cls.q3_numGlobalServerAddresses;
	}
	else
	{
		cls.q3_nummplayerservers = count;
		total = count;
	}

	common->Printf("%d servers parsed (total %d)\n", numservers, total);
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
			common->DPrintf("challengeResponse: %d\n", clc.q3_challenge);
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
		CL_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!String::ICmp(c, "statusResponse"))
	{
		CL_ServerStatusResponse(from, msg);
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
	if (!String::NCmp(c, "getserversResponse", 18))
	{
		CL_ServersResponsePacket(from, msg);
		return;
	}

	common->DPrintf("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent(netadr_t from, QMsg* msg)
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

	if (!CL_Netchan_Process(&clc.netchan, msg))
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
	CL_ParseServerMessage(msg);

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (clc.demorecording && !clc.q3_demowaiting)
	{
		CL_WriteDemoMessage(msg, headerBytes);
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
		if (++cl.timeoutcount > 5)		// timeoutcount saves debugger
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

	if (cls.q3_cddialog)
	{
		// bring up the cd error dialog if needed
		cls.q3_cddialog = false;
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NEED_CD);
	}
	else if (cls.state == CA_DISCONNECTED && !(in_keyCatchers & KEYCATCH_UI) &&
			 !com_sv_running->integer)
	{
		// if disconnected, bring up the menu
		S_StopAllSounds();
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
	}

	// if recording an avi, lock to a fixed fps
	if (cl_avidemo->integer && msec)
	{
		// save the current screen
		if (cls.state == CA_ACTIVE || cl_forceavidemo->integer)
		{
			Cbuf_ExecuteText(EXEC_NOW, "screenshot silent\n");
		}
		// fixed time for next frame'
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
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update audio
	S_Update();

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef()
{
	R_Shutdown(true);
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer(void)
{
	// this sets up the renderer and calls R_Init
	R_BeginRegistration(&cls.glconfig);

	viddef.width = SCREEN_WIDTH;
	viddef.height = SCREEN_HEIGHT;
#if 0
	// adjust for wide screens
	if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640)
	{
		*x += 0.5 * (cls.glconfig.vidWidth - (cls.glconfig.vidHeight * 640 / 480));
	}
#endif

	// load character sets
	cls.charSetShader = R_RegisterShader("gfx/2d/bigchars");
	cls.whiteShader = R_RegisterShader("white");
	cls.consoleShader = R_RegisterShader("console");
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers(void)
{
	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.q3_rendererStarted)
	{
		cls.q3_rendererStarted = true;
		CL_InitRenderer();
	}

	if (!cls.q3_soundStarted)
	{
		cls.q3_soundStarted = true;
		S_Init();
	}

	if (!cls.q3_soundRegistered)
	{
		cls.q3_soundRegistered = true;
		S_BeginRegistration();
	}

	if (!cls.q3_uiStarted)
	{
		cls.q3_uiStarted = true;
		CL_InitUI();
	}
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


//===========================================================================================


void CL_SetModel_f(void)
{
	char* arg;
	char name[256];

	arg = Cmd_Argv(1);
	if (arg[0])
	{
		Cvar_Set("model", arg);
		Cvar_Set("headmodel", arg);
	}
	else
	{
		Cvar_VariableStringBuffer("model", name, sizeof(name));
		common->Printf("model is set to %s\n", name);
	}
}

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

	CL_InitInput();

	//
	// register our variables
	//
	cl_motd = Cvar_Get("cl_motd", "1", 0);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	cl_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	cl_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
	cl_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);

	cl_timedemo = Cvar_Get("timedemo", "0", 0);
	cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);

	cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get("cg_autoswitch", "1", CVAR_ARCHIVE);

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);


	// userinfo
	Cvar_Get("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get("g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("teamtask", "0", CVAR_USERINFO);
	Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);


	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);
	Cmd_AddCommand("localservers", CL_LocalServers_f);
	Cmd_AddCommand("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("setenv", CL_Setenv_f);
	Cmd_AddCommand("ping", CL_Ping_f);
	Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
	Cmd_AddCommand("showip", CL_ShowIP_f);
	Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
	Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);
	Cmd_AddCommand("model", CL_SetModel_f);
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

	CL_ShutdownUI();

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

	Cvar_Set("cl_running", "0");

	recursive = false;

	Com_Memset(&cls, 0, sizeof(cls));

	common->Printf("-----------------------\n");

}

static void CL_SetServerInfo(q3serverInfo_t* server, const char* info, int ping)
{
	if (server)
	{
		if (info)
		{
			server->clients = String::Atoi(Info_ValueForKey(info, "clients"));
			String::NCpyZ(server->hostName,Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH_Q3);
			String::NCpyZ(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH_Q3);
			server->maxClients = String::Atoi(Info_ValueForKey(info, "sv_maxclients"));
			String::NCpyZ(server->game,Info_ValueForKey(info, "game"), MAX_NAME_LENGTH_Q3);
			server->gameType = String::Atoi(Info_ValueForKey(info, "gametype"));
			server->netType = String::Atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = String::Atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = String::Atoi(Info_ValueForKey(info, "maxping"));
			server->punkbuster = String::Atoi(Info_ValueForKey(info, "punkbuster"));
		}
		server->ping = ping;
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char* info, int ping)
{
	int i;

	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_localServers[i].adr))
		{
			CL_SetServerInfo(&cls.q3_localServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_mplayerServers[i].adr))
		{
			CL_SetServerInfo(&cls.q3_mplayerServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_GLOBAL_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_globalServers[i].adr))
		{
			CL_SetServerInfo(&cls.q3_globalServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_favoriteServers[i].adr))
		{
			CL_SetServerInfo(&cls.q3_favoriteServers[i], info, ping);
		}
	}

}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket(netadr_t from, QMsg* msg)
{
	int i, type;
	char info[MAX_INFO_STRING_Q3];
	const char* infoString;
	int prot;

	infoString = msg->ReadString();

	// if this isn't the correct protocol version, ignore it
	prot = String::Atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != PROTOCOL_VERSION)
	{
		common->DPrintf("Different protocol info packet: %s\n", infoString);
		return;
	}

	// iterate servers waiting for ping response
	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (cl_pinglist[i].adr.port && !cl_pinglist[i].time && SOCK_CompareAdr(from, cl_pinglist[i].adr))
		{
			// calc ping time
			cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;
			common->DPrintf("ping time %dms from %s\n", cl_pinglist[i].time, SOCK_AdrToString(from));

			// save of info
			String::NCpyZ(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch (from.type)
			{
			case NA_BROADCAST:
			case NA_IP:
				type = 1;
				break;

			default:
				type = 0;
				break;
			}
			Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type), MAX_INFO_STRING_Q3);
			CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if (cls.q3_pingUpdateSource != AS_LOCAL)
	{
		return;
	}

	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		// empty slot
		if (cls.q3_localServers[i].adr.port == 0)
		{
			break;
		}

		// avoid duplicate
		if (SOCK_CompareAdr(from, cls.q3_localServers[i].adr))
		{
			return;
		}
	}

	if (i == MAX_OTHER_SERVERS_Q3)
	{
		common->DPrintf("MAX_OTHER_SERVERS_Q3 hit, dropping infoResponse\n");
		return;
	}

	// add this to the list
	cls.q3_numlocalservers = i + 1;
	cls.q3_localServers[i].adr = from;
	cls.q3_localServers[i].clients = 0;
	cls.q3_localServers[i].hostName[0] = '\0';
	cls.q3_localServers[i].mapName[0] = '\0';
	cls.q3_localServers[i].maxClients = 0;
	cls.q3_localServers[i].maxPing = 0;
	cls.q3_localServers[i].minPing = 0;
	cls.q3_localServers[i].ping = -1;
	cls.q3_localServers[i].game[0] = '\0';
	cls.q3_localServers[i].gameType = 0;
	cls.q3_localServers[i].netType = from.type;
	cls.q3_localServers[i].punkbuster = 0;

	String::NCpyZ(info, msg->ReadString(), MAX_INFO_STRING_Q3);
	if (String::Length(info))
	{
		if (info[String::Length(info) - 1] != '\n')
		{
			String::Cat(info, sizeof(info), "\n");
		}
		common->Printf("%s: %s", SOCK_AdrToString(from), info);
	}
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t* CL_GetServerStatus(netadr_t from)
{
	serverStatus_t* serverStatus;
	int i, oldest, oldestTime;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (SOCK_CompareAdr(from, cl_serverStatusList[i].address))
		{
			return &cl_serverStatusList[i];
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (cl_serverStatusList[i].retrieved)
		{
			return &cl_serverStatusList[i];
		}
	}
	oldest = -1;
	oldestTime = 0;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime)
		{
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	if (oldest != -1)
	{
		return &cl_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS - 1)];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus(char* serverAddress, char* serverStatusString, int maxLen)
{
	int i;
	netadr_t to;
	serverStatus_t* serverStatus;

	// if no server address then reset all server status requests
	if (!serverAddress)
	{
		for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		{
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = true;
		}
		return false;
	}
	// get the address
	if (!SOCK_StringToAdr(serverAddress, &to, Q3PORT_SERVER))
	{
		return false;
	}
	serverStatus = CL_GetServerStatus(to);
	// if no server status string then reset the server status request for this address
	if (!serverStatusString)
	{
		serverStatus->retrieved = true;
		return false;
	}

	// if this server status request has the same address
	if (SOCK_CompareAdr(to, serverStatus->address))
	{
		// if we recieved an response for this server status request
		if (!serverStatus->pending)
		{
			String::NCpyZ(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = true;
			serverStatus->startTime = 0;
			return true;
		}
		// resend the request regularly
		else if (serverStatus->startTime < Com_Milliseconds() - cl_serverStatusResendTime->integer)
		{
			serverStatus->print = false;
			serverStatus->pending = true;
			serverStatus->retrieved = false;
			serverStatus->time = 0;
			serverStatus->startTime = Com_Milliseconds();
			NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
			return false;
		}
	}
	// if retrieved
	else if (serverStatus->retrieved)
	{
		serverStatus->address = to;
		serverStatus->print = false;
		serverStatus->pending = true;
		serverStatus->retrieved = false;
		serverStatus->startTime = Com_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
		return false;
	}
	return false;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse(netadr_t from, QMsg* msg)
{
	const char* s;
	char info[MAX_INFO_STRING_Q3];
	int i, l, score, ping;
	int len;
	serverStatus_t* serverStatus;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (SOCK_CompareAdr(from, cl_serverStatusList[i].address))
		{
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if (!serverStatus)
	{
		return;
	}

	s = msg->ReadStringLine();

	len = 0;
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

	if (serverStatus->print)
	{
		common->Printf("Server settings:\n");
		// print cvars
		while (*s)
		{
			for (i = 0; i < 2 && *s; i++)
			{
				if (*s == '\\')
				{
					s++;
				}
				l = 0;
				while (*s)
				{
					info[l++] = *s;
					if (l >= MAX_INFO_STRING_Q3 - 1)
					{
						break;
					}
					s++;
					if (*s == '\\')
					{
						break;
					}
				}
				info[l] = '\0';
				if (i)
				{
					common->Printf("%s\n", info);
				}
				else
				{
					common->Printf("%-24s", info);
				}
			}
		}
	}

	len = String::Length(serverStatus->string);
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	if (serverStatus->print)
	{
		common->Printf("\nPlayers:\n");
		common->Printf("num: score: ping: name:\n");
	}
	for (i = 0, s = msg->ReadStringLine(); *s; s = msg->ReadStringLine(), i++)
	{

		len = String::Length(serverStatus->string);
		String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

		if (serverStatus->print)
		{
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if (s)
			{
				s = strchr(s + 1, ' ');
			}
			if (s)
			{
				s++;
			}
			else
			{
				s = "unknown";
			}
			common->Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
		}
	}
	len = String::Length(serverStatus->string);
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	serverStatus->time = Com_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = false;
	if (serverStatus->print)
	{
		serverStatus->retrieved = true;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f(void)
{
	const char* message;
	int i, j;
	netadr_t to;

	common->Printf("Scanning for servers on the local network...\n");

	// reset the list, waiting for response
	cls.q3_numlocalservers = 0;
	cls.q3_pingUpdateSource = AS_LOCAL;

	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		qboolean b = cls.q3_localServers[i].visible;
		Com_Memset(&cls.q3_localServers[i], 0, sizeof(cls.q3_localServers[i]));
		cls.q3_localServers[i].visible = b;
	}
	Com_Memset(&to, 0, sizeof(to));

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for (i = 0; i < 2; i++)
	{
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for (j = 0; j < NUM_SERVER_PORTS; j++)
		{
			to.port = BigShort((short)(Q3PORT_SERVER + j));

			to.type = NA_BROADCAST;
			NET_SendPacket(NS_CLIENT, String::Length(message), message, to);
		}
	}
}

/*
==================
CL_GlobalServers_f
==================
*/
void CL_GlobalServers_f(void)
{
	netadr_t to;
	int i;
	int count;
	char* buffptr;
	char command[1024];

	if (Cmd_Argc() < 3)
	{
		common->Printf("usage: globalservers <master# 0-1> <protocol> [keywords]\n");
		return;
	}

	cls.q3_masterNum = String::Atoi(Cmd_Argv(1));

	common->Printf("Requesting servers from the master...\n");

	// reset the list, waiting for response
	// -1 is used to distinguish a "no response"

	if (cls.q3_masterNum == 1)
	{
		SOCK_StringToAdr(MASTER_SERVER_NAME, &to, PORT_MASTER);
		cls.q3_nummplayerservers = -1;
		cls.q3_pingUpdateSource = AS_MPLAYER;
	}
	else
	{
		SOCK_StringToAdr(MASTER_SERVER_NAME, &to, PORT_MASTER);
		cls.q3_numglobalservers = -1;
		cls.q3_pingUpdateSource = AS_GLOBAL;
	}
	to.type = NA_IP;

	sprintf(command, "getservers %s", Cmd_Argv(2));

	// tack on keywords
	buffptr = command + String::Length(command);
	count   = Cmd_Argc();
	for (i = 3; i < count; i++)
		buffptr += sprintf(buffptr, " %s", Cmd_Argv(i));

	NET_OutOfBandPrint(NS_SERVER, to, command);
}


/*
==================
CL_GetPing
==================
*/
void CL_GetPing(int n, char* buf, int buflen, int* pingtime)
{
	const char* str;
	int time;
	int maxPing;

	if (!cl_pinglist[n].adr.port)
	{
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = SOCK_AdrToString(cl_pinglist[n].adr);
	String::NCpyZ(buf, str, buflen);

	time = cl_pinglist[n].time;
	if (!time)
	{
		// check for timeout
		time = cls.realtime - cl_pinglist[n].start;
		maxPing = Cvar_VariableIntegerValue("cl_maxPing");
		if (maxPing < 100)
		{
			maxPing = 100;
		}
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

	*pingtime = time;
}

/*
==================
CL_UpdateServerInfo
==================
*/
void CL_UpdateServerInfo(int n)
{
	if (!cl_pinglist[n].adr.port)
	{
		return;
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo(int n, char* buf, int buflen)
{
	if (!cl_pinglist[n].adr.port)
	{
		// empty slot
		if (buflen)
		{
			buf[0] = '\0';
		}
		return;
	}

	String::NCpyZ(buf, cl_pinglist[n].info, buflen);
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing(int n)
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
	{
		return;
	}

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount(void)
{
	int i;
	int count;
	ping_t* pingptr;

	count   = 0;
	pingptr = cl_pinglist;

	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		if (pingptr->adr.port)
		{
			count++;
		}
	}

	return (count);
}

/*
==================
CL_GetFreePing
==================
*/
ping_t* CL_GetFreePing(void)
{
	ping_t* pingptr;
	ping_t* best;
	int oldest;
	int i;
	int time;

	pingptr = cl_pinglist;
	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (cls.realtime - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return (pingptr);
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best    = cl_pinglist;
	oldest  = INT_MIN;
	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// scan for oldest
		time = cls.realtime - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return (best);
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f(void)
{
	netadr_t to;
	ping_t* pingptr;
	char* server;

	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: ping [server]\n");
		return;
	}

	Com_Memset(&to, 0, sizeof(netadr_t));

	server = Cmd_Argv(1);

	if (!SOCK_StringToAdr(server, &to, Q3PORT_SERVER))
	{
		return;
	}

	pingptr = CL_GetFreePing();

	Com_Memcpy(&pingptr->adr, &to, sizeof(netadr_t));
	pingptr->start = cls.realtime;
	pingptr->time  = 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean CL_UpdateVisiblePings_f(int source)
{
	int slots, i;
	char buff[MAX_STRING_CHARS];
	int pingTime;
	int max;
	qboolean status = false;

	if (source < 0 || source > AS_FAVORITES)
	{
		return false;
	}

	cls.q3_pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if (slots < MAX_PINGREQUESTS)
	{
		q3serverInfo_t* server = NULL;

		max = (source == AS_GLOBAL) ? MAX_GLOBAL_SERVERS_Q3 : MAX_OTHER_SERVERS_Q3;
		switch (source)
		{
		case AS_LOCAL:
			server = &cls.q3_localServers[0];
			max = cls.q3_numlocalservers;
			break;
		case AS_MPLAYER:
			server = &cls.q3_mplayerServers[0];
			max = cls.q3_nummplayerservers;
			break;
		case AS_GLOBAL:
			server = &cls.q3_globalServers[0];
			max = cls.q3_numglobalservers;
			break;
		case AS_FAVORITES:
			server = &cls.q3_favoriteServers[0];
			max = cls.q3_numfavoriteservers;
			break;
		}
		for (i = 0; i < max; i++)
		{
			if (server[i].visible)
			{
				if (server[i].ping == -1)
				{
					int j;

					if (slots >= MAX_PINGREQUESTS)
					{
						break;
					}
					for (j = 0; j < MAX_PINGREQUESTS; j++)
					{
						if (!cl_pinglist[j].adr.port)
						{
							continue;
						}
						if (SOCK_CompareAdr(cl_pinglist[j].adr, server[i].adr))
						{
							// already on the list
							break;
						}
					}
					if (j >= MAX_PINGREQUESTS)
					{
						status = true;
						for (j = 0; j < MAX_PINGREQUESTS; j++)
						{
							if (!cl_pinglist[j].adr.port)
							{
								break;
							}
						}
						Com_Memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
						cl_pinglist[j].start = cls.realtime;
						cl_pinglist[j].time = 0;
						NET_OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx");
						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (server[i].ping == 0)
				{
					// if we are updating global servers
					if (source == AS_GLOBAL)
					{
						//
						if (cls.q3_numGlobalServerAddresses > 0)
						{
							// overwrite this server with one from the additional global servers
							cls.q3_numGlobalServerAddresses--;
							CL_InitServerInfo(&server[i], &cls.q3_globalServerAddresses[cls.q3_numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if (slots)
	{
		status = true;
	}
	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (!cl_pinglist[i].adr.port)
		{
			continue;
		}
		CL_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
		if (pingTime != 0)
		{
			CL_ClearPing(i);
			status = true;
		}
	}

	return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f(void)
{
	netadr_t to;
	char* server;
	serverStatus_t* serverStatus;

	Com_Memset(&to, 0, sizeof(netadr_t));

	if (Cmd_Argc() != 2)
	{
		if (cls.state != CA_ACTIVE || clc.demoplaying)
		{
			common->Printf("Not connected to a server.\n");
			common->Printf("Usage: serverstatus [server]\n");
			return;
		}
		server = cls.servername;
	}
	else
	{
		server = Cmd_Argv(1);
	}

	if (!SOCK_StringToAdr(server, &to, Q3PORT_SERVER))
	{
		return;
	}

	NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");

	serverStatus = CL_GetServerStatus(to);
	serverStatus->address = to;
	serverStatus->print = true;
	serverStatus->pending = true;
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

/*
=================
bool CL_CDKeyValidate
=================
*/
qboolean CL_CDKeyValidate(const char* key, const char* checksum)
{
	char ch;
	byte sum;
	char chs[3];
	int i, len;

	len = String::Length(key);
	if (len != CDKEY_LEN)
	{
		return false;
	}

	if (checksum && String::Length(checksum) != CDCHKSUM_LEN)
	{
		return false;
	}

	sum = 0;
	// for loop gets rid of conditional assignment warning
	for (i = 0; i < len; i++)
	{
		ch = *key++;
		if (ch >= 'a' && ch <= 'z')
		{
			ch -= 32;
		}
		switch (ch)
		{
		case '2':
		case '3':
		case '7':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'G':
		case 'H':
		case 'J':
		case 'L':
		case 'P':
		case 'R':
		case 'S':
		case 'T':
		case 'W':
			sum += ch;
			continue;
		default:
			return false;
		}
	}

	sprintf(chs, "%02x", sum);

	if (checksum && !String::ICmp(chs, checksum))
	{
		return true;
	}

	if (!checksum)
	{
		return true;
	}

	return false;
}

float* CL_GetSimOrg()
{
	return NULL;
}
