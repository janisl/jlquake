// sv_main.c -- server main program

/*
 * $Header: /H2 Mission Pack/SV_MAIN.C 26    3/20/98 12:12a Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

char localmodels[MAX_MODELS_H2][5];				// inline model names for precache

Cvar* sv_sound_distance;

Cvar* sv_idealrollscale;

int sv_kingofhill;
qboolean skip_start = false;
int num_intro_msg = 0;

//============================================================================

void Sv_Edicts_f(void);

//JL Used in progs
Cvar* sv_walkpitch;

/*
===============
SV_Init
===============
*/
void SV_Init(void)
{
	int i;

	SVQH_RegisterPhysicsCvars();
	svqh_idealpitchscale = Cvar_Get("sv_idealpitchscale","0.8", 0);
	sv_idealrollscale = Cvar_Get("sv_idealrollscale","0.8", 0);
	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);
	sv_walkpitch = Cvar_Get("sv_walkpitch", "0", 0);
	sv_sound_distance = Cvar_Get("sv_sound_distance","800", CVAR_ARCHIVE);
	svh2_update_player    = Cvar_Get("sv_update_player","1", CVAR_ARCHIVE);
	svh2_update_monsters  = Cvar_Get("sv_update_monsters","1", CVAR_ARCHIVE);
	svh2_update_missiles  = Cvar_Get("sv_update_missiles","1", CVAR_ARCHIVE);
	svh2_update_misc      = Cvar_Get("sv_update_misc","1", CVAR_ARCHIVE);
	sv_ce_scale         = Cvar_Get("sv_ce_scale","0", CVAR_ARCHIVE);
	sv_ce_max_size      = Cvar_Get("sv_ce_max_size","0", CVAR_ARCHIVE);

	Cmd_AddCommand("sv_edicts", Sv_Edicts_f);

	for (i = 0; i < MAX_MODELS_H2; i++)
		sprintf(localmodels[i], "*%i", i);

	sv_kingofhill = 0;	//Initialize King of Hill to world
}

void SV_Edicts(const char* Name)
{
	fileHandle_t FH;
	int i;
	qhedict_t* e;

	FH = FS_FOpenFileWrite(Name);
	if (!FH)
	{
		common->Printf("Could not open %s\n",Name);
		return;
	}

	FS_Printf(FH,"Number of Edicts: %d\n",sv.qh_num_edicts);
	FS_Printf(FH,"Server Time: %f\n",sv.qh_time);
	FS_Printf(FH,"\n");
	FS_Printf(FH,"Num.     Time Class Name                     Model                          Think                                    Touch                                    Use\n");
	FS_Printf(FH,"---- -------- ------------------------------ ------------------------------ ---------------------------------------- ---------------------------------------- ----------------------------------------\n");

	for (i = 1; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		FS_Printf(FH,"%3d. %8.2f %-30s %-30s %-40s %-40s %-40s\n",
			i,e->GetNextThink(),PR_GetString(e->GetClassName()),PR_GetString(e->GetModel()),
			PR_GetString(pr_functions[e->GetThink()].s_name),PR_GetString(pr_functions[e->GetTouch()].s_name),
			PR_GetString(pr_functions[e->GetUse()].s_name));
	}
	FS_FCloseFile(FH);
}

