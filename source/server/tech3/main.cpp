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

Cvar* get_gameType;

//	Converts newlines to "\n" so a line prints nicer
static const char* SVT3_ExpandNewlines(const char* in)
{
	static char string[1024];
	int l = 0;
	while (*in && l < (int)sizeof(string) - 3)
	{
		if (*in == '\n')
		{
			string[l++] = '\\';
			string[l++] = 'n';
		}
		else
		{
			// NERVE - SMF - HACK - strip out localization tokens before string command is displayed in syscon window
			if (GGameType & (GAME_WolfMP | GAME_ET) &&
				(!String::NCmp(in, "[lon]", 5) || !String::NCmp(in, "[lof]", 5)))
			{
				in += 5;
				continue;
			}

			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

//	The given command will be transmitted to the client, and is guaranteed to
// not have future snapshot_t executed before it is executed
void SVT3_AddServerCommand(client_t* client, const char* cmd)
{
	client->q3_reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SVT3_DropClient()
	// doesn't cause a recursive drop client
	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	if (client->q3_reliableSequence - client->q3_reliableAcknowledge == maxReliableCommands + 1)
	{
		common->Printf("===== pending server commands =====\n");
		int i;
		for (i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
		{
			common->Printf("cmd %5d: %s\n", i, SVT3_GetReliableCommand(client, i & (maxReliableCommands - 1)));
		}
		common->Printf("cmd %5d: %s\n", i, cmd);
		SVT3_DropClient(client, "Server command overflow");
		return;
	}
	int index = client->q3_reliableSequence & (maxReliableCommands - 1);
	SVT3_AddReliableCommand(client, index, cmd);
}

//	Sends a reliable command string to be interpreted by
// the client game module: "cp", "print", "chat", etc
// A NULL client will broadcast to all clients
void SVT3_SendServerCommand(client_t* cl, const char* fmt, ...)
{
	va_list argptr;
	byte message[MAX_MSGLEN];
	va_start(argptr,fmt);
	Q_vsnprintf((char*)message, sizeof(message), fmt, argptr);
	va_end(argptr);

	// do not forward server command messages that would be too big to clients
	// ( q3infoboom / q3msgboom stuff )
	if (GGameType & (GAME_WolfMP | GAME_ET) && String::Length((char*)message) > 1022)
	{
		return;
	}

	if (cl != NULL)
	{
		SVT3_AddServerCommand(cl, (char*)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !String::NCmp((char*)message, "print", 5))
	{
		common->Printf("broadcast: %s\n", SVT3_ExpandNewlines((char*)message));
	}

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < sv_maxclients->integer; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// Ridah, don't need to send messages to AI
		if (client->q3_entity && client->q3_entity->GetSvFlagCastAI())
		{
			continue;
		}
		if (GGameType & GAME_ET && client->q3_entity && client->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			continue;
		}
		SVT3_AddServerCommand(client, (char*)message);
	}
}
