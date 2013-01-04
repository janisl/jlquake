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

#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../botlib/local.h"
#include "../public.h"
#include "../../common/common_defs.h"

int bot_enable;

int SVT3_BotAllocateClient(int clientNum)
{
	int i;
	client_t* cl;
	// Arnout: added possibility to request a clientnum
	if (clientNum > 0)
	{
		if (clientNum >= sv_maxclients->integer)
		{
			return -1;
		}

		cl = &svs.clients[clientNum];
		if (cl->state != CS_FREE)
		{
			return -1;
		}
		else
		{
			i = clientNum;
		}
	}
	else
	{
		// find a client slot
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
			if (!(GGameType & GAME_Quake3) && i < 1)
			{
				continue;
			}
			// done.
			if (cl->state == CS_FREE)
			{
				break;
			}
		}
	}

	if (i == sv_maxclients->integer)
	{
		return -1;
	}

	cl->q3_entity = SVT3_EntityNum(i);
	if (GGameType & GAME_WolfSP)
	{
		cl->ws_gentity = SVWS_GentityNum(i);
	}
	else if (GGameType & GAME_WolfMP)
	{
		cl->wm_gentity = SVWM_GentityNum(i);
	}
	else if (GGameType & GAME_ET)
	{
		cl->et_gentity = SVET_GentityNum(i);
	}
	else
	{
		cl->q3_gentity = SVQ3_GentityNum(i);
	}
	cl->q3_entity->SetNumber(i);
	cl->state = CS_ACTIVE;
	cl->q3_lastPacketTime = svs.q3_time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

void SVT3_BotFreeClient(int clientNum)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		common->Error("SVT3_BotFreeClient: bad clientNum: %i", clientNum);
	}
	client_t* cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if (cl->q3_entity)
	{
		cl->q3_entity->SetSvFlags(cl->q3_entity->GetSvFlags() & ~Q3SVF_BOT);
	}
}

int SVT3_BotLibSetup()
{
	if (!bot_enable)
	{
		return 0;
	}

	if (GGameType & GAME_ET)
	{
		static Cvar* bot_norcd;
		static Cvar* bot_frameroutingupdates;

		// RF, set RCD calculation status
		bot_norcd = Cvar_Get("bot_norcd", "0", 0);
		BotLibVarSet("bot_norcd", bot_norcd->string);

		// RF, set AAS routing max per frame
		if (SVET_GameIsSinglePlayer())
		{
			bot_frameroutingupdates = Cvar_Get("bot_frameroutingupdates", "9999999", 0);
		}
		else
		{
			// more restrictive in multiplayer
			bot_frameroutingupdates = Cvar_Get("bot_frameroutingupdates", "1000", 0);
		}
		BotLibVarSet("bot_frameroutingupdates", bot_frameroutingupdates->string);

		// added single player
		return BotLibSetup((SVET_GameIsSinglePlayer() || SVET_GameIsCoop()));
	}
	else
	{
		return BotLibSetup(false);
	}
}

void SVT3_BotInitCvars()
{
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Get("bot_enable", "0", 0);					//enable the bot
	}
	else
	{
		Cvar_Get("bot_enable", "1", 0);					//enable the bot
	}
	Cvar_Get("bot_developer", "0", CVAR_CHEAT);			//bot developer mode
	Cvar_Get("bot_debug", "0", CVAR_CHEAT);				//enable bot debugging
	Cvar_Get("bot_groundonly", "1", 0);					//only show ground faces of areas
	Cvar_Get("bot_reachability", "0", 0);				//show all reachabilities to other areas
	if (GGameType & GAME_ET)
	{
		Cvar_Get("bot_thinktime", "50", CVAR_CHEAT);	//msec the bots thinks
		Cvar_Get("bot_nochat", "1", 0);					//disable chats
		Cvar_Get("bot_rocketjump", "0", 0);				//enable rocket jumping
		Cvar_Get("bot_norcd", "0", 0);					//enable creation of RCD file
	}
	else
	{
		Cvar_Get("bot_thinktime", "100", CVAR_CHEAT);	//msec the bots thinks
		Cvar_Get("bot_nochat", "0", 0);					//disable chats
		Cvar_Get("bot_rocketjump", "1", 0);				//enable rocket jumping
	}
	Cvar_Get("bot_reloadcharacters", "0", 0);			//reload the bot characters each time
	Cvar_Get("bot_testichat", "0", 0);					//test ichats
	Cvar_Get("bot_testrchat", "0", 0);					//test rchats
	Cvar_Get("bot_fastchat", "0", 0);					//fast chatting bots
	Cvar_Get("bot_grapple", "0", 0);					//enable grapple
	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		Cvar_Get("bot_miniplayers", "0", 0);			//minimum players in a team or the game
	}
	if (GGameType & GAME_Quake3)
	{
		Cvar_Get("bot_minplayers", "0", 0);					//minimum players in a team or the game
		Cvar_Get("bot_visualizejumppads", "0", CVAR_CHEAT);	//show jumppads
		Cvar_Get("bot_forceclustering", "0", 0);			//force cluster calculations
		Cvar_Get("bot_forcereachability", "0", 0);			//force reachability calculations
		Cvar_Get("bot_forcewrite", "0", 0);					//force writing aas file
		Cvar_Get("bot_aasoptimize", "0", 0);				//no aas file optimisation
		Cvar_Get("bot_saveroutingcache", "0", 0);			//save routing cache
		Cvar_Get("bot_maxdebugpolys", "2", 0);				//maximum number of debug polys
		Cvar_Get("bot_testsolid", "0", CVAR_CHEAT);			//test for solid areas
		Cvar_Get("bot_testclusters", "0", CVAR_CHEAT);		//test the AAS clusters
		Cvar_Get("bot_pause", "0", CVAR_CHEAT);				//pause the bots thinking
		Cvar_Get("bot_report", "0", CVAR_CHEAT);			//get a full report in ctf
		Cvar_Get("bot_challenge", "0", 0);					//challenging bot
		Cvar_Get("bot_interbreedchar", "", CVAR_CHEAT);		//bot character used for interbreeding
		Cvar_Get("bot_interbreedbots", "10", CVAR_CHEAT);	//number of bots used for interbreeding
		Cvar_Get("bot_interbreedcycle", "20", CVAR_CHEAT);	//bot interbreeding cycle
		Cvar_Get("bot_interbreedwrite", "", CVAR_CHEAT);	//write interbreeded bots to this file
	}

	if (GGameType & GAME_ET)
	{
		bot_enable = Cvar_VariableIntegerValue("bot_enable");
	}
}

