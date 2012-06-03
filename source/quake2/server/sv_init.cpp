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

#include "server.h"

server_static_t svs;				// persistant server info
server_t sv;						// local server

/*
================
SV_FindIndex

================
*/
int SV_FindIndex(char* name, int start, int max, qboolean create)
{
	int i;

	if (!name || !name[0])
	{
		return 0;
	}

	for (i = 1; i < max && sv.configstrings[start + i][0]; i++)
		if (!String::Cmp(sv.configstrings[start + i], name))
		{
			return i;
		}

	if (!create)
	{
		return 0;
	}

	if (i == max)
	{
		Com_Error(ERR_DROP, "*Index: overflow");
	}

	String::NCpy(sv.configstrings[start + i], name, sizeof(sv.configstrings[i]));

	if (sv.state != ss_loading)
	{	// send the update to everyone
		sv.multicast.Clear();
		sv.multicast.WriteChar(q2svc_configstring);
		sv.multicast.WriteShort(start + i);
		sv.multicast.WriteString2(name);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}

	return i;
}


int SV_ModelIndex(char* name)
{
	return SV_FindIndex(name, Q2CS_MODELS, MAX_MODELS_Q2, true);
}

int SV_SoundIndex(char* name)
{
	return SV_FindIndex(name, Q2CS_SOUNDS, MAX_SOUNDS_Q2, true);
}

