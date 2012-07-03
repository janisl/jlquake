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

//	clear/free any download vars
void SVT3_CloseDownload(client_t* cl)
{
	// EOF
	if (cl->download)
	{
		FS_FCloseFile(cl->download);
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (int i = 0; i < MAX_DOWNLOAD_WINDOW; i++)
	{
		if (cl->downloadBlocks[i])
		{
			Mem_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}

}

//	Called when the player is totally leaving the server, either willingly
// or unwillingly.  This is NOT called if the entire server is quiting
// or crashing -- SV_FinalMessage() will handle that
void SVT3_DropClient(client_t* drop, const char* reason)
{
	int i;
	challenge_t* challenge;

	if (drop->state == CS_ZOMBIE)
	{
		return;		// already dropped
	}

	bool isBot = (drop->q3_entity && (drop->q3_entity->GetSvFlags() & Q3SVF_BOT)) ||
		drop->netchan.remoteAddress.type == NA_BOT;

	if (!isBot && !(GGameType & GAME_WolfSP))
	{
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[0];

		for (i = 0; i < MAX_CHALLENGES; i++, challenge++)
		{
			if (SOCK_CompareAdr(drop->netchan.remoteAddress, challenge->adr))
			{
				challenge->connected = false;
				break;
			}
		}
	}

	// Kill any download
	SVT3_CloseDownload(drop);

	// tell everyone why they got dropped
	if (GGameType & GAME_WolfSP)
	{
		// Ridah, no need to tell the player if an AI drops
		if (!(drop->q3_entity && drop->q3_entity->GetSvFlagCastAI()))
		{
			SVT3_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVT3_SendServerCommand(NULL, "print \"[lof]%s" S_COLOR_WHITE " [lon]%s\n\"", drop->name, reason);
	}
	else if (GGameType & GAME_ET)
	{
		if ((!SVET_GameIsSinglePlayer()) || (!isBot))
		{
			// Gordon: we want this displayed elsewhere now
			SVT3_SendServerCommand(NULL, "cpm \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
		}
	}
	else
	{
		SVT3_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
	}

	common->DPrintf("Going to CS_ZOMBIE for %s\n", drop->name);
	drop->state = CS_ZOMBIE;		// become free in a few seconds

	if (drop->download)
	{
		FS_FCloseFile(drop->download);
		drop->download = 0;
	}

	// call the prog function for removing a client
	// this will remove the body, among other things
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameClientDisconnect(drop);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameClientDisconnect(drop);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameClientDisconnect(drop);
	}
	else
	{
		SVQ3_GameClientDisconnect(drop);
	}

	if (!(GGameType & GAME_WolfSP) || !(drop->q3_entity && drop->q3_entity->GetSvFlagCastAI()))
	{
		// add the disconnect command
		SVT3_SendServerCommand(drop, "disconnect \"%s\"", reason);
	}

	if (drop->netchan.remoteAddress.type == NA_BOT)
	{
		SVT3_BotFreeClient(drop - svs.clients);
	}

	// nuke user info
	SVT3_SetUserinfo(drop - svs.clients, "");

	if (GGameType & GAME_WolfSP)
	{
		// RF, nuke reliable commands
		SVWS_FreeReliableCommandsForClient(drop);
	}

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			break;
		}
	}
	if (i == sv_maxclients->integer)
	{
		SVT3_Heartbeat_f();
	}
}
