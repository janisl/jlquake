/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_main.c  -- client main loop

#include "client.h"
#include "../../client/sound/local.h"
#include <limits.h>
#include "../../server/server.h"
#include "../../server/tech3/local.h"

#ifdef __linux__
#include <sys/stat.h>
#endif

Cvar* cl_wavefilerecord;
Cvar* cl_nodelta;

Cvar* cl_motd;
Cvar* cl_autoupdate;			// DHM - Nerve

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeout;
Cvar* cl_maxpackets;
Cvar* cl_packetdup;
Cvar* cl_timeNudge;
Cvar* cl_showTimeDelta;
Cvar* cl_freezeDemo;

Cvar* cl_shownuments;			// DHM - Nerve
Cvar* cl_visibleClients;		// DHM - Nerve
Cvar* cl_showSend;
Cvar* cl_showServerCommands;	// NERVE - SMF
Cvar* cl_timedemo;
Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_activeAction;

Cvar* cl_motdString;

Cvar* cl_allowDownload;

Cvar* cl_serverStatusResendTime;
Cvar* cl_trn;
Cvar* cl_missionStats;
Cvar* cl_waitForFire;

// DHM - Nerve :: Auto-Update
Cvar* cl_updateavailable;
Cvar* cl_updatefiles;
// DHM - Nerve

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

// DHM - Nerve :: Have we heard from the auto-update server this session?
qboolean autoupdateChecked;
qboolean autoupdateStarted;
// TTimo : moved from char* to array (was getting the char* from va(), broke on big downloads)
char autoupdateFilename[MAX_QPATH];
// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

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
not have future wmusercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char* cmd)
{
	int index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if (clc.q3_reliableSequence - clc.q3_reliableAcknowledge > MAX_RELIABLE_COMMANDS_WOLF)
	{
		common->Error("Client command overflow");
	}
	clc.q3_reliableSequence++;
	index = clc.q3_reliableSequence & (MAX_RELIABLE_COMMANDS_WOLF - 1);
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
	byte bufData[MAX_MSGLEN_WOLF];
	QMsg buf;
	int i;
	int len;
	wmentityState_t* ent;
	wmentityState_t nullstate;
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
	if (!Cvar_VariableValue("g_synchronousClients"))
	{
		common->Printf(S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		String::NCpyZ(demoName, s, sizeof(demoName));
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, WMPROTOCOL_VERSION);
	}
	else
	{
		int number;

		// scan for a free demo name
		for (number = 0; number <= 9999; number++)
		{
			CL_DemoFilename(number, demoName);
			String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, WMPROTOCOL_VERSION);

			len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;	// file doesn't exist
			}
		}
	}

	// open the demo file

#ifdef __MACOS__	//DAJ MacOS file typing
	{
		extern _MSL_IMP_EXP_C long _fcreator, _ftype;
		_ftype = 'WlfB';
		_fcreator = 'WlfM';
	}
