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
#include "../../client/public.h"
#include "../worldsector.h"
#include "../public.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/event_queue.h"
#include "../../common/system.h"

#define RELIABLE_COMMANDS_CHARS     384		// we can scale this down from the max of 1024, since not all commands are going to use that many chars

bool SVT3_AddReliableCommand(client_t* cl, int index, const char* cmd)
{
	if (!(GGameType & GAME_WolfSP))
	{
		String::NCpyZ(cl->q3_reliableCommands[index], cmd, sizeof(cl->q3_reliableCommands[index]));
		return true;
	}

	if (!cl->ws_reliableCommands.bufSize)
	{
		return false;
	}

	int length = String::Length(cmd);

	if ((cl->ws_reliableCommands.rover - cl->ws_reliableCommands.buf) + length + 1 >= cl->ws_reliableCommands.bufSize)
	{
		// go back to the start
		cl->ws_reliableCommands.rover = cl->ws_reliableCommands.buf;
	}

	// make sure this position won't overwrite another command
	int i;
	char* ch;
	for (i = length, ch = cl->ws_reliableCommands.rover; i && !*ch; i--, ch++)
	{
		// keep going until we find a bad character, or enough space is found
	}
	// if the test failed
	if (i)
	{
		// find a valid spot to place the new string
		// start at the beginning (keep it simple)
		for (i = 0, ch = cl->ws_reliableCommands.buf; i < cl->ws_reliableCommands.bufSize; i++, ch++)
		{
			if (!*ch && (!i || !*(ch - 1)))			// make sure we dont start at the terminator of another string
			{
				// see if this is the start of a valid segment
				int j;
				char* ch2;
				for (ch2 = ch, j = 0; i < cl->ws_reliableCommands.bufSize - 1 && j < length + 1 && !*ch2; i++, ch2++, j++)
				{
					// loop
				}
				//
				if (j == length + 1)
				{
					// valid segment found
					cl->ws_reliableCommands.rover = ch;
					break;
				}
				//
				if (i == cl->ws_reliableCommands.bufSize - 1)
				{
					// ran out of room, not enough space for string
					return false;
				}
				//
				ch = &cl->ws_reliableCommands.buf[i];	// continue where ch2 left off
			}
		}
	}

	// insert the command at the rover
	cl->ws_reliableCommands.commands[index] = cl->ws_reliableCommands.rover;
	String::NCpyZ(cl->ws_reliableCommands.commands[index], cmd, length + 1);
	cl->ws_reliableCommands.commandLengths[index] = length;

	// move the rover along
	cl->ws_reliableCommands.rover += length + 1;

	return true;
}

const char* SVT3_GetReliableCommand(client_t* cl, int index)
{
	if (!(GGameType & GAME_WolfSP))
	{
		return cl->q3_reliableCommands[index];
	}

	static const char* nullStr = "";
	if (!cl->ws_reliableCommands.bufSize)
	{
		return nullStr;
	}

	if (!cl->ws_reliableCommands.commandLengths[index])
	{
		return nullStr;
	}

	return cl->ws_reliableCommands.commands[index];
}

static void SVT3_SendConfigStringToClients(int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;

	// send it to all the clients if we aren't
	// spawning a new server
	if (sv.state == SS_GAME || sv.q3_restarting)
	{
		// send the data to all relevent clients
		client_t* client = svs.clients;
		for (int i = 0; i < sv_maxclients->integer; i++, client++)
		{
			if (client->state < CS_PRIMED)
			{
				continue;
			}
			// do not always send server info to all clients
			if (index == Q3CS_SERVERINFO && client->q3_entity && (client->q3_entity->GetSvFlagNoServerInfo()))
			{
				continue;
			}

			// RF, don't send to bot/AI
			if (GGameType & GAME_WolfSP && client->q3_entity && client->q3_entity->GetSvFlagCastAI())
			{
				continue;
			}
			if (GGameType & GAME_WolfMP && svt3_gametype->integer == Q3GT_SINGLE_PLAYER &&
				client->q3_entity && client->q3_entity->GetSvFlagCastAI())
			{
				continue;
			}
			if (GGameType & GAME_ET && (SVET_GameIsSinglePlayer() || SVET_GameIsCoop()) &&
				client->q3_entity && (client->q3_entity->GetSvFlags() & Q3SVF_BOT))
			{
				continue;
			}

			const char* val = sv.q3_configstrings[index];
			int len = String::Length(val);
			if (len >= maxChunkSize)
			{
				int sent = 0;
				int remaining = len;
				while (remaining > 0)
				{
					const char* cmd;
					if (sent == 0)
					{
						cmd = "bcs0";
					}
					else if (remaining < maxChunkSize)
					{
						cmd = "bcs2";
					}
					else
					{
						cmd = "bcs1";
					}
					char buf[MAX_STRING_CHARS];
					String::NCpyZ(buf, &val[sent], maxChunkSize);

					SVT3_SendServerCommand(client, "%s %i \"%s\"\n", cmd, index, buf);

					sent += (maxChunkSize - 1);
					remaining -= (maxChunkSize - 1);
				}
			}
			else
			{
				// standard cs, just send it
				SVT3_SendServerCommand(client, "cs %i \"%s\"\n", index, val);
			}
		}
	}
}

