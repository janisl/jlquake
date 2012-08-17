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
void SVQ2_CreateBaseline()
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

void SVQ2_CheckForSavegame()
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
void SVQ2_SpawnServer(const char* server, const char* spawnpoint, serverState_t serverstate, bool attractloop, bool loadgame)
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
