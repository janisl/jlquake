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
		SCR_BeginLoadingPlaque();
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
	SOCK_StringToAdr("192.246.40.37", &master_adr[0], PORT_MASTER);

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
		SCR_BeginLoadingPlaque();			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_CINEMATIC, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".dm2"))
	{
		SCR_BeginLoadingPlaque();			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_DEMO, attractloop, loadgame);
	}
	else if (l > 4 && !String::Cmp(level + l - 4, ".pcx"))
	{
		SCR_BeginLoadingPlaque();			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SpawnServer(level, spawnpoint, SS_PIC, attractloop, loadgame);
	}
	else
	{
		SCR_BeginLoadingPlaque();			// for local system
		SVQ2_BroadcastCommand("changing\n");
		SVQ2_SendClientMessages();
		SVQ2_SpawnServer(level, spawnpoint, SS_GAME, attractloop, loadgame);
		Cbuf_CopyToDefer();
	}

	SVQ2_BroadcastCommand("reconnect\n");
}
