/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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
	char mapname[MAX_QPATH];
	qboolean killBots, cheat;
	char expanded[MAX_QPATH];
	// TTimo: unused
//	int			savegameTime = -1;

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
		Com_Printf("Can't find map %s\n", expanded);
		return;
	}

	Cvar_Set("gamestate", va("%i", GS_INITIALIZE));				// NERVE - SMF - reset gamestate on map/devmap
	Cvar_Set("savegame_loading", "0");		// make sure it's turned off

	Cvar_Set("g_currentRound", "0");				// NERVE - SMF - reset the current round
	Cvar_Set("g_nextTimeLimit", "0");				// NERVE - SMF - reset the next time limit

	// force latched values to get set
	// DHM - Nerve :: default to WMGT_WOLF
	Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH2);

	// Rafael gameskill
	Cvar_Get("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2);
	// done

	cmd = Cmd_Argv(0);
	if (String::NICmp(cmd, "sp", 2) == 0)
	{
		Cvar_SetValue("g_gametype", Q3GT_SINGLE_PLAYER);
		Cvar_SetValue("g_doWarmup", 0);
		// may not set sv_maxclients directly, always set latched
#ifdef __MACOS__
		Cvar_SetLatched("sv_maxclients", "16");		//DAJ HOG
#else
		Cvar_SetLatched("sv_maxclients", "32");		// Ridah, modified this
#endif
		cmd += 2;
		killBots = qtrue;
		if (!String::ICmp(cmd, "devmap"))
		{
			cheat = qtrue;
		}
		else
		{
			cheat = qfalse;
		}
	}
	else
	{
		if (!String::ICmp(cmd, "devmap"))
		{
			cheat = qtrue;
			killBots = qtrue;
		}
		else
		{
			cheat = qfalse;
			killBots = qfalse;
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
	SV_SpawnServer(mapname, killBots);

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
	char* denied;
	qboolean isBot;
	int delay = 0;
	gamestate_t new_gs, old_gs;		// NERVE - SMF
	int worldspawnflags;			// DHM - Nerve
	int nextgt;						// DHM - Nerve
	wmsharedEntity_t* world;

	// make sure we aren't restarting twice in the same frame
	if (com_frameTime == sv.q3_serverId)
	{
		return;
	}

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (sv.q3_restartTime)
	{
		return;
	}

	// DHM - Nerve :: Check for invalid gametype
	svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);
	nextgt = svt3_gametype->integer;

	world = SVWM_GentityNum(Q3ENTITYNUM_WORLD);
	worldspawnflags = world->r.worldflags;
	if  (
		(nextgt == WMGT_WOLF && (worldspawnflags & 1)) ||
		(nextgt == WMGT_WOLF_STOPWATCH && (worldspawnflags & 2)) ||
		((nextgt == WMGT_WOLF_CP || nextgt == WMGT_WOLF_CPH) && (worldspawnflags & 4))
		)
	{

		if (!(worldspawnflags & 1))
		{
			Cvar_Set("g_gametype", "5");
		}
		else
		{
			Cvar_Set("g_gametype", "7");
		}

		svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);
	}
	// dhm

	if (Cmd_Argc() > 1)
	{
		delay = String::Atoi(Cmd_Argv(1));
	}

	if (delay)
	{
		sv.q3_restartTime = svs.q3_time + delay * 1000;
		SVT3_SetConfigstring(Q3CS_WARMUP, va("%i", sv.q3_restartTime));
		return;
	}

	// NERVE - SMF - read in gamestate or just default to GS_PLAYING
	old_gs = (gamestate_t) String::Atoi(Cvar_VariableString("gamestate"));

	if (Cmd_Argc() > 2)
	{
		new_gs = (gamestate_t) String::Atoi(Cmd_Argv(2));
	}
	else
	{
		new_gs = GS_PLAYING;
	}

	if (!SVT3_TransitionGameState(new_gs, old_gs, delay))
	{
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if (sv_maxclients->modified)
	{
		char mapname[MAX_QPATH];

		Com_Printf("sv_maxclients variable change -- restarting.\n");
		// restart the map the slow way
		String::NCpyZ(mapname, Cvar_VariableString("mapname"), sizeof(mapname));

		SV_SpawnServer(mapname, qfalse);
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
	sv.q3_restarting = qtrue;

	Cvar_Set("sv_serverRestarting", "1");

	SVT3_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		VM_Call(gvm, WMGAME_RUN_FRAME, svs.q3_time);
		svs.q3_time += 100;
	}

	sv.state = SS_GAME;
	sv.q3_restarting = qfalse;

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
			isBot = qtrue;
		}
		else
		{
			isBot = qfalse;
		}

		// add the map_restart command
		SVT3_AddServerCommand(client, "map_restart\n");

		// connect the client again, without the firstTime flag
		denied = (char*)VM_ExplicitArgPtr(gvm, VM_Call(gvm, WMGAME_CLIENT_CONNECT, i, qfalse, isBot));
		if (denied)
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SVT3_DropClient(client, denied);
			Com_Printf("SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);		// bk010125
			continue;
		}

		client->state = CS_ACTIVE;

		SV_ClientEnterWorld(client, &client->wm_lastUsercmd);
	}

	// run another frame to allow things to look at all the players
	VM_Call(gvm, WMGAME_RUN_FRAME, svs.q3_time);
	svs.q3_time += 100;

	Cvar_Set("sv_serverRestarting", "0");
}