void SVT3_SetConfigstring(int index, const char* val)
{
	if (index < 0 || index >= (GGameType & GAME_ET ? MAX_CONFIGSTRINGS_ET :
		GGameType & GAME_WolfMP ? MAX_CONFIGSTRINGS_WM :
		GGameType & GAME_WolfSP ? MAX_CONFIGSTRINGS_WS : MAX_CONFIGSTRINGS_Q3))
	{
		common->Error("SVT3_SetConfigstring: bad index %i\n", index);
	}

	if (!val)
	{
		val = "";
	}

	// don't bother broadcasting an update if no change
	if (!String::Cmp(val, sv.q3_configstrings[index]))
	{
		return;
	}

	// change the string in sv
	Mem_Free(sv.q3_configstrings[index]);
	sv.q3_configstrings[index] = __CopyString(val);

	if (GGameType & GAME_ET)
	{
		sv.et_configstringsmodified[index] = true;
	}
	else
	{
		SVT3_SendConfigStringToClients(index);
	}
}

void SVT3_SetConfigstringNoUpdate(int index, const char* val)
{
	if (index < 0 || index >= (GGameType & GAME_ET ? MAX_CONFIGSTRINGS_ET :
		GGameType & GAME_WolfMP ? MAX_CONFIGSTRINGS_WM :
		GGameType & GAME_WolfSP ? MAX_CONFIGSTRINGS_WS : MAX_CONFIGSTRINGS_Q3))
	{
		common->Error("SVT3_SetConfigstring: bad index %i\n", index);
	}

	if (!val)
	{
		val = "";
	}

	// don't bother broadcasting an update if no change
	if (!String::Cmp(val, sv.q3_configstrings[index]))
	{
		return;
	}

	// change the string in sv
	Mem_Free(sv.q3_configstrings[index]);
	sv.q3_configstrings[index] = __CopyString(val);
}

void SVET_UpdateConfigStrings()
{
	for (int index = 0; index < MAX_CONFIGSTRINGS_ET; index++)
	{

		if (!sv.et_configstringsmodified[index])
		{
			continue;
		}
		sv.et_configstringsmodified[index] = false;

		SVT3_SendConfigStringToClients(index);
	}
}

void SVT3_GetConfigstring(int index, char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		common->Error("SVT3_GetConfigstring: bufferSize == %i", bufferSize);
	}
	if (index < 0 || index >= (GGameType & GAME_ET ? MAX_CONFIGSTRINGS_ET :
		GGameType & GAME_WolfMP ? MAX_CONFIGSTRINGS_WM :
		GGameType & GAME_WolfSP ? MAX_CONFIGSTRINGS_WS : MAX_CONFIGSTRINGS_Q3))
	{
		common->Error("SVT3_GetConfigstring: bad index %i\n", index);
	}
	if (!sv.q3_configstrings[index])
	{
		buffer[0] = 0;
		return;
	}

	String::NCpyZ(buffer, sv.q3_configstrings[index], bufferSize);
}

void SVT3_SetUserinfo(int index, const char* val)
{
	if (index < 0 || index >= sv_maxclients->integer)
	{
		common->Error("SVT3_SetUserinfo: bad index %i\n", index);
	}

	if (!val)
	{
		val = "";
	}

	String::NCpyZ(svs.clients[index].userinfo, val, MAX_INFO_STRING_Q3);
	String::NCpyZ(svs.clients[index].name, Info_ValueForKey(val, "name"), sizeof(svs.clients[index].name));
}

void SVT3_GetUserinfo(int index, char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		common->Error("SVT3_GetUserinfo: bufferSize == %i", bufferSize);
	}
	if (index < 0 || index >= sv_maxclients->integer)
	{
		common->Error("SVT3_GetUserinfo: bad index %i\n", index);
	}
	String::NCpyZ(buffer, svs.clients[index].userinfo, bufferSize);
}

//	Entity baselines are used to compress non-delta messages
// to the clients -- only the fields that differ from the
// baseline will be transmitted
static void SVT3_CreateBaseline()
{
	for (int entnum = 1; entnum < sv.q3_num_entities; entnum++)
	{
		idEntity3* ent = SVT3_EntityNum(entnum);
		if (!ent->GetLinked())
		{
			continue;
		}
		ent->SetNumber(entnum);

		//
		// take current state as baseline
		//
		if (GGameType & GAME_WolfSP)
		{
			wssharedEntity_t* svent = SVWS_GentityNum(entnum);
			sv.q3_svEntities[entnum].ws_baseline = svent->s;
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmsharedEntity_t* svent = SVWM_GentityNum(entnum);
			sv.q3_svEntities[entnum].wm_baseline = svent->s;
		}
		else if (GGameType & GAME_ET)
		{
			etsharedEntity_t* svent = SVET_GentityNum(entnum);
			sv.q3_svEntities[entnum].et_baseline = svent->s;
		}
		else
		{
			q3sharedEntity_t* svent = SVQ3_GentityNum(entnum);
			sv.q3_svEntities[entnum].q3_baseline = svent->s;
		}
	}
}

