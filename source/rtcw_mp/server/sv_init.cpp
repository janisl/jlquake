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

/*
 * name:		sv_init.c
 *
 * desc:
 *
*/

#include "server.h"

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer(char* server, qboolean killBots)
{
	int i;
	int checksum;
	qboolean isBot;
	char systemInfo[MAX_INFO_STRING_Q3];
	const char* p;

	// shut down the existing game if it is running
	SVT3_ShutdownGameProgs();

	Com_Printf("------ Server Initialization ------\n");
	Com_Printf("Server: %s\n",server);

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll();

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

	// clear collision map data		// (SA) NOTE: TODO: used in missionpack
	CM_ClearMap();

	// wipe the entire per-level structure
	SVT3_ClearServer();

	// MrE: main zone should be pretty much emtpy at this point
	// except for file system data and cached renderer data
	Z_LogHeap();

	// allocate empty config strings
	for (i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
	{
		sv.q3_configstrings[i] = __CopyString("");
	}

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		sv.q3_entities[i] = new idWolfMPEntity();
	}

	// init client structures and svs.q3_numSnapshotEntities
	if (!Cvar_VariableValue("sv_running"))
	{
		SVT3_Startup();
	}
	else
	{
		// check for maxclients change
		if (sv_maxclients->modified)
		{
			SVT3_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences(0);

	// allocate the snapshot entities on the hunk
	svs.wm_snapshotEntities = (wmentityState_t*)Hunk_Alloc(sizeof(wmentityState_t) * svs.q3_numSnapshotEntities, h_high);
	svs.q3_nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.q3_snapFlagServerBit ^= Q3SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set("nextmap", "map_restart 0");
//	Cvar_Set( "nextmap", va("map %s", server) );

	// just set it to a negative number,so the cgame knows not to draw the percent bar
	Cvar_Set("com_expectedhunkusage", "-1");

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

#if !defined(DO_LIGHT_DEDICATED)
	// get a new checksum feed and restart the file system
	srand(Sys_Milliseconds());
	sv.q3_checksumFeed = (((int)rand() << 16) ^ rand()) ^ Sys_Milliseconds();

	// DO_LIGHT_DEDICATED
	// only comment out when you need a new pure checksum string and it's associated random feed
	//Com_DPrintf("SV_SpawnServer checksum feed: %p\n", sv.q3_checksumFeed);

#else	// DO_LIGHT_DEDICATED implementation below
		// we are not able to randomize the checksum feed since the feed is used as key for pure_checksum computations
		// files.c 1776 : pack->pure_checksum = Com_BlockChecksumKey( fs_headerLongs, 4 * fs_numHeaderLongs, LittleLong(fs_checksumFeed) );
		// we request a fake randomized feed, files.c knows the answer
	srand(Sys_Milliseconds());
	sv.q3_checksumFeed = FS_RandChecksumFeed();
#endif
	FS_Restart(sv.q3_checksumFeed);

	CM_LoadMap(va("maps/%s.bsp", server), qfalse, &checksum);

	// set serverinfo visible name
	Cvar_Set("mapname", server);

	Cvar_Set("sv_mapChecksum", va("%i",checksum));

	// serverid should be different each time
	sv.q3_serverId = com_frameTime;
	sv.q3_restartedServerId = sv.q3_serverId;
	sv.q3_checksumFeedServerId = sv.q3_serverId;
	Cvar_Set("sv_serverid", va("%i", sv.q3_serverId));

	// clear physics interaction links
	SV_ClearWorld();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	Cvar_Set("sv_serverRestarting", "1");

	// load and spawn all other entities
	SVT3_InitGameProgs();

	// don't allow a map_restart if game is modified
	svt3_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		VM_Call(gvm, WMGAME_RUN_FRAME, svs.q3_time);
		SVT3_BotFrame(svs.q3_time);
		svs.q3_time += 100;
	}

	// create a baseline for more efficient communications
	SVT3_CreateBaseline();

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			char* denied;

			if (svs.clients[i].netchan.remoteAddress.type == NA_BOT)
			{
				if (killBots || Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
				{
					SVT3_DropClient(&svs.clients[i], "");
					continue;
				}
				isBot = qtrue;
			}
			else
			{
				isBot = qfalse;
			}

			// connect the client again
			denied = (char*)VM_ExplicitArgPtr(gvm, VM_Call(gvm, WMGAME_CLIENT_CONNECT, i, qfalse, isBot));		// firstTime = qfalse
			if (denied)
			{
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SVT3_DropClient(&svs.clients[i], denied);
			}
			else
			{
				if (!isBot)
				{
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else
				{
					client_t* client;
					wmsharedEntity_t* ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SVWM_GentityNum(i);
					ent->s.number = i;
					client->wm_gentity = ent;
					client->q3_entity = SVT3_EntityNum(i);

					client->q3_deltaMessage = -1;
					client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately

					VM_Call(gvm, WMGAME_CLIENT_BEGIN, i);
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	VM_Call(gvm, WMGAME_RUN_FRAME, svs.q3_time);
	SVT3_BotFrame(svs.q3_time);
	svs.q3_time += 100;

	if (svt3_pure->integer)
	{
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		p = FS_LoadedPakChecksums();
		Cvar_Set("sv_paks", p);
		if (String::Length(p) == 0)
		{
			Com_Printf("WARNING: sv_pure set but no PK3 files loaded\n");
		}
		p = FS_LoadedPakNames();
		Cvar_Set("sv_pakNames", p);
	}
	else
	{
		Cvar_Set("sv_paks", "");
		Cvar_Set("sv_pakNames", "");
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	// NOTE: we consider the referencedPaks as 'required for operation'

	// we want the server to reference the mp_bin pk3 that the client is expected to load from
	SVT3_TouchCGameDLL();

	p = FS_ReferencedPakChecksums();
	Cvar_Set("sv_referencedPaks", p);
	p = FS_ReferencedPakNames();
	Cvar_Set("sv_referencedPakNames", p);

	// save systeminfo and serverinfo strings
	String::NCpyZ(systemInfo, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING), sizeof(systemInfo));
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SVT3_SetConfigstring(Q3CS_SYSTEMINFO, systemInfo);

	SVT3_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3));
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// NERVE - SMF
	SVT3_SetConfigstring(WMCS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO, MAX_INFO_STRING_Q3));
	cvar_modifiedFlags &= ~CVAR_WOLFINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SVT3_Heartbeat_f();

	Hunk_SetMark();

	Cvar_Set("sv_serverRestarting", "0");

	Com_Printf("-----------------------------------\n");
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init(void)
{
	SV_AddOperatorCommands();

	// serverinfo vars
	Cvar_Get("dmflags", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("fraglimit", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	// DHM - Nerve :: default to WMGT_WOLF
	svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);

	// Rafael gameskill
	sv_gameskill = Cvar_Get("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2);
	// done

	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "WolfHost", CVAR_SERVERINFO | CVAR_ARCHIVE);
#ifdef __MACOS__
	sv_maxclients = Cvar_Get("sv_maxclients", "16", CVAR_SERVERINFO | CVAR_LATCH2);					//DAJ HOG
#else
	sv_maxclients = Cvar_Get("sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH2);					// NERVE - SMF - changed to 20 from 8
#endif

	svt3_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_allowAnonymous = Cvar_Get("sv_allowAnonymous", "0", CVAR_SERVERINFO);
	sv_friendlyFire = Cvar_Get("g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE);				// NERVE - SMF
	sv_maxlives = Cvar_Get("g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_SERVERINFO);		// NERVE - SMF
	sv_tourney = Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);									// NERVE - SMF

	// systeminfo
	Cvar_Get("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_serverid = Cvar_Get("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	svt3_pure = Cvar_Get("sv_pure", "1", CVAR_SYSTEMINFO);
	Cvar_Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	// server vars
	sv_rconPassword = Cvar_Get("rconPassword", "", CVAR_TEMP);
	sv_privatePassword = Cvar_Get("sv_privatePassword", "", CVAR_TEMP);
	sv_fps = Cvar_Get("sv_fps", "20", CVAR_TEMP);
	sv_timeout = Cvar_Get("sv_timeout", "240", CVAR_TEMP);
	sv_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	sv_allowDownload = Cvar_Get("sv_allowDownload", "1", CVAR_ARCHIVE);
	sv_master[0] = Cvar_Get("sv_master1", "wolfmaster.idsoftware.com", 0);			// NERVE - SMF - wolfMP master server
	sv_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	sv_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	sv_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	sv_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	sv_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get("sv_showloss", "0", 0);
	svt3_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);

	sv_onlyVisibleClients = Cvar_Get("sv_onlyVisibleClients", "0", 0);			// DHM - Nerve

	sv_showAverageBPS = Cvar_Get("sv_showAverageBPS", "0", 0);				// NERVE - SMF - net debugging

	// NERVE - SMF - create user set cvars
	Cvar_Get("g_userTimeLimit", "0", 0);
	Cvar_Get("g_userAlliedRespawnTime", "0", 0);
	Cvar_Get("g_userAxisRespawnTime", "0", 0);
	Cvar_Get("g_maxlives", "0", 0);
	Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);
	Cvar_Get("g_altStopwatchMode", "0", CVAR_ARCHIVE);
	Cvar_Get("g_minGameClients", "8", CVAR_SERVERINFO);
	Cvar_Get("g_complaintlimit", "3", CVAR_ARCHIVE);
	Cvar_Get("gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM);
	Cvar_Get("g_currentRound", "0", CVAR_WOLFINFO);
	Cvar_Get("g_nextTimeLimit", "0", CVAR_WOLFINFO);
	// -NERVE - SMF

	// TTimo - some UI additions
	// NOTE: sucks to have this hardcoded really, I suppose this should be in UI
	Cvar_Get("g_axismaxlives", "0", 0);
	Cvar_Get("g_alliedmaxlives", "0", 0);
	Cvar_Get("g_fastres", "0", CVAR_ARCHIVE);
	Cvar_Get("g_fastResMsec", "1000", CVAR_ARCHIVE);

	// ATVI Tracker Wolfenstein Misc #273
	Cvar_Get("g_voteFlags", "255", CVAR_ARCHIVE | CVAR_SERVERINFO);

	// ATVI Tracker Wolfenstein Misc #263
	Cvar_Get("g_antilag", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);

	// the download netcode tops at 18/20 kb/s, no need to make you think you can go above
	svt3_dl_maxRate = Cvar_Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE);

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage(const char* message)
{
	int i, j;
	client_t* cl;

	// send it twice, ignoring rate
	for (j = 0; j < 2; j++)
	{
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state >= CS_CONNECTED)
			{
				// don't send a disconnect to a local client
				if (cl->netchan.remoteAddress.type != NA_LOOPBACK)
				{
					SVT3_SendServerCommand(cl, "print \"%s\"", message);
					SVT3_SendServerCommand(cl, "disconnect");
				}
				// force a snapshot to be sent
				cl->q3_nextSnapshotTime = -1;
				SV_SendClientSnapshot(cl);
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown(const char* finalmsg)
{
	if (!com_sv_running || !com_sv_running->integer)
	{
		return;
	}

	Com_Printf("----- Server Shutdown -----\n");

	if (svs.clients && !com_errorEntered)
	{
		SV_FinalMessage(finalmsg);
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SVT3_ShutdownGameProgs();

	// free current level
	SVT3_ClearServer();

	// free server static data
	if (svs.clients)
	{
		Mem_Free(svs.clients);
	}
	memset(&svs, 0, sizeof(svs));

	Cvar_Set("sv_running", "0");

	Com_Printf("---------------------------\n");

	// disconnect any local clients
	CL_Disconnect(qfalse);
}
