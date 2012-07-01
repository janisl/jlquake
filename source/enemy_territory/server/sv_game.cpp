/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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
void SV_GameDropClient(int clientNum, const char* reason, int length)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
	if (length)
	{
		SV_TempBanNetAddress(svs.clients[clientNum].netchan.remoteAddress, length);
	}
}

/*
====================
SV_GameBinaryMessageReceived
====================
*/
void SV_GameBinaryMessageReceived(int cno, const char* buf, int buflen, int commandTime)
{
	VM_Call(gvm, ETGAME_MESSAGERECEIVED, cno, buf, buflen, commandTime);
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
//------
	case ETG_DROP_CLIENT:
		SV_GameDropClient(args[1], (char*)VMA(2), args[3]);
		return 0;
	case ETG_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
//------
	case ETG_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case ETG_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case ETG_SET_USERINFO:
		SV_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case ETG_GET_USERINFO:
		SV_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
//------
	case ETG_GETTAG:
		return SV_GetTag(args[1], args[2], (char*)VMA(3), (orientation_t*)VMA(4));

	case ETG_REGISTERTAG:
		return SV_LoadTag((char*)VMA(1));
//------
	default:
		return SVET_GameSystemCalls(args);
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
	VM_Call(gvm, ETGAME_SHUTDOWN, qfalse);
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
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].et_gentity = NULL;
		svs.clients[i].q3_entity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, ETGAME_INIT, svs.q3_time, Com_Milliseconds(), restart);
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
	VM_Call(gvm, ETGAME_SHUTDOWN, qtrue);

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
	sv.et_num_tagheaders = 0;
	sv.et_num_tags = 0;

	// load the dll
	gvm = VM_Create("qagame", SV_GameSystemCalls, VMI_NATIVE);
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

	return VM_Call(gvm, ETGAME_CONSOLE_COMMAND);
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag(int clientNum, char* tagname, orientation_t* _or);

qboolean SV_GetTag(int clientNum, int tagFileNumber, char* tagname, orientation_t* _or)
{
	int i;

	if (tagFileNumber > 0 && tagFileNumber <= sv.et_num_tagheaders)
	{
		for (i = sv.et_tagHeadersExt[tagFileNumber - 1].start; i < sv.et_tagHeadersExt[tagFileNumber - 1].start + sv.et_tagHeadersExt[tagFileNumber - 1].count; i++)
		{
			if (!String::ICmp(sv.et_tags[i].name, tagname))
			{
				VectorCopy(sv.et_tags[i].origin, _or->origin);
				VectorCopy(sv.et_tags[i].axis[0], _or->axis[0]);
				VectorCopy(sv.et_tags[i].axis[1], _or->axis[1]);
				VectorCopy(sv.et_tags[i].axis[2], _or->axis[2]);
				return qtrue;
			}
		}
	}

	// Gordon: lets try and remove the inconsitancy between ded/non-ded servers...
	// Gordon: bleh, some code in clientthink_real really relies on this working on player models...
#ifndef DEDICATED	// TTimo: dedicated only binary defines DEDICATED
	if (com_dedicated->integer)
	{
		return qfalse;
	}

	return CL_GetTag(clientNum, tagname, _or);
#else
	return qfalse;
#endif
}
