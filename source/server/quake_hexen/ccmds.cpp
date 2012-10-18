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

#define Q1_SAVEGAME_VERSION    5
#define H2_SAVEGAME_VERSION    5

#define ShortTime "%m/%d/%Y %H:%M"

int qhw_fp_messages = 4, qhw_fp_persecond = 4, qhw_fp_secondsdead = 10;
char qhw_fp_msg[255] = { 0 };

static double h2_old_time;

static bool svqhw_allow_cheats;

netadr_t rcon_from;

static int SVH2_LoadGamestate(char* level, char* startspot, int ClientsMode);

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

	CL_Disconnect(true);
	SVQH_Shutdown();

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
static void SVQ1_SavegameComment(char* text)
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
static void SVH2_SavegameComment(char* text)
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

static char* GetLine(char*& ReadPos)
{
	char* Line = ReadPos;
	while (*ReadPos)
	{
		if (*ReadPos == '\r')
		{
			*ReadPos = 0;
			ReadPos++;
		}
		if (*ReadPos == '\n')
		{
			*ReadPos = 0;
			ReadPos++;
			break;
		}
		ReadPos++;
	}
	return Line;
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

static void SVH2_RestoreClients()
{
	double time_diff;

	if (SVH2_LoadGamestate(NULL,NULL,1))
	{
		return;
	}

	time_diff = sv.qh_time - h2_old_time;

	client_t* host_client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state >= CS_CONNECTED)
		{
			qhedict_t* ent = host_client->qh_edict;

			ent->SetTeam((host_client->qh_colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->h2_playerclass);

			// copy spawn parms out of the client_t

			for (int j = 0; j < NUM_SPAWN_PARMS; j++)
			{
				pr_globalVars.parm1[j] = host_client->qh_spawn_parms[j];
			}

			// call the spawn function

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(ent);
			G_FLOAT(OFS_PARM0) = time_diff;
			PR_ExecuteProgram(*pr_globalVars.ClientReEnter);
		}
	}
	SVH2_SaveGamestate(true);
}

static int SVH2_LoadGamestate(char* level, char* startspot, int ClientsMode)
{
	char name[MAX_OSPATH];
	char mapname[MAX_QPATH];
	float time, sk;
	int i;
	qhedict_t* ent;
	int entnum;
	int version;
	qboolean auto_correct = false;

	if (ClientsMode == 1)
	{
		sprintf(name, "clients.gip");
	}
	else
	{
		sprintf(name, "%s.gip", level);

		if (ClientsMode != 2 && ClientsMode != 3)
		{
			common->Printf("Loading game from %s...\n", name);
		}
	}

	Array<byte> Buffer;
	int len = FS_ReadFile(name, Buffer);
	if (len <= 0)
	{
		if (ClientsMode == 2)
		{
			common->Printf("ERROR: couldn't open.\n");
		}

		return -1;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	version = String::Atoi(GetLine(ReadPos));

	if (version != H2_SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, H2_SAVEGAME_VERSION);
		return -1;
	}

	if (ClientsMode != 1)
	{
		GetLine(ReadPos);
		sk = String::Atof(GetLine(ReadPos));
		Cvar_SetValue("skill", sk);

		String::Cpy(mapname, GetLine(ReadPos));
		time = String::Atof(GetLine(ReadPos));

		SVQH_SpawnServer(mapname, startspot);

		if (sv.state == SS_DEAD)
		{
			common->Printf("Couldn't load map\n");
			return -1;
		}

		// load the light styles
		for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
		{
			char* Style = GetLine(ReadPos);
			char* Tmp = (char*)Mem_Alloc(String::Length(Style) + 1);
			String::Cpy(Tmp, Style);
			sv.qh_lightstyles[i] = Tmp;
		}
		ReadPos = const_cast<char*>(SVH2_LoadEffects(ReadPos));
	}

	// load the edicts out of the savegame file
	const char* start = ReadPos;
	while (start)
	{
		char* token = String::Parse1(&start);
		if (!start)
		{
			break;		// end of file
		}
		entnum = String::Atoi(token);
		token = String::Parse1(&start);
		if (String::Cmp(token, "{"))
		{
			common->FatalError("First token isn't a brace");
		}

		// parse an edict

		if (entnum == -1)
		{
			start = ED_ParseGlobals(start);
			// Need to restore this
			*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);
		}
		else
		{
			ent = QH_EDICT_NUM(entnum);
			Com_Memset(&ent->v, 0, progs->entityfields * 4);
			//ent->free = false;
			start = ED_ParseEdict(start, ent);

			if (ClientsMode == 1 || ClientsMode == 2 || ClientsMode == 3)
			{
				ent->SetStatsRestored(true);
			}

			// link it into the bsp tree
			if (!ent->free)
			{
				SVQH_LinkEdict(ent, false);
				if (ent->v.modelindex && ent->GetModel())
				{
					i = SVQH_ModelIndex(PR_GetString(ent->GetModel()));
					if (i != ent->v.modelindex)
					{
						ent->v.modelindex = i;
						auto_correct = true;
					}
				}
			}
		}
	}

	//sv.num_edicts = entnum;
	if (ClientsMode == 0)
	{
		sv.qh_time = time;
		sv.qh_paused = true;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		SVH2_RestoreClients();
	}
	else if (ClientsMode == 2)
	{
		sv.qh_time = time;
	}
	else if (ClientsMode == 3)
	{
		sv.qh_time = time;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		SVH2_RestoreClients();
	}

	if (ClientsMode != 1 && auto_correct)
	{
		common->DPrintf("*** Auto-corrected model indexes!\n");
	}

	return 0;
}

