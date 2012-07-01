/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// sv_game.c -- interface to the game dll

#include "server.h"

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int clientNum, const char* text)
{
	if (clientNum == -1)
	{
		SV_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char* reason)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}

//==============================================

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
qintptr SV_GameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
//----
	case Q3G_DROP_CLIENT:
		SV_GameDropClient(args[1], (char*)VMA(2));
		return 0;
	case Q3G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
//----
	case Q3G_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case Q3G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case Q3G_SET_USERINFO:
		SV_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case Q3G_GET_USERINFO:
		SV_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
//----
	default:
		return SVQ3_GameSystemCalls(args);
	}
	return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, Q3GAME_SHUTDOWN, qfalse);
	VM_Free(gvm);
	gvm = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart)
{
	int i;

	// start the entity parsing at the beginning
	sv.q3_entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].q3_gentity = NULL;
		svs.clients[i].q3_entity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, Q3GAME_INIT, svs.q3_time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, Q3GAME_SHUTDOWN, qtrue);

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if (!gvm)		// bk001212 - as done below
	{
		Com_Error(ERR_FATAL, "VM_Restart on game failed");
	}

	SV_InitGameVM(qtrue);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void)
{
	Cvar* var;

	var = Cvar_Get("bot_enable", "1", CVAR_LATCH2);
	if (var)
	{
		bot_enable = var->integer;
	}
	else
	{
		bot_enable = 0;
	}

	// load the dll or bytecode
	gvm = VM_Create("qagame", SV_GameSystemCalls, (vmInterpret_t)(int)Cvar_VariableValue("vm_game"));
	if (!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Create on game failed");
	}

	SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void)
{
	if (sv.state != SS_GAME)
	{
		return qfalse;
	}

	return VM_Call(gvm, Q3GAME_CONSOLE_COMMAND);
}
