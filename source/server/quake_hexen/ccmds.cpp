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
#include "../progsvm/progsvm.h"
#include "local.h"
#include "../hexen2/local.h"
#include "../../client/public.h"
#include <time.h>

#define ShortTime "%m/%d/%Y %H:%M"

int qhw_fp_messages = 4, qhw_fp_persecond = 4, qhw_fp_secondsdead = 10;
char qhw_fp_msg[255] = { 0 };

//	handle a
//	map <servername>
//	command from the console.  Active clients are kicked off.
static void SVQH_Map_f()
{
	if (Cmd_Argc() < 2)		//no map name given
	{
		common->Printf("map <levelname>: start a new server\nCurrently on: %s\n", GGameType & GAME_Hexen2 ? SVH2_GetMapName() : SVQ1_GetMapName());
		return;
	}

	CLQH_StopDemoLoop();		// stop demo loop in case this fails

	CL_Disconnect();
	SVQH_Shutdown(false);

	CL_ClearKeyCatchers();			// remove console or menu
	SCRQH_BeginLoadingPlaque();

	if (GGameType & GAME_Hexen2)
	{
		info_mask = 0;
		if (!svqh_coop->value && svqh_deathmatch->value)
		{
			info_mask2 = 0x80000000;
		}
		else
		{
			info_mask2 = 0;
		}
	}

	svs.qh_serverflags = 0;			// haven't completed an episode yet
	char name[MAX_QPATH];
	String::Cpy(name, Cmd_Argv(1));
	SVQH_SpawnServer(name, NULL);
	if (sv.state == SS_DEAD)
	{
		return;
	}

	if (!com_dedicated->integer)
	{
		CLQH_GetSpawnParams();

		Cmd_ExecuteString("connect local");
	}
}

//	Goes to a new map, taking all clients along
static void SVQH_Changelevel_f()
{
	if (Cmd_Argc() < 2)
	{
		common->Printf("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (sv.state == SS_DEAD || CL_IsDemoPlaying())
	{
		common->Printf("Only the server may changelevel\n");
		return;
	}

	char level[MAX_QPATH];
	String::Cpy(level, Cmd_Argv(1));
	char _startspot[MAX_QPATH];
	char* startspot;
	if (!(GGameType & GAME_Hexen2) || Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SVQH_SaveSpawnparms();
	SVQH_SpawnServer(level, startspot);
}

//	handle a
//	map <mapname>
//	command from the console or progs.
void SVQHW_Map_f()
{
	if (Cmd_Argc() < 2)
	{
		common->Printf("map <levelname> : continue game on a new level\n");
		return;
	}
	char level[MAX_QPATH];
	String::Cpy(level, Cmd_Argv(1));
	char _startspot[MAX_QPATH];
	char* startspot;
	if (!(GGameType & GAME_Hexen2) || Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	// check to make sure the level exists
	char expanded[MAX_QPATH];
	sprintf(expanded, "maps/%s.bsp", level);
	fileHandle_t f;
	FS_FOpenFileRead(expanded, &f, true);
	if (!f)
	{
		common->Printf("Can't find %s\n", expanded);
		return;
	}
	FS_FCloseFile(f);

	SVQH_BroadcastCommand("changing\n");
	SVQHW_SendMessagesToAll();

	SVQH_SpawnServer(level, startspot);

	SVQH_BroadcastCommand("reconnect\n");
}

//	Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
void SVQ1_SavegameComment(char* text)
{
	for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
	{
		text[i] = ' ';
	}
	Com_Memcpy(text, SVQ1_GetMapName(), String::Length(SVQ1_GetMapName()));
	char kills[20];
	sprintf(kills,"kills:%3i/%3i", static_cast<int>(*pr_globalVars.killed_monsters), static_cast<int>(*pr_globalVars.total_monsters));
	Com_Memcpy(text + 22, kills, String::Length(kills));
	// convert space to _ to make stdio happy
	for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
	{
		if (text[i] == ' ')
		{
			text[i] = '_';
		}
	}
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

//	Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
void SVH2_SavegameComment(char* text)
{
	for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
	{
		text[i] = ' ';
	}
	Com_Memcpy(text, SVH2_GetMapName(), String::Length(SVH2_GetMapName()));

	time_t TempTime = time(NULL);
	tm* tblock = localtime(&TempTime);
	char kills[20];
	strftime(kills,sizeof(kills),ShortTime,tblock);

	Com_Memcpy(text + 21, kills, String::Length(kills));
	// convert space to _ to make stdio happy
	for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
	{
		if (text[i] == ' ')
		{
			text[i] = '_';
		}
	}
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

void SVH2_SaveGamestate(bool clientsOnly)
{
	int start;
	int end;
	char name[MAX_OSPATH];
	if (clientsOnly)
	{
		start = 1;
		end = svs.qh_maxclients + 1;

		sprintf(name, "clients.gip");
	}
	else
	{
		start = 1;
		end = sv.qh_num_edicts;

		sprintf(name, "%s.gip", sv.name);
	}

	fileHandle_t f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", H2_SAVEGAME_VERSION);

	if (!clientsOnly)
	{
		char comment[SAVEGAME_COMMENT_LENGTH + 1];
		SVH2_SavegameComment(comment);
		FS_Printf(f, "%s\n", comment);
		FS_Printf(f, "%f\n", qh_skill->value);
		FS_Printf(f, "%s\n", sv.name);
		FS_Printf(f, "%f\n", sv.qh_time);

		// write the light styles

		for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
		{
			if (sv.qh_lightstyles[i])
			{
				FS_Printf(f, "%s\n", sv.qh_lightstyles[i]);
			}
			else
			{
				FS_Printf(f,"m\n");
			}
		}
		SVH2_SaveEffects(f);
		FS_Printf(f,"-1\n");
		ED_WriteGlobals(f);
	}

	client_t* host_client = svs.clients;

	//  to save the client states
	for (int i = start; i < end; i++)
	{
		qhedict_t* ent = QH_EDICT_NUM(i);
		if ((int)ent->GetFlags() & H2FL_ARCHIVE_OVERRIDE)
		{
			continue;
		}
		if (clientsOnly)
		{
			if (host_client->state >= CS_CONNECTED)
			{
				FS_Printf(f, "%i\n",i);
				ED_Write(f, ent);
				FS_Flush(f);
			}
			host_client++;
		}
		else
		{
			FS_Printf(f, "%i\n",i);
			ED_Write(f, ent);
			FS_Flush(f);
		}
	}

	FS_FCloseFile(f);
}

void SVQH_InitOperatorCommands()
{
	Cmd_AddCommand("map", SVQH_Map_f);
	Cmd_AddCommand("changelevel", SVQH_Changelevel_f);
}

void SVQHW_InitOperatorCommands()
{
	Cmd_AddCommand("map", SVQHW_Map_f);
}