//	Restarts the current server for a dead player
static void SVQ1_Restart_f()
{
	if (CL_IsDemoPlaying() || sv.state == SS_DEAD)
	{
		return;
	}

	char mapname[MAX_QPATH];
	String::Cpy(mapname, sv.name);	// must copy out, because it gets cleared in sv_spawnserver
	SVQH_SpawnServer(mapname, "");
}

//	Restarts the current server for a dead player
static void SVH2_Restart_f()
{
	if (CL_IsDemoPlaying() || sv.state == SS_DEAD)
	{
		return;
	}

	char mapname[MAX_QPATH];
	String::Cpy(mapname, sv.name);	// must copy out, because it gets cleared in sv_spawnserver
	char startspot[MAX_QPATH];
	String::Cpy(startspot, sv.h2_startspot);

	if (Cmd_Argc() == 2 && String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		if (SVH2_LoadGamestate(mapname, startspot, 3))
		{
			SVQH_SpawnServer(mapname, startspot);
			SVH2_RestoreClients();
		}
	}
	else
	{
		SVQH_SpawnServer(mapname, startspot);
	}
}

// changing levels within a unit
static void SVH2_Changelevel2_f()
{
	if (Cmd_Argc() < 2)
	{
		common->Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
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
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SVQH_SaveSpawnparms();

	// save the current level's state
	h2_old_time = sv.qh_time;
	SVH2_SaveGamestate(false);

	// try to restore the new level
	if (SVH2_LoadGamestate(level, startspot, 0))
	{
		SVQH_SpawnServer(level, startspot);
		SVH2_RestoreClients();
	}
}

static void SVQ1_Savegame_f()
{
	if (sv.state == SS_DEAD)
	{
		common->Printf("Not playing a local game.\n");
		return;
	}

	if (CLQH_GetIntermission())
	{
		common->Printf("Can't save in intermission.\n");
		return;
	}

	if (svs.qh_maxclients != 1)
	{
		common->Printf("Can't save multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		common->Printf("Relative pathnames are not allowed.\n");
		return;
	}

	for (int i = 0; i < svs.qh_maxclients; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED && (svs.clients[i].qh_edict->GetHealth() <= 0))
		{
			common->Printf("Can't savegame with a dead player\n");
			return;
		}
	}

	char name[256];
	String::NCpyZ(name, Cmd_Argv(1), sizeof(name));
	String::DefaultExtension(name, sizeof(name), ".sav");

	common->Printf("Saving game to %s...\n", name);
	fileHandle_t f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", Q1_SAVEGAME_VERSION);
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	SVQ1_SavegameComment(comment);
	FS_Printf(f, "%s\n", comment);
	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		FS_Printf(f, "%f\n", svs.clients->qh_spawn_parms[i]);
	}
	FS_Printf(f, "%d\n", svqh_current_skill);
	FS_Printf(f, "%s\n", sv.name);
	FS_Printf(f, "%f\n",sv.qh_time);

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


	ED_WriteGlobals(f);
	for (int i = 0; i < sv.qh_num_edicts; i++)
	{
		ED_Write(f, QH_EDICT_NUM(i));
		FS_Flush(f);
	}
	FS_FCloseFile(f);
	common->Printf("done.\n");
}

