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

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeNudge;
Cvar* cl_freezeDemo;

Cvar* cl_visibleClients;		// DHM - Nerve
Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_motdString;

Cvar* cl_wwwDownload;

Cvar* cl_trn;

Cvar* cl_defaultProfile;

Cvar* cl_demorecording;	// fretn
Cvar* cl_demofilename;	// bani
Cvar* cl_demooffset;	// bani

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

void CL_ShowIP_f(void);

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
	CLT3_StartHunkUsers();

#ifdef _WIN32
	Sys_In_Restart_f();	// fretn
#endif
	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.q3_cgameStarted = true;
		CLT3_InitCGame();
		// send pure checksums
		CLT3_SendPureChecksums();
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

//====================================================================

/*
=================
CLT3_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CLT3_PacketEvent(netadr_t from, QMsg* msg)
{
	int headerBytes;

	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		CLT3_ConnectionlessPacket(from, msg);
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
		// taken from CLT3_ParseDownload
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
			CLET_ClearStaticDownload();
			common->Error("%s", error);
		}
		else
		{
			// see CLT3_ParseDownload, same abort strategy
			common->Printf("Download failure while getting '%s'\n", cls.et_downloadName);
			CL_AddReliableCommand("wwwdl fail");
			clc.et_bWWWDlAborting = true;
		}
		return;
	}

	clc.et_bWWWDl = false;
	CLT3_NextDownload();
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
	CLT3_SendCmd();

	// resend a connection request if necessary
	CLT3_CheckForResend();

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
	clt3_motd = Cvar_Get("cl_motd", "1", 0);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	clwm_shownuments = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
	cl_visibleClients = Cvar_Get("cl_visibleClients", "0", CVAR_TEMP);
	clt3_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	clt3_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	clt3_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
	clt3_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);
	clet_autorecord = Cvar_Get("clet_autorecord", "0", CVAR_TEMP);

	cl_timedemo = Cvar_Get("timedemo", "0", 0);
	cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	clt3_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	clt3_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	clt3_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE);
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

	//
	// register our commands
	//
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("ui_restart", CL_UI_Restart_f);				// NERVE - SMF
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CLT3_Record_f);
	Cmd_AddCommand("demo", CLT3_PlayDemo_f);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CLT3_StopRecord_f);
	Cmd_AddCommand("connect", CLT3_Connect_f);
	Cmd_AddCommand("reconnect", CLT3_Reconnect_f);
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

	Cmd_AddCommand("wav_record", CLET_WavRecord_f);
	Cmd_AddCommand("wav_stoprecord", CLET_WavStopRecord_f);
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
		CLET_WavStopRecord_f();
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