static void SVT3_BoundMaxClients(int minimum)
{
	// get the current maxclients value
	Cvar_Get("sv_maxclients", GGameType & (GAME_WolfMP | GAME_ET) ? "20" : "8", 0);

	// allow many bots in single player. note that this pretty much means all previous
	// settings will be ignored (including the one set through "seta sv_maxclients <num>"
	// in user profile's wolfconfig_mp.cfg). also that if the user subsequently start
	// the server in multiplayer mode, the number of clients will still be the number
	// set here, which may be wrong - we can certainly just set it to a sensible number
	// when it is not in single player mode in the else part of the if statement when
	// necessary
	if (GGameType & GAME_ET && (SVET_GameIsSinglePlayer() || SVET_GameIsCoop()))
	{
		Cvar_Set("sv_maxclients", "64");
	}

	sv_maxclients->modified = false;

	if (sv_maxclients->integer < minimum)
	{
		Cvar_Set("sv_maxclients", va("%i", minimum));
	}
	else if (sv_maxclients->integer > (GGameType & GAME_WolfSP ? MAX_CLIENTS_WS :
		GGameType & GAME_WolfMP ? MAX_CLIENTS_WM :
		GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_Q3))
	{
		Cvar_Set("sv_maxclients", va("%i", (GGameType & GAME_WolfSP ? MAX_CLIENTS_WS :
			GGameType & GAME_WolfMP ? MAX_CLIENTS_WM :
			GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_Q3)));
	}
}

void SVWS_InitReliableCommandsForClient(client_t* cl, int commands)
{
	if (!commands)
	{
		Com_Memset(&cl->ws_reliableCommands, 0, sizeof(cl->ws_reliableCommands));
	}

	cl->ws_reliableCommands.bufSize = commands * RELIABLE_COMMANDS_CHARS;
	cl->ws_reliableCommands.buf = (char*)Mem_ClearedAlloc(cl->ws_reliableCommands.bufSize);
	cl->ws_reliableCommands.commandLengths = (int*)Mem_ClearedAlloc(commands * sizeof(*cl->ws_reliableCommands.commandLengths));
	cl->ws_reliableCommands.commands = (char**)Mem_ClearedAlloc(commands * sizeof(*cl->ws_reliableCommands.commands));

	cl->ws_reliableCommands.rover = cl->ws_reliableCommands.buf;
}

void SVWS_FreeReliableCommandsForClient(client_t* cl)
{
	if (!cl->ws_reliableCommands.bufSize)
	{
		return;
	}
	Mem_Free(cl->ws_reliableCommands.buf);
	Mem_Free(cl->ws_reliableCommands.commandLengths);
	Mem_Free(cl->ws_reliableCommands.commands);

	Com_Memset(&cl->ws_reliableCommands, 0, sizeof(cl->ws_reliableCommands));
}

void SVWS_FreeAcknowledgedReliableCommands(client_t* cl)
{
	if (!cl->ws_reliableCommands.bufSize)
	{
		return;
	}

	int realAck = (cl->q3_reliableAcknowledge) & (MAX_RELIABLE_COMMANDS_WOLF - 1);
	// move backwards one command, since we need the most recently acknowledged
	// command for netchan decoding
	int ack = (cl->q3_reliableAcknowledge - 1) & (MAX_RELIABLE_COMMANDS_WOLF - 1);

	if (!cl->ws_reliableCommands.commands[ack])
	{
		// no new commands acknowledged
		return;
	}

	while (cl->ws_reliableCommands.commands[ack])
	{
		// clear the string
		memset(cl->ws_reliableCommands.commands[ack], 0, cl->ws_reliableCommands.commandLengths[ack]);
		// clear the pointer
		cl->ws_reliableCommands.commands[ack] = NULL;
		cl->ws_reliableCommands.commandLengths[ack] = 0;
		// move the the previous command
		ack--;
		if (ack < 0)
		{
			ack = (MAX_RELIABLE_COMMANDS_WOLF - 1);
		}
		if (ack == realAck)
		{
			// never free the actual most recently acknowledged command
			break;
		}
	}
}

//	Called when a host starts a map when it wasn't running
// one before.  Successive map or map_restart commands will
// NOT cause this to be called, unless the game is exited to
// the menu system first.
static void SVT3_Startup()
{
	if (svs.initialized)
	{
		common->Error("SVT3_Startup: svs.initialized");
	}
	SVT3_BoundMaxClients(1);

	svs.clients = (client_t*)Mem_ClearedAlloc(sizeof(client_t) * sv_maxclients->integer);
	if (com_dedicated->integer)
	{
		svs.q3_numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP_Q3 * 64;
	}
	else
	{
		// we don't need nearly as many when playing locally
		svs.q3_numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}
	svs.initialized = true;

	Cvar_Set("sv_running", "1");
}