static void SVQ1_Loadgame_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("load <savename> : load a game\n");
		return;
	}

	CLQH_StopDemoLoop();		// stop demo loop in case this fails

	char name[MAX_OSPATH];
	String::NCpyZ(name, Cmd_Argv(1), sizeof(name));
	String::DefaultExtension(name, sizeof(name), ".sav");

	// we can't call SCRQH_BeginLoadingPlaque, because too much stack space has
	// been used.  The menu calls it before stuffing loadgame command
	//SCRQH_BeginLoadingPlaque ();

	Array<byte> Buffer;
	common->Printf("Loading game from %s...\n", name);
	int FileLen = FS_ReadFile(name, Buffer);
	if (FileLen <= 0)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	int version = String::Atoi(GetLine(ReadPos));
	if (version != Q1_SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, Q1_SAVEGAME_VERSION);
		return;
	}
	GetLine(ReadPos);
	float spawn_parms[NUM_SPAWN_PARMS];
	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		spawn_parms[i] = String::Atof(GetLine(ReadPos));
	}
	// this silliness is so we can load 1.06 save files, which have float skill values
	svqh_current_skill = (int)(String::Atof(GetLine(ReadPos)) + 0.1);
	Cvar_SetValue("skill", (float)svqh_current_skill);

	char mapname[MAX_QPATH];
	String::Cpy(mapname, GetLine(ReadPos));
	float time = String::Atof(GetLine(ReadPos));

	CL_Disconnect(true);
	SVQH_Shutdown();

	SVQH_SpawnServer(mapname, "");
	if (sv.state == SS_DEAD)
	{
		common->Printf("Couldn't load map\n");
		return;
	}
	sv.qh_paused = true;		// pause until all clients connect
	sv.loadgame = true;

	// load the light styles

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		char* Style = GetLine(ReadPos);
		char* Tmp = (char*)Mem_Alloc(String::Length(Style) + 1);
		String::Cpy(Tmp, Style);
		sv.qh_lightstyles[i] = Tmp;
	}

	// load the edicts out of the savegame file
	int entnum = -1;		// -1 is the globals
	const char* start = ReadPos;
	while (start)
	{
		const char* token = String::Parse1(&start);
		if (!start)
		{
			break;		// end of file
		}
		if (String::Cmp(token,"{"))
		{
			common->FatalError("First token isn't a brace %s", token);
		}

		if (entnum == -1)
		{
			// parse the global vars
			start = ED_ParseGlobals(start);
		}
		else
		{
			// parse an edict
			qhedict_t* ent = QH_EDICT_NUM(entnum);
			Com_Memset(&ent->v, 0, progs->entityfields * 4);
			ent->free = false;
			start = ED_ParseEdict(start, ent);

			// link it into the bsp tree
			if (!ent->free)
			{
				SVQH_LinkEdict(ent, false);
			}
		}

		entnum++;
	}

	sv.qh_num_edicts = entnum;
	sv.qh_time = time;

	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		svs.clients->qh_spawn_parms[i] = spawn_parms[i];
	}

	if (!com_dedicated->integer)
	{
		CLQH_EstablishConnection("local");
	}
}

void SVH2_RemoveGIPFiles(const char* path)
{
	if (!fs_homepath)
	{
		return;
	}
	char* netpath;
	if (path)
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, path);
	}
	else
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "");
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ".gip", NULL, &numSysFiles, false);
	for (int i = 0; i < numSysFiles; i++)
	{
		if (path)
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s/%s", path, sysFiles[i]));
		}
		else
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, sysFiles[i]);
		}
		FS_Remove(netpath);
	}
	Sys_FreeFileList(sysFiles);
}

static void SVH2_CopyFiles(const char* source, const char* ext, const char* dest)
{
	char* netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, source);
	if (!source[0])
	{
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ext, NULL, &numSysFiles, false);
	for (int i = 0; i < numSysFiles; i++)
	{
		char* srcpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", source, sysFiles[i]));
		char* dstpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", dest, sysFiles[i]));
		FS_CopyFile(srcpath, dstpath);
	}
	Sys_FreeFileList(sysFiles);
}

static void SVH2_Savegame_f()
{
	if (sv.state == SS_DEAD)
	{
		common->Printf("Not playing a local game.\n");
		return;
	}

	if (CLQH_GetIntermission())
	{
		common->Printf("Can't save in intermission.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		common->Printf("Relative pathnames are not allowed.\n");
		return;
	}

	for (int i = 0; i < svs.qh_maxclients; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED && (svs.clients[i].qh_edict->GetHealth() <= 0))
		{
			common->Printf("Can't savegame with a dead player\n");
			return;
		}
	}

	SVH2_SaveGamestate(false);

	SVH2_RemoveGIPFiles(Cmd_Argv(1));

	char* netname = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "clients.gip");
	FS_Remove(netname);

	char dest[MAX_OSPATH];
	sprintf(dest, "%s/", Cmd_Argv(1));
	common->Printf("Saving game to %s...\n", Cmd_Argv(1));

	SVH2_CopyFiles("", ".gip", dest);

	sprintf(dest,"%s/info.dat", Cmd_Argv(1));
	fileHandle_t f = FS_FOpenFileWrite(dest);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", H2_SAVEGAME_VERSION);
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	SVH2_SavegameComment(comment);
	FS_Printf(f, "%s\n", comment);
	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		FS_Printf(f, "%f\n", svs.clients->qh_spawn_parms[i]);
	}
	FS_Printf(f, "%d\n", svqh_current_skill);
	FS_Printf(f, "%s\n", sv.name);
	FS_Printf(f, "%f\n", sv.qh_time);
	FS_Printf(f, "%d\n", svs.qh_maxclients);
	FS_Printf(f, "%f\n", svqh_deathmatch->value);
	FS_Printf(f, "%f\n", svqh_coop->value);
	FS_Printf(f, "%f\n", svqh_teamplay->value);
	FS_Printf(f, "%f\n", h2_randomclass->value);
	FS_Printf(f, "%f\n", Cvar_VariableValue("_cl_playerclass"));
	FS_Printf(f, "%d\n", info_mask);
	FS_Printf(f, "%d\n", info_mask2);

	FS_FCloseFile(f);
}

