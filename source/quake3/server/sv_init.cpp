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
	char systemInfo[16384];
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
	svs.q3_snapshotEntities = (q3entityState_t*)Hunk_Alloc(sizeof(q3entityState_t) * svs.q3_numSnapshotEntities, h_high);
	svs.q3_nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.q3_snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set("nextmap", "map_restart 0");
//	Cvar_Set( "nextmap", va("map %s", server) );

	// wipe the entire per-level structure
	SVT3_ClearServer();
	for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
	{
		sv.q3_configstrings[i] = __CopyString("");
	}

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		sv.q3_entities[i] = new idQuake3Entity();
	}

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	srand(Com_Milliseconds());
	sv.q3_checksumFeed = (((int)rand() << 16) ^ rand()) ^ Com_Milliseconds();
	FS_Restart(sv.q3_checksumFeed);

	CM_LoadMap(va("maps/%s.bsp", server), qfalse, &checksum);

	// set serverinfo visible name
	Cvar_Set("mapname", server);

	Cvar_Set("sv_mapChecksum", va("%i",checksum));

	// serverid should be different each time
	sv.q3_serverId = com_frameTime;
	sv.q3_restartedServerId = sv.q3_serverId;	// I suppose the init here is just to be safe
	sv.q3_checksumFeedServerId = sv.q3_serverId;
	Cvar_Set("sv_serverid", va("%i", sv.q3_serverId));

	// clear physics interaction links
	SV_ClearWorld();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SVT3_InitGameProgs();

	// don't allow a map_restart if game is modified
	svt3_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		VM_Call(gvm, Q3GAME_RUN_FRAME, svs.q3_time);
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
				if (killBots)
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
			denied = (char*)VM_ExplicitArgPtr(gvm, VM_Call(gvm, Q3GAME_CLIENT_CONNECT, i, qfalse, isBot));		// firstTime = qfalse
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
					q3sharedEntity_t* ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SVQ3_GentityNum(i);
					ent->s.number = i;
					client->q3_gentity = ent;
					client->q3_entity = SVT3_EntityNum(i);

					client->q3_deltaMessage = -1;
					client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately

					VM_Call(gvm, Q3GAME_CLIENT_BEGIN, i);
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	VM_Call(gvm, Q3GAME_RUN_FRAME, svs.q3_time);
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

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if (com_dedicated->integer)
		{
			SVT3_TouchCGame();
		}
	}
	else
	{
		Cvar_Set("sv_paks", "");
		Cvar_Set("sv_pakNames", "");
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
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

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SVT3_Heartbeat_f();

	Hunk_SetMark();

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
	Cvar_Get("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svt3_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH2);
	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	sv_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_maxclients = Cvar_Get("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH2);

	sv_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);

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
	sv_timeout = Cvar_Get("sv_timeout", "200", CVAR_TEMP);
	sv_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	sv_allowDownload = Cvar_Get("sv_allowDownload", "0", CVAR_SERVERINFO);
	sv_master[0] = Cvar_Get("sv_master1", MASTER_SERVER_NAME, 0);
	sv_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	sv_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	sv_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	sv_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	sv_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);
	sv_strictAuth = Cvar_Get("sv_strictAuth", "1", CVAR_ARCHIVE);

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
	Com_Memset(&svs, 0, sizeof(svs));

	Cvar_Set("sv_running", "0");
	Cvar_Set("ui_singlePlayerActive", "0");

	Com_Printf("---------------------------\n");

	// disconnect any local clients
	CL_Disconnect(qfalse);
}