void Sv_Edicts_f(void)
{
	const char* Name;

	if (sv.state == SS_DEAD)
	{
		common->Printf("This command can only be executed on a server running a map\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Name = "edicts.txt";
	}
	else
	{
		Name = Cmd_Argv(1);
	}

	SV_Edicts(Name);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

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

	SVQH_ClientPrintf(client, 0, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, HEXEN2_VERSION, pr_crc);

	client->qh_message.WriteByte(h2svc_serverinfo);
	client->qh_message.WriteLong(PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		client->qh_message.WriteByte(GAME_DEATHMATCH);
		client->qh_message.WriteShort(sv_kingofhill);
	}
	else
	{
		client->qh_message.WriteByte(GAME_COOP);
	}

	client->qh_message.WriteString2(SVH2_GetMapName());

	for (s = sv.qh_model_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

	for (s = sv.qh_sound_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

// send music
	client->qh_message.WriteByte(h2svc_cdtrack);
//	client->message.WriteByte(sv.qh_edicts->v.soundtype);
//	client->message.WriteByte(sv.qh_edicts->v.soundtype);
	client->qh_message.WriteByte(sv.h2_cd_track);
	client->qh_message.WriteByte(sv.h2_cd_track);

	client->qh_message.WriteByte(h2svc_midi_name);
	client->qh_message.WriteString2(sv.h2_midi_name);

// set view
	client->qh_message.WriteByte(h2svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(h2svc_signonnum);
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
	float spawn_parms[NUM_SPAWN_PARMS];
	int entnum;
	qhedict_t* svent;

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
	client->h2_send_all_v = true;
	client->qh_netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->state = CS_CONNECTED;
	client->qh_edict = ent;

	client->qh_message.InitOOB(client->qh_messageBuffer, MAX_MSGLEN_H2);
	client->qh_message.allowoverflow = true;		// we can catch it

	client->datagram.InitOOB(client->datagramBuffer, MAX_MSGLEN_H2);

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		svent = QH_EDICT_NUM(entnum);
//		Com_Memcpy(&svent->baseline[clientnum],&svent->baseline[MAX_BASELINES-1],sizeof(h2entity_state_t));
	}
	Com_Memset(&sv.h2_states[clientnum],0,sizeof(h2client_state2_t));

	if (sv.loadgame)
	{
		Com_Memcpy(client->qh_spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
//	else
//	{
	// call the progs to get default spawn parms for the new client
	//	PR_ExecuteProgram (pr_global_struct->SetNewParms);
	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
//	}

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
			if (svs.clients[i].state < CS_CONNECTED)
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
//	int client_num = 1<<(MAX_BASELINES-1);

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
		VectorCopy(svent->GetOrigin(), svent->h2_baseline.origin);
		VectorCopy(svent->GetAngles(), svent->h2_baseline.angles);
		svent->h2_baseline.frame = svent->GetFrame();
		svent->h2_baseline.skinnum = svent->GetSkin();
		svent->h2_baseline.scale = (int)(svent->GetScale() * 100.0) & 255;
		svent->h2_baseline.drawflags = svent->GetDrawFlags();
		svent->h2_baseline.abslight = (int)(svent->GetAbsLight() * 255.0) & 255;
		if (entnum > 0  && entnum <= svs.qh_maxclients)
		{
			svent->h2_baseline.colormap = entnum;
			svent->h2_baseline.modelindex = 0;	//SVQH_ModelIndex("models/paladin.mdl");
		}
		else
		{
			svent->h2_baseline.colormap = 0;
			svent->h2_baseline.modelindex =
				SVQH_ModelIndex(PR_GetString(svent->GetModel()));
		}
		Com_Memset(svent->h2_baseline.ClearCount,99,sizeof(svent->h2_baseline.ClearCount));

		//
		// add to the message
		//
		sv.qh_signon.WriteByte(h2svc_spawnbaseline);
		sv.qh_signon.WriteShort(entnum);

		sv.qh_signon.WriteShort(svent->h2_baseline.modelindex);
		sv.qh_signon.WriteByte(svent->h2_baseline.frame);
		sv.qh_signon.WriteByte(svent->h2_baseline.colormap);
		sv.qh_signon.WriteByte(svent->h2_baseline.skinnum);
		sv.qh_signon.WriteByte(svent->h2_baseline.scale);
		sv.qh_signon.WriteByte(svent->h2_baseline.drawflags);
		sv.qh_signon.WriteByte(svent->h2_baseline.abslight);
		for (i = 0; i < 3; i++)
		{
			sv.qh_signon.WriteCoord(svent->h2_baseline.origin[i]);
			sv.qh_signon.WriteAngle(svent->h2_baseline.angles[i]);
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

	msg.WriteChar(h2svc_stufftext);
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
	int i;

	svs.qh_serverflags = *pr_globalVars.serverflags;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		// call the progs to get default spawn parms for the new client
//		*pr_globalVars.self = EDICT_TO_PROG(host_client->edict);
//		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
//		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
//			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float scr_centertime_off;

void SV_SpawnServer(char* server, char* startspot)
{
	qhedict_t* ent;
	int i;
	qboolean stats_restored;

	// let's not have any servers with no name
	if (sv_hostname->string[0] == 0)
	{
		Cvar_Set("hostname", "UNNAMED");
	}
#ifndef DEDICATED
	scr_centertime_off = 0;
#endif

	common->DPrintf("SpawnServer: %s\n",server);
	if (svs.qh_changelevel_issued)
	{
		stats_restored = true;
		SVH2_SaveGamestate(true);
	}
	else
	{
		stats_restored = false;
	}

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

	svqh_current_skill = (int)(qh_skill->value + 0.1);
	if (svqh_current_skill < 0)
	{
		svqh_current_skill = 0;
	}
	if (svqh_current_skill > 4)
	{
		svqh_current_skill = 4;
	}

	Cvar_SetValue("skill", (float)svqh_current_skill);

//
// set up the new server
//
	Host_ClearMemory();

	//Com_Memset(&sv, 0, sizeof(sv));

	String::Cpy(sv.name, server);
	if (startspot)
	{
		String::Cpy(sv.h2_startspot, startspot);
	}

// load progs to get entity field count

#ifndef DEDICATED
	total_loading_size = 100;
	current_loading_size = 0;
	loading_stage = 1;
#endif
	PR_LoadProgs();
#ifndef DEDICATED
	current_loading_size += 60;
	SCR_UpdateScreen();
#endif

// allocate server memory
	Com_Memset(sv.h2_Effects,0,sizeof(sv.h2_Effects));

	sv.h2_states = (h2client_state2_t*)Hunk_AllocName(svs.qh_maxclients * sizeof(h2client_state2_t), "states");
	Com_Memset(sv.h2_states,0,svs.qh_maxclients * sizeof(h2client_state2_t));

	sv.qh_edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_QH * pr_edict_size, "edicts");

	//JL WTF????
	sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, MAX_MSGLEN_H2);

	sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, MAX_MSGLEN_H2);

	sv.qh_signon.InitOOB(sv.qh_signonBuffer, MAX_MSGLEN_H2);

// leave slots at start for clients only
	sv.qh_num_edicts = svs.qh_maxclients + 1 + max_temp_edicts->value;
	for (i = 0; i < svs.qh_maxclients; i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
		svs.clients[i].h2_send_all_v = true;
	}

	for (i = 0; i < max_temp_edicts->value; i++)
	{
		ent = QH_EDICT_NUM(i + svs.qh_maxclients + 1);
		ED_ClearEdict(ent);

		ent->free = true;
		ent->freetime = -999;
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

	*pr_globalVars.randomclass = randomclass->value;

	*pr_globalVars.mapname = PR_SetString(sv.name);
	*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);

	// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

#ifndef DEDICATED
	current_loading_size += 20;
	SCR_UpdateScreen();
#endif
	ED_LoadFromFile(CM_EntityString());

// all setup is completed, any further precache statements are errors
	sv.state = SS_GAME;

	SVQH_SetMoveVars();

	// run two frames to allow everything to settle
	host_frametime = 0.1;
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);

#ifndef DEDICATED
	current_loading_size += 20;
	SCR_UpdateScreen();
#endif
// create a baseline for more efficient communications
	SV_CreateBaseline();

// send serverinfo to all connected clients
	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SV_SendServerinfo(host_client);
		}

	svs.qh_changelevel_issued = false;		// now safe to issue another

	common->DPrintf("Server spawned.\n");

#ifndef DEDICATED
	total_loading_size = 0;
	loading_stage = 0;
#endif
}