static void SVH2_Loadgame_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("load <savename> : load a game\n");
		return;
	}

	CLQH_StopDemoLoop();		// stop demo loop in case this fails
	CL_Disconnect(true);
	SVH2_RemoveGIPFiles(NULL);

	common->Printf("Loading game from %s...\n", Cmd_Argv(1));

	char dest[MAX_OSPATH];
	sprintf(dest, "%s/info.dat", Cmd_Argv(1));

	Array<byte> Buffer;
	int Len = FS_ReadFile(dest, Buffer);
	if (Len <= 0)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	int version = String::Atoi(GetLine(ReadPos));

	if (version != H2_SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, H2_SAVEGAME_VERSION);
		return;
	}
	GetLine(ReadPos);
	float spawn_parms[NUM_SPAWN_PARMS];
	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		spawn_parms[i] = String::Atof(GetLine(ReadPos));
	}
	// this silliness is so we can load 1.06 save files, which have float skill values
	svqh_current_skill = (int)(String::Atof(GetLine(ReadPos)) + 0.1);
	Cvar_SetValue("skill", (float)svqh_current_skill);

	Cvar_SetValue("deathmatch", 0);
	Cvar_SetValue("coop", 0);
	Cvar_SetValue("teamplay", 0);
	Cvar_SetValue("randomclass", 0);

	char mapname[MAX_QPATH];
	String::Cpy(mapname, GetLine(ReadPos));
	//	time, ignored
	GetLine(ReadPos);

	int tempi = -1;
	tempi = String::Atoi(GetLine(ReadPos));
	if (tempi >= 1)
	{
		svs.qh_maxclients = tempi;
	}

	float tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("deathmatch", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("coop", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("teamplay", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("randomclass", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("_cl_playerclass", tempf);
	}

	info_mask = String::Atoi(GetLine(ReadPos));
	info_mask2 = String::Atoi(GetLine(ReadPos));

	sprintf(dest, "%s/", Cmd_Argv(1));
	SVH2_CopyFiles(dest, ".gip", "");

	SVH2_LoadGamestate(mapname, NULL, 2);

	SVQH_SaveSpawnparms();

	qhedict_t* ent = QH_EDICT_NUM(1);

	Cvar_SetValue("_cl_playerclass", ent->GetPlayerClass());//this better be the same as above...

	if (GGameType & GAME_H2Portals)
	{
		// this may be rudundant with the setting in PR_LoadProgs, but not sure so its here too
		*pr_globalVars.cl_playerclass = ent->GetPlayerClass();
	}

	svs.clients->h2_playerclass = ent->GetPlayerClass();

	sv.qh_paused = true;		// pause until all clients connect
	sv.loadgame = true;

	if (!com_dedicated->integer)
	{
		CLQH_EstablishConnection("local");
	}
}

static void SVQH_ConStatus_f()
{
	if (sv.state == SS_DEAD)
	{
		CL_ForwardKnownCommandToServer();
		return;
	}

	common->Printf("host:    %s\n", Cvar_VariableString("hostname"));
	common->Printf("version: " JLQUAKE_VERSION_STRING "\n");
	SOCK_ShowIP();
	common->Printf("map:     %s\n", sv.name);
	common->Printf("players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
	client_t* client = svs.clients;
	for (int j = 0; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		int seconds = (int)(net_time - client->qh_netconnection->connecttime);
		int minutes = seconds / 60;
		int hours = 0;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
			{
				minutes -= (hours * 60);
			}
		}
		else
		{
			hours = 0;
		}
		common->Printf("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
		common->Printf("   %s\n", client->qh_netconnection->address);
	}
}

static void SVQHW_Status_f()
{
	float cpu = (svs.qh_stats.latched_active + svs.qh_stats.latched_idle);
	if (cpu)
	{
		cpu = 100 * svs.qh_stats.latched_active / cpu;
	}
	float avg = 1000 * svs.qh_stats.latched_active / QHW_STATFRAMES;
	float pak = (float)svs.qh_stats.latched_packets / QHW_STATFRAMES;

	SOCK_ShowIP();
	common->Printf("port             : %d\n", svqhw_net_port);
	common->Printf("cpu utilization  : %3i%%\n",(int)cpu);
	common->Printf("avg response time: %i ms\n",(int)avg);
	common->Printf("packets/frame    : %5.2f\n", pak);
	int t_limit = Cvar_VariableValue("timelimit");
	int f_limit = Cvar_VariableValue("fraglimit");
	if (GGameType & GAME_HexenWorld && hw_dmMode->value == HWDM_SIEGE)
	{
		int num_min = floor((t_limit * 60) - sv.qh_time);
		int num_sec = (int)(t_limit - num_min) % 60;
		num_min = (num_min - num_sec) / 60;
		common->Printf("timeleft         : %i:", num_min);
		common->Printf("%2i\n", num_sec);
		common->Printf("deflosses        : %3i/%3i\n", static_cast<int>(floor(*pr_globalVars.defLosses)), f_limit);
		common->Printf("attlosses        : %3i/%3i\n", static_cast<int>(floor(*pr_globalVars.attLosses)), f_limit * 2);
	}
	else
	{
		common->Printf("time             : %5.2f\n", sv.qh_time);
		common->Printf("timelimit        : %i\n", t_limit);
		common->Printf("fraglimit        : %i\n", f_limit);
	}

	// min fps lat drp
	if (rd_buffer)
	{
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		common->Printf("name               userid frags\n");
		common->Printf("  address          ping drop\n");
		common->Printf("  ---------------- ---- -----\n");
		client_t* cl = svs.clients;
		for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}

			common->Printf("%-16.16s  ", cl->name);

			common->Printf("%6i %5i", cl->qh_userid, (int)cl->qh_edict->GetFrags());
			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else
			{
				common->Printf("\n");
			}

			const char* s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("  %-16.16s", s);
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %5.2f\n",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence);
		}
	}
	else
	{
		if (GGameType & GAME_HexenWorld)
		{
			common->Printf("frags userid address         name            ping drop  siege\n");
		}
		else
		{
			common->Printf("frags userid address         name            ping drop  qport\n");
		}
		common->Printf("----- ------ --------------- --------------- ---- ----- -----\n");
		client_t* cl = svs.clients;
		for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}
			common->Printf("%5i %6i ", (int)cl->qh_edict->GetFrags(),  cl->qh_userid);

			const char* s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("%s", s);
			int l = 16 - String::Length(s);
			for (int j = 0; j < l; j++)
			{
				common->Printf(" ");
			}

			common->Printf("%s", cl->name);
			l = 16 - String::Length(cl->name);
			for (int j = 0; j < l; j++)
			{
				common->Printf(" ");
			}
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %3.1f",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence);
			if (GGameType & GAME_QuakeWorld)
			{
				common->Printf(" %4i", cl->netchan.qport);
			}

			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else if (GGameType & GAME_HexenWorld)
			{
				common->Printf(" ");
				switch (cl->h2_playerclass)
				{
				case CLASS_PALADIN:
					common->Printf("P");
					break;
				case CLASS_CLERIC:
					common->Printf("C");
					break;
				case CLASS_NECROMANCER:
					common->Printf("N");
					break;
				case CLASS_THEIF:
					common->Printf("A");
					break;
				case CLASS_DEMON:
					common->Printf("S");
					break;
				case CLASS_DWARF:
					common->Printf("D");
					break;
				default:
					common->Printf("?");
					break;
				}
				switch (cl->hw_siege_team)
				{
				case HWST_DEFENDER:
					common->Printf("D");
					break;
				case HWST_ATTACKER:
					common->Printf("A");
					break;
				default:
					common->Printf("?");
					break;
				}
				if ((int)cl->h2_old_v.flags2 & 65536)	//defender of crown
				{
					common->Printf("D");
				}
				else
				{
					common->Printf("-");
				}
				if ((int)cl->h2_old_v.flags2 & 524288)	//has siege key
				{
					common->Printf("K");
				}
				else
				{
					common->Printf("-");
				}
				common->Printf("\n");
			}
			else
			{
				common->Printf("\n");
			}
		}
	}
	common->Printf("\n");
}

