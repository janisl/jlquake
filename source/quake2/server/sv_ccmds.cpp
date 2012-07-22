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

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f(void)
{
	int i, slot;

	// only dedicated servers send heartbeats
	if (!com_dedicated->value)
	{
		common->Printf("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_SetLatched("public", "1");

	for (i = 1; i < MAX_MASTERS; i++)
	{
		Com_Memset(&master_adr[i], 0, sizeof(master_adr[i]));
	}

	slot = 1;		// slot 0 will always contain the id master
	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (slot == MAX_MASTERS)
		{
			break;
		}

		if (!SOCK_StringToAdr(Cmd_Argv(i), &master_adr[slot], PORT_MASTER))
		{
			common->Printf("Bad address: %s\n", Cmd_Argv(i));
			continue;
		}

		common->Printf("Master server at %s\n", SOCK_AdrToString(master_adr[slot]));

		common->Printf("Sending a ping.\n");

		NET_OutOfBandPrint(NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.q2_last_heartbeat = -9999999;
}

/*
==============
SV_ReadServerFile

==============
*/
void SV_ReadServerFile(void)
{
	fileHandle_t f;
	char name[MAX_OSPATH], string[128];
	char comment[32];
	char mapcmd[MAX_TOKEN_CHARS_Q2];

	common->DPrintf("SV_ReadServerFile()\n");

	FS_FOpenFileRead("save/current/server.ssv", &f, true);
	if (!f)
	{
		common->Printf("Couldn't read %s\n", name);
		return;
	}
	// read the comment field
	FS_Read(comment, sizeof(comment), f);

	// read the mapcmd
	FS_Read(mapcmd, sizeof(mapcmd), f);

	// read all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	while (1)
	{
		if (!FS_Read(name, sizeof(name), f))
		{
			break;
		}
		FS_Read(string, sizeof(string), f);
		common->DPrintf("Set %s = %s\n", name, string);
		Cvar_Set(name, string);
	}

	FS_FCloseFile(f);

	// start a new game fresh with new cvars
	SV_InitGame();

	String::Cpy(svs.q2_mapcmd, mapcmd);

	// read game state
	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), "save/current/game.ssv"));
	ge->ReadGame(name);
}


//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f(void)
{
	SV_Map(true, Cmd_Argv(1), false);
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
void SV_GameMap_f(void)
{
	char* map;
	int i;
	client_t* cl;
	qboolean* savedInuse;

	if (Cmd_Argc() != 2)
	{
		common->Printf("USAGE: gamemap <map>\n");
		return;
	}

	common->DPrintf("SV_GameMap(%s)\n", Cmd_Argv(1));

	// check for clearing the current savegame
	map = Cmd_Argv(1);
	if (map[0] == '*')
	{
		// wipe all the *.sav files
		SVQ2_WipeSavegame("current");
	}
	else
	{	// save the map just exited
		if (sv.state == SS_GAME)
		{
			// clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			savedInuse = (qboolean*)malloc(sv_maxclients->value * sizeof(qboolean));
			for (i = 0,cl = svs.clients; i < sv_maxclients->value; i++,cl++)
			{
				savedInuse[i] = cl->q2_edict->inuse;
				cl->q2_edict->inuse = false;
			}

			SVQ2_WriteLevelFile();

			// we must restore these for clients to transfer over correctly
			for (i = 0,cl = svs.clients; i < sv_maxclients->value; i++,cl++)
				cl->q2_edict->inuse = savedInuse[i];
			free(savedInuse);
		}
	}

	// start up the next map
	SV_Map(false, Cmd_Argv(1), false);

	// archive server state
	String::NCpy(svs.q2_mapcmd, Cmd_Argv(1), sizeof(svs.q2_mapcmd) - 1);

	// copy off the level to the autosave slot
	if (!com_dedicated->value)
	{
		SVQ2_WriteServerFile(true);
		SVQ2_CopySaveGame("current", "save0");
	}
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f(void)
{
	char* map;
	char expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr(map, "."))
	{
		String::Sprintf(expanded, sizeof(expanded), "maps/%s.bsp", map);
		if (FS_ReadFile(expanded, NULL) == -1)
		{
			common->Printf("Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = SS_DEAD;		// don't save current level when changing
	SVQ2_WipeSavegame("current");
	SV_GameMap_f();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/


/*
==============
SV_Loadgame_f

==============
*/
void SV_Loadgame_f(void)
{
	char name[MAX_OSPATH];
	fileHandle_t f;
	char* dir;

	if (Cmd_Argc() != 2)
	{
		common->Printf("USAGE: loadgame <directory>\n");
		return;
	}

	common->Printf("Loading game...\n");

	dir = Cmd_Argv(1);
	if (strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\"))
	{
		common->Printf("Bad savedir.\n");
	}

	// make sure the server.ssv file exists
	String::Sprintf(name, sizeof(name), "save/%s/server.ssv", Cmd_Argv(1));
	FS_FOpenFileRead(name, &f, true);
	if (!f)
	{
		common->Printf("No such savegame: %s\n", name);
		return;
	}
	FS_FCloseFile(f);

	SVQ2_CopySaveGame(Cmd_Argv(1), "current");

	SV_ReadServerFile();

	// go to the map
	sv.state = SS_DEAD;		// don't save current level when changing
	SV_Map(false, svs.q2_mapcmd, true);
}



/*
==============
SV_Savegame_f

==============
*/
void SV_Savegame_f(void)
{
	char* dir;

	if (sv.state != SS_GAME)
	{
		common->Printf("You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		common->Printf("Can't savegame in a deathmatch\n");
		return;
	}

	if (!String::Cmp(Cmd_Argv(1), "current"))
	{
		common->Printf("Can't save to 'current'\n");
		return;
	}

	if (sv_maxclients->value == 1 && svs.clients[0].q2_edict->client->ps.stats[Q2STAT_HEALTH] <= 0)
	{
		common->Printf("\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\"))
	{
		common->Printf("Bad savedir.\n");
	}

	common->Printf("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SVQ2_WriteLevelFile();

	// save server state
	SVQ2_WriteServerFile(false);

	// copy it off
	SVQ2_CopySaveGame("current", dir);

	common->Printf("Done.\n");
}

/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f(void)
{
	if (!svs.initialized)
	{
		return;
	}
	SV_Shutdown("Server was killed.\n", false);
	NET_Config(false);		// close network sockets
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	Cmd_AddCommand("heartbeat", SVQ2_Heartbeat_f);
	Cmd_AddCommand("kick", SVQ2_Kick_f);
	Cmd_AddCommand("status", SVQ2_Status_f);
	Cmd_AddCommand("serverinfo", SVQ2_Serverinfo_f);
	Cmd_AddCommand("dumpuser", SVQ2_DumpUser_f);

	Cmd_AddCommand("map", SV_Map_f);
	Cmd_AddCommand("demomap", SV_DemoMap_f);
	Cmd_AddCommand("gamemap", SV_GameMap_f);
	Cmd_AddCommand("setmaster", SV_SetMaster_f);

	if (com_dedicated->value)
	{
		Cmd_AddCommand("say", SVQ2_ConSay_f);
	}

	Cmd_AddCommand("serverrecord", SVQ2_ServerRecord_f);
	Cmd_AddCommand("serverstop", SVQ2_ServerStop_f);

	Cmd_AddCommand("save", SV_Savegame_f);
	Cmd_AddCommand("load", SV_Loadgame_f);

	Cmd_AddCommand("killserver", SV_KillServer_f);

	Cmd_AddCommand("sv", SVQ2_ServerCommand_f);
}