#endif
	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;
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
	for (i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
	{
		if (!cl.wm_gameState.stringOffsets[i])
		{
			continue;
		}
		s = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[i];
		buf.WriteByte(q3svc_configstring);
		buf.WriteShort(i);
		buf.WriteBigString(s);
	}

	// baselines
	memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		ent = &cl.wm_entityBaselines[i];
		if (!ent->number)
		{
			continue;
		}
		buf.WriteByte(q3svc_baseline);
		MSGWM_WriteDeltaEntity(&buf, &nullstate, ent, true);
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

  Wave file saving functions

====================
*/

void CL_WriteWaveOpen()
{
	// we will just save it as a 16bit stereo 22050kz pcm file
	clc.wm_wavefile = FS_FOpenFileWrite("demodata.pcm");
	clc.wm_wavetime = -1;
}

void CL_WriteWaveClose()
{
	// and we're outta here
	FS_FCloseFile(clc.wm_wavefile);
}


void CL_WriteWaveFilePacket()
{
	int total, i;
	if (clc.wm_wavetime == -1)
	{
		clc.wm_wavetime = s_soundtime;
		return;
	}

	total = s_soundtime - clc.wm_wavetime;
	clc.wm_wavetime = s_soundtime;

	for (i = 0; i < total; i++)
	{
		int parm;
		short out;
		parm =  (paintbuffer[i].left) >> 8;
		if (parm > 32767)
		{
			parm = 32767;
		}
		if (parm < -32768)
		{
			parm = -32768;
		}
		out = parm;
		FS_Write(&out, 2, clc.wm_wavefile);
		parm =  (paintbuffer[i].right) >> 8;
		if (parm > 32767)
		{
			parm = 32767;
		}
		if (parm < -32768)
		{
			parm = -32768;
		}
		out = parm;
		FS_Write(&out, 2, clc.wm_wavefile);
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


//	CL_FlushMemory();	//----(SA)	MEM NOTE: in missionpack, this is moved to CL_DownloadsComplete


	// open the demo file
	arg = Cmd_Argv(1);
	String::Sprintf(extension, sizeof(extension), ".dm_%d", WMPROTOCOL_VERSION);
	if (!String::ICmp(arg + String::Length(arg) - String::Length(extension), extension))
	{
		String::Sprintf(name, sizeof(name), "demos/%s", arg);
	}
	else
	{
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", arg, WMPROTOCOL_VERSION);
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

	if (Cvar_VariableValue("cl_wavefilerecord"))
	{
		CL_WriteWaveOpen();
		clc.wm_waverecording = true;
	}

	String::NCpyZ(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

	// read demo messages until connected
	while (cls.state >= CA_CONNECTED && cls.state < CA_PRIMED)
	{
		CL_ReadDemoMessage();
		if (clc.wm_waverecording)
		{
			CL_WriteWaveFilePacket();
		}
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.q3_firstDemoFrameSkipped = false;
	if (clc.wm_waverecording)
	{
		CL_WriteWaveClose();
		clc.wm_waverecording = false;
	}
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
	CLT3_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	R_Shutdown(false);			// don't destroy window or context

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
		memset(&cl.wm_gameState, 0, sizeof(cl.wm_gameState));
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

	memset(&cl, 0, sizeof(cl));
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

	if (clc.download)
	{
		FS_FCloseFile(clc.download);
		clc.download = 0;
	}
	*clc.downloadTempName = *clc.downloadName = 0;
	Cvar_Set("cl_downloadName", "");

	autoupdateStarted = false;
	autoupdateFilename[0] = '\0';

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
	memset(&clc, 0, sizeof(clc));

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
	int i, j, l;
	Cvar* fs;

	if (!cls.q3_authorizeServer.port)
	{
		common->Printf("Resolving %s\n", WMAUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(WMAUTHORIZE_SERVER_NAME, &cls.q3_authorizeServer, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}

		common->Printf("%s resolved to %i.%i.%i.%i:%i\n", WMAUTHORIZE_SERVER_NAME,
			cls.q3_authorizeServer.ip[0], cls.q3_authorizeServer.ip[1],
			cls.q3_authorizeServer.ip[2], cls.q3_authorizeServer.ip[3],
			BigShort(cls.q3_authorizeServer.port));
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
		strcat(buffer, "=");

		for (i = 2; i < argc; i++)
		{
			strcat(buffer, Cmd_Argv(i));
			strcat(buffer, " ");
		}

#ifdef _WIN32
		_putenv(buffer);
#else
		putenv(buffer);
#endif
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

	S_StopAllSounds();		// NERVE - SMF

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
		SVT3_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SVT3_Frame(0);

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
		common->Printf("You must set 'rconpassword' before\n"
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
void CL_Vid_Restart_f(void)
{

	// RF, don't show percent bar, since the memory usage will just sit at the same level anyway
	Cvar_Set("com_expectedhunkusage", "-1");

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
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
	autoupdateChecked = false;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	CIN_CloseAllVideos();

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
CL_UI_Restart_f

Restart the ui subsystem
=================
*/
void CL_UI_Restart_f(void)				// NERVE - SMF
{	// shutdown the UI
	CL_ShutdownUI();

	autoupdateChecked = false;

	// init the UI
	CL_InitUI();
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

	for (i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
	{
		ofs = cl.wm_gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		common->Printf("%4i: %s\n", i, cl.wm_gameState.stringData + ofs);
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

#ifndef _WIN32
	const char* fs_write_path;
#endif
	char* fn;

	// DHM - Nerve :: Auto-update (not finished yet)
	if (autoupdateStarted)
	{

		if ((String::Length(autoupdateFilename) > 4))
		{
#ifdef _WIN32
			// win32's Sys_StartProcess prepends the current dir
			fn = va("%s/%s", FS_ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT), autoupdateFilename);
#else
			fs_write_path = Cvar_VariableString("fs_homepath");
			fn = FS_BuildOSPath(fs_write_path, FS_ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT), autoupdateFilename);
#ifdef __linux__
			Sys_Chmod(fn, S_IXUSR);
#endif
#endif
			Sys_StartProcess(fn, true);
		}

		autoupdateStarted = false;
		CL_Disconnect(true);
		return;
	}

	// if we downloaded files we need to restart the file system
	if (clc.downloadRestart)
	{
		clc.downloadRestart = false;

		FS_Restart(clc.q3_checksumFeed);	// We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand("donedl");

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	cls.state = CA_LOADING;

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
	char* dir = FS_ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT);

	if (autoupdateStarted && SOCK_CompareAdr(cls.wm_autoupdateServer, clc.q3_serverAddress))
	{
		if (String::Length(cl_updatefiles->string) > 4)
		{
			String::NCpyZ(autoupdateFilename, cl_updatefiles->string, sizeof(autoupdateFilename));
			String::NCpyZ(clc.downloadList, va("@%s/%s@%s/%s", dir, cl_updatefiles->string, dir, cl_updatefiles->string), MAX_INFO_STRING_Q3);
			cls.state = CA_CONNECTED;
			CL_NextDownload();
			return;
		}
	}
	else
	{
		// whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
		if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false))
		{
			Cvar_Set("com_missingFiles", missingfiles);
		}
		else
		{
			Cvar_Set("com_missingFiles", "");
		}

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
		Info_SetValueForKey(info, "protocol", va("%i", WMPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
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

		NET_OutOfBandData(NS_CLIENT, clc.q3_serverAddress, (byte*)&data[0], i + 10);
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

	// drop the connection
	message = CL_TranslateStringBuf("Server disconnected for unknown reason\n");
	common->Printf(message);
	Cvar_Set("com_errorMessage", message);
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
		Cvar_Set("com_errorMessage", clc.q3_serverMessage);
	}
	else if (!String::NICmp(s, "[err_prot]", 10))
	{
		String::NCpyZ(clc.q3_serverMessage, s + 10, sizeof(clc.q3_serverMessage));
		Cvar_Set("com_errorMessage", CL_TranslateStringBuf(PROTOCOL_MISMATCH_ERROR_LONG));
	}
	else
	{
		String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
	}
	common->Printf("%s", clc.q3_serverMessage);
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
	server->allowAnonymous = 0;
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
		max = MAX_GLOBAL_SERVERS_WM;
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
		if (cls.q3_numGlobalServerAddresses < MAX_GLOBAL_SERVERS_WM)
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

		// DHM - Nerve :: If we have completed a connection to the Auto-Update server...
		if (autoupdateChecked && SOCK_CompareAdr(cls.wm_autoupdateServer, clc.q3_serverAddress))
		{
			// Mark the client as being in the process of getting an update
			if (cl_updateavailable->integer)
			{
				autoupdateStarted = true;
			}
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
		CL_PrintPacket(from, msg);
		return;
	}

	// DHM - Nerve :: Auto-update server response message
	if (!String::ICmp(c, "updateResponse"))
	{
		CL_UpdateInfoPacket(from);
		return;
	}
	// DHM - Nerve

	// NERVE - SMF - bugfix, make this compare first n chars so it doesnt bail if token is parsed incorrectly
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
			Cvar_Set("com_errorMessage", CL_TranslateStringBuf("Server connection timed out."));
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

#ifdef __MACOS__	//DAJ MacOS file typing
	{
		extern _MSL_IMP_EXP_C long _fcreator, _ftype;
		_ftype = 'WlfB';
		_fcreator = 'WlfM';
	}
#endif
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

	// load character sets
	cls.charSetShader = R_RegisterShader("gfx/2d/hudchars");
	cls.whiteShader = R_RegisterShader("white");

// JPW NERVE

	cls.consoleShader = R_RegisterShader("console-16bit");	// JPW NERVE shader works with 16bit
	cls.consoleShader2 = R_RegisterShader("console2-16bit");	// JPW NERVE same
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

// DHM - Nerve
void CL_CheckAutoUpdate(void)
{
	int validServerNum = 0;
	int i = 0, rnd = 0;
	netadr_t temp;
	char* servername;

	if (!cl_autoupdate->integer)
	{
		return;
	}

	// Only check once per session
	if (autoupdateChecked)
	{
		return;
	}

	srand(Com_Milliseconds());

	// Find out how many update servers have valid DNS listings
	for (i = 0; i < MAX_AUTOUPDATE_SERVERS; i++)
	{
		if (SOCK_StringToAdr(cls.wm_autoupdateServerNames[i], &temp, Q3PORT_SERVER))
		{
			validServerNum++;
		}
	}

	// Pick a random server
	if (validServerNum > 1)
	{
		rnd = rand() % validServerNum;
	}
	else
	{
		rnd = 0;
	}

	servername = cls.wm_autoupdateServerNames[rnd];

	common->DPrintf("Resolving AutoUpdate Server... ");
	if (!SOCK_StringToAdr(servername, &cls.wm_autoupdateServer, Q3PORT_SERVER))
	{
		common->DPrintf("Couldn't resolve first address, trying default...");

		// Fall back to the first one
		if (!SOCK_StringToAdr(cls.wm_autoupdateServerNames[0], &cls.wm_autoupdateServer, Q3PORT_SERVER))
		{
			common->DPrintf("Failed to resolve any Auto-update servers.\n");
			autoupdateChecked = true;
			return;
		}
	}
	cls.wm_autoupdateServer.port = BigShort(Q3PORT_SERVER);
	common->DPrintf("%i.%i.%i.%i:%i\n", cls.wm_autoupdateServer.ip[0], cls.wm_autoupdateServer.ip[1],
		cls.wm_autoupdateServer.ip[2], cls.wm_autoupdateServer.ip[3],
		BigShort(cls.wm_autoupdateServer.port));

	NET_OutOfBandPrint(NS_CLIENT, cls.wm_autoupdateServer, "getUpdateInfo \"%s\" \"%s\"\n", Q3_VERSION, CPUSTRING);

	CL_RequestMotd();

	autoupdateChecked = true;
}

void CL_GetAutoUpdate(void)
{

	// Don't try and get an update if we haven't checked for one
	if (!autoupdateChecked)
	{
		return;
	}

	// Make sure there's a valid update file to request
	if (String::Length(cl_updatefiles->string) < 5)
	{
		return;
	}

	common->DPrintf("Connecting to auto-update server...\n");

	S_StopAllSounds();		// NERVE - SMF

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// clear any previous "server full" type messages
	clc.q3_serverMessage[0] = 0;

	if (com_sv_running->integer)
	{
		// if running a local server, kill it
		SVT3_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SVT3_Frame(0);

	CL_Disconnect(true);
	Con_Close();

	String::NCpyZ(cls.servername, "Auto-Updater", sizeof(cls.servername));

	if (cls.wm_autoupdateServer.type == NA_BAD)
	{
		common->Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}

	// Copy auto-update server address to Server connect address
	memcpy(&clc.q3_serverAddress, &cls.wm_autoupdateServer, sizeof(netadr_t));

	common->DPrintf("%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
		clc.q3_serverAddress.ip[0], clc.q3_serverAddress.ip[1],
		clc.q3_serverAddress.ip[2], clc.q3_serverAddress.ip[3],
		BigShort(clc.q3_serverAddress.port));

	cls.state = CA_CONNECTING;

	in_keyCatchers = 0;
	clc.q3_connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.q3_connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", "Auto-Updater");
}
// DHM - Nerve

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
void CL_startSingleplayer_f(void)
{
#if defined(__linux__)
	Sys_StartProcess("./wolfsp.x86", true);
#else
	Sys_StartProcess("WolfSP.exe", true);
#endif
}

// NERVE - SMF
void CL_buyNow_f(void)
{
	Sys_OpenURL("http://www.activision.com/games/wolfenstein/purchase.html", true);
}

// NERVE - SMF
void CL_singlePlayLink_f(void)
{
	Sys_OpenURL("http://www.activision.com/games/wolfenstein/home.html", true);
}

//===========================================================================================

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	common->Printf("----- Client Initialization -----\n");

	Con_Init();

	CL_ClearState();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput();

	CL_SharedInit();
	//
	// register our variables
	//
	cl_motd = Cvar_Get("cl_motd", "1", 0);
	cl_autoupdate = Cvar_Get("cl_autoupdate", "1", CVAR_ARCHIVE);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	cl_shownuments = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
	cl_visibleClients = Cvar_Get("cl_visibleClients", "0", CVAR_TEMP);
	cl_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
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

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// -NERVE - SMF - disabled autoswitch by default
	Cvar_Get("cg_autoswitch", "0", CVAR_ARCHIVE);

	// Rafael - particle switch
	Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE);
	// done

	cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

	// NERVE - SMF
	Cvar_Get("cg_drawCompass", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_descriptiveText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE);
	Cvar_Get("cg_uselessNostalgia", "0", CVAR_ARCHIVE);		// JPW NERVE
	Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_cursorHints", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
	Cvar_Get("cg_teamChatsOnly", "0", CVAR_ARCHIVE);
	Cvar_Get("cg_noVoiceChats", "0", CVAR_ARCHIVE);
	Cvar_Get("cg_noVoiceText", "0", CVAR_ARCHIVE);
	Cvar_Get("cg_crosshairSize", "48", CVAR_ARCHIVE);
	Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomstepsniper", "2", CVAR_ARCHIVE);

	Cvar_Get("mp_playerType", "0", 0);
	Cvar_Get("mp_currentPlayerType", "0", 0);
	Cvar_Get("mp_weapon", "0", 0);
	Cvar_Get("mp_team", "0", 0);
	Cvar_Get("mp_currentTeam", "0", 0);
	// -NERVE - SMF

	// userinfo
	Cvar_Get("name", "WolfPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE);			// NERVE - SMF - changed from 3000
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
//	Cvar_Get ("model", "american", CVAR_USERINFO | CVAR_ARCHIVE );	// temp until we have an skeletal american model
	Cvar_Get("model", "multi", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("head", "default", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("color", "4", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);

//----(SA) added
	Cvar_Get("cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE);
//----(SA) end

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE | CVAR_USERINFO);

	cl_missionStats = Cvar_Get("g_missionStats", "0", CVAR_ROM);
	cl_waitForFire = Cvar_Get("cl_waitForFire", "0", CVAR_ROM);

	// DHM - Nerve :: Auto-update
	cl_updateavailable = Cvar_Get("cl_updateavailable", "0", CVAR_ROM);
	cl_updatefiles = Cvar_Get("cl_updatefiles", "", CVAR_ROM);

	String::NCpyZ(cls.wm_autoupdateServerNames[0], AUTOUPDATE_SERVER1_NAME, MAX_QPATH);
	String::NCpyZ(cls.wm_autoupdateServerNames[1], AUTOUPDATE_SERVER2_NAME, MAX_QPATH);
	String::NCpyZ(cls.wm_autoupdateServerNames[2], AUTOUPDATE_SERVER3_NAME, MAX_QPATH);
	String::NCpyZ(cls.wm_autoupdateServerNames[3], AUTOUPDATE_SERVER4_NAME, MAX_QPATH);
	String::NCpyZ(cls.wm_autoupdateServerNames[4], AUTOUPDATE_SERVER5_NAME, MAX_QPATH);
	// DHM - Nerve

	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
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
	Cmd_AddCommand("localservers", CL_LocalServers_f);
	Cmd_AddCommand("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("setenv", CL_Setenv_f);
	Cmd_AddCommand("ping", CL_Ping_f);
	Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
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

	Cmd_AddCommand("startSingleplayer", CL_startSingleplayer_f);		// NERVE - SMF
	Cmd_AddCommand("buyNow", CL_buyNow_f);								// NERVE - SMF
	Cmd_AddCommand("singlePlayLink", CL_singlePlayLink_f);				// NERVE - SMF

	Cmd_AddCommand("setRecommended", CL_SetRecommended_f);

	CL_InitRef();

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	// DHM - Nerve
	autoupdateChecked = false;
	autoupdateStarted = false;

	CL_InitTranslation();	// NERVE - SMF - localization

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


static void CL_SetServerInfo(q3serverInfo_t* server, const char* info, int ping)
{
	if (server)
	{
		if (info)
		{
			server->clients = String::Atoi(Info_ValueForKey(info, "clients"));
			String::NCpyZ(server->hostName,Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH_WM);
			String::NCpyZ(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH_WM);
			server->maxClients = String::Atoi(Info_ValueForKey(info, "sv_maxclients"));
			String::NCpyZ(server->game,Info_ValueForKey(info, "game"), MAX_NAME_LENGTH_WM);
			server->gameType = String::Atoi(Info_ValueForKey(info, "gametype"));
			server->netType = String::Atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = String::Atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = String::Atoi(Info_ValueForKey(info, "maxping"));
			server->allowAnonymous = String::Atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
			server->friendlyFire = String::Atoi(Info_ValueForKey(info, "friendlyFire"));			// NERVE - SMF
			server->maxlives = String::Atoi(Info_ValueForKey(info, "maxlives"));					// NERVE - SMF
			server->tourney = String::Atoi(Info_ValueForKey(info, "tourney"));							// NERVE - SMF
			server->punkbuster = String::Atoi(Info_ValueForKey(info, "punkbuster"));				// DHM - Nerve
			String::NCpyZ(server->gameName, Info_ValueForKey(info, "gamename"), MAX_NAME_LENGTH_WM);		// Arnout
			server->antilag = String::Atoi(Info_ValueForKey(info, "g_antilag"));
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

	for (i = 0; i < MAX_GLOBAL_SERVERS_WM; i++)
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
	const char* str;
	const char* infoString;
	int prot;
	const char* gameName;

	infoString = msg->ReadString();

	// Arnout: if this isn't the correct game, ignore it
	gameName = Info_ValueForKey(infoString, "gamename");
	if (!gameName[0] || String::ICmp(gameName, WMGAMENAME_STRING))
	{
		common->DPrintf("Different game info packet: %s\n", infoString);
		return;
	}

	// if this isn't the correct protocol version, ignore it
	prot = String::Atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != WMPROTOCOL_VERSION)
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
				str = "udp";
				type = 1;
				break;

			default:
				str = "???";
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
	cls.q3_localServers[i].allowAnonymous = 0;
	cls.q3_localServers[i].friendlyFire = 0;			// NERVE - SMF
	cls.q3_localServers[i].maxlives = 0;				// NERVE - SMF
	cls.q3_localServers[i].tourney = 0;					// NERVE - SMF
	cls.q3_localServers[i].punkbuster = 0;				// DHM - Nerve
	cls.q3_localServers[i].gameName[0] = '\0';			// Arnout

	String::NCpyZ(info, msg->ReadString(), MAX_INFO_STRING_Q3);
	if (String::Length(info))
	{
		if (info[String::Length(info) - 1] != '\n')
		{
			strncat(info, "\n", sizeof(info));
		}
		common->Printf("%s: %s", SOCK_AdrToString(from), info);
	}
}

/*
===================
CL_UpdateInfoPacket
===================
*/
void CL_UpdateInfoPacket(netadr_t from)
{

	if (cls.wm_autoupdateServer.type == NA_BAD)
	{
		common->DPrintf("CL_UpdateInfoPacket:  Auto-Updater has bad address\n");
		return;
	}

	common->DPrintf("Auto-Updater resolved to %i.%i.%i.%i:%i\n",
		cls.wm_autoupdateServer.ip[0], cls.wm_autoupdateServer.ip[1],
		cls.wm_autoupdateServer.ip[2], cls.wm_autoupdateServer.ip[3],
		BigShort(cls.wm_autoupdateServer.port));

	if (!SOCK_CompareAdr(from, cls.wm_autoupdateServer))
	{
		common->DPrintf("CL_UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
			from.ip[0], from.ip[1], from.ip[2], from.ip[3],
			BigShort(from.port));
		return;
	}

	Cvar_Set("cl_updateavailable", Cmd_Argv(1));

	if (!String::ICmp(cl_updateavailable->string, "1"))
	{
		Cvar_Set("cl_updatefiles", Cmd_Argv(2));
		VM_Call(uivm, UI_SET_ACTIVE_MENU, WMUIMENU_WM_AUTOUPDATE);
	}
}
// DHM - Nerve

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
		else if (serverStatus->startTime < Sys_Milliseconds() - cl_serverStatusResendTime->integer)
		{
			serverStatus->print = false;
			serverStatus->pending = true;
			serverStatus->retrieved = false;
			serverStatus->time = 0;
			serverStatus->startTime = Sys_Milliseconds();
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
		serverStatus->startTime = Sys_Milliseconds();
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

	serverStatus->time = Sys_Milliseconds();
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
		SOCK_StringToAdr("master.quake3world.com", &to, Q3PORT_MASTER);
		cls.q3_nummplayerservers = -1;
		cls.q3_pingUpdateSource = AS_MPLAYER;
	}
	else
	{
		SOCK_StringToAdr(WMMASTER_SERVER_NAME, &to, Q3PORT_MASTER);
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

	memset(&to, 0, sizeof(netadr_t));

	server = Cmd_Argv(1);

	if (!SOCK_StringToAdr(server, &to, Q3PORT_SERVER))
	{
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy(&pingptr->adr, &to, sizeof(netadr_t));
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

		max = (source == AS_GLOBAL) ? MAX_GLOBAL_SERVERS_WM : MAX_OTHER_SERVERS_Q3;
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
						memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
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
			sum = (sum << 1) ^ ch;
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

// NERVE - SMF
/*
=======================
CL_AddToLimboChat

=======================
*/
void CL_AddToLimboChat(const char* str)
{
	int len;
	char* p, * ls;
	int lastcolor;
	int chatHeight;
	int i;

	chatHeight = LIMBOCHAT_HEIGHT_WA;
	cl.wa_limboChatPos = LIMBOCHAT_HEIGHT_WA - 1;
	len = 0;

	// copy old strings
	for (i = cl.wa_limboChatPos; i > 0; i--)
	{
		String::Cpy(cl.wa_limboChatMsgs[i], cl.wa_limboChatMsgs[i - 1]);
	}

	// copy new string
	p = cl.wa_limboChatMsgs[0];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str)
	{
		if (len > LIMBOCHAT_WIDTH_WA - 1)
		{
			break;
		}

		if (Q_IsColorString(str))
		{
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ')
		{
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;
}

/*
=======================
CL_GetLimboString

=======================
*/
qboolean CL_GetLimboString(int index, char* buf)
{
	if (index >= LIMBOCHAT_HEIGHT_WA)
	{
		return false;
	}

	String::NCpy(buf, cl.wa_limboChatMsgs[index], 140);
	return true;
}
// -NERVE - SMF

/*
=======================
CL_OpenURLForCvar
=======================
*/
void CL_OpenURL(const char* url)
{
	if (!url || !String::Length(url))
	{
		common->Printf(CL_TranslateStringBuf("invalid/empty URL\n"));
		return;
	}
	Sys_OpenURL(url, true);
}

float* CL_GetSimOrg()
{
	return NULL;
}