static void SVQH_ConSay_f()
{
	if (Cmd_Argc() < 2)
	{
		return;
	}

	char* p = Cmd_ArgsUnmodified();
	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	// turn on color set 1
	char text[64];
	sprintf(text, "%c<%s> ", 1, sv_hostname->string);

	int j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	client_t* client = svs.clients;
	for (j = 0; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
	}
}

static void SVQHW_ConSay_f()
{
	if (Cmd_Argc() < 2)
	{
		return;
	}

	char text[1024];
	if (GGameType & GAME_HexenWorld)
	{
		if (hw_dmMode->value == HWDM_SIEGE)
		{
			String::Cpy(text, "GOD SAYS: ");
		}
		else
		{
			String::Cpy(text, "ServerAdmin: ");
		}
	}
	else
	{
		String::Cpy(text, "console: ");
	}

	char* p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}

//	Kicks a user off of the server
static void SVQH_ConKick_f()
{
	if (sv.state == SS_DEAD)
	{
		CL_ForwardKnownCommandToServer();
		return;
	}

	int i;
	client_t* kicked_client;
	bool byNumber = false;
	if (Cmd_Argc() > 2 && String::Cmp(Cmd_Argv(1), "#") == 0)
	{
		i = String::Atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.qh_maxclients)
		{
			return;
		}
		if (svs.clients[i].state < CS_CONNECTED)
		{
			return;
		}
		kicked_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		kicked_client = svs.clients;
		for (i = 0; i < svs.qh_maxclients; i++, kicked_client++)
		{
			if (kicked_client->state < CS_CONNECTED)
			{
				continue;
			}
			if (String::ICmp(kicked_client->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.qh_maxclients)
	{
		const char* who;
		if (com_dedicated->integer)
		{
			who = "Console";
		}
		else
		{
			who = Cvar_VariableString("_cl_name");
		}

		const char* message = NULL;
		if (Cmd_Argc() > 2)
		{
			message = Cmd_ArgsUnmodified();
			String::Parse1(&message);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
				{
					message++;
				}
				message += String::Length(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
			{
				message++;
			}
		}
		if (message)
		{
			SVQH_ClientPrintf(kicked_client, 0, "Kicked by %s: %s\n", who, message);
		}
		else
		{
			SVQH_ClientPrintf(kicked_client, 0, "Kicked by %s\n", who);
		}
		SVQH_DropClient(kicked_client, false);
	}
}

//	Kick a user off of the server
static void SVQHW_Kick_f()
{
	int uid = String::Atoi(Cmd_Argv(1));

	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SVQH_BroadcastPrintf message
			SVQH_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game\n");
			SVQHW_DropClient(cl);

			if (GGameType & GAME_HexenWorld)
			{
				*pr_globalVars.time = sv.qh_time;
				*pr_globalVars.self = EDICT_TO_PROG(cl->qh_edict);
				PR_ExecuteProgram(*pr_globalVars.ClientKill);
			}
			return;
		}
	}

	common->Printf("Couldn't find user number %i\n", uid);
}

static void SVHW_Smite_f()
{
	int uid = String::Atoi(Cmd_Argv(1));

	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			if (cl->h2_old_v.health <= 0)
			{
				common->Printf("%s is already dead!\n", cl->name);
				return;
			}
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s was Smitten by GOD!\n", cl->name);

			//save this state
			int old_self = *pr_globalVars.self;

			//call the hc SmitePlayer function
			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(cl->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.SmitePlayer);

			//restore current state
			*pr_globalVars.self = old_self;
			return;
		}
	}
	common->Printf("Couldn't find user number %i\n", uid);
}