static void SVT3_ChangeMaxClients()
{
	int oldMaxClients;
	int i;
	client_t* oldClients;
	int count;

	// get the highest client number in use
	count = 0;
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			if (i > count)
			{
				count = i;
			}
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SVT3_BoundMaxClients(count);
	// if still the same
	if (sv_maxclients->integer == oldMaxClients)
	{
		return;
	}

	// RF, free reliable commands for clients outside the NEW maxclients limit
	if (GGameType & GAME_WolfSP && oldMaxClients > sv_maxclients->integer)
	{
		for (i = sv_maxclients->integer; i < oldMaxClients; i++)
		{
			SVWS_FreeReliableCommandsForClient(&svs.clients[i]);
		}
	}

	oldClients = (client_t*)Mem_Alloc(count * sizeof(client_t));
	// copy the clients to hunk memory
	for (i = 0; i < count; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			oldClients[i] = svs.clients[i];
		}
		else
		{
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	Mem_Free(svs.clients);

	// allocate new clients
	svs.clients = (client_t*)Mem_ClearedAlloc(sv_maxclients->integer * sizeof(client_t));
	Com_Memset(svs.clients, 0, sv_maxclients->integer * sizeof(client_t));

	// copy the clients over
	for (i = 0; i < count; i++)
	{
		if (oldClients[i].state >= CS_CONNECTED)
		{
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Mem_Free(oldClients);

	// allocate new snapshot entities
	if (com_dedicated->integer)
	{
		svs.q3_numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP_Q3 * 64;
	}
	else
	{
		// we don't need nearly as many when playing locally
		svs.q3_numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}

	// RF, allocate reliable commands for newly created client slots
	if (GGameType & GAME_WolfSP && oldMaxClients < sv_maxclients->integer)
	{
		if (svt3_gametype->integer == Q3GT_SINGLE_PLAYER)
		{
			for (i = oldMaxClients; i < sv_maxclients->integer; i++)
			{
				// must be an AI slot
				SVWS_InitReliableCommandsForClient(&svs.clients[i], 0);
			}
		}
		else
		{
			for (i = oldMaxClients; i < sv_maxclients->integer; i++)
			{
				SVWS_InitReliableCommandsForClient(&svs.clients[i], MAX_RELIABLE_COMMANDS_WOLF);
			}
		}
	}
}

void SVT3_ClearServer()
{
	for (int i = 0; i < BIGGEST_MAX_CONFIGSTRINGS_T3; i++)
	{
		if (sv.q3_configstrings[i])
		{
			Mem_Free(sv.q3_configstrings[i]);
		}
	}
	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		if (sv.q3_entities[i])
		{
			delete sv.q3_entities[i];
		}
	}
	if (sv.q3_gamePlayerStates)
	{
		for (int i = 0; i < sv_maxclients->integer; i++)
		{
			if (sv.q3_gamePlayerStates[i])
			{
				delete sv.q3_gamePlayerStates[i];
			}
		}
		delete[] sv.q3_gamePlayerStates;
	}
	Com_Memset(&sv, 0, sizeof(sv));

	if (svs.q3_snapshotEntities)
	{
		Mem_Free(svs.q3_snapshotEntities);
		svs.q3_snapshotEntities = NULL;
	}
	if (svs.ws_snapshotEntities)
	{
		Mem_Free(svs.ws_snapshotEntities);
		svs.ws_snapshotEntities = NULL;
	}
	if (svs.wm_snapshotEntities)
	{
		Mem_Free(svs.wm_snapshotEntities);
		svs.wm_snapshotEntities = NULL;
	}
	if (svs.et_snapshotEntities)
	{
		Mem_Free(svs.et_snapshotEntities);
		svs.et_snapshotEntities = NULL;
	}
}

//	touch the cgame.vm so that a pure client can load it if it's in a seperate pk3
static void SVT3_TouchCGame()
{
	char filename[MAX_QPATH];
	String::Sprintf(filename, sizeof(filename), "vm/%s.qvm", "cgame");
	fileHandle_t f;
	FS_FOpenFileRead(filename, &f, false);
	if (f)
	{
		FS_FCloseFile(f);
	}
}

//	touch the cgame DLL so that a pure client (with DLL sv_pure support) can load do the correct checks
static void SVT3_TouchCGameDLL()
{
	char* filename;
	filename = Sys_GetMPDllName("cgame");
	fileHandle_t f;
	FS_FOpenFileRead_Filtered(filename, &f, false, FS_EXCLUDE_DIR);
	if (f)
	{
		FS_FCloseFile(f);
	}
	else if (GGameType & GAME_ET && svt3_pure->integer)		// ydnar: so we can work the damn game
	{
		common->Error("Failed to locate cgame DLL for pure server mode");
	}
}

//	Used by SVT3_Shutdown to send a final message to all
// connected clients before the server goes down.  The messages are sent immediately,
// not just stuck on the outgoing message list, because the server is going
// to totally exit after returning from this function.
void SVT3_FinalCommand(const char* cmd, bool disconnect)
{
	// send it twice, ignoring rate
	for (int j = 0; j < 2; j++)
	{
		client_t* cl = svs.clients;
		for (int i = 0; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state >= CS_CONNECTED)
			{
				// don't send a disconnect to a local client
				if (cl->netchan.remoteAddress.type != NA_LOOPBACK)
				{
					SVT3_SendServerCommand(cl, "%s", cmd);

					// ydnar: added this so map changes can use this functionality
					if (disconnect)
					{
						SVT3_SendServerCommand(cl, "disconnect");
					}
				}
				// force a snapshot to be sent
				cl->q3_nextSnapshotTime = -1;
				SVT3_SendClientSnapshot(cl);
			}
		}
	}
}

//	Change the server to a new map, taking all connected
// clients along with it.
// This is NOT called for map_restart
void SVT3_SpawnServer(const char* server, bool killBots)
{
	// Ridah, enforce maxclients in single player, so there is enough room for AI characters
	if (GGameType & GAME_WolfSP)
	{
		static Cvar* g_gametype, * bot_enable;

		// Rafael gameskill
		static Cvar* g_gameskill;

		if (!g_gameskill)
		{
			g_gameskill = Cvar_Get("g_gameskill", "2", CVAR_SERVERINFO | CVAR_LATCH2 | CVAR_ARCHIVE);		// (SA) new default '2' (was '1')
		}
		// done

		if (!g_gametype)
		{
			g_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH2 | CVAR_ARCHIVE);
		}
		if (!bot_enable)
		{
			bot_enable = Cvar_Get("bot_enable", "1", CVAR_LATCH2);
		}
		if (g_gametype->integer == 2)
		{
			if (sv_maxclients->latchedString)
			{
				// it's been modified, so grab the new value
				Cvar_Get("sv_maxclients", "8", 0);
			}
			if (sv_maxclients->integer < MAX_CLIENTS_WS)
			{
				Cvar_SetValue("sv_maxclients", WSMAX_SP_CLIENTS);
			}
			if (!bot_enable->integer)
			{
				Cvar_Set("bot_enable", "1");
			}
		}
	}

	// ydnar: broadcast a level change to all connected clients
	if (GGameType & GAME_ET && svs.clients && !com_errorEntered)
	{
		SVT3_FinalCommand("spawnserver", false);
	}

	// shut down the existing game if it is running
	SVT3_ShutdownGameProgs();

	common->Printf("------ Server Initialization ------\n");
	common->Printf("Server: %s\n",server);

	// if not running a dedicated server CLT3_MapLoading will connect the client to the server
	// also print some status stuff
	CLT3_MapLoading();

	// make sure all the client stuff is unloaded
	CLT3_ShutdownAll();

	// init client structures and svs.q3_numSnapshotEntities
	if (!Cvar_VariableValue("sv_running"))
	{
		SVT3_Startup();
	}
	else
	{
		// check for maxclients change
		if (sv_maxclients->modified)
		{
			SVT3_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences(0);

	// wipe the entire per-level structure
	SVT3_ClearServer();

	// allocate empty config strings
	for (int i = 0; i < BIGGEST_MAX_CONFIGSTRINGS_T3; i++)
	{
		sv.q3_configstrings[i] = __CopyString("");
		if (i < MAX_CONFIGSTRINGS_ET)
		{
			sv.et_configstringsmodified[i] = false;
		}
	}

	sv.q3_gamePlayerStates = new idPlayerState3*[sv_maxclients->integer];
	if (GGameType & GAME_Quake3)
	{
		for (int i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			sv.q3_entities[i] = new idQuake3Entity();
		}
		for (int i = 0; i < sv_maxclients->integer; i++)
		{
			sv.q3_gamePlayerStates[i] = new idQuake3PlayerState();
		}

		// allocate the snapshot entities on the hunk
		svs.q3_snapshotEntities = (q3entityState_t*)Mem_ClearedAlloc(sizeof(q3entityState_t) * svs.q3_numSnapshotEntities);
	}
	else if (GGameType & GAME_WolfSP)
	{
		for (int i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			sv.q3_entities[i] = new idWolfSPEntity();
		}
		for (int i = 0; i < sv_maxclients->integer; i++)
		{
			sv.q3_gamePlayerStates[i] = new idWolfSPPlayerState();
		}

		// allocate the snapshot entities on the hunk
		svs.ws_snapshotEntities = (wsentityState_t*)Mem_ClearedAlloc(sizeof(wsentityState_t) * svs.q3_numSnapshotEntities);
	}
	else if (GGameType & GAME_WolfMP)
	{
		for (int i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			sv.q3_entities[i] = new idWolfMPEntity();
		}
		for (int i = 0; i < sv_maxclients->integer; i++)
		{
			sv.q3_gamePlayerStates[i] = new idWolfMPPlayerState();
		}

		// allocate the snapshot entities on the hunk
		svs.wm_snapshotEntities = (wmentityState_t*)Mem_ClearedAlloc(sizeof(wmentityState_t) * svs.q3_numSnapshotEntities);
	}
	else
	{
		for (int i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			sv.q3_entities[i] = new idETEntity();
		}
		for (int i = 0; i < sv_maxclients->integer; i++)
		{
			sv.q3_gamePlayerStates[i] = new idETPlayerState();
		}

		// allocate the snapshot entities on the hunk
		svs.et_snapshotEntities = (etentityState_t*)Mem_ClearedAlloc(sizeof(etentityState_t) * svs.q3_numSnapshotEntities);
	}
	svs.q3_nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.q3_snapFlagServerBit ^= Q3SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set("nextmap", "map_restart 0");

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		// just set it to a negative number,so the cgame knows not to draw the percent bar
		Cvar_Set("com_expectedhunkusage", "-1");
	}

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	srand(Com_Milliseconds());
	sv.q3_checksumFeed = (((int)rand() << 16) ^ rand()) ^ Com_Milliseconds();
	FS_Restart(sv.q3_checksumFeed);

	int checksum;
	CM_LoadMap(va("maps/%s.bsp", server), false, &checksum);

	// set serverinfo visible name
	Cvar_Set("mapname", server);

	Cvar_Set("sv_mapChecksum", va("%i",checksum));

	// serverid should be different each time
	sv.q3_serverId = com_frameTime;
	sv.q3_restartedServerId = sv.q3_serverId;	// I suppose the init here is just to be safe
	sv.q3_checksumFeedServerId = sv.q3_serverId;
	Cvar_Set("sv_serverid", va("%i", sv.q3_serverId));

	// clear physics interaction links
	SV_ClearWorld();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("sv_serverRestarting", "1");
	}

	// load and spawn all other entities
	SVT3_InitGameProgs();

	if (!(GGameType & GAME_ET))
	{
		// don't allow a map_restart if game is modified
		svt3_gametype->modified = false;
	}

	// run a few frames to allow everything to settle
	for (int i = 0; i < (GGameType & GAME_ET ? ETGAME_INIT_FRAMES : 3); i++)
	{
		SVT3_GameRunFrame(svs.q3_time);
		SVT3_BotFrame(svs.q3_time);
		svs.q3_time += Q3FRAMETIME;
	}

	// create a baseline for more efficient communications
	SVT3_CreateBaseline();

	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			bool isBot;
			if (svs.clients[i].netchan.remoteAddress.type == NA_BOT)
			{
				if (killBots || (GGameType & (GAME_WolfSP | GAME_WolfMP) && Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER) ||
					(GGameType & GAME_ET && (SVET_GameIsSinglePlayer() || SVET_GameIsCoop())))
				{
					SVT3_DropClient(&svs.clients[i], GGameType & GAME_WolfSP ? " gametype is Single Player" : "");		//DAJ added message
					continue;
				}
				isBot = true;
			}
			else
			{
				isBot = false;
			}

			// connect the client again
			const char* denied = SVT3_GameClientConnect(i, false, isBot);		// firstTime = false
			if (denied)
			{
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SVT3_DropClient(&svs.clients[i], denied);
			}
			else
			{
				if (!isBot)
				{
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else
				{
					client_t* client = &svs.clients[i];
					client->state = CS_ACTIVE;
					idEntity3* ent = SVT3_EntityNum(i);
					ent->SetNumber(i);
					if (GGameType & GAME_Quake3)
					{
						client->q3_gentity = SVQ3_GentityNum(i);
					}
					else if (GGameType & GAME_WolfSP)
					{
						client->ws_gentity = SVWS_GentityNum(i);
					}
					else if (GGameType & GAME_WolfMP)
					{
						client->wm_gentity = SVWM_GentityNum(i);
					}
					else
					{
						client->et_gentity = SVET_GentityNum(i);
					}
					client->q3_entity = ent;

					client->q3_deltaMessage = -1;
					client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately

					SVT3_GameClientBegin(i);
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	SVT3_GameRunFrame(svs.q3_time);
	SVT3_BotFrame(svs.q3_time);
	svs.q3_time += Q3FRAMETIME;

	if (svt3_pure->integer)
	{
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		const char* p = FS_LoadedPakChecksums();
		Cvar_Set("sv_paks", p);
		if (String::Length(p) == 0)
		{
			common->Printf("WARNING: sv_pure set but no PK3 files loaded\n");
		}
		p = FS_LoadedPakNames();
		Cvar_Set("sv_pakNames", p);

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if (GGameType & (GAME_Quake3 | GAME_WolfSP) && com_dedicated->integer)
		{
			SVT3_TouchCGame();
		}
	}
	else
	{
		Cvar_Set("sv_paks", "");
		Cvar_Set("sv_pakNames", "");
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// NOTE: we consider the referencedPaks as 'required for operation'

		// we want the server to reference the mp_bin pk3 that the client is expected to load from
		SVT3_TouchCGameDLL();
	}
	const char* p = FS_ReferencedPakChecksums();
	Cvar_Set("sv_referencedPaks", p);
	p = FS_ReferencedPakNames();
	Cvar_Set("sv_referencedPakNames", p);

	// save systeminfo and serverinfo strings
	char systemInfo[16384];
	String::NCpyZ(systemInfo, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING), sizeof(systemInfo));
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SVT3_SetConfigstring(Q3CS_SYSTEMINFO, systemInfo);

	SVT3_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	if (GGameType & GAME_WolfMP)
	{
		// NERVE - SMF
		SVT3_SetConfigstring(WMCS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}
	if (GGameType & GAME_ET)
	{
		// NERVE - SMF
		SVT3_SetConfigstring(ETCS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SVT3_Heartbeat_f();

	if (GGameType & GAME_ET)
	{
		SVET_UpdateConfigStrings();
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Set("sv_serverRestarting", "0");
	}

	common->Printf("-----------------------------------\n");
}

//	Called when each game quits,
// before Sys_Quit or Sys_Error
void SVT3_Shutdown(const char* finalmsg)
{
	if (!com_sv_running || !com_sv_running->integer)
	{
		return;
	}

	common->Printf("----- Server Shutdown -----\n");

	if (svs.clients && !com_errorEntered)
	{
		SVT3_FinalCommand(va("print \"%s\"", finalmsg), true);
	}

	SVT3_MasterShutdown();
	SVT3_ShutdownGameProgs();

	// free current level
	SVT3_ClearServer();

	// free server static data
	if (svs.clients)
	{
		Mem_Free(svs.clients);
	}
	Com_Memset(&svs, 0, sizeof(svs));
	svs.et_serverLoad = -1;

	Cvar_Set("sv_running", "0");
	if (GGameType & GAME_Quake3)
	{
		Cvar_Set("ui_singlePlayerActive", "0");
	}

	common->Printf("---------------------------\n");

	// disconnect any local clients
	CL_Disconnect(false);
}

//	Only called at main exe startup, not for each game
void SVT3_Init()
{
	SVT3_AddOperatorCommands();

	// serverinfo vars
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Cvar_Get("dmflags", "0", 0);
		Cvar_Get("fraglimit", "0", 0);
	}
	else
	{
		Cvar_Get("dmflags", "0", CVAR_SERVERINFO);
		Cvar_Get("fraglimit", "20", CVAR_SERVERINFO);
	}
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	if (GGameType & GAME_WolfMP)
	{
		// DHM - Nerve :: default to WMGT_WOLF
		svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);
	}
	else if (GGameType & GAME_ET)
	{
		svt3_gametype = Cvar_Get("g_gametype", va("%i", comet_gameInfo.defaultGameType), CVAR_SERVERINFO | CVAR_LATCH2);
	}
	else
	{
		svt3_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH2);
	}
	if (GGameType & GAME_WolfSP)
	{
		svt3_gameskill = Cvar_Get("g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH2);
	}
	if (GGameType & GAME_WolfMP)
	{
		svt3_gameskill = Cvar_Get("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2);
	}
	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	if (GGameType & GAME_Quake3)
	{
		Cvar_Get("protocol", va("%i", Q3PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	}
	else if (GGameType & GAME_WolfSP)
	{
		Cvar_Get("protocol", va("%i", WSPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	}
	else if (GGameType & GAME_WolfMP)
	{
		Cvar_Get("protocol", va("%i", WMPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	}
	else
	{
		Cvar_Get("protocol", va("%i", ETPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	}
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	svt3_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	if (GGameType & GAME_WolfMP)
	{
		sv_hostname = Cvar_Get("sv_hostname", "WolfHost", CVAR_SERVERINFO | CVAR_ARCHIVE);
	}
	else if (GGameType & GAME_ET)
	{
		sv_hostname = Cvar_Get("sv_hostname", "ETHost", CVAR_SERVERINFO | CVAR_ARCHIVE);
	}
	else
	{
		sv_hostname = Cvar_Get("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		sv_maxclients = Cvar_Get("sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH2);					// NERVE - SMF - changed to 20 from 8
	}
	else
	{
		sv_maxclients = Cvar_Get("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH2);
	}

	svt3_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);
	if (!(GGameType & GAME_Quake3))
	{
		svt3_allowAnonymous = Cvar_Get("sv_allowAnonymous", "0", CVAR_SERVERINFO);
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		svt3_friendlyFire = Cvar_Get("g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE);				// NERVE - SMF
		svt3_maxlives = Cvar_Get("g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_SERVERINFO);		// NERVE - SMF
	}
	if (GGameType & GAME_WolfMP)
	{
		svwm_tourney = Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);									// NERVE - SMF
	}
	if (GGameType & GAME_ET)
	{
		svet_needpass = Cvar_Get("g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM);
	}

	// systeminfo
	if (GGameType & GAME_WolfSP)
	{
		Cvar_Get("sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM);
		svt3_pure = Cvar_Get("sv_pure", "0", CVAR_SYSTEMINFO);
	}
	else
	{
		Cvar_Get("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
		svt3_pure = Cvar_Get("sv_pure", "1", CVAR_SYSTEMINFO);
	}
	Cvar_Get("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	// server vars
	svt3_rconPassword = Cvar_Get("rconPassword", "", CVAR_TEMP);
	svt3_privatePassword = Cvar_Get("sv_privatePassword", "", CVAR_TEMP);
	svt3_fps = Cvar_Get("sv_fps", "20", CVAR_TEMP);
	if (GGameType & GAME_Quake3)
	{
		svt3_timeout = Cvar_Get("sv_timeout", "200", CVAR_TEMP);
	}
	else if (GGameType & GAME_WolfSP)
	{
		svt3_timeout = Cvar_Get("sv_timeout", "120", CVAR_TEMP);
	}
	else
	{
		svt3_timeout = Cvar_Get("sv_timeout", "240", CVAR_TEMP);
	}
	svt3_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	if (GGameType & GAME_Quake3)
	{
		svt3_allowDownload = Cvar_Get("sv_allowDownload", "0", CVAR_SERVERINFO);
	}
	else if (GGameType & GAME_WolfSP)
	{
		svt3_allowDownload = Cvar_Get("sv_allowDownload", "1", 0);
	}
	else
	{
		svt3_allowDownload = Cvar_Get("sv_allowDownload", "1", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_Quake3)
	{
		svt3_master[0] = Cvar_Get("sv_master1", Q3MASTER_SERVER_NAME, 0);
	}
	else if (GGameType & GAME_WolfSP)
	{
		svt3_master[0] = Cvar_Get("sv_master1", WSMASTER_SERVER_NAME, 0);
	}
	else if (GGameType & GAME_WolfMP)
	{
		svt3_master[0] = Cvar_Get("sv_master1", WMMASTER_SERVER_NAME, 0);			// NERVE - SMF - wolfMP master server
	}
	else
	{
		svt3_master[0] = Cvar_Get("sv_master1", ETMASTER_SERVER_NAME, 0);
	}
	svt3_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	svt3_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	svt3_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	svt3_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	svt3_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	Cvar_Get("sv_showloss", "0", 0);
	svt3_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	svt3_killserver = Cvar_Get("sv_killserver", "0", 0);
	Cvar_Get("sv_mapChecksum", "", CVAR_ROM);
	svt3_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);
	if (GGameType & GAME_Quake3)
	{
		svq3_strictAuth = Cvar_Get("sv_strictAuth", "1", CVAR_ARCHIVE);
	}

	if (GGameType & (GAME_WolfSP | GAME_ET))
	{
		svt3_reloading = Cvar_Get("g_reloading", "0", CVAR_ROM);		//----(SA)	added
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		svwm_onlyVisibleClients = Cvar_Get("sv_onlyVisibleClients", "0", 0);			// DHM - Nerve

		svwm_showAverageBPS = Cvar_Get("sv_showAverageBPS", "0", 0);				// NERVE - SMF - net debugging
	}

	if (GGameType & GAME_ET)
	{
		svet_tempbanmessage = Cvar_Get("sv_tempbanmessage", "You have been kicked and are temporarily banned from joining this server.", 0);
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// NERVE - SMF - create user set cvars
		Cvar_Get("g_userTimeLimit", "0", 0);
		Cvar_Get("g_userAlliedRespawnTime", "0", 0);
		Cvar_Get("g_userAxisRespawnTime", "0", 0);
		Cvar_Get("g_maxlives", "0", 0);
		Cvar_Get("g_altStopwatchMode", "0", CVAR_ARCHIVE);
		Cvar_Get("g_minGameClients", "8", CVAR_SERVERINFO);
		Cvar_Get("gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM);
		Cvar_Get("g_currentRound", "0", CVAR_WOLFINFO);
		Cvar_Get("g_nextTimeLimit", "0", CVAR_WOLFINFO);

		// TTimo - some UI additions
		// NOTE: sucks to have this hardcoded really, I suppose this should be in UI
		Cvar_Get("g_axismaxlives", "0", 0);
		Cvar_Get("g_alliedmaxlives", "0", 0);
		Cvar_Get("g_fastres", "0", CVAR_ARCHIVE);
		Cvar_Get("g_fastResMsec", "1000", CVAR_ARCHIVE);

		// the download netcode tops at 18/20 kb/s, no need to make you think you can go above
		svt3_dl_maxRate = Cvar_Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_WolfMP)
	{
		Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);
		Cvar_Get("g_complaintlimit", "3", CVAR_ARCHIVE);
		// -NERVE - SMF

		// ATVI Tracker Wolfenstein Misc #273
		Cvar_Get("g_voteFlags", "255", CVAR_ARCHIVE | CVAR_SERVERINFO);

		// ATVI Tracker Wolfenstein Misc #263
		Cvar_Get("g_antilag", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	}
	if (GGameType & GAME_ET)
	{
		Cvar_Get("g_complaintlimit", "6", CVAR_ARCHIVE);
		// -NERVE - SMF

		// ATVI Tracker Wolfenstein Misc #273
		Cvar_Get("g_voteFlags", "0", CVAR_ROM | CVAR_SERVERINFO);

		// ATVI Tracker Wolfenstein Misc #263
		Cvar_Get("g_antilag", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);

		Cvar_Get("g_needpass", "0", CVAR_SERVERINFO);

		svet_wwwDownload = Cvar_Get("sv_wwwDownload", "0", CVAR_ARCHIVE);
		svet_wwwBaseURL = Cvar_Get("sv_wwwBaseURL", "", CVAR_ARCHIVE);
		svet_wwwDlDisconnected = Cvar_Get("sv_wwwDlDisconnected", "0", CVAR_ARCHIVE);
		svet_wwwFallbackURL = Cvar_Get("sv_wwwFallbackURL", "", CVAR_ARCHIVE);

		//bani
		sv_packetloss = Cvar_Get("sv_packetloss", "0", CVAR_CHEAT);
		sv_packetdelay = Cvar_Get("sv_packetdelay", "0", CVAR_CHEAT);

		// fretn - note: redirecting of clients to other servers relies on this,
		// ET://someserver.com
		svet_fullmsg = Cvar_Get("sv_fullmsg", "Server is full.", CVAR_ARCHIVE);
	}

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();

	svs.et_serverLoad = -1;
}
