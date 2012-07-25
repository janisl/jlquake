/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

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
	char smapname[MAX_QPATH];
	char mapname[MAX_QPATH];
	qboolean killBots, cheat, buildScript;
	char expanded[MAX_QPATH];
	int savegameTime = -1;

	map = Cmd_Argv(1);
	if (!map)
	{
		return;
	}

	buildScript = Cvar_VariableIntegerValue("com_buildScript");

	if (!buildScript && sv_reloading->integer && sv_reloading->integer != RELOAD_NEXTMAP)		// game is in 'reload' mode, don't allow starting new maps yet.
	{
		return;
	}

	// Ridah: trap a savegame load
	if (strstr(map, ".svg"))
	{
		// open the savegame, read the mapname, and copy it to the map string
		char savemap[MAX_QPATH];
		byte* buffer;
		int size, csize;

		if (!(strstr(map, "save/") == map))
		{
			String::Sprintf(savemap, sizeof(savemap), "save/%s", map);
		}
		else
		{
			String::Cpy(savemap, map);
		}

		size = FS_ReadFile(savemap, NULL);
		if (size < 0)
		{
			common->Printf("Can't find savegame %s\n", savemap);
			return;
		}

		//buffer = Hunk_AllocateTempMemory(size);
		FS_ReadFile(savemap, (void**)&buffer);

		if (String::ICmp(savemap, "save/current.svg") != 0)
		{
			// copy it to the current savegame file
			FS_WriteFile("save/current.svg", buffer, size);
			// make sure it is the correct size
			csize = FS_ReadFile("save/current.svg", NULL);
			if (csize != size)
			{
				FS_FreeFile(buffer);
				FS_Delete("save/current.svg");
// TTimo
#ifdef __linux__
				Com_Error(ERR_DROP, "Unable to save game.\n\nPlease check that you have at least 5mb free of disk space in your home directory.");
#else
				Com_Error(ERR_DROP, "Insufficient free disk space.\n\nPlease free at least 5mb of free space on game drive.");
#endif
				return;
			}
		}

		// set the cvar, so the game knows it needs to load the savegame once the clients have connected
		Cvar_Set("savegame_loading", "1");
		// set the filename
		Cvar_Set("savegame_filename", savemap);

		// the mapname is at the very start of the savegame file
		String::Sprintf(savemap, sizeof(savemap), (char*)(buffer + sizeof(int)));				// skip the version
		String::NCpyZ(smapname, savemap, sizeof(smapname));
		map = smapname;

		savegameTime = *(int*)(buffer + sizeof(int) + MAX_QPATH);

		if (savegameTime >= 0)
		{
			svs.q3_time = savegameTime;
		}

		FS_FreeFile(buffer);
	}
	else
	{
		Cvar_Set("savegame_loading", "0");		// make sure it's turned off
		// set the filename
		Cvar_Set("savegame_filename", "");
	}
	// done.

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

	// Rafael gameskill
	Cvar_Get("g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH2);
	// done

	Cvar_SetValue("g_episode", 0);	//----(SA) added

	cmd = Cmd_Argv(0);
	if (String::NICmp(cmd, "sp", 2) == 0)
	{
		Cvar_SetValue("g_gametype", Q3GT_SINGLE_PLAYER);
		Cvar_SetValue("g_doWarmup", 0);
		// may not set sv_maxclients directly, always set latched
		Cvar_SetLatched("sv_maxclients", "32");		// Ridah, modified this
		cmd += 2;
		killBots = true;
		if (!String::ICmp(cmd, "devmap"))
		{
			cheat = true;
		}
		else
		{
			cheat = false;
		}
	}
	else
	{
		if (!String::ICmp(cmd, "devmap"))
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
		if (svt3_gametype->integer == Q3GT_SINGLE_PLAYER)		// (SA) no pause by default in sp
		{
			delay = 0;
		}
		else
		{
			delay = 5;
		}
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

		SV_SpawnServer(mapname, false);
		return;
	}

	// Ridah, check for loading a saved game
	if (Cvar_VariableIntegerValue("savegame_loading"))
	{
		// open the current savegame, and find out what the time is, everything else we can ignore
		const char* savemap = "save/current.svg";
		byte* buffer;
		int size, savegameTime;

		size = FS_ReadFile(savemap, NULL);
		if (size < 0)
		{
			common->Printf("Can't find savegame %s\n", savemap);
			return;
		}

		//buffer = Hunk_AllocateTempMemory(size);
		FS_ReadFile(savemap, (void**)&buffer);

		// the mapname is at the very start of the savegame file
		savegameTime = *(int*)(buffer + sizeof(int) + MAX_QPATH);

		if (savegameTime >= 0)
		{
			svs.q3_time = savegameTime;
		}

		FS_FreeFile(buffer);
	}
	// done.

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.q3_snapFlagServerBit ^= Q3SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	sv.q3_restartedServerId = sv.q3_serverId;
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
		VM_Call(gvm, WSGAME_RUN_FRAME, svs.q3_time);
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
		denied = (char*)VM_ExplicitArgPtr(gvm, VM_Call(gvm, WSGAME_CLIENT_CONNECT, i, false, isBot));
		if (denied)
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SVT3_DropClient(client, denied);
			common->Printf("SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);		// bk010125
			continue;
		}

		client->state = CS_ACTIVE;

		SVWS_ClientEnterWorld(client, &client->ws_lastUsercmd);
	}

	// run another frame to allow things to look at all the players
	VM_Call(gvm, WSGAME_RUN_FRAME, svs.q3_time);
	svs.q3_time += 100;
}