//	Make a master server current
static void SVQHW_SetMaster_f()
{
	Com_Memset(&master_adr, 0, sizeof(master_adr));

	for (int i = 1; i < Cmd_Argc(); i++)
	{
		if (!String::Cmp(Cmd_Argv(i), "none") || !SOCK_StringToAdr(Cmd_Argv(i), &master_adr[i - 1], GGameType & GAME_HexenWorld ? HWPORT_MASTER : QWPORT_MASTER))
		{
			common->Printf("Setting nomaster mode.\n");
			return;
		}

		common->Printf("Master server at %s\n", SOCK_AdrToString(master_adr[i - 1]));

		common->Printf("Sending a ping.\n");

		char data[2];
		data[0] = A2A_PING;
		data[1] = 0;
		NET_SendPacket(NS_SERVER, 2, data, master_adr[i - 1]);
	}

	svs.qh_last_heartbeat = -99999;
}

static void SVQHW_Heartbeat_f()
{
	svs.qh_last_heartbeat = -9999;
}

static void SVQHW_Fraglogfile_f()
{
	if (svqhw_fraglogfile)
	{
		common->Printf("Frag file logging off.\n");
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
		return;
	}

	// find an unused name
	int i;
	char name[MAX_OSPATH];
	for (i = 0; i < 1000; i++)
	{
		sprintf(name, "frag_%i.log", i);
		if (!FS_FileExists(name))
		{
			// can't read it, so create this one
			svqhw_fraglogfile = FS_FOpenFileWrite(name);
			if (!svqhw_fraglogfile)
			{
				i = 1000;	// give error
			}
			break;
		}
	}
	if (i == 1000)
	{
		common->Printf("Can't open any logfiles.\n");
		svqhw_fraglogfile = 0;
		return;
	}

	common->Printf("Logging frags to %s.\n", name);
}

static client_t* SVQHW_SetPlayer()
{
	int idnum = String::Atoi(Cmd_Argv(1));

	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == idnum)
		{
			return cl;
		}
	}
	common->Printf("Userid %i is not on the server\n", idnum);
	return NULL;
}

