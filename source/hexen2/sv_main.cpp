// sv_main.c -- server main program

/*
 * $Header: /H2 Mission Pack/SV_MAIN.C 26    3/20/98 12:12a Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

Cvar* sv_sound_distance;

Cvar* sv_idealrollscale;

qboolean skip_start = false;
int num_intro_msg = 0;

extern int host_hunklevel;

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
		sprintf(svqh_localmodels[i], "*%i", i);

	svh2_kingofhill = 0;	//Initialize King of Hill to world
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

	SVQH_SendServerinfo(client);
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
