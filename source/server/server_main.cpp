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

#include "server.h"
#include "quake_hexen/local.h"
#include "quake2/local.h"
#include "tech3/local.h"

serverStatic_t svs;					// persistant server info
server_t sv;						// local server

Cvar* sv_maxclients;

netadr_t master_adr[MAX_MASTERS];		// address of group servers

static ucmd_t* SV_GetUserCommands()
{
	if (GGameType & GAME_Quake)
	{
		if (GGameType & GAME_QuakeWorld)
		{
			return qw_ucmds;
		}
		return q1_ucmds;
	}
	if (GGameType & GAME_Hexen2)
	{
		if (GGameType & GAME_HexenWorld)
		{
			return hw_ucmds;
		}
		return h2_ucmds;
	}
	if (GGameType & GAME_Quake2)
	{
		return q2_ucmds;
	}
	if (GGameType & GAME_ET)
	{
		return et_ucmds;
	}
	return q3_ucmds;
}

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s, !!(GGameType & GAME_Quake2));

	// see if it is a server level command
	for (ucmd_t* u = SV_GetUserCommands(); u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			if (preMapRestart && !u->allowedpostmapchange)
			{
				continue;
			}

			u->func(cl);
			return;
		}
	}

	if (GGameType & GAME_QuakeHexen)
	{
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			NET_OutOfBandPrint(NS_SERVER, cl->netchan.remoteAddress, "Bad user command: %s\n", Cmd_Argv(0));
		}
		else
		{
			common->DPrintf("%s tried to %s\n", cl->name, s);
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		if (sv.state == SS_GAME)
		{
			ge->ClientCommand(cl->q2_edict);
		}
	}
	else
	{
		if (clientOK)
		{
			// pass unknown strings to the game
			if (sv.state == SS_GAME)
			{
				SVT3_GameClientCommand(cl - svs.clients);
			}
		}
		else
		{
			common->DPrintf("client text ignored for %s: %s\n", cl->name, Cmd_Argv(0));
		}
	}
}

bool SV_IsServerActive()
{
	return sv.state == SS_GAME;
}

void SV_CvarChanged(Cvar* var)
{
	if (!(GGameType & GAME_QuakeHexen))
	{
		return;
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		if (var->flags & CVAR_SERVERINFO && var->name[0] != '*')
		{
			Info_SetValueForKey(svs.qh_info, var->name, var->string, MAX_SERVERINFO_STRING,
				64, 64, !svqh_highchars || !svqh_highchars->value, false);
			if (sv.state != SS_DEAD)
			{
				if (GGameType & GAME_HexenWorld)
				{
					SVQH_BroadcastCommand("fullserverinfo \"%s\"\n", svs.qh_info);
				}
				else
				{
					SVQW_SendServerInfoChange(var->name, var->string);
				}
			}
		}
	}
	else
	{
		if ((var->flags & CVAR_SERVERINFO))
		{
			if (sv.state != SS_DEAD)
			{
				SVQH_BroadcastPrintf(0, "\"%s\" changed to \"%s\"\n", var->name, var->string);
			}
		}
	}
}

int SVQH_GetMaxClients()
{
	return svs.qh_maxclients;
}

int SVQH_GetMaxClientsLimit()
{
	return svs.qh_maxclientslimit;
}

void SV_Init()
{
	if (GGameType & GAME_QuakeHexen)
	{
		SVQH_Init();
	}
	else if (GGameType & GAME_Quake2)
	{
		SVQ2_Init();
	}
	else
	{
		SVT3_Init();
	}
}

void SV_Shutdown(const char* finalMessage)
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQHW_Shutdown(finalMessage);
	}
	else if (GGameType & GAME_QuakeHexen)
	{
		SVQH_Shutdown();
	}
	else if (GGameType & GAME_Quake2)
	{
		SVQ2_Shutdown(finalMessage, false);
	}
	else
	{
		SVT3_Shutdown(finalMessage);
	}
}

void SV_Frame(int msec)
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQHW_ServerFrame(msec);
	}
	else if (GGameType & GAME_QuakeHexen)
	{
		SVQH_ServerFrame(msec * 0.001);
	}
	else if (GGameType & GAME_Quake2)
	{
		SVQ2_Frame(msec);
	}
	else
	{
		SVT3_Frame(msec);
	}
}