//	Sets client to godmode
static void SVQHW_God_f()
{
	if (!svqhw_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	client_t* client = SVQHW_SetPlayer();
	if (!client)
	{
		return;
	}

	client->qh_edict->SetFlags((int)client->qh_edict->GetFlags() ^ QHFL_GODMODE);
	if (!((int)client->qh_edict->GetFlags() & QHFL_GODMODE))
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "godmode OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "godmode ON\n");
	}
}

static void SVQHW_Noclip_f()
{
	if (!svqhw_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	client_t* client = SVQHW_SetPlayer();
	if (!client)
	{
		return;
	}

	if (client->qh_edict->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_NOCLIP);
		SVQH_ClientPrintf(client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(client, PRINT_HIGH, "noclip OFF\n");
	}
}

static void SVQHW_Give_f()
{
	if (!svqhw_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	client_t* client = SVQHW_SetPlayer();
	if (!client)
	{
		return;
	}

	char* t = Cmd_Argv(2);
	int v = String::Atoi(Cmd_Argv(3));

	if (GGameType & GAME_HexenWorld)
	{
		switch (t[0])
		{
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			client->qh_edict->SetItems((int)client->qh_edict->GetItems() | H2IT_WEAPON2 << (t[0] - '2'));
			break;
		case 'h':
			client->qh_edict->SetHealth(v);
			break;
		}
	}
	else
	{
		switch (t[0])
		{
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			client->qh_edict->SetItems((int)client->qh_edict->GetItems() | Q1IT_SHOTGUN << (t[0] - '2'));
			break;

		case 's':
			client->qh_edict->SetAmmoShells(v);
			break;
		case 'n':
			client->qh_edict->SetAmmoNails(v);
			break;
		case 'r':
			client->qh_edict->SetAmmoRockets(v);
			break;
		case 'h':
			client->qh_edict->SetHealth(v);
			break;
		case 'c':
			client->qh_edict->SetAmmoCells(v);
			break;
		}
	}
}

void SVQW_SendServerInfoChange(const char* key, const char* value)
{
	if (!sv.state)
	{
		return;
	}

	sv.qh_reliable_datagram.WriteByte(qwsvc_serverinfo);
	sv.qh_reliable_datagram.WriteString2(key);
	sv.qh_reliable_datagram.WriteString2(value);
}

//  Examine or change the serverinfo string
static void SVQHW_Serverinfo_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Server info settings:\n");
		Info_Print(svs.qh_info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(svs.qh_info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value, false);

	// if this is a cvar, change it too
	Cvar_UpdateIfExists(Cmd_Argv(1), Cmd_Argv(2));

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_BroadcastCommand("fullserverinfo \"%s\"\n", svs.qh_info);
	}
	else
	{
		SVQW_SendServerInfoChange(Cmd_Argv(1), Cmd_Argv(2));
	}
}

static void SVQHW_Localinfo_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Local info settings:\n");
		Info_Print(qhw_localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(qhw_localinfo, Cmd_Argv(1), Cmd_Argv(2), QHMAX_LOCALINFO_STRING, 64, 64, !svqh_highchars->value, false);
}

//	Examine a users info strings
static void SVQHW_User_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: info <userid>\n");
		return;
	}

	client_t* client = SVQHW_SetPlayer();
	if (!client)
	{
		return;
	}

	Info_Print(client->userinfo);
}

static void SVQHW_Floodprot_f()
{
	if (Cmd_Argc() == 1)
	{
		if (qhw_fp_messages)
		{
			common->Printf("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n",
				qhw_fp_messages, qhw_fp_persecond, qhw_fp_secondsdead);
			return;
		}
		else
		{
			common->Printf("No floodprots enabled.\n");
		}
	}

	if (Cmd_Argc() != 4)
	{
		common->Printf("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		common->Printf("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	int arg1 = String::Atoi(Cmd_Argv(1));
	int arg2 = String::Atoi(Cmd_Argv(2));
	int arg3 = String::Atoi(Cmd_Argv(3));

	if (arg1 <= 0 || arg2 <= 0 || arg3 <= 0)
	{
		common->Printf("All values must be positive numbers\n");
		return;
	}

	if (arg1 > 10)
	{
		common->Printf("Can only track up to 10 messages.\n");
		return;
	}

	qhw_fp_messages = arg1;
	qhw_fp_persecond = arg2;
	qhw_fp_secondsdead = arg3;
}

static void SVQHW_Floodprotmsg_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Current msg: %s\n", qhw_fp_msg);
		return;
	}
	else if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: floodprotmsg \"<message>\"\n");
		return;
	}
	sprintf(qhw_fp_msg, "%s", Cmd_Argv(1));
}

//	Sets the gamedir and path to a different directory.
static void SVQHW_Gamedir_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Current gamedir: %s\n", fs_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: gamedir <newdir>\n");
		return;
	}

	char* dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	FS_SetGamedirQHW(dir);
	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

