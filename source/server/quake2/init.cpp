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

#include "../server.h"
#include "local.h"

static int SVQ2_FindIndex(const char* name, int start, int max)
{
	if (!name || !name[0])
	{
		return 0;
	}

	int i;
	for (i = 1; i < max && sv.q2_configstrings[start + i][0]; i++)
	{
		if (!String::Cmp(sv.q2_configstrings[start + i], name))
		{
			return i;
		}
	}

	if (i == max)
	{
		common->Error("*Index: overflow");
	}

	String::NCpy(sv.q2_configstrings[start + i], name, sizeof(sv.q2_configstrings[i]));

	if (sv.state != SS_LOADING)
	{
		// send the update to everyone
		sv.multicast.Clear();
		sv.multicast.WriteChar(q2svc_configstring);
		sv.multicast.WriteShort(start + i);
		sv.multicast.WriteString2(name);
		SVQ2_Multicast(vec3_origin, Q2MULTICAST_ALL_R);
	}

	return i;
}

int SVQ2_ModelIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_MODELS, MAX_MODELS_Q2);
}

int SVQ2_SoundIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_SOUNDS, MAX_SOUNDS_Q2);
}

int SVQ2_ImageIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_IMAGES, MAX_IMAGES_Q2);
}

//	Entity baselines are used to compress the update messages
// to the clients -- only the fields that differ from the
// baseline will be transmitted
static void SVQ2_CreateBaseline()
{
	for (int entnum = 1; entnum < ge->num_edicts; entnum++)
	{
		q2edict_t* svent = Q2_EDICT_NUM(entnum);
		if (!svent->inuse)
		{
			continue;
		}
		if (!svent->s.modelindex && !svent->s.sound && !svent->s.effects)
		{
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy(svent->s.origin, svent->s.old_origin);
		sv.q2_baselines[entnum] = svent->s;
	}
}

static void SVQ2_CheckForSavegame()
{
	if (svq2_noreload->value)
	{
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		return;
	}

	char name[MAX_OSPATH];
	String::Sprintf(name, sizeof(name), "save/current/%s.sav", sv.name);
	if (!FS_FileExists(name))
	{
		// no savegame
		return;

	}
	SV_ClearWorld();

	// get configstrings and areaportals
	SVQ2_ReadLevelFile();

	if (!sv.loadgame)
	{
		// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		serverState_t previousState = sv.state;

		sv.state = SS_LOADING;
		for (int i = 0; i < 100; i++)
		{
			ge->RunFrame();
		}

		sv.state = previousState;
	}
}

//	Change the server to a new map, taking all connected
// clients along with it.
static void SVQ2_SpawnServer(const char* server, const char* spawnpoint, serverState_t serverstate, bool attractloop, bool loadgame)
{
	if (attractloop)
	{
		Cvar_SetLatched("paused", "0");
	}

	common->Printf("------- Server Initialization -------\n");

	common->DPrintf("SpawnServer: %s\n",server);
	if (sv.q2_demofile)
	{
		FS_FCloseFile(sv.q2_demofile);
	}

	svs.spawncount++;		// any partially connected client will be
							// restarted
	sv.state = SS_DEAD;
	ComQ2_SetServerState(sv.state);

	// wipe the entire per-level structure
	Com_Memset(&sv, 0, sizeof(sv));
	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.q2_attractloop = attractloop;

	// save name for levels that don't set message
	String::Cpy(sv.q2_configstrings[Q2CS_NAME], server);
	if (Cvar_VariableValue("deathmatch"))
	{
		sprintf(sv.q2_configstrings[Q2CS_AIRACCEL], "%g", svq2_airaccelerate->value);
		pmq2_airaccelerate = svq2_airaccelerate->value;
	}
	else
	{
		String::Cpy(sv.q2_configstrings[Q2CS_AIRACCEL], "0");
		pmq2_airaccelerate = 0;
	}

	sv.multicast.InitOOB(sv.multicastBuffer, MAX_MSGLEN_Q2);

	String::Cpy(sv.name, server);

	// leave slots at start for clients only
	for (int i = 0; i < sv_maxclients->value; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state > CS_CONNECTED)
		{
			svs.clients[i].state = CS_CONNECTED;
		}
		svs.clients[i].q2_lastframe = -1;
	}

	sv.q2_time = 1000;

	String::Cpy(sv.name, server);
	String::Cpy(sv.q2_configstrings[Q2CS_NAME], server);

	int checksum;
	if (serverstate != SS_GAME)
	{
		CM_LoadMap("", false, &checksum);	// no real map
	}
	else
	{
		String::Sprintf(sv.q2_configstrings[Q2CS_MODELS + 1],sizeof(sv.q2_configstrings[Q2CS_MODELS + 1]),
			"maps/%s.bsp", server);
		CM_LoadMap(sv.q2_configstrings[Q2CS_MODELS + 1], false, &checksum);
	}
	sv.models[1] = 0;
	String::Sprintf(sv.q2_configstrings[Q2CS_MAPCHECKSUM],sizeof(sv.q2_configstrings[Q2CS_MAPCHECKSUM]),
		"%i", (unsigned)checksum);

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	for (int i = 1; i < CM_NumInlineModels(); i++)
	{
		String::Sprintf(sv.q2_configstrings[Q2CS_MODELS + 1 + i], sizeof(sv.q2_configstrings[Q2CS_MODELS + 1 + i]),
			"*%i", i);
		sv.models[i + 1] = CM_InlineModel(i);
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = SS_LOADING;
	ComQ2_SetServerState(sv.state);

	// load and spawn all other entities
	ge->SpawnEntities(sv.name, CM_EntityString(), spawnpoint);

	// run two frames to allow everything to settle
	ge->RunFrame();
	ge->RunFrame();

	// all precaches are complete
	sv.state = serverstate;
	ComQ2_SetServerState(sv.state);

	// create a baseline for more efficient communications
	SVQ2_CreateBaseline();

	// check for a savegame
	SVQ2_CheckForSavegame();

	// set serverinfo variable
	Cvar_Get("mapname", "", CVAR_SERVERINFO | CVAR_INIT);
	Cvar_Set("mapname", sv.name);

	common->Printf("-------------------------------------\n");
}

//	A brand new game has been started
void SVQ2_InitGame()
{
	int i;
	q2edict_t* ent;

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SVQ2_Shutdown("Server restarted\n", true);
	}
	else
	{
		// make sure the client is down
		CL_Drop();
		SCR_BeginLoadingPlaque(false);
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars();

	svs.initialized = true;

	if (Cvar_VariableValue("coop") && Cvar_VariableValue("deathmatch"))
	{
		common->Printf("Deathmatch and Coop both set, disabling Coop\n");
		Cvar_Set("coop", "0");
	}

	// dedicated servers are can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if (com_dedicated->value)
	{
		if (!Cvar_VariableValue("coop"))
		{
			Cvar_Set("deathmatch", "1");
		}
	}

	// init clients
	if (Cvar_VariableValue("deathmatch"))
	{
		if (sv_maxclients->value <= 1)
		{
			Cvar_Set("maxclients", "8");
		}
		else if (sv_maxclients->value > MAX_CLIENTS_Q2)
		{
			Cvar_Set("maxclients", va("%i", MAX_CLIENTS_Q2));
		}
	}
	else if (Cvar_VariableValue("coop"))
	{
		if (sv_maxclients->value <= 1 || sv_maxclients->value > 4)
		{
			Cvar_Set("maxclients", "4");
		}
	}
	else	// non-deathmatch, non-coop is one player
	{
		Cvar_Set("maxclients", "1");
	}

	svs.spawncount = rand();
	svs.clients = (client_t*)Mem_ClearedAlloc(sizeof(client_t) * sv_maxclients->value);
	svs.q2_num_client_entities = sv_maxclients->value * UPDATE_BACKUP_Q2 * 64;
	svs.q2_client_entities = (q2entity_state_t*)Mem_ClearedAlloc(sizeof(q2entity_state_t) * svs.q2_num_client_entities);

	// init network stuff
	NET_Config((sv_maxclients->value > 1));

	// heartbeats will always be sent to the id master
	svs.q2_last_heartbeat = -99999;		// send immediately
	SOCK_StringToAdr("192.246.40.37", &master_adr[0], Q2PORT_MASTER);

	// init game
	SVQ2_InitGameProgs();
	for (i = 0; i < sv_maxclients->value; i++)
	{
		ent = Q2_EDICT_NUM(i + 1);
		ent->s.number = i + 1;
		svs.clients[i].q2_edict = ent;
		Com_Memset(&svs.clients[i].q2_lastUsercmd, 0, sizeof(svs.clients[i].q2_lastUsercmd));
	}
}

//  the full syntax is:
//
//  map [*]<map>$<startspot>+<nextserver>
//
// command from the console or progs.
// Map can also be a.cin, .pcx, or .dm2 file
// Nextserver is used to allow a cinematic to play, then proceed to
// another level:
//
//    map tram.cin+jail_e3
void SVQ2_Map(bool attractloop, const char* levelstring, bool loadgame)
{
	char level[MAX_QPATH];
	char* ch;
	int l;
	char spawnpoint[MAX_QPATH];

	sv.loadgame = loadgame;
	sv.q2_attractloop = attractloop;

	if (sv.state == SS_DEAD && !sv.loadgame)
	{
		SVQ2_InitGame();	// the game is just starting

	}
	String::Cpy(level, levelstring);

	// if there is a + in the map, set nextserver to the remainder
	ch = strstr(level, "+");
	if (ch)
	{
		*ch = 0;
		Cvar_SetLatched("nextserver", va("gamemap \"%s\"", ch + 1));
	}
	else
	{
		Cvar_SetLatched("nextserver", "");
	}

	//ZOID special hack for end game screen in coop mode
	if (Cvar_VariableValue("coop") && !String::ICmp(level, "victory.pcx"))
	{
		Cvar_SetLatched("nextserver", "gamemap \"*base1\"");
	}

	// if there is a $, use the remainder as a spawnpoint
	ch = strstr(level, "$");
	if (ch)
	{
		*ch = 0;
		String::Cpy(spawnpoint, ch + 1);
	}
	else
	{
		spawnpoint[0] = 0;
	}

	// skip the end-of-unit flag if necessary
	if (level[0] == '*')
	{
		memmove(level, level + 1, String::Length(level));
	}

	l = String::Length(level);
	if (l > 4 && !String::Cmp(level + l - 4, ".cin"))
	{
		SCR_BeginLoadingPlaque(false);			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_CINEMATIC, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".dm2"))
	{
		SCR_BeginLoadingPlaque(false);			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_DEMO, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".pcx"))
	{
		SCR_BeginLoadingPlaque(false);			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_PIC, attractloop, loadgame);
	}
	else
	{
		SCR_BeginLoadingPlaque(false);			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SendClientMessages();
		SVQ2_SpawnServer(level, spawnpoint, SS_GAME, attractloop, loadgame);
		Cbuf_CopyToDefer();
	}

	SVQ2_BroadcastCommand("reconnect\n");
}
