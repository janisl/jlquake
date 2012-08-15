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
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f(void)
{
	char* cmd;
	char* map;
	qboolean killBots, cheat;
	char expanded[MAX_QPATH];
	char mapname[MAX_QPATH];

	map = Cmd_Argv(1);
	if (!map)
	{
		return;
	}

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	String::Sprintf(expanded, sizeof(expanded), "maps/%s.bsp", map);
	if (FS_ReadFile(expanded, NULL) == -1)
	{
		common->Printf("Can't find map %s\n", expanded);
		return;
	}

	// force latched values to get set
	Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH2);

	cmd = Cmd_Argv(0);
	if (String::NICmp(cmd, "sp", 2) == 0)
	{
		Cvar_SetValue("g_gametype", Q3GT_SINGLE_PLAYER);
		Cvar_SetValue("g_doWarmup", 0);
		// may not set sv_maxclients directly, always set latched
		Cvar_SetLatched("sv_maxclients", "8");
		cmd += 2;
		cheat = false;
		killBots = true;
	}
	else
	{
		if (!String::ICmp(cmd, "devmap") || !String::ICmp(cmd, "spdevmap"))
		{
			cheat = true;
			killBots = true;
		}
		else
		{
			cheat = false;
			killBots = false;
		}
		if (svt3_gametype->integer == Q3GT_SINGLE_PLAYER)
		{
			Cvar_SetValue("g_gametype", Q3GT_FFA);
		}
	}

	// save the map name here cause on a map restart we reload the q3config.cfg
	// and thus nuke the arguments of the map command
	String::NCpyZ(mapname, map, sizeof(mapname));

	// start up the map
	SVT3_SpawnServer(mapname, killBots);

	// set the cheat value
	// if the level was started with "map <levelname>", then
	// cheats will not be allowed.  If started with "devmap <levelname>"
	// then cheats will be allowed
	if (cheat)
	{
		Cvar_Set("sv_cheats", "1");
	}
	else
	{
		Cvar_Set("sv_cheats", "0");
	}
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f(void)
{
	int i;
	client_t* client;
	const char* denied;
	qboolean isBot;
	int delay;

	// make sure we aren't restarting twice in the same frame
	if (com_frameTime == sv.q3_serverId)
	{
		return;
	}

	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (sv.q3_restartTime)
	{
		return;
	}

	if (Cmd_Argc() > 1)
	{
		delay = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		delay = 5;
	}
	if (delay && !Cvar_VariableValue("g_doWarmup"))
	{
		sv.q3_restartTime = svs.q3_time + delay * 1000;
		SVT3_SetConfigstring(Q3CS_WARMUP, va("%i", sv.q3_restartTime));
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if (sv_maxclients->modified || svt3_gametype->modified)
	{
		char mapname[MAX_QPATH];

		common->Printf("variable change -- restarting.\n");
		// restart the map the slow way
		String::NCpyZ(mapname, Cvar_VariableString("mapname"), sizeof(mapname));

		SVT3_SpawnServer(mapname, false);
		return;
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.q3_snapFlagServerBit ^= Q3SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.q3_serverId = com_frameTime;
	Cvar_Set("sv_serverid", va("%i", sv.q3_serverId));

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.q3_restarting = true;

	SVT3_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		SVT3_GameRunFrame(svs.q3_time);
		svs.q3_time += 100;
	}

	sv.state = SS_GAME;
	sv.q3_restarting = false;

	// connect and begin all the clients
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if (client->state < CS_CONNECTED)
		{
			continue;
		}

		if (client->netchan.remoteAddress.type == NA_BOT)
		{
			isBot = true;
		}
		else
		{
			isBot = false;
		}

		// add the map_restart command
		SVT3_AddServerCommand(client, "map_restart\n");

		// connect the client again, without the firstTime flag
		denied = SVT3_GameClientConnect(i, false, isBot);
		if (denied)
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SVT3_DropClient(client, denied);
			common->Printf("SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);		// bk010125
			continue;
		}

		client->state = CS_ACTIVE;

		SVQ3_ClientEnterWorld(client, &client->q3_lastUsercmd);
	}

	// run another frame to allow things to look at all the players
	SVT3_GameRunFrame(svs.q3_time);
	svs.q3_time += 100;
}

