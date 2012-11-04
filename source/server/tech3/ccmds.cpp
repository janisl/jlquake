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
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

/*
===============================================================================
OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

void SVT3_Heartbeat_f()
{
	svs.q3_nextHeartbeatTime = -9999999;
}

void SVET_TempBanNetAddress(netadr_t address, int length)
{
	int oldesttime = 0;
	int oldest = -1;
	for (int i = 0; i < MAX_TEMPBAN_ADDRESSES; i++)
	{
		if (!svs.et_tempBanAddresses[i].endtime || svs.et_tempBanAddresses[i].endtime < svs.q3_time)
		{
			// found a free slot
			svs.et_tempBanAddresses[i].adr = address;
			svs.et_tempBanAddresses[i].endtime = svs.q3_time + (length * 1000);
			return;
		}
		else
		{
			if (oldest == -1 || oldesttime > svs.et_tempBanAddresses[i].endtime)
			{
				oldesttime = svs.et_tempBanAddresses[i].endtime;
				oldest = i;
			}
		}
	}

	svs.et_tempBanAddresses[oldest].adr = address;
	svs.et_tempBanAddresses[oldest].endtime = svs.q3_time + length;
}

//	Returns the player with name from Cmd_Argv(1)
static client_t* SVT3_GetPlayerByName()
{
	client_t* cl;
	int i;
	char* s;
	char cleanName[64];

	// make sure server is running
	if (!com_sv_running->integer)
	{
		return NULL;
	}

	if (Cmd_Argc() < 2)
	{
		common->Printf("No player specified.\n");
		return NULL;
	}

	s = Cmd_Argv(1);

	// check for a name match
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state <= CS_ZOMBIE)
		{
			continue;
		}
		if (!String::ICmp(cl->name, s))
		{
			return cl;
		}

		String::NCpyZ(cleanName, cl->name, sizeof(cleanName));
		String::CleanStr(cleanName);
		if (!String::ICmp(cleanName, s))
		{
			return cl;
		}
	}

	common->Printf("Player %s is not on the server\n", s);

	return NULL;
}

//	Returns the player with idnum from Cmd_Argv(1)
static client_t* SVT3_GetPlayerByNum()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		return NULL;
	}

	if (Cmd_Argc() < 2)
	{
		common->Printf("No player specified.\n");
		return NULL;
	}

	const char* s = Cmd_Argv(1);

	for (int i = 0; s[i]; i++)
	{
		if (s[i] < '0' || s[i] > '9')
		{
			common->Printf("Bad slot number: %s\n", s);
			return NULL;
		}
	}
	int idnum = String::Atoi(s);
	if (idnum < 0 || idnum >= sv_maxclients->integer)
	{
		common->Printf("Bad client slot: %i\n", idnum);
		return NULL;
	}

	client_t* cl = &svs.clients[idnum];
	if (!cl->state)
	{
		common->Printf("Client %i is not active\n", idnum);
		return NULL;
	}
	return cl;
}

//	Kick a user off of the server  FIXME: move to game
static void SVT3_Kick_f()
{
	client_t* cl;
	int i;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: kick <player name>\nkick all = kick everyone\nkick allbots = kick all bots\n");
		return;
	}

	cl = SVT3_GetPlayerByName();
	if (!cl)
	{
		if (!String::ICmp(Cmd_Argv(1), "all"))
		{
			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
			{
				if (!cl->state)
				{
					continue;
				}
				if (cl->netchan.remoteAddress.type == NA_LOOPBACK)
				{
					continue;
				}
				SVT3_DropClient(cl, GGameType & GAME_WolfMP ? "player kicked" : "was kicked");		// JPW NERVE to match front menu message
				cl->q3_lastPacketTime = svs.q3_time;	// in case there is a funny zombie
			}
		}
		else if (!String::ICmp(Cmd_Argv(1), "allbots"))
		{
			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
			{
				if (!cl->state)
				{
					continue;
				}
				if (cl->netchan.remoteAddress.type != NA_BOT)
				{
					continue;
				}
				SVT3_DropClient(cl, "was kicked");
				cl->q3_lastPacketTime = svs.q3_time;	// in case there is a funny zombie
			}
		}
		return;
	}
	if (cl->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		SVT3_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	SVT3_DropClient(cl, GGameType & GAME_WolfMP ? "player kicked" : "was kicked");		// JPW NERVE to match front menu message
	cl->q3_lastPacketTime = svs.q3_time;	// in case there is a funny zombie
}

//	Kick a user off of the server  FIXME: move to game
static void SVT3_KickNum_f()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: kicknum <client number>\n");
		return;
	}

	client_t* cl = SVT3_GetPlayerByNum();
	if (!cl)
	{
		return;
	}
	if (cl->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		SVT3_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	SVT3_DropClient(cl, GGameType & GAME_WolfMP ? "player kicked" : "was kicked");
	cl->q3_lastPacketTime = svs.q3_time;	// in case there is a funny zombie
}

static void SVT3_Status_f()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	common->Printf("map: %s\n", svt3_mapname->string);

	common->Printf("num score ping name            lastmsg address               qport rate\n");
	common->Printf("--- ----- ---- --------------- ------- --------------------- ----- -----\n");
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		common->Printf("%3i ", i);
		idPlayerState3* ps = SVT3_GameClientNum(i);
		common->Printf("%5i ", ps->GetPersistantScore());

		if (cl->state == CS_CONNECTED)
		{
			common->Printf("CNCT ");
		}
		else if (cl->state == CS_ZOMBIE)
		{
			common->Printf("ZMBI ");
		}
		else
		{
			int ping = cl->ping < 9999 ? cl->ping : 9999;
			common->Printf("%4i ", ping);
		}

		common->Printf("%s", cl->name);
		// TTimo adding a S_COLOR_WHITE to reset the color
		common->Printf(S_COLOR_WHITE);
		int l = 16 - String::LengthWithoutColours(cl->name);
		for (int j = 0; j < l; j++)
		{
			common->Printf(" ");
		}

		common->Printf("%7i ", svs.q3_time - cl->q3_lastPacketTime);

		const char* s = SOCK_AdrToString(cl->netchan.remoteAddress);
		common->Printf("%s", s);
		l = 22 - String::Length(s);
		for (int j = 0; j < l; j++)
		{
			common->Printf(" ");
		}

		common->Printf("%5i", cl->netchan.qport);

		common->Printf(" %5i", cl->rate);

		common->Printf("\n");
	}
	common->Printf("\n");
}

static void SVT3_ConSay_f()
{
	char* p;
	char text[1024];

	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		return;
	}

	String::Cpy(text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	SVT3_SendServerCommand(NULL, "chat \"%s\n\"", text);
}

//	Examine the serverinfo string
static void SVT3_Serverinfo_f()
{
	common->Printf("Server info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
}

static void SVT3_Systeminfo_f()
{
	common->Printf("System info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SYSTEMINFO, MAX_INFO_STRING_Q3));
}

//	Examine all a users info strings FIXME: move to game
static void SVT3_DumpUser_f()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		common->Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: info <userid>\n");
		return;
	}

	client_t* cl = SVT3_GetPlayerByName();
	if (!cl)
	{
		return;
	}

	common->Printf("userinfo\n");
	common->Printf("--------\n");
	Info_Print(cl->userinfo);
}

static bool SVT3_CheckTransitionGameState(gamestate_t new_gs, gamestate_t old_gs)
{
	if (old_gs == new_gs && new_gs != GS_PLAYING)
	{
		return false;
	}

	if (old_gs == GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP)
	{
		return false;
	}

	if (old_gs == GS_INTERMISSION && new_gs != GS_WARMUP)
	{
		return false;
	}

	if (old_gs == GS_RESET && (new_gs != GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP))
	{
		return false;
	}

	return true;
}

static bool SVT3_TransitionGameState(gamestate_t new_gs, gamestate_t old_gs, int delay)
{
	if (!(GGameType & GAME_ET) || (!SVET_GameIsSinglePlayer() && !SVET_GameIsCoop()))
	{
		// we always do a warmup before starting match
		if (old_gs == GS_INTERMISSION && new_gs == GS_PLAYING)
		{
			new_gs = GS_WARMUP;
		}
	}

	// check if its a valid state transition
	if (!SVT3_CheckTransitionGameState(new_gs, old_gs))
	{
		return false;
	}

	if (new_gs == GS_RESET)
	{
		if (GGameType & GAME_WolfMP && String::Atoi(Cvar_VariableString("g_noTeamSwitching")))
		{
			new_gs = GS_WAITING_FOR_PLAYERS;
		}
		else
		{
			new_gs = GS_WARMUP;
		}
	}

	Cvar_Set("gamestate", va("%i", new_gs));

	return true;
}

static void SVET_FieldInfo_f()
{
	MSGET_PrioritiseEntitystateFields();
	MSGET_PrioritisePlayerStateFields();
}

bool SVET_TempBanIsBanned(netadr_t address)
{
	for (int i = 0; i < MAX_TEMPBAN_ADDRESSES; i++)
	{
		if (svs.et_tempBanAddresses[i].endtime && svs.et_tempBanAddresses[i].endtime > svs.q3_time)
		{
			if (SOCK_CompareAdr(address, svs.et_tempBanAddresses[i].adr))
			{
				return true;
			}
		}
	}

	return false;
}

//	Restart the server on a different map
static void SVT3_Map_f()
{
	char* map = Cmd_Argv(1);
	if (!map)
	{
		return;
	}

	if (GGameType & GAME_ET && !comet_gameInfo.spEnabled)
	{
		if (!String::ICmp(Cmd_Argv(0), "spdevmap") || !String::ICmp(Cmd_Argv(0), "spmap"))
		{
			common->Printf("Single Player is not enabled.\n");
			return;
		}
	}

	char smapname[MAX_QPATH];
	if (GGameType & GAME_WolfSP || (GGameType & GAME_ET && SVET_GameIsSinglePlayer()))
	{
		// game is in 'reload' mode, don't allow starting new maps yet.
		if (svt3_reloading->integer && svt3_reloading->integer != RELOAD_NEXTMAP)
		{
			return;
		}

		// Trap a savegame load
		const char* saveExtension = GGameType & GAME_ET ? ".sav" : ".svg";
		if (strstr(map, saveExtension))
		{
			// open the savegame, read the mapname, and copy it to the map string
			char savemap[MAX_QPATH];
			char savedir[MAX_QPATH];
			byte* buffer;
			int size, csize;

			const char* cl_profileStr = Cvar_VariableString("cl_profile");
			if (GGameType & GAME_ET && comet_gameInfo.usesProfiles && cl_profileStr[0])
			{
				String::Sprintf(savedir, sizeof(savedir), "profiles/%s/save/", cl_profileStr);
			}
			else
			{
				String::NCpyZ(savedir, "save/", sizeof(savedir));
			}

			if (!(strstr(map, savedir) == map))
			{
				String::Sprintf(savemap, sizeof(savemap), "%s%s", savedir, map);
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

			FS_ReadFile(savemap, (void**)&buffer);

			if (String::ICmp(savemap, va("%scurrent%s", savedir, saveExtension)) != 0)
			{
				// copy it to the current savegame file
				FS_WriteFile(va("%scurrent%s", savedir, saveExtension), buffer, size);
				// make sure it is the correct size
				csize = FS_ReadFile(va("%scurrent%s", savedir, saveExtension), NULL);
				if (csize != size)
				{
					FS_FreeFile(buffer);
					FS_Delete(va("%scurrent%s", savedir, saveExtension));
// TTimo
#ifdef __linux__
					common->Error("Unable to save game.\n\nPlease check that you have at least 5mb free of disk space in your home directory.");
#else
					common->Error("Insufficient free disk space.\n\nPlease free at least 5mb of free space on game drive.");
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

			int savegameTime = *(int*)(buffer + sizeof(int) + MAX_QPATH);

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
	}
	else if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("savegame_loading", "0");		// make sure it's turned off
		// set the filename
		Cvar_Set("savegame_filename", "");
	}

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	char expanded[MAX_QPATH];
	String::Sprintf(expanded, sizeof(expanded), "maps/%s.bsp", map);
	if (FS_ReadFile(expanded, NULL) == -1)
	{
		common->Printf("Can't find map %s\n", expanded);
		return;
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("gamestate", va("%i", GS_INITIALIZE));				// NERVE - SMF - reset gamestate on map/devmap

		Cvar_Set("g_currentRound", "0");				// NERVE - SMF - reset the current round
		Cvar_Set("g_nextTimeLimit", "0");				// NERVE - SMF - reset the next time limit
	}

	// force latched values to get set
	if (GGameType & (GAME_Quake3 | GAME_WolfSP))
	{
		Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH2);
	}
	if (GGameType & GAME_WolfMP)
	{
		// DHM - Nerve :: default to WMGT_WOLF
		Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH2);

		// Rafael gameskill
		Cvar_Get("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2);
		// done
	}

	if (GGameType & GAME_WolfSP)
	{
		// Rafael gameskill
		Cvar_Get("g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH2);
		// done

		Cvar_SetValue("g_episode", 0);	//----(SA) added
	}

	// START	Mad Doctor I changes, 8/14/2002.  Need a way to force load a single player map as single player
	if (GGameType & GAME_ET && (!String::ICmp(Cmd_Argv(0), "spdevmap") || !String::ICmp(Cmd_Argv(0), "spmap")))
	{
		// This is explicitly asking for a single player load of this map
		Cvar_Set("g_gametype", va("%i", comet_gameInfo.defaultSPGameType));
		// force latched values to get set
		Cvar_Get("g_gametype", va("%i", comet_gameInfo.defaultSPGameType), CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH2);
		// enable bot support for AI
		Cvar_Set("bot_enable", "1");
	}

	char* cmd = Cmd_Argv(0);
	bool killBots;
	bool cheat;
	if (!(GGameType & GAME_ET) && String::NICmp(cmd, "sp", 2) == 0)
	{
		Cvar_SetValue("g_gametype", Q3GT_SINGLE_PLAYER);
		Cvar_SetValue("g_doWarmup", 0);
		// may not set sv_maxclients directly, always set latched
		Cvar_SetLatched("sv_maxclients", GGameType & GAME_Quake3 ? "8" : "32");		// Ridah, modified this
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
		if (!(GGameType & GAME_ET) && svt3_gametype->integer == Q3GT_SINGLE_PLAYER)
		{
			Cvar_SetValue("g_gametype", Q3GT_FFA);
		}
	}

	// save the map name here cause on a map restart we reload the q3config.cfg
	// and thus nuke the arguments of the map command
	char mapname[MAX_QPATH];
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

//	Completely restarts a level, but doesn't send a new gamestate to the clients.
// This allows fair starts with variable load times.
static void SVT3_MapRestart_f()
{
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

	if (!(GGameType & GAME_ET) && sv.q3_restartTime)
	{
		return;
	}

	if (GGameType & GAME_WolfMP)
	{
		// DHM - Nerve :: Check for invalid gametype
		svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);
		int nextgt = svt3_gametype->integer;

		wmsharedEntity_t* world = SVWM_GentityNum(Q3ENTITYNUM_WORLD);
		int worldspawnflags = world->r.worldflags;
		if  ((nextgt == WMGT_WOLF && (worldspawnflags & 1)) ||
			(nextgt == WMGT_WOLF_STOPWATCH && (worldspawnflags & 2)) ||
			((nextgt == WMGT_WOLF_CP || nextgt == WMGT_WOLF_CPH) && (worldspawnflags & 4)))
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
	}

	int delay;
	if (Cmd_Argc() > 1)
	{
		delay = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		if (GGameType & (GAME_WolfMP | GAME_ET) ||
			(GGameType & GAME_WolfSP && svt3_gametype->integer == Q3GT_SINGLE_PLAYER))		// (SA) no pause by default in sp
		{
			delay = 0;
		}
		else
		{
			delay = 5;
		}
	}

	if (delay && (GGameType & (GAME_WolfMP | GAME_ET) || !Cvar_VariableValue("g_doWarmup")))
	{
		sv.q3_restartTime = svs.q3_time + delay * 1000;
		SVT3_SetConfigstring(Q3CS_WARMUP, va("%i", sv.q3_restartTime));
		return;
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// NERVE - SMF - read in gamestate or just default to GS_PLAYING
		gamestate_t old_gs = (gamestate_t) String::Atoi(Cvar_VariableString("gamestate"));

		gamestate_t new_gs;
		if (GGameType & GAME_ET && (SVET_GameIsSinglePlayer() || SVET_GameIsCoop()))
		{
			new_gs = GS_PLAYING;
		}
		else
		{
			if (Cmd_Argc() > 2)
			{
				new_gs = (gamestate_t) String::Atoi(Cmd_Argv(2));
			}
			else
			{
				new_gs = GS_PLAYING;
			}
		}

		if (!SVT3_TransitionGameState(new_gs, old_gs, delay))
		{
			return;
		}
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if (sv_maxclients->modified || (GGameType & (GAME_Quake3 | GAME_WolfSP) && svt3_gametype->modified))
	{
		char mapname[MAX_QPATH];

		common->Printf("variable change -- restarting.\n");
		// restart the map the slow way
		String::NCpyZ(mapname, Cvar_VariableString("mapname"), sizeof(mapname));

		SVT3_SpawnServer(mapname, false);
		return;
	}

	// Check for loading a saved game
	if (GGameType & (GAME_WolfSP | GAME_ET) && Cvar_VariableIntegerValue("savegame_loading"))
	{
		// open the current savegame, and find out what the time is, everything else we can ignore
		char savemap[MAX_QPATH];
		byte* buffer;
		int size, savegameTime;

		if (GGameType & GAME_ET)
		{
			if (comet_gameInfo.usesProfiles)
			{
				const char* cl_profileStr = Cvar_VariableString("cl_profile");
				String::Sprintf(savemap, sizeof(savemap), "profiles/%s/save/current.sav", cl_profileStr);
			}
			else
			{
				String::NCpyZ(savemap, "save/current.sav", sizeof(savemap));
			}
		}
		else
		{
			String::NCpyZ(savemap, "save/current.svg", sizeof(savemap));
		}

		size = FS_ReadFile(savemap, NULL);
		if (size < 0)
		{
			common->Printf("Can't find savegame %s\n", savemap);
			return;
		}

		FS_ReadFile(savemap, (void**)&buffer);

		// the mapname is at the very start of the savegame file
		savegameTime = *(int*)(buffer + sizeof(int) + MAX_QPATH);

		if (savegameTime >= 0)
		{
			svs.q3_time = savegameTime;
		}

		FS_FreeFile(buffer);
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.q3_snapFlagServerBit ^= Q3SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	if (GGameType & GAME_WolfSP)
	{
		sv.q3_restartedServerId = sv.q3_serverId;
	}
	sv.q3_serverId = com_frameTime;
	Cvar_Set("sv_serverid", va("%i", sv.q3_serverId));

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.q3_restarting = true;

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("sv_serverRestarting", "1");
	}

	SVT3_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (int i = 0; i < (GGameType & GAME_ET ? ETGAME_INIT_FRAMES : 3); i++)
	{
		SVT3_GameRunFrame(svs.q3_time);
		svs.q3_time += Q3FRAMETIME;
	}

	sv.state = SS_GAME;
	sv.q3_restarting = false;

	// connect and begin all the clients
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t* client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if (client->state < CS_CONNECTED)
		{
			continue;
		}

		bool isBot;
		if (client->netchan.remoteAddress.type == NA_BOT)
		{
			if (GGameType & GAME_ET && (SVET_GameIsSinglePlayer() || SVET_GameIsCoop()))
			{
				continue;	// dont carry across bots in single player
			}
			isBot = true;
		}
		else
		{
			isBot = false;
		}

		// add the map_restart command
		SVT3_AddServerCommand(client, "map_restart\n");

		// connect the client again, without the firstTime flag
		const char* denied = SVT3_GameClientConnect(i, false, isBot);
		if (denied)
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SVT3_DropClient(client, denied);
			if (!(GGameType & GAME_ET) || !SVET_GameIsSinglePlayer() || !isBot)
			{
				common->Printf("SVT3_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);		// bk010125
			}
			continue;
		}

		client->state = CS_ACTIVE;

		if (GGameType & GAME_Quake3)
		{
			SVQ3_ClientEnterWorld(client, &client->q3_lastUsercmd);
		}
		else if (GGameType & GAME_WolfSP)
		{
			SVWS_ClientEnterWorld(client, &client->ws_lastUsercmd);
		}
		else if (GGameType & GAME_WolfMP)
		{
			SVWM_ClientEnterWorld(client, &client->wm_lastUsercmd);
		}
		else
		{
			SVET_ClientEnterWorld(client, &client->et_lastUsercmd);
		}
	}

	// run another frame to allow things to look at all the players
	SVT3_GameRunFrame(svs.q3_time);
	svs.q3_time += Q3FRAMETIME;

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("sv_serverRestarting", "0");
	}
}

static void SVT3_LoadGame_f()
{
	if (!(GGameType & GAME_WolfMP))
	{
		// dont allow command if another loadgame is pending
		if (Cvar_VariableIntegerValue("savegame_loading"))
		{
			return;
		}
		if (svt3_reloading->integer)
		{
			return;
		}
	}

	char filename[MAX_QPATH];
	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	if (!filename[0])
	{
		common->Printf("You must specify a savegame to load\n");
		return;
	}

	const char* cl_profileStr = Cvar_VariableString("cl_profile");
	char savedir[MAX_QPATH];
	if (GGameType & GAME_ET && comet_gameInfo.usesProfiles && cl_profileStr[0])
	{
		String::Sprintf(savedir, sizeof(savedir), "profiles/%s/save/", cl_profileStr);
	}
	else
	{
		String::NCpyZ(savedir, "save/", sizeof(savedir));
	}
	const char* saveExtension = GGameType & GAME_ET ? "sav" : "svg";

	if (GGameType & GAME_ET)
	{
		// go through a va to avoid vsnprintf call with same source and target
		String::NCpyZ(filename, va("%s%s", savedir, filename), sizeof(filename));
	}
	else
	{
		if (String::NCmp(filename, "save/", 5) && String::NCmp(filename, "save\\", 5))
		{
			String::NCpyZ(filename, va("save/%s", filename), sizeof(filename));
		}
	}

	// enforce .svg extension
	if (!strstr(filename, ".") || String::NCmp(strstr(filename, ".") + 1, saveExtension, 3))
	{
		String::Cat(filename, sizeof(filename), ".");
		String::Cat(filename, sizeof(filename), saveExtension);
	}
	// use '/' instead of '\' for directories
	while (strstr(filename, "\\"))
	{
		*(char*)strstr(filename, "\\") = '/';
	}

	int size = FS_ReadFile(filename, NULL);
	if (size < 0)
	{
		common->Printf("Can't find savegame %s\n", filename);
		return;
	}

	char* buffer;
	FS_ReadFile(filename, (void**)&buffer);

	// read the mapname, if it is the same as the current map, then do a fast load
	char mapname[MAX_QPATH];
	String::Sprintf(mapname, sizeof(mapname), buffer + sizeof(int));

	if (com_sv_running->integer && (com_frameTime != sv.q3_serverId))
	{
		// check mapname
		if (!String::ICmp(mapname, svt3_mapname->string))			// same

		{
			if (String::ICmp(filename, va("%scurrent.%s", savedir, saveExtension)) != 0)
			{
				// copy it to the current savegame file
				FS_WriteFile(va("%scurrent.%s", savedir, saveExtension), buffer, size);
			}

			FS_FreeFile(buffer);

			Cvar_Set("savegame_loading", "2");		// 2 means it's a restart, so stop rendering until we are loaded
			// set the filename
			Cvar_Set("savegame_filename", filename);
			// quick-restart the server
			SVT3_MapRestart_f();	// savegame will be loaded after restart

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
static void SVT3_Ban_f()
{
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

	client_t* cl = SVT3_GetPlayerByName();

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

//	Ban a user from being able to play on this server through the auth server
static void SVT3_BanNum_f()
{
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

	client_t* cl = SVT3_GetPlayerByNum();
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

static void SVT3_KillServer_f()
{
	SVT3_Shutdown("killserver");
}

static void SVT3_GameCompleteStatus_f()
{
	SVT3_MasterGameCompleteStatus();
}

void SVT3_AddOperatorCommands()
{
	static bool initialized;

	if (initialized)
	{
		return;
	}
	initialized = true;

	Cmd_AddCommand("heartbeat", SVT3_Heartbeat_f);
	Cmd_AddCommand("status", SVT3_Status_f);
	Cmd_AddCommand("serverinfo", SVT3_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SVT3_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SVT3_DumpUser_f);
	Cmd_AddCommand("map_restart", SVT3_MapRestart_f);
	Cmd_AddCommand("sectorlist", SVT3_SectorList_f);
	Cmd_AddCommand("map", SVT3_Map_f);
	Cmd_AddCommand("devmap", SVT3_Map_f);
	Cmd_AddCommand("spmap", SVT3_Map_f);
	Cmd_AddCommand("spdevmap", SVT3_Map_f);
	Cmd_AddCommand("killserver", SVT3_KillServer_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SVT3_ConSay_f);
	}
	if (!(GGameType & GAME_ET))
	{
		Cmd_AddCommand("kick", SVT3_Kick_f);
		Cmd_AddCommand("banUser", SVT3_Ban_f);
		Cmd_AddCommand("banClient", SVT3_BanNum_f);
		Cmd_AddCommand("clientkick", SVT3_KickNum_f);
	}
	if (!(GGameType & GAME_Quake3))
	{
		Cmd_AddCommand("loadgame", SVT3_LoadGame_f);
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cmd_AddCommand("gameCompleteStatus", SVT3_GameCompleteStatus_f);
	}
	if (GGameType & GAME_ET)
	{
		Cmd_AddCommand("fieldinfo", SVET_FieldInfo_f);
	}
}