void SVT3_BotInitBotLib()
{
	if (debugpolygons)
	{
		Mem_Free(debugpolygons);
	}
	bot_maxdebugpolys = GGameType & GAME_Quake3 ? Cvar_VariableIntegerValue("bot_maxdebugpolys") : 128;
	debugpolygons = (bot_debugpoly_t*)Mem_Alloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys);
}

bool SVT3_BotGetConsoleMessage(int client, char* buf, int size)
{
	client_t* cl = &svs.clients[client];
	cl->q3_lastPacketTime = svs.q3_time;

	if (cl->q3_reliableAcknowledge == cl->q3_reliableSequence)
	{
		return false;
	}

	cl->q3_reliableAcknowledge++;
	int index = cl->q3_reliableAcknowledge & ((GGameType & GAME_Quake3 ?
		MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF) - 1);

	const char* msg = SVT3_GetReliableCommand(cl, index);
	if (!msg || !msg[0])
	{
		return false;
	}

	if (!(GGameType & GAME_ET))
	{
		String::NCpyZ(buf, msg, size);
	}
	return true;
}

int SVT3_BotGetSnapshotEntity(int client, int sequence)
{
	client_t* cl = &svs.clients[client];
	q3clientSnapshot_t* frame = &cl->q3_frames[cl->netchan.outgoingSequence & PACKET_MASK_Q3];
	if (sequence < 0 || sequence >= frame->num_entities)
	{
		return -1;
	}
	if (GGameType & GAME_WolfSP)
	{
		return svs.ws_snapshotEntities[(frame->first_entity + sequence) % svs.q3_numSnapshotEntities].number;
	}
	if (GGameType & GAME_WolfMP)
	{
		return svs.wm_snapshotEntities[(frame->first_entity + sequence) % svs.q3_numSnapshotEntities].number;
	}
	if (GGameType & GAME_ET)
	{
		return svs.et_snapshotEntities[(frame->first_entity + sequence) % svs.q3_numSnapshotEntities].number;
	}
	return svs.q3_snapshotEntities[(frame->first_entity + sequence) % svs.q3_numSnapshotEntities].number;
}

void SVT3_BotFrame(int time)
{
	if (!bot_enable)
	{
		return;
	}
	//NOTE: maybe the game is already shutdown
	if (!gvm)
	{
		return;
	}
	if (GGameType & GAME_WolfSP)
	{
		SVWS_BotFrame(time);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_BotFrame(time);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_BotFrame(time);
	}
	else
	{
		SVQ3_BotFrame(time);
	}
}

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value)
{
	static Cvar* bot_debug, * bot_highlightarea;
	bot_debugpoly_t* poly;
	int i;

	if (!bot_enable)
	{
		return;
	}
	if (!debugpolygons)
	{
		return;
	}
	//bot debugging
	if (!bot_debug)
	{
		bot_debug = Cvar_Get("bot_debug", "0", 0);
	}
	//get the hightlight area
	if (!bot_highlightarea)
	{
		bot_highlightarea = Cvar_Get("bot_highlightarea", "0", 0);
	}

	if (bot_debug->integer)
	{
		BotLibVarSet("bot_highlightarea", bot_highlightarea->string);
	}
	//draw all debug polys
	for (i = 0; i < bot_maxdebugpolys; i++)
	{
		poly = &debugpolygons[i];
		if (!poly->inuse)
		{
			continue;
		}
		drawPoly(poly->color, poly->numPoints, (float*)poly->points);
	}
}