//	Sets the fake *gamedir to a different directory.
void SVQHW_FakeGamedir_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Current *gamedir: %s\n", Info_ValueForKey(svs.qh_info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	char* dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

static void SVQW_Snap(int uid)
{
	int i;
	client_t* cl = svs.clients;
	for (i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			break;
		}
	}
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Printf("userid not found\n");
		return;
	}

	char pcxname[80];
	sprintf(pcxname, "%d-00.pcx", uid);

	for (i = 0; i <= 99; i++)
	{
		pcxname[String::Length(pcxname) - 6] = i / 10 + '0';
		pcxname[String::Length(pcxname) - 5] = i % 10 + '0';
		char checkname[MAX_OSPATH];
		sprintf(checkname, "snap/%s", pcxname);
		if (!FS_FileExists(checkname))
		{
			break;	// file doesn't exist
		}
	}
	if (i == 100)
	{
		common->Printf("Snap: Couldn't create a file, clean some out.\n");
		return;
	}
	char checkname[MAX_OSPATH];
	sprintf(checkname, "snap/%s", pcxname);
	String::Cpy(cl->qw_uploadfn, checkname);

	Com_Memcpy(&cl->qw_snap_from, &rcon_from, sizeof(rcon_from));
	if (rd_buffer)
	{
		cl->qw_remote_snap = true;
	}
	else
	{
		cl->qw_remote_snap = false;
	}

	SVQH_SendClientCommand(cl, "cmd snap");
	common->Printf("Requesting snap from user %d...\n", uid);
}

static void SVQW_Snap_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage:  snap <userid>\n");
		return;
	}

	int uid = String::Atoi(Cmd_Argv(1));

	SVQW_Snap(uid);
}

static void SVQW_SnapAll_f()
{
	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (cl->state < CS_CONNECTED || cl->qh_spectator)
		{
			continue;
		}
		SVQW_Snap(cl->qh_userid);
	}
}

void SVQH_InitOperatorCommands()
{
	Cmd_AddCommand("maxplayers", MaxPlayers_f);
	Cmd_AddCommand("map", SVQH_Map_f);
	Cmd_AddCommand("changelevel", SVQH_Changelevel_f);
	Cmd_AddCommand("status", SVQH_ConStatus_f);
	Cmd_AddCommand("kick", SVQH_ConKick_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SVQH_ConSay_f);
	}
	if (GGameType & GAME_Quake)
	{
		Cmd_AddCommand("restart", SVQ1_Restart_f);
		Cmd_AddCommand("save", SVQ1_Savegame_f);
		Cmd_AddCommand("load", SVQ1_Loadgame_f);
	}
	else
	{
		Cmd_AddCommand("restart", SVH2_Restart_f);
		Cmd_AddCommand("changelevel2", SVH2_Changelevel2_f);
		Cmd_AddCommand("save", SVH2_Savegame_f);
		Cmd_AddCommand("load", SVH2_Loadgame_f);
	}
}

void SVQHW_InitOperatorCommands()
{
	if (COM_CheckParm("-cheats"))
	{
		svqhw_allow_cheats = true;
		Info_SetValueForKey(svs.qh_info, "*cheats", "ON", MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}

	Cmd_AddCommand("map", SVQHW_Map_f);
	Cmd_AddCommand("status", SVQHW_Status_f);
	Cmd_AddCommand("say", SVQHW_ConSay_f);
	Cmd_AddCommand("kick", SVQHW_Kick_f);
	Cmd_AddCommand("setmaster", SVQHW_SetMaster_f);
	Cmd_AddCommand("heartbeat", SVQHW_Heartbeat_f);
	Cmd_AddCommand("fraglogfile", SVQHW_Fraglogfile_f);
	Cmd_AddCommand("god", SVQHW_God_f);
	Cmd_AddCommand("noclip", SVQHW_Noclip_f);
	Cmd_AddCommand("give", SVQHW_Give_f);
	Cmd_AddCommand("serverinfo", SVQHW_Serverinfo_f);
	Cmd_AddCommand("localinfo", SVQHW_Localinfo_f);
	Cmd_AddCommand("user", SVQHW_User_f);
	Cmd_AddCommand("floodprot", SVQHW_Floodprot_f);
	Cmd_AddCommand("floodprotmsg", SVQHW_Floodprotmsg_f);
	Cmd_AddCommand("gamedir", SVQHW_Gamedir_f);
	Cmd_AddCommand("sv_gamedir", SVQHW_FakeGamedir_f);
	if (GGameType & GAME_QuakeWorld)
	{
		Cmd_AddCommand("snap", SVQW_Snap_f);
		Cmd_AddCommand("snapall", SVQW_SnapAll_f);
	}
	else
	{
		Cmd_AddCommand("smite", SVHW_Smite_f);
	}
}