/*
=================
SV_LoadGame_f
=================
*/
void    SV_LoadGame_f(void)
{
	char filename[MAX_QPATH], mapname[MAX_QPATH];
	char* buffer;
	int size;

	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	if (!filename[0])
	{
		Com_Printf("You must specify a savegame to load\n");
		return;
	}
	if (String::NCmp(filename, "save/", 5) && String::NCmp(filename, "save\\", 5))
	{
		String::NCpyZ(filename, va("save/%s", filename), sizeof(filename));
	}
	if (!strstr(filename, ".svg"))
	{
		String::Cat(filename, sizeof(filename), ".svg");
	}

	size = FS_ReadFile(filename, NULL);
	if (size < 0)
	{
		Com_Printf("Can't find savegame %s\n", filename);
		return;
	}

	FS_ReadFile(filename, (void**)&buffer);

	// read the mapname, if it is the same as the current map, then do a fast load
	String::Sprintf(mapname, sizeof(mapname), buffer + sizeof(int));

	if (com_sv_running->integer && (com_frameTime != sv.q3_serverId))
	{
		// check mapname
		if (!String::ICmp(mapname, svt3_mapname->string))			// same

		{
			if (String::ICmp(filename, "save/current.svg") != 0)
			{
				// copy it to the current savegame file
				FS_WriteFile("save/current.svg", buffer, size);
			}

			FS_FreeFile(buffer);

			Cvar_Set("savegame_loading", "2");		// 2 means it's a restart, so stop rendering until we are loaded
			SV_MapRestart_f();	// savegame will be loaded after restart

			return;
		}
	}

	FS_FreeFile(buffer);

	// otherwise, do a slow load
	if (Cvar_VariableIntegerValue("sv_cheats"))
	{
		Cbuf_ExecuteText(EXEC_APPEND, va("spdevmap %s", filename));
	}
	else		// no cheats
	{
		Cbuf_ExecuteText(EXEC_APPEND, va("spmap %s", filename));
	}
}

//	Ban a user from being able to play on this server through the auth
// server
void SVT3_Ban_f()
{
	client_t* cl;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: banUser <player name>\n");
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
		Com_Printf("Resolving %s\n", authorizeServerName);
		if (!SOCK_StringToAdr(authorizeServerName, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			Com_Printf("Couldn't resolve address\n");
			return;
		}
		Com_Printf("%s resolved to %s\n", authorizeServerName, SOCK_AdrToString(svs.q3_authorizeAddress));
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
			"banUser %s", SOCK_BaseAdrToString(cl->netchan.remoteAddress));
		Com_Printf("%s was banned from coming back\n", cl->name);
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
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: banClient <client number>\n");
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
		Com_Printf("Resolving %s\n", WMAUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(WMAUTHORIZE_SERVER_NAME, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			Com_Printf("Couldn't resolve address\n");
			return;
		}
		Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", WMAUTHORIZE_SERVER_NAME,
			svs.q3_authorizeAddress.ip[0], svs.q3_authorizeAddress.ip[1],
			svs.q3_authorizeAddress.ip[2], svs.q3_authorizeAddress.ip[3],
			BigShort(svs.q3_authorizeAddress.port));
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
			"banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1],
			cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);
		Com_Printf("%s was banned from coming back\n", cl->name);
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

/*
=================
SV_GameCompleteStatus_f

NERVE - SMF
=================
*/
void SV_GameCompleteStatus_f(void)
{
	SV_MasterGameCompleteStatus();
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
	initialized = qtrue;

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
	Cmd_AddCommand("gameCompleteStatus", SV_GameCompleteStatus_f);			// NERVE - SMF
	Cmd_AddCommand("devmap", SV_Map_f);
	Cmd_AddCommand("spmap", SV_Map_f);
	Cmd_AddCommand("spdevmap", SV_Map_f);
	Cmd_AddCommand("loadgame", SV_LoadGame_f);
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