//	Ban a user from being able to play on this server through the auth
// server
void SVT3_Ban_f()
{
	client_t* cl;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: banUser <player name>\n");
		return;
	}

	cl = SVT3_GetPlayerByName();

	if (!cl)
	{
		return;
	}

	if (cl->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		SVT3_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	// look up the authorize server's IP
	if (!svs.q3_authorizeAddress.ip[0] && svs.q3_authorizeAddress.type != NA_BAD)
	{
		const char* authorizeServerName = GGameType & GAME_WolfSP ? WSAUTHORIZE_SERVER_NAME :
			GGameType & GAME_WolfMP ? WMAUTHORIZE_SERVER_NAME : Q3AUTHORIZE_SERVER_NAME;
		common->Printf("Resolving %s\n", authorizeServerName);
		if (!SOCK_StringToAdr(authorizeServerName, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}
		common->Printf("%s resolved to %s\n", authorizeServerName, SOCK_AdrToString(svs.q3_authorizeAddress));
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
			"banUser %s", SOCK_BaseAdrToString(cl->netchan.remoteAddress));
		common->Printf("%s was banned from coming back\n", cl->name);
	}
}

/*
==================
SV_BanNum_f

Ban a user from being able to play on this server through the auth
server
==================
*/
static void SV_BanNum_f(void)
{
	client_t* cl;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: banClient <client number>\n");
		return;
	}

	cl = SVT3_GetPlayerByNum();
	if (!cl)
	{
		return;
	}
	if (cl->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		SVT3_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	// look up the authorize server's IP
	if (!svs.q3_authorizeAddress.ip[0] && svs.q3_authorizeAddress.type != NA_BAD)
	{
		common->Printf("Resolving %s\n", Q3AUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(Q3AUTHORIZE_SERVER_NAME, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}
		common->Printf("%s resolved to %s\n", Q3AUTHORIZE_SERVER_NAME, SOCK_AdrToString(svs.q3_authorizeAddress));
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
			"banUser %s", SOCK_BaseAdrToString(cl->netchan.remoteAddress));
		common->Printf("%s was banned from coming back\n", cl->name);
	}
}

/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f(void)
{
	SV_Shutdown("killserver");
}

//===========================================================

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands(void)
{
	static qboolean initialized;

	if (initialized)
	{
		return;
	}
	initialized = true;

	Cmd_AddCommand("heartbeat", SVT3_Heartbeat_f);
	Cmd_AddCommand("kick", SVT3_Kick_f);
	Cmd_AddCommand("banUser", SVT3_Ban_f);
	Cmd_AddCommand("banClient", SV_BanNum_f);
	Cmd_AddCommand("clientkick", SVT3_KickNum_f);
	Cmd_AddCommand("status", SVT3_Status_f);
	Cmd_AddCommand("serverinfo", SVT3_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SVT3_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SVT3_DumpUser_f);
	Cmd_AddCommand("map_restart", SV_MapRestart_f);
	Cmd_AddCommand("sectorlist", SVT3_SectorList_f);
	Cmd_AddCommand("map", SV_Map_f);
	Cmd_AddCommand("devmap", SV_Map_f);
	Cmd_AddCommand("spmap", SV_Map_f);
	Cmd_AddCommand("spdevmap", SV_Map_f);
	Cmd_AddCommand("killserver", SV_KillServer_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SVT3_ConSay_f);
	}
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands(void)
{
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand("heartbeat");
	Cmd_RemoveCommand("kick");
	Cmd_RemoveCommand("banUser");
	Cmd_RemoveCommand("banClient");
	Cmd_RemoveCommand("status");
	Cmd_RemoveCommand("serverinfo");
	Cmd_RemoveCommand("systeminfo");
	Cmd_RemoveCommand("dumpuser");
	Cmd_RemoveCommand("map_restart");
	Cmd_RemoveCommand("sectorlist");
	Cmd_RemoveCommand("say");
#endif
}
