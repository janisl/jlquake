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

Cvar* allow_download;
Cvar* allow_download_players;
Cvar* allow_download_models;
Cvar* allow_download_sounds;
Cvar* allow_download_maps;

Cvar* svq2_enforcetime;

//	Builds the string that is sent as heartbeats and status replies
const char* SVQ2_StatusString()
{
	static char status[MAX_MSGLEN_Q2 - 16];

	String::Cpy(status, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false));
	String::Cat(status, sizeof(status), "\n");
	int statusLength = String::Length(status);

	for (int i = 0; i < sv_maxclients->value; i++)
	{
		client_t* cl = &svs.clients[i];
		if (cl->state == CS_CONNECTED || cl->state == CS_ACTIVE)
		{
			char player[1024];
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				cl->q2_edict->client->ps.stats[Q2STAT_FRAGS], cl->ping, cl->name);
			int playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				// can't hold any more
				break;
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}
