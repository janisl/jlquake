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

//	Also called by SVT3_DropClient, SV_DirectConnect, and SV_SpawnServer
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
client_t* SVT3_GetPlayerByName()
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
client_t* SVT3_GetPlayerByNum()
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
void SVT3_Kick_f()
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
void SVT3_KickNum_f()
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

void SVT3_Status_f()
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
		if (GGameType & GAME_WolfSP)
		{
			wsplayerState_t* ps = SVWS_GameClientNum(i);
			common->Printf("%5i ", ps->persistant[WSPERS_SCORE]);
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmplayerState_t* ps = SVWM_GameClientNum(i);
			common->Printf("%5i ", ps->persistant[WMPERS_SCORE]);
		}
		else if (GGameType & GAME_ET)
		{
			etplayerState_t* ps = SVET_GameClientNum(i);
			common->Printf("%5i ", ps->persistant[ETPERS_SCORE]);
		}
		else
		{
			q3playerState_t* ps = SVQ3_GameClientNum(i);
			common->Printf("%5i ", ps->persistant[Q3PERS_SCORE]);
		}

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

void SVT3_ConSay_f()
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
void SVT3_Serverinfo_f()
{
	common->Printf("Server info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
}

void SVT3_Systeminfo_f()
{
	common->Printf("System info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SYSTEMINFO, MAX_INFO_STRING_Q3));
}

//	Examine all a users info strings FIXME: move to game
void SVT3_DumpUser_f()
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

bool SVT3_TransitionGameState(gamestate_t new_gs, gamestate_t old_gs, int delay)
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

void SVET_FieldInfo_f()
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
