/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// sv_main.c -- server main program

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

char localmodels[MAX_MODELS_Q1][5];				// inline model names for precache

//============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init(void)
{
	int i;

	SVQH_RegisterPhysicsCvars();
	svqh_idealpitchscale = Cvar_Get("sv_idealpitchscale", "0.8", 0);
	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);

	for (i = 0; i < MAX_MODELS_Q1; i++)
		sprintf(localmodels[i], "*%i", i);
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo(client_t* client)
{
	const char** s;

	SVQH_ClientPrintf(client, 0, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, VERSION, pr_crc);

	client->qh_message.WriteByte(q1svc_serverinfo);
	client->qh_message.WriteLong(PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		client->qh_message.WriteByte(GAME_DEATHMATCH);
	}
	else
	{
		client->qh_message.WriteByte(GAME_COOP);
	}

	client->qh_message.WriteString2(SVQ1_GetMapName());

	for (s = sv.qh_model_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

	for (s = sv.qh_sound_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

// send music
	client->qh_message.WriteByte(q1svc_cdtrack);
	client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
	client->qh_message.WriteByte(sv.qh_edicts->GetSounds());

// set view
	client->qh_message.WriteByte(q1svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(q1svc_signonnum);
	client->qh_message.WriteByte(1);

	client->qh_sendsignon = true;
	client->state = CS_CONNECTED;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient(int clientnum)
{
	qhedict_t* ent;
	client_t* client;
	int edictnum;
	qsocket_t* netconnection;
	int i;
	float spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients + clientnum;

	common->DPrintf("Client %s connected\n", client->qh_netconnection->address);

	edictnum = clientnum + 1;

	ent = QH_EDICT_NUM(edictnum);

// set up the client_t
	netconnection = client->qh_netconnection;
	netchan_t netchan = client->netchan;

	if (sv.loadgame)
	{
		Com_Memcpy(spawn_parms, client->qh_spawn_parms, sizeof(spawn_parms));
	}
	Com_Memset(client, 0, sizeof(*client));
	client->qh_netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->state = CS_CONNECTED;
	client->qh_edict = ent;
	client->qh_message.InitOOB(client->qh_messageBuffer, MAX_MSGLEN_Q1);
	client->qh_message.allowoverflow = true;		// we can catch it

	if (sv.loadgame)
	{
		Com_Memcpy(client->qh_spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
	else
	{
		// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram(*pr_globalVars.SetNewParms);
		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			client->qh_spawn_parms[i] = pr_globalVars.parm1[i];
	}

	SV_SendServerinfo(client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients(void)
{
	qsocket_t* ret;
	int i;

//
// check for new connections
//
	while (1)
	{
		netadr_t addr;
		ret = NET_CheckNewConnections(&addr);
		if (!ret)
		{
			break;
		}

		//
		// init a new client structure
		//
		for (i = 0; i < svs.qh_maxclients; i++)
			if (svs.clients[i].state == CS_FREE)
			{
				break;
			}
		if (i == svs.qh_maxclients)
		{
			common->FatalError("Host_CheckForNewClients: no free clients");
		}

		Netchan_Setup(NS_SERVER, &svs.clients[i].netchan, addr, 0);
		svs.clients[i].netchan.lastReceived = net_time * 1000;
		svs.clients[i].qh_netconnection = ret;
		SV_ConnectClient(i);

		net_activeconnections++;
	}
}

/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline(void)
{
	int i;
	qhedict_t* svent;
	int entnum;

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		// get the current server version
		svent = QH_EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		if (entnum > svs.qh_maxclients && !svent->v.modelindex)
		{
			continue;
		}

		//
		// create entity baseline
		//
		VectorCopy(svent->GetOrigin(), svent->q1_baseline.origin);
		VectorCopy(svent->GetAngles(), svent->q1_baseline.angles);
		svent->q1_baseline.frame = svent->GetFrame();
		svent->q1_baseline.skinnum = svent->GetSkin();
		if (entnum > 0 && entnum <= svs.qh_maxclients)
		{
			svent->q1_baseline.colormap = entnum;
			svent->q1_baseline.modelindex = SVQH_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->q1_baseline.colormap = 0;
			svent->q1_baseline.modelindex =
				SVQH_ModelIndex(PR_GetString(svent->GetModel()));
		}

		//
		// add to the message
		//
		sv.qh_signon.WriteByte(q1svc_spawnbaseline);
		sv.qh_signon.WriteShort(entnum);

		sv.qh_signon.WriteByte(svent->q1_baseline.modelindex);
		sv.qh_signon.WriteByte(svent->q1_baseline.frame);
		sv.qh_signon.WriteByte(svent->q1_baseline.colormap);
		sv.qh_signon.WriteByte(svent->q1_baseline.skinnum);
		for (i = 0; i < 3; i++)
		{
			sv.qh_signon.WriteCoord(svent->q1_baseline.origin[i]);
			sv.qh_signon.WriteAngle(svent->q1_baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect(void)
{
	byte data[128];
	QMsg msg;

	msg.InitOOB(data, sizeof(data));

	msg.WriteChar(q1svc_stufftext);
	msg.WriteString2("reconnect\n");
	NET_SendToAll(&msg, 5);

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		Cmd_ExecuteString("reconnect\n");
	}
#endif
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms(void)
{
	int i, j;

	svs.qh_serverflags = *pr_globalVars.serverflags;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		// call the progs to get default spawn parms for the new client
		*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.SetChangeParms);
		for (j = 0; j < NUM_SPAWN_PARMS; j++)
			host_client->qh_spawn_parms[j] = pr_globalVars.parm1[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
#ifndef DEDICATED
extern float scr_centertime_off;
#endif

void SV_SpawnServer(char* server)
{
	qhedict_t* ent;
	int i;

	// let's not have any servers with no name
	if (!sv_hostname || sv_hostname->string[0] == 0)
	{
		Cvar_Set("hostname", "UNNAMED");
	}
#ifndef DEDICATED
	scr_centertime_off = 0;
#endif

	common->DPrintf("SpawnServer: %s\n",server);
	svs.qh_changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (sv.state != SS_DEAD)
	{
		SV_SendReconnect();
	}

//
// make cvars consistant
//
	if (svqh_coop->value)
	{
		Cvar_SetValue("deathmatch", 0);
	}
	svqh_current_skill = (int)(qh_skill->value + 0.5);
	if (svqh_current_skill < 0)
	{
		svqh_current_skill = 0;
	}
	if (svqh_current_skill > 3)
	{
		svqh_current_skill = 3;
	}

	Cvar_SetValue("skill", (float)svqh_current_skill);

//
// set up the new server
//
	Host_ClearMemory();

	Com_Memset(&sv, 0, sizeof(sv));

	String::Cpy(sv.name, server);

// load progs to get entity field count
	PR_LoadProgs();

// allocate server memory
	sv.qh_edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_QH * pr_edict_size, "edicts");

	sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, MAX_DATAGRAM_QH);

	sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, MAX_DATAGRAM_QH);

	sv.qh_signon.InitOOB(sv.qh_signonBuffer, MAX_MSGLEN_Q1);

// leave slots at start for clients only
	sv.qh_num_edicts = svs.qh_maxclients + 1;
	for (i = 0; i < svs.qh_maxclients; i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
	}

	sv.state = SS_LOADING;
	sv.qh_paused = false;

	sv.qh_time = 1.0;

	String::Cpy(sv.name, server);
	sprintf(sv.qh_modelname,"maps/%s.bsp", server);
	CM_LoadMap(sv.qh_modelname, false, NULL);
	sv.models[1] = 0;

//
// clear world interaction links
//
	SV_ClearWorld();

	sv.qh_sound_precache[0] = PR_GetString(0);

	sv.qh_model_precache[0] = PR_GetString(0);
	sv.qh_model_precache[1] = sv.qh_modelname;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.qh_model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

//
// load the rest of the entities
//
	ent = QH_EDICT_NUM(0);
	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.qh_modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(QHSOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (svqh_coop->value)
	{
		*pr_globalVars.coop = svqh_coop->value;
	}
	else
	{
		*pr_globalVars.deathmatch = svqh_deathmatch->value;
	}

	*pr_globalVars.mapname = PR_SetString(sv.name);

// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

	ED_LoadFromFile(CM_EntityString());

// all setup is completed, any further precache statements are errors
	sv.state = SS_GAME;

	SVQH_SetMoveVars();

	// run two frames to allow everything to settle
	host_frametime = 0.1;
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);

// create a baseline for more efficient communications
	SV_CreateBaseline();

// send serverinfo to all connected clients
	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SV_SendServerinfo(host_client);
		}

	common->DPrintf("Server spawned.\n");
}
