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

extern int host_hunklevel;

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
		sprintf(svqh_localmodels[i], "*%i", i);
}

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