int SV_ImageIndex(char* name)
{
	return SV_FindIndex(name, Q2CS_IMAGES, MAX_IMAGES_Q2, true);
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline(void)
{
	q2edict_t* svent;
	int entnum;

	for (entnum = 1; entnum < ge->num_edicts; entnum++)
	{
		svent = EDICT_NUM(entnum);
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
		sv.baselines[entnum] = svent->s;
	}
}


/*
=================
SV_CheckForSavegame
=================
*/
void SV_CheckForSavegame(void)
{
	char name[MAX_OSPATH];
	int i;

	if (sv_noreload->value)
	{
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		return;
	}

	String::Sprintf(name, sizeof(name), "save/current/%s.sav", sv.name);
	if (!FS_FileExists(name))
	{
		return;		// no savegame

	}
	SV_ClearWorld();

	// get configstrings and areaportals
	SV_ReadLevelFile();

	if (!sv.loadgame)
	{	// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		server_state_t previousState;			// PGM

		previousState = sv.state;				// PGM
		sv.state = ss_loading;					// PGM
		for (i = 0; i < 100; i++)
			ge->RunFrame();

		sv.state = previousState;				// PGM
	}
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
void SV_SpawnServer(char* server, char* spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
{
	int i;
	int checksum;

	if (attractloop)
	{
		Cvar_SetLatched("paused", "0");
	}

	Com_Printf("------- Server Initialization -------\n");

	Com_DPrintf("SpawnServer: %s\n",server);
	if (sv.demofile)
	{
		FS_FCloseFile(sv.demofile);
	}

	svs.spawncount++;		// any partially connected client will be
							// restarted
	sv.state = ss_dead;
	Com_SetServerState(sv.state);

	// wipe the entire per-level structure
	Com_Memset(&sv, 0, sizeof(sv));
	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	// save name for levels that don't set message
	String::Cpy(sv.configstrings[Q2CS_NAME], server);
	if (Cvar_VariableValue("deathmatch"))
	{
		sprintf(sv.configstrings[Q2CS_AIRACCEL], "%g", sv_airaccelerate->value);
		pm_airaccelerate = sv_airaccelerate->value;
	}
	else
	{
		String::Cpy(sv.configstrings[Q2CS_AIRACCEL], "0");
		pm_airaccelerate = 0;
	}

	sv.multicast.InitOOB(sv.multicast_buf, sizeof(sv.multicast_buf));

	String::Cpy(sv.name, server);

	// leave slots at start for clients only
	for (i = 0; i < maxclients->value; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state > CS_CONNECTED)
		{
			svs.clients[i].state = CS_CONNECTED;
		}
		svs.clients[i].q2_lastframe = -1;
	}

	sv.time = 1000;

	String::Cpy(sv.name, server);
	String::Cpy(sv.configstrings[Q2CS_NAME], server);

	if (serverstate != ss_game)
	{
		CM_LoadMap("", false, &checksum);	// no real map
	}
	else
	{
		String::Sprintf(sv.configstrings[Q2CS_MODELS + 1],sizeof(sv.configstrings[Q2CS_MODELS + 1]),
			"maps/%s.bsp", server);
		CM_LoadMap(sv.configstrings[Q2CS_MODELS + 1], false, &checksum);
	}
	sv.models[1] = 0;
	String::Sprintf(sv.configstrings[Q2CS_MAPCHECKSUM],sizeof(sv.configstrings[Q2CS_MAPCHECKSUM]),
		"%i", (unsigned)checksum);

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		String::Sprintf(sv.configstrings[Q2CS_MODELS + 1 + i], sizeof(sv.configstrings[Q2CS_MODELS + 1 + i]),
			"*%i", i);
		sv.models[i + 1] = CM_InlineModel(i);
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState(sv.state);

	// load and spawn all other entities
	ge->SpawnEntities(sv.name, const_cast<char*>(CM_EntityString()), spawnpoint);

	// run two frames to allow everything to settle
	ge->RunFrame();
	ge->RunFrame();

	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState(sv.state);

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check for a savegame
	SV_CheckForSavegame();

	// set serverinfo variable
	Cvar_Get("mapname", "", CVAR_SERVERINFO | CVAR_INIT);
	Cvar_Set("mapname", sv.name);

	Com_Printf("-------------------------------------\n");
}

/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame(void)
{
	int i;
	q2edict_t* ent;

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown("Server restarted\n", true);
	}
	else
	{
		// make sure the client is down
		CL_Drop();
		SCR_BeginLoadingPlaque();
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars();

	svs.initialized = true;

	if (Cvar_VariableValue("coop") && Cvar_VariableValue("deathmatch"))
	{
		Com_Printf("Deathmatch and Coop both set, disabling Coop\n");
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
		if (maxclients->value <= 1)
		{
			Cvar_Set("maxclients", "8");
		}
		else if (maxclients->value > MAX_CLIENTS_Q2)
		{
			Cvar_Set("maxclients", va("%i", MAX_CLIENTS_Q2));
		}
	}
	else if (Cvar_VariableValue("coop"))
	{
		if (maxclients->value <= 1 || maxclients->value > 4)
		{
			Cvar_Set("maxclients", "4");
		}
	}
	else	// non-deathmatch, non-coop is one player
	{
		Cvar_Set("maxclients", "1");
	}

	svs.spawncount = rand();
	svs.clients = (client_t*)Z_Malloc(sizeof(client_t) * maxclients->value);
	svs.num_client_entities = maxclients->value * UPDATE_BACKUP_Q2 * 64;
	svs.client_entities = (q2entity_state_t*)Z_Malloc(sizeof(q2entity_state_t) * svs.num_client_entities);

	// init network stuff
	NET_Config((maxclients->value > 1));

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
	SOCK_StringToAdr("192.246.40.37", &master_adr[0], PORT_MASTER);

	// init game
	SV_InitGameProgs();
	for (i = 0; i < maxclients->value; i++)
	{
		ent = EDICT_NUM(i + 1);
		ent->s.number = i + 1;
		svs.clients[i].q2_edict = ent;
		Com_Memset(&svs.clients[i].q2_lastUsercmd, 0, sizeof(svs.clients[i].q2_lastUsercmd));
	}
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a.cin, .pcx, or .dm2 file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

    map tram.cin+jail_e3
======================
*/
void SV_Map(qboolean attractloop, char* levelstring, qboolean loadgame)
{
	char level[MAX_QPATH];
	char* ch;
	int l;
	char spawnpoint[MAX_QPATH];

	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	if (sv.state == ss_dead && !sv.loadgame)
	{
		SV_InitGame();	// the game is just starting

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
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SpawnServer(level, spawnpoint, ss_cinematic, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".dm2"))
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SpawnServer(level, spawnpoint, ss_demo, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".pcx"))
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SpawnServer(level, spawnpoint, ss_pic, attractloop, loadgame);
	}
	else
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SendClientMessages();
		SV_SpawnServer(level, spawnpoint, ss_game, attractloop, loadgame);
		Cbuf_CopyToDefer();
	}

	SV_BroadcastCommand("reconnect\n");
}