/*
=================
SV_LoadGame_f
=================
*/
void    SV_LoadGame_f(void)
{
	char filename[MAX_QPATH], mapname[MAX_QPATH];
	byte* buffer;
	int size;

	// dont allow command if another loadgame is pending
	if (Cvar_VariableIntegerValue("savegame_loading"))
	{
		return;
	}
	if (sv_reloading->integer)
	{
		// (SA) disabling
//	if(sv_reloading->integer && sv_reloading->integer != RELOAD_FAILED )	// game is in 'reload' mode, don't allow starting new maps yet.
		return;
	}

	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	if (!filename[0])
	{
		common->Printf("You must specify a savegame to load\n");
		return;
	}
	if (String::NCmp(filename, "save/", 5) && String::NCmp(filename, "save\\", 5))
	{
		String::NCpyZ(filename, va("save/%s", filename), sizeof(filename));
	}
	// enforce .svg extension
	if (!strstr(filename, ".") || String::NCmp(strstr(filename, ".") + 1, "svg", 3))
	{
		String::Cat(filename, sizeof(filename), ".svg");
	}
	// use '/' instead of '\' for directories
	while (strstr(filename, "\\"))
	{
		*(char*)strstr(filename, "\\") = '/';
	}

	size = FS_ReadFile(filename, NULL);
	if (size < 0)
	{
		common->Printf("Can't find savegame %s\n", filename);
		return;
	}

	//buffer = Hunk_AllocateTempMemory(size);
	FS_ReadFile(filename, (void**)&buffer);

	// read the mapname, if it is the same as the current map, then do a fast load
	String::Sprintf(mapname, sizeof(mapname), (const char*)(buffer + sizeof(int)));

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
			// set the filename
			Cvar_Set("savegame_filename", filename);
			// quick-restart the server
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
		common->Printf("Resolving %s\n", WSAUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(WSAUTHORIZE_SERVER_NAME, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}
		common->Printf("%s resolved to %i.%i.%i.%i:%i\n", WSAUTHORIZE_SERVER_NAME,
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
	Cmd_AddCommand("spmap", SV_Map_f);
	Cmd_AddCommand("map", SV_Map_f);
	Cmd_AddCommand("devmap", SV_Map_f);
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
