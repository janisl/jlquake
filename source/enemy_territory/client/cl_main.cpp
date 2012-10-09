/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_main.c  -- client main loop

#include "client.h"
#include <limits.h>

#include "../../server/public.h"

Cvar* cl_wavefilerecord;
Cvar* cl_nodelta;

Cvar* cl_motd;

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeout;
Cvar* cl_maxpackets;
Cvar* cl_packetdup;
Cvar* cl_timeNudge;
Cvar* cl_freezeDemo;

Cvar* cl_visibleClients;		// DHM - Nerve
Cvar* cl_showSend;
Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_motdString;

Cvar* cl_allowDownload;
Cvar* cl_wwwDownload;

Cvar* cl_trn;
Cvar* cl_missionStats;
Cvar* cl_waitForFire;

Cvar* cl_defaultProfile;

Cvar* cl_demorecording;	// fretn
Cvar* cl_demofilename;	// bani
Cvar* cl_demooffset;	// bani

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

void CL_CheckForResend(void);
void CL_ShowIP_f(void);

// fretn
void CL_WavStopRecord_f(void);

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future etusercmd_t executed before it is executed
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

	// NOTE TTimo: what is the randomize for?
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
	Cvar_Set("cl_demorecording", "0");	// fretn
	Cvar_Set("cl_demofilename", "");	// bani
	Cvar_Set("cl_demooffset", "0");		// bani
	common->Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename(int number, char* fileName)
{
	if (number < 0 || number > 9999)
	{
		String::Sprintf(fileName, MAX_OSPATH, "demo9999");	// fretn - removed .tga
		return;
	}

	String::Sprintf(fileName, MAX_OSPATH, "demo%04i", number);
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
	int len;
	char* s;

	if (Cmd_Argc() > 2)
	{
		common->Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		common->Printf("Already recording.\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("You must be in a level to record.\n");
		return;
	}


	// ATVI Wolfenstein Misc #479 - changing this to a warning
	// sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	//if ( !Cvar_VariableValue( "g_synchronousClients" ) ) {
	//	common->Printf (S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	//}

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		String::NCpyZ(demoName, s, sizeof(demoName));
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, ETPROTOCOL_VERSION);
	}
	else
	{
		int number;

		// scan for a free demo name
		for (number = 0; number <= 9999; number++)
		{
			CL_DemoFilename(number, demoName);
			String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, ETPROTOCOL_VERSION);

			len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;	// file doesn't exist
			}
		}
	}

	CLT3_Record(demoName, name);
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

	// fretn
	if (clc.wm_waverecording)
	{
		CL_WriteWaveClose();
		clc.wm_waverecording = false;
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
	byte bufData[MAX_MSGLEN_WOLF];
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
		common->Error("CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN_WOLF");
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
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH], extension[32];
	char* arg;
	int prot_ver;

	if (Cmd_Argc() != 2)
	{
		common->Printf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(true);


//	CL_FlushMemory();	//----(SA)	MEM NOTE: in missionpack, this is moved to CL_DownloadsComplete


	// open the demo file
	arg = Cmd_Argv(1);
	prot_ver = ETPROTOCOL_VERSION - 1;
	while (prot_ver <= ETPROTOCOL_VERSION && !clc.demofile)
	{
		String::Sprintf(extension, sizeof(extension), ".dm_%d", prot_ver);
		if (!String::ICmp(arg + String::Length(arg) - String::Length(extension), extension))
		{
			String::Sprintf(name, sizeof(name), "demos/%s", arg);
		}
		else
		{
			String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", arg, prot_ver);
		}
		FS_FOpenFileRead(name, &clc.demofile, true);
		prot_ver++;
	}
	if (!clc.demofile)
	{
		common->Error("couldn't open %s", name);
		return;
	}
	String::NCpyZ(clc.q3_demoName, Cmd_Argv(1), sizeof(clc.q3_demoName));

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = true;

	if (Cvar_VariableValue("cl_wavefilerecord"))
	{
		CL_WriteWaveOpen();
	}

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
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void)
{

	// clear sounds
	S_DisableSounds();
	// download subsystem
	DL_Shutdown();
	// shutdown CGame
	CLT3_ShutdownCGame();
	// shutdown UI
	CLT3_ShutdownUI();

	// shutdown the renderer
	R_Shutdown(false);			// don't destroy window or context

	CLET_DoPurgeCache();

	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_rendererStarted = false;
	cls.q3_soundRegistered = false;

	// Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
	if (clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if (clc.wm_waverecording)
	{
		CL_WavStopRecord_f();
	}
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CLT3_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{

	// shutdown all the client stuff
	CL_ShutdownAll();

	CIN_CloseAllVideos();
	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear collision map data
		CM_ClearMap();
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
		memset(cls.q3_updateInfoString, 0, sizeof(cls.q3_updateInfoString));
		memset(clc.q3_serverMessage, 0, sizeof(clc.q3_serverMessage));
		memset(&cl.et_gameState, 0, sizeof(cl.et_gameState));
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
CL_ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void CL_ClearStaticDownload(void)
{
	assert(!cls.et_bWWWDlDisconnected);		// reset before calling
	cls.et_downloadRestart = false;
	cls.et_downloadTempName[0] = '\0';
	cls.et_downloadName[0] = '\0';
	cls.et_originalDownloadName[0] = '\0';
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit_f, so it shouldn't cause any errors
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

	if (!cls.et_bWWWDlDisconnected)
	{
		if (clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		*cls.et_downloadTempName = *cls.et_downloadName = 0;
		Cvar_Set("cl_downloadName", "");
	}

	if (clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

	if (showMainMenu)
	{
		UI_ForceMenuOff();
	}

	SCR_StopCinematic();
	S_ClearSoundBuffer(true);		//----(SA)	modified
#if 1
	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if (cls.state >= CA_CONNECTED)
	{
		CL_AddReliableCommand("disconnect");
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}
#endif
	CL_ClearState();

	// wipe the client connection
	Com_Memset(&clc, 0, sizeof(clc));

	if (!cls.et_bWWWDlDisconnected)
	{
		CL_ClearStaticDownload();
	}

	// allow cheats locally
	Cvar_Set("sv_cheats", "1");

	// not connected to a pure server anymore
	cl_connectedToPureServer = false;

	// show_bug.cgi?id=589
	// don't try a restart if uivm is NULL, as we might be in the middle of a restart already
	if (uivm && cls.state > CA_DISCONNECTED)
	{
		// restart the UI
		cls.state = CA_DISCONNECTED;

		// shutdown the UI
		CLT3_ShutdownUI();

		// init the UI
		CLT3_InitUI();
	}
	else
	{
		cls.state = CA_DISCONNECTED;
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
	common->Printf("Resolving %s\n", MOTD_SERVER_NAME);
	if (!SOCK_StringToAdr(MOTD_SERVER_NAME, &cls.q3_updateServer, PORT_MOTD))
	{
		common->Printf("Couldn't resolve address\n");
		return;
	}
	common->Printf("%s resolved to %i.%i.%i.%i:%i\n", MOTD_SERVER_NAME,
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

#ifdef AUTHORIZE_SUPPORT

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

	if (!cls.authorizeServer.port)
	{
		common->Printf("Resolving %s\n", ETAUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(ETAUTHORIZE_SERVER_NAME, &cls.authorizeServer, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}

		common->Printf("%s resolved to %i.%i.%i.%i:%i\n", ETAUTHORIZE_SERVER_NAME,
			cls.authorizeServer.ip[0], cls.authorizeServer.ip[1],
			cls.authorizeServer.ip[2], cls.authorizeServer.ip[3],
			BigShort(cls.authorizeServer.port));
	}
	if (cls.authorizeServer.type == NA_BAD)
	{
		return;
	}

	CLT3_CDKeyForAuthorize(nums);

	fs = Cvar_Get("cl_anonymous", "0", CVAR_INIT | CVAR_SYSTEMINFO);
	NET_OutOfBandPrint(NS_CLIENT, cls.authorizeServer, va("getKeyAuthorize %i %s", fs->integer, nums));
}
#endif	// AUTHORIZE_SUPPORT

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
			common->Printf("%s undefined\n", Cmd_Argv(1));
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
	char ip_port[MAX_STRING_CHARS];

	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: connect [server]\n");
		return;
	}

	S_StopAllSounds();		// NERVE - SMF

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");
	Cvar_Set("ui_connecting", "1");

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
		Cvar_Set("ui_connecting", "0");
		return;
	}
	if (clc.q3_serverAddress.port == 0)
	{
		clc.q3_serverAddress.port = BigShort(Q3PORT_SERVER);
	}

	String::NCpyZ(ip_port, SOCK_AdrToString(clc.q3_serverAddress), sizeof(ip_port));
	common->Printf("%s resolved to %s\n", cls.servername, ip_port);

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


	Cvar_Set("cl_avidemo", "0");

	// show_bug.cgi?id=507
	// prepare to catch a connection process that would turn bad
	Cvar_Set("com_errorDiagnoseIP", SOCK_AdrToString(clc.q3_serverAddress));
	// ATVI Wolfenstein Misc #439
	// we need to setup a correct default for this, otherwise the first val we set might reappear
	Cvar_Set("com_errorMessage", "");

	in_keyCatchers = 0;
	clc.q3_connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.q3_connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", server);
	Cvar_Set("cl_currentServerIP", ip_port);

	// Gordon: um, couldnt this be handled
	// NERVE - SMF - reset some cvars
	Cvar_Set("mp_playerType", "0");
	Cvar_Set("mp_currentPlayerType", "0");
	Cvar_Set("mp_weapon", "0");
	Cvar_Set("mp_team", "0");
	Cvar_Set("mp_currentTeam", "0");

	Cvar_Set("ui_limboOptions", "0");
	Cvar_Set("ui_limboPrevOptions", "0");
	Cvar_Set("ui_limboObjective", "0");
	// -NERVE - SMF

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
		common->Printf("You must set 'rconPassword' before\n"
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

	// ATVI Wolfenstein Misc #284
	strcat(message, Cmd_Cmd() + 5);

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
	String::Sprintf(cMsg, sizeof(cMsg), "Va ");
	String::Cat(cMsg, sizeof(cMsg), va("%d ", cl.q3_serverId));
	String::Cat(cMsg, sizeof(cMsg), pChecksums);
	for (i = 0; i < 2; i++)
	{
		cMsg[i] += 13 + (i * 2);
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

#ifdef _WIN32
extern void Sys_In_Restart_f(void);		// fretn
#endif

void CL_Vid_Restart_f(void)
{
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
	CL_StartHunkUsers();

#ifdef _WIN32
	Sys_In_Restart_f();	// fretn
#endif
	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.q3_cgameStarted = true;
		CLT3_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_UI_Restart_f

Restart the ui subsystem
=================
*/
void CL_UI_Restart_f(void)				// NERVE - SMF
{	// shutdown the UI
	CLT3_ShutdownUI();

	// init the UI
	CLT3_InitUI();
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

	for (i = 0; i < MAX_CONFIGSTRINGS_ET; i++)
	{
		ofs = cl.et_gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		common->Printf("%4i: %s\n", i, cl.et_gameState.stringData + ofs);
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

/*
==============
CL_EatMe_f

Eat misc console commands to prevent exploits
==============
*/
void CL_EatMe_f(void)
{
	//do nothing kthxbye
}

/*
==============
CL_WavRecord_f
==============
*/

void CL_WavRecord_f(void)
{
	if (clc.wm_wavefile)
	{
		common->Printf("Already recording a wav file\n");
		return;
	}

	CL_WriteWaveOpen();
}

/*
==============
CL_WavStopRecord_f
==============
*/

void CL_WavStopRecord_f(void)
{
	if (!clc.wm_wavefile)
	{
		common->Printf("Not recording a wav file\n");
		return;
	}

	CL_WriteWaveClose();
	Cvar_Set("cl_waverecording", "0");
	Cvar_Set("cl_wavefilename", "");
	Cvar_Set("cl_waveoffset", "0");
	clc.wm_waverecording = false;
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
	if (cls.et_downloadRestart)
	{
		cls.et_downloadRestart = false;

		FS_Restart(clc.q3_checksumFeed);	// We possibly downloaded a pak, restart the file system to load it

		if (!cls.et_bWWWDlDisconnected)
		{
			// inform the server so we get new gamestate info
			CL_AddReliableCommand("donedl");
		}
		// we can reset that now
		cls.et_bWWWDlDisconnected = false;
		CL_ClearStaticDownload();

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// TTimo: I wonder if that happens - it should not but I suspect it could happen if a download fails in the middle or is aborted
	assert(!cls.et_bWWWDlDisconnected);

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
	CLT3_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
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
		CLT3_BeginDownload(localName, remoteName);

		cls.et_downloadRestart = true;

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

	// TTimo
	// init some of the www dl data
	clc.et_bWWWDl = false;
	clc.et_bWWWDlAborting = false;
	cls.et_bWWWDlDisconnected = false;
	CL_ClearStaticDownload();

	// whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
	if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false))
	{
		Cvar_Set("com_missingFiles", missingfiles);
	}
	else
	{
		Cvar_Set("com_missingFiles", "");
	}

	// reset the redirect checksum tracking
	clc.et_redirectedList[0] = '\0';

	if (cl_allowDownload->integer && FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), true))
	{
		// this gets printed to UI, i18n
		common->Printf(CL_TranslateStringBuf("Need paks: %s\n"), clc.downloadList);

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
	char pkt[1024 + 1];		// EVEN BALANCE - T.RAY
	int pktlen;				// EVEN BALANCE - T.RAY

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
#ifdef AUTHORIZE_SUPPORT
		if (!SOCK_IsLANAddress(clc.q3_serverAddress))
		{
			CL_RequestAuthorization();
		}
#endif	// AUTHORIZE_SUPPORT

		// EVEN BALANCE - T.RAY
		String::Cpy(pkt, "getchallenge");
		pktlen = String::Length(pkt);
		NET_OutOfBandPrint(NS_CLIENT, clc.q3_serverAddress, pkt);
		break;

	case CA_CHALLENGING:
		// sending back the challenge
		port = Cvar_VariableValue("net_qport");

		String::NCpyZ(info, Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3), sizeof(info));
		Info_SetValueForKey(info, "protocol", va("%i", ETPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "qport", va("%i", port), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "challenge", va("%i", clc.q3_challenge), MAX_INFO_STRING_Q3);

		String::Cpy(data, "connect ");

		data[8] = '\"';				// NERVE - SMF - spaces in name bugfix

		for (i = 0; i < String::Length(info); i++)
		{
			data[9 + i] = info[i];		// + (clc.q3_challenge)&0x3;
		}
		data[9 + i] = '\"';		// NERVE - SMF - spaces in name bugfix
		data[10 + i] = 0;

		// EVEN BALANCE - T.RAY
		pktlen = i + 10;
		memcpy(pkt, &data[0], pktlen);

		NET_OutOfBandData(NS_CLIENT, clc.q3_serverAddress, (byte*)pkt, pktlen);
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		common->FatalError("CL_CheckForResend: bad cls.state");
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
	const char* message;

	if (cls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE TTimo:
	// there used to be a  clc.q3_lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionLessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.q3_lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if (cls.realtime - clc.q3_lastPacketTime < 3000)
	{
		return;
	}

	// if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
	if (!cls.et_bWWWDlDisconnected)
	{
		// drop the connection
		message = "Server disconnected for unknown reason";
		common->Printf("%s", message);
		Cvar_Set("com_errorMessage", message);
		CL_Disconnect(true);
	}
	else
	{
		CL_Disconnect(false);
		Cvar_Set("ui_connecting", "1");
		Cvar_Set("ui_dl_running", "1");
	}
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
CL_PrintPackets
an OOB message from server, with potential markups
print OOB are the only messages we handle markups in
[err_dialog]: used to indicate that the connection should be aborted
  no further information, just do an error diagnostic screen afterwards
[err_prot]: HACK. This is a protocol error. The client uses a custom
  protocol error message (client sided) in the diagnostic window.
  The space for the error message on the connection screen is limited
  to 256 chars.
===================
*/
void CL_PrintPacket(netadr_t from, QMsg* msg)
{
	const char* s;
	s = msg->ReadBigString();
	if (!String::NICmp(s, "[err_dialog]", 12))
	{
		String::NCpyZ(clc.q3_serverMessage, s + 12, sizeof(clc.q3_serverMessage));
		// Cvar_Set("com_errorMessage", clc.serverMessage );
		common->Error("%s", clc.q3_serverMessage);
	}
	else if (!String::NICmp(s, "[err_prot]", 10))
	{
		String::NCpyZ(clc.q3_serverMessage, s + 10, sizeof(clc.q3_serverMessage));
		common->Error("%s", CL_TranslateStringBuf(PROTOCOL_MISMATCH_ERROR_LONG));
	}
	else if (!String::NICmp(s, "[err_update]", 12))
	{
		String::NCpyZ(clc.q3_serverMessage, s + 12, sizeof(clc.q3_serverMessage));
		common->Error("%s", clc.q3_serverMessage);
	}
	else if (!String::NICmp(s, "ET://", 5))					// fretn
	{
		String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
		Cvar_Set("com_errorMessage", clc.q3_serverMessage);
		common->Error("%s", clc.q3_serverMessage);
	}
	else
	{
		String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
	}
	common->Printf("%s", clc.q3_serverMessage);
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
			if (Cmd_Argc() > 2)
			{
				clc.wm_onlyVisibleClients = String::Atoi(Cmd_Argv(2));				// DHM - Nerve
			}
			else
			{
				clc.wm_onlyVisibleClients = 0;
			}
			cls.state = CA_CHALLENGING;
			clc.q3_connectPacketCount = 0;
			clc.q3_connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.q3_serverAddress = from;
			common->DPrintf("challenge: %d\n", clc.q3_challenge);
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
		CL_PrintPacket(from, msg);
		return;
	}

	// NERVE - SMF - bugfix, make this compare first n chars so it doesnt bail if token is parsed incorrectly
	// echo request from server
	if (!String::NCmp(c, "getserversResponse", 18))
	{
		CLT3_ServersResponsePacket(from, msg);
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

	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		CL_ConnectionlessPacket(from, msg);
		return;
	}

	clc.q3_lastPacketTime = cls.realtime;

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
		if (++cl.timeoutcount > 5)			// timeoutcount saves debugger
		{
			Cvar_Set("com_errorMessage", "Server connection timed out.");
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
CL_WWWDownload
==================
*/
void CL_WWWDownload(void)
{
	char* to_ospath;
	dlStatus_t ret;
	static qboolean bAbort = false;

	if (clc.et_bWWWDlAborting)
	{
		if (!bAbort)
		{
			common->DPrintf("CL_WWWDownload: WWWDlAborting\n");
			bAbort = true;
		}
		return;
	}
	if (bAbort)
	{
		common->DPrintf("CL_WWWDownload: WWWDlAborting done\n");
		bAbort = false;
	}

	ret = DL_DownloadLoop();

	if (ret == DL_CONTINUE)
	{
		return;
	}

	if (ret == DL_DONE)
	{
		// taken from CL_ParseDownload
		// we work with OS paths
		clc.download = 0;
		to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), cls.et_originalDownloadName, "");
		to_ospath[String::Length(to_ospath) - 1] = '\0';
		if (rename(cls.et_downloadTempName, to_ospath))
		{
			FS_CopyFile(cls.et_downloadTempName, to_ospath);
			remove(cls.et_downloadTempName);
		}
		*cls.et_downloadTempName = *cls.et_downloadName = 0;
		Cvar_Set("cl_downloadName", "");
		if (cls.et_bWWWDlDisconnected)
		{
			// reconnect to the server, which might send us to a new disconnected download
			Cbuf_ExecuteText(EXEC_APPEND, "reconnect\n");
		}
		else
		{
			CL_AddReliableCommand("wwwdl done");
			// tracking potential web redirects leading us to wrong checksum - only works in connected mode
			if (String::Length(clc.et_redirectedList) + String::Length(cls.et_originalDownloadName) + 1 >= (int)sizeof(clc.et_redirectedList))
			{
				// just to be safe
				common->Printf("ERROR: redirectedList overflow (%s)\n", clc.et_redirectedList);
			}
			else
			{
				strcat(clc.et_redirectedList, "@");
				strcat(clc.et_redirectedList, cls.et_originalDownloadName);
			}
		}
	}
	else
	{
		if (cls.et_bWWWDlDisconnected)
		{
			// in a connected download, we'd tell the server about failure and wait for a reply
			// but in this case we can't get anything from server
			// if we just reconnect it's likely we'll get the same disconnected download message, and error out again
			// this may happen for a regular dl or an auto update
			const char* error = va("Download failure while getting '%s'\n", cls.et_downloadName);	// get the msg before clearing structs
			cls.et_bWWWDlDisconnected = false;	// need clearing structs before ERR_DROP, or it goes into endless reload
			CL_ClearStaticDownload();
			common->Error("%s", error);
		}
		else
		{
			// see CL_ParseDownload, same abort strategy
			common->Printf("Download failure while getting '%s'\n", cls.et_downloadName);
			CL_AddReliableCommand("wwwdl fail");
			clc.et_bWWWDlAborting = true;
		}
		return;
	}

	clc.et_bWWWDl = false;
	CL_NextDownload();
}

/*
==================
CL_WWWBadChecksum

FS code calls this when doing FS_ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool CL_WWWBadChecksum(const char* pakname)
{
	if (strstr(clc.et_redirectedList, va("@%s", pakname)))
	{
		common->Printf("WARNING: file %s obtained through download redirect has wrong checksum\n", pakname);
		common->Printf("         this likely means the server configuration is broken\n");
		if (String::Length(clc.et_badChecksumList) + String::Length(pakname) + 1 >= (int)sizeof(clc.et_badChecksumList))
		{
			common->Printf("ERROR: badChecksumList overflowed (%s)\n", clc.et_badChecksumList);
			return false;
		}
		strcat(clc.et_badChecksumList, "@");
		strcat(clc.et_badChecksumList, pakname);
		common->DPrintf("bad checksums: %s\n", clc.et_badChecksumList);
		return true;
	}
	return false;
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

	if (cls.state == CA_DISCONNECTED && !(in_keyCatchers & KEYCATCH_UI) &&
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

	// wwwdl download may survive a server disconnect
	if ((cls.state == CA_CONNECTED && clc.et_bWWWDl) || cls.et_bWWWDlDisconnected)
	{
		CL_WWWDownload();
	}

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update the sound
	S_Update();

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
CL_SetRecommended_f
================
*/
void CL_SetRecommended_f(void)
{
	Com_SetRecommended();
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
		CLT3_InitUI();
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

// RF, trap manual client damage commands so users can't issue them manually
void CL_ClientDamageCommand(void)
{
	// do nothing
}

// NERVE - SMF
/*void CL_startSingleplayer_f( void ) {
#if defined(__linux__)
    Sys_StartProcess( "./wolfsp.x86", true );
#else
    Sys_StartProcess( "WolfSP.exe", true );
#endif
}*/

// NERVE - SMF
// fretn unused
#if 0
void CL_buyNow_f(void)
{
	Sys_OpenURL("http://www.activision.com/games/wolfenstein/purchase.html", true);
}

// NERVE - SMF
void CL_singlePlayLink_f(void)
{
	Sys_OpenURL("http://www.activision.com/games/wolfenstein/home.html", true);
}
#endif

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

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	clwm_shownuments = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
	cl_visibleClients = Cvar_Get("cl_visibleClients", "0", CVAR_TEMP);
	clt3_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	cl_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	clt3_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
	clt3_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);
	clet_autorecord = Cvar_Get("clet_autorecord", "0", CVAR_TEMP);

	cl_timedemo = Cvar_Get("timedemo", "0", 0);
	cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE);
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE);

	clet_profile = Cvar_Get("cl_profile", "", CVAR_ROM);
	cl_defaultProfile = Cvar_Get("cl_defaultProfile", "", CVAR_ROM);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// -NERVE - SMF - disabled autoswitch by default
	Cvar_Get("cg_autoswitch", "0", CVAR_ARCHIVE);

	// Rafael - particle switch
	Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE);
	// done

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

	//bani - make these cvars visible to cgame
	cl_demorecording = Cvar_Get("cl_demorecording", "0", CVAR_ROM);
	cl_demofilename = Cvar_Get("cl_demofilename", "", CVAR_ROM);
	cl_demooffset = Cvar_Get("cl_demooffset", "0", CVAR_ROM);

	//bani
	cl_packetloss = Cvar_Get("cl_packetloss", "0", CVAR_CHEAT);
	cl_packetdelay = Cvar_Get("cl_packetdelay", "0", CVAR_CHEAT);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

	// NERVE - SMF
	Cvar_Get("cg_drawCompass", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_descriptiveText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE);
//	Cvar_Get( "cg_uselessNostalgia", "0", CVAR_ARCHIVE ); // JPW NERVE
	Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_cursorHints", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
//	Cvar_Get( "cg_teamChatsOnly", "0", CVAR_ARCHIVE );
//	Cvar_Get( "cg_noVoiceChats", "0", CVAR_ARCHIVE );
//	Cvar_Get( "cg_noVoiceText", "0", CVAR_ARCHIVE );
	Cvar_Get("cg_crosshairSize", "48", CVAR_ARCHIVE);
	Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomstepsniper", "2", CVAR_ARCHIVE);

//	Cvar_Get( "mp_playerType", "0", 0 );
//	Cvar_Get( "mp_currentPlayerType", "0", 0 );
//	Cvar_Get( "mp_weapon", "0", 0 );
//	Cvar_Get( "mp_team", "0", 0 );
//	Cvar_Get( "mp_currentTeam", "0", 0 );
	// -NERVE - SMF

	// userinfo
	Cvar_Get("name", "ETPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE);			// NERVE - SMF - changed from 3000
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
//	Cvar_Get ("model", "american", CVAR_USERINFO | CVAR_ARCHIVE );	// temp until we have an skeletal american model
//	Arnout - no need // Cvar_Get ("model", "multi", CVAR_USERINFO | CVAR_ARCHIVE );
//	Arnout - no need // Cvar_Get ("head", "default", CVAR_USERINFO | CVAR_ARCHIVE );
//	Arnout - no need // Cvar_Get ("color", "4", CVAR_USERINFO | CVAR_ARCHIVE );
//	Arnout - no need // Cvar_Get ("handicap", "0", CVAR_USERINFO | CVAR_ARCHIVE );
//	Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_ARCHIVE);

//----(SA) added
	Cvar_Get("cg_autoactivate", "1", CVAR_ARCHIVE);
//----(SA) end

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE);

	cl_missionStats = Cvar_Get("g_missionStats", "0", CVAR_ROM);
	cl_waitForFire = Cvar_Get("cl_waitForFire", "0", CVAR_ROM);

	//
	// register our commands
	//
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("ui_restart", CL_UI_Restart_f);				// NERVE - SMF
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f);
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
	// NERVE - SMF - don't do this in multiplayer
	// RF, add this command so clients can't bind a key to send client damage commands to the server
//	Cmd_AddCommand ("cld", CL_ClientDamageCommand );

//	Cmd_AddCommand ( "startSingleplayer", CL_startSingleplayer_f );		// NERVE - SMF
//	fretn - unused
//	Cmd_AddCommand ( "buyNow", CL_buyNow_f );							// NERVE - SMF
//	Cmd_AddCommand ( "singlePlayLink", CL_singlePlayLink_f );			// NERVE - SMF

	Cmd_AddCommand("setRecommended", CL_SetRecommended_f);

	//bani - we eat these commands to prevent exploits
	Cmd_AddCommand("userinfo", CL_EatMe_f);

	Cmd_AddCommand("wav_record", CL_WavRecord_f);
	Cmd_AddCommand("wav_stoprecord", CL_WavStopRecord_f);
	Cvar_Get("cl_waverecording", "0", CVAR_ROM);
	Cvar_Get("cl_wavefilename", "", CVAR_ROM);
	Cvar_Get("cl_waveoffset", "0", CVAR_ROM);

	CL_InitRef();

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	CL_InitTranslation();		// NERVE - SMF - localization

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

	if (clc.wm_waverecording)		// fretn - write wav header when we quit
	{
		CL_WavStopRecord_f();
	}

	CL_Disconnect(true);

	S_Shutdown();
	DL_Shutdown();
	CL_ShutdownRef();

	CLT3_ShutdownUI();

	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("snd_reload");
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
	Cmd_RemoveCommand("wav_record");
	Cmd_RemoveCommand("wav_stoprecord");
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
