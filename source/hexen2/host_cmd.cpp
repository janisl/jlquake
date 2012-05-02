/*
 * $Header: /H2 Mission Pack/Host_cmd.c 25    4/01/98 4:53p Jmonroe $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#undef GetClassName
#endif
#include <time.h>

extern Cvar* pausable;
extern Cvar* sv_flypitch;
extern Cvar* sv_walkpitch;

int current_skill;
static double old_time;

void RestoreClients(void);

unsigned int info_mask, info_mask2;

#define TESTSAVE

int LoadGamestate(char* level, char* startspot, int ClientsMode);

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f(void);

void Host_Quit_f(void)
{
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state != CA_DEDICATED)
	{
		M_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
	Host_ShutdownServer(false);

	Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f(void)
{
	client_t* client;
	int seconds;
	int minutes;
	int hours = 0;
	int j;
	void (* print)(const char* fmt, ...);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer();
			return;
		}
		print = Con_Printf;
	}
	else
	{
		print = SV_ClientPrintf;
	}

	print("host:    %s\n", Cvar_VariableString("hostname"));
	print("version: %4.2f\n", HEXEN2_VERSION);
	SOCK_ShowIP();
	print("map:     %s\n", sv.name);
	print("players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients);
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active)
		{
			continue;
		}
		seconds = (int)(net_time - client->netconnection->connecttime);
		minutes = seconds / 60;
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
		print("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->edict->GetFrags(), hours, minutes, seconds);
		print("   %s\n", client->netconnection->address);
	}
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if ((pr_global_struct->deathmatch ||
		 pr_global_struct->coop || skill->value > 2) && !host_client->privileged)
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ FL_GODMODE);
	if (!((int)sv_player->GetFlags() & FL_GODMODE))
	{
		SV_ClientPrintf("godmode OFF\n");
	}
	else
	{
		SV_ClientPrintf("godmode ON\n");
	}
}

void Host_Notarget_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if ((pr_global_struct->deathmatch || skill->value > 2) && !host_client->privileged)
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ FL_NOTARGET);
	if (!((int)sv_player->GetFlags() & FL_NOTARGET))
	{
		SV_ClientPrintf("notarget OFF\n");
	}
	else
	{
		SV_ClientPrintf("notarget ON\n");
	}
}

void Host_Noclip_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if ((pr_global_struct->deathmatch ||
		 pr_global_struct->coop || skill->value > 2) && !host_client->privileged)
	{
		return;
	}

	if (sv_player->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		sv_player->SetMoveType(QHMOVETYPE_NOCLIP);
		SV_ClientPrintf("noclip ON\n");
	}
	else
	{
		sv_player->SetMoveType(QHMOVETYPE_WALK);
		SV_ClientPrintf("noclip OFF\n");
	}
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f(void)
{
	int i, j;
	float total;
	client_t* client;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	SV_ClientPrintf("Client ping times:\n");
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
		{
			continue;
		}
		total = 0;
		for (j = 0; j < NUM_PING_TIMES; j++)
			total += client->ping_times[j];
		total /= NUM_PING_TIMES;
		SV_ClientPrintf("%4i %s\n", (int)(total * 1000), client->name);
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f(void)
{
	int i;
	char name[MAX_QPATH];

	if (Cmd_Argc() < 2)		//no map name given
	{
		Con_Printf("map <levelname>: start a new server\nCurrently on: %s\n",cl.qh_levelname);
		return;
	}

	if (cmd_source != src_command)
	{
		return;
	}

	cls.qh_demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect();
	Host_ShutdownServer(false);

	in_keyCatchers = 0;			// remove console or menu
	SCR_BeginLoadingPlaque();

	info_mask = 0;
	if (!coop->value && deathmatch->value)
	{
		info_mask2 = 0x80000000;
	}
	else
	{
		info_mask2 = 0;
	}

	svs.serverflags = 0;			// haven't completed an episode yet
	String::Cpy(name, Cmd_Argv(1));
	SV_SpawnServer(name, NULL);
	if (!sv.active)
	{
		return;
	}

	if (cls.state != CA_DEDICATED)
	{
		loading_stage = 2;

		String::Cpy(cls.qh_spawnparms, "");

		for (i = 2; i < Cmd_Argc(); i++)
		{
			String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), Cmd_Argv(i));
			String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), " ");
		}

		Cmd_ExecuteString("connect local", src_command);
	}
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f(void)
{
	char level[MAX_QPATH];
	char _startspot[MAX_QPATH];
	char* startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (!sv.active || clc.demoplaying)
	{
		Con_Printf("Only the server may changelevel\n");
		return;
	}

	String::Cpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms();
	SV_SpawnServer(level, startspot);

	//updatePlaqueMessage();
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f(void)
{
	char mapname[MAX_QPATH];
	char startspot[MAX_QPATH];

	if (clc.demoplaying || !sv.active)
	{
		return;
	}

	if (cmd_source != src_command)
	{
		return;
	}

	String::Cpy(mapname, sv.name);	// must copy out, because it gets cleared
	String::Cpy(startspot, sv.startspot);

	if (Cmd_Argc() == 2 && String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		if (LoadGamestate(mapname, startspot, 3))
		{
			SV_SpawnServer(mapname, startspot);
			RestoreClients();
		}
	}
	else
	{
		// in sv_spawnserver
		SV_SpawnServer(mapname, startspot);
	}

//	updatePlaqueMessage();
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f(void)
{
	CL_ClearParticles();	//jfm: for restarts which didn't use to clear parts.

	//updatePlaqueMessage();

	SCR_BeginLoadingPlaque();
	clc.qh_signon = 0;		// need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f(void)
{
	char name[MAX_QPATH];

	cls.qh_demonum = -1;		// stop demo loop in case this fails
	if (clc.demoplaying)
	{
		CL_StopPlayback();
		CL_Disconnect();
	}
	String::Cpy(name, Cmd_Argv(1));
	CL_EstablishConnection(name);
	Host_Reconnect_f();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define SAVEGAME_VERSION    5

#define ShortTime "%m/%d/%Y %H:%M"


/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(char* text)
{
	int i;
	char kills[20];
	struct tm* tblock;
	time_t TempTime;

	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		text[i] = ' ';
	Com_Memcpy(text, cl.qh_levelname, String::Length(cl.qh_levelname));
//	sprintf (kills,"kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);

	TempTime = time(NULL);
	tblock = localtime(&TempTime);
	strftime(kills,sizeof(kills),ShortTime,tblock);

	Com_Memcpy(text + 21, kills, String::Length(kills));
// convert space to _ to make stdio happy
	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		if (text[i] == ' ')
		{
			text[i] = '_';
		}
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f(void)
{
	fileHandle_t f;
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	char dest[MAX_OSPATH];

	if (cmd_source != src_command)
	{
		return;
	}

	if (!sv.active)
	{
		Con_Printf("Not playing a local game.\n");
		return;
	}

	if (cl.qh_intermission)
	{
		Con_Printf("Can't save in intermission.\n");
		return;
	}

#ifndef TESTSAVE
	if (svs.maxclients != 1)
	{
		Con_Printf("Can't save multiplayer games.\n");
		return;
	}
#endif

	if (Cmd_Argc() != 2)
	{
		Con_Printf("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf("Relative pathnames are not allowed.\n");
		return;
	}

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->GetHealth() <= 0))
		{
			Con_Printf("Can't savegame with a dead player\n");
			return;
		}
	}


	SaveGamestate(false);

	CL_RemoveGIPFiles(Cmd_Argv(1));

	char* netname = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "clients.gip");
	FS_Remove(netname);

	sprintf(dest, "%s/", Cmd_Argv(1));
	Con_Printf("Saving game to %s...\n", Cmd_Argv(1));

	CL_CopyFiles("", ".gip", dest);

	sprintf(dest,"%s/info.dat", Cmd_Argv(1));
	f = FS_FOpenFileWrite(dest);
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment(comment);
	FS_Printf(f, "%s\n", comment);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		FS_Printf(f, "%f\n", svs.clients->spawn_parms[i]);
	FS_Printf(f, "%d\n", current_skill);
	FS_Printf(f, "%s\n", sv.name);
	FS_Printf(f, "%f\n",sv.time);
	FS_Printf(f, "%d\n",svs.maxclients);
	FS_Printf(f, "%f\n",deathmatch->value);
	FS_Printf(f, "%f\n",coop->value);
	FS_Printf(f, "%f\n",teamplay->value);
	FS_Printf(f, "%f\n",randomclass->value);
	FS_Printf(f, "%f\n",cl_playerclass->value);
	FS_Printf(f, "%d\n",info_mask);
	FS_Printf(f, "%d\n",info_mask2);

	FS_FCloseFile(f);
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

/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f(void)
{
	char mapname[MAX_QPATH];
	float time;
	int i;
	qhedict_t* ent;
	int version;
	float tempf;
	int tempi;
	float spawn_parms[NUM_SPAWN_PARMS];
	char dest[MAX_OSPATH];

	if (cmd_source != src_command)
	{
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf("load <savename> : load a game\n");
		return;
	}

	cls.qh_demonum = -1;		// stop demo loop in case this fails
	CL_Disconnect();
	CL_RemoveGIPFiles(NULL);

	Con_Printf("Loading game from %s...\n", Cmd_Argv(1));

	sprintf(dest, "%s/info.dat", Cmd_Argv(1));

	Array<byte> Buffer;
	int Len = FS_ReadFile(dest, Buffer);
	if (Len <= 0)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	version = String::Atoi(GetLine(ReadPos));

	if (version != SAVEGAME_VERSION)
	{
		Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return;
	}
	GetLine(ReadPos);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		spawn_parms[i] = String::Atof(GetLine(ReadPos));
	}
	// this silliness is so we can load 1.06 save files, which have float skill values
	current_skill = (int)(String::Atof(GetLine(ReadPos)) + 0.1);
	Cvar_SetValue("skill", (float)current_skill);

	Cvar_SetValue("deathmatch", 0);
	Cvar_SetValue("coop", 0);
	Cvar_SetValue("teamplay", 0);
	Cvar_SetValue("randomclass", 0);

	String::Cpy(mapname, GetLine(ReadPos));
	time = String::Atof(GetLine(ReadPos));

	tempi = -1;
	tempi = String::Atoi(GetLine(ReadPos));
	if (tempi >= 1)
	{
		svs.maxclients = tempi;
	}

	tempf = String::Atof(GetLine(ReadPos));
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
	CL_CopyFiles(dest, ".gip", "");

	LoadGamestate(mapname, NULL, 2);

	SV_SaveSpawnparms();

	ent = EDICT_NUM(1);

	Cvar_SetValue("_cl_playerclass", ent->GetPlayerClass());//this better be the same as above...

#ifdef MISSIONPACK
	// this may be rudundant with the setting in PR_LoadProgs, but not sure so its here too
	pr_global_struct->cl_playerclass = ent->GetPlayerClass();
#endif

	svs.clients->playerclass = ent->GetPlayerClass();

	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

	if (cls.state != CA_DEDICATED)
	{
		CL_EstablishConnection("local");
		Host_Reconnect_f();
	}
}

void SaveGamestate(qboolean ClientsOnly)
{
	char name[MAX_OSPATH];
	fileHandle_t f;
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	qhedict_t* ent;
	int start,end;

	if (ClientsOnly)
	{
		start = 1;
		end = svs.maxclients + 1;

		sprintf(name, "clients.gip");
	}
	else
	{
		start = 1;
		end = sv.num_edicts;

		sprintf(name, "%s.gip", sv.name);

//		Con_Printf ("Saving game to %s...\n", name);
	}

	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", SAVEGAME_VERSION);

	if (!ClientsOnly)
	{
		Host_SavegameComment(comment);
		FS_Printf(f, "%s\n", comment);
		FS_Printf(f, "%f\n", skill->value);
		FS_Printf(f, "%s\n", sv.name);
		FS_Printf(f, "%f\n", sv.time);

		// write the light styles

		for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
		{
			if (sv.lightstyles[i])
			{
				FS_Printf(f, "%s\n", sv.lightstyles[i]);
			}
			else
			{
				FS_Printf(f,"m\n");
			}
		}
		SV_SaveEffects(f);
		FS_Printf(f,"-1\n");
		ED_WriteGlobals(f);
	}

	host_client = svs.clients;

//	for (i=svs.maxclients+1 ; i<sv.num_edicts ; i++)
//  to save the client states
	for (i = start; i < end; i++)
	{
		ent = EDICT_NUM(i);
		if ((int)ent->GetFlags() & FL_ARCHIVE_OVERRIDE)
		{
			continue;
		}
		if (ClientsOnly)
		{
			if (host_client->active)
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

void RestoreClients(void)
{
	int i,j;
	qhedict_t* ent;
	double time_diff;

	if (LoadGamestate(NULL,NULL,1))
	{
		return;
	}

	time_diff = sv.time - old_time;

	for (i = 0,host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		if (host_client->active)
		{
			ent = host_client->edict;

			//ent->v.colormap = NUM_FOR_EDICT(ent);
			ent->SetTeam((host_client->colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->playerclass);

			// copy spawn parms out of the client_t

			for (j = 0; j < NUM_SPAWN_PARMS; j++)
				(&pr_global_struct->parm1)[j] = host_client->spawn_parms[j];

			// call the spawn function

			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(ent);
			G_FLOAT(OFS_PARM0) = time_diff;
			PR_ExecuteProgram(pr_global_struct->ClientReEnter);
		}
	SaveGamestate(true);
}

int LoadGamestate(char* level, char* startspot, int ClientsMode)
{
	char name[MAX_OSPATH];
	char mapname[MAX_QPATH];
	float time, sk;
	int i;
	qhedict_t* ent;
	int entnum;
	int version;
//	float	spawn_parms[NUM_SPAWN_PARMS];
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
			Con_Printf("Loading game from %s...\n", name);
		}
	}

	Array<byte> Buffer;
	int len = FS_ReadFile(name, Buffer);
	if (len <= 0)
	{
		if (ClientsMode == 2)
		{
			Con_Printf("ERROR: couldn't open.\n");
		}

		return -1;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	version = String::Atoi(GetLine(ReadPos));

	if (version != SAVEGAME_VERSION)
	{
		Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return -1;
	}

	if (ClientsMode != 1)
	{
		GetLine(ReadPos);
		sk = String::Atof(GetLine(ReadPos));
		Cvar_SetValue("skill", sk);

		String::Cpy(mapname, GetLine(ReadPos));
		time = String::Atof(GetLine(ReadPos));

		SV_SpawnServer(mapname, startspot);

		if (!sv.active)
		{
			Con_Printf("Couldn't load map\n");
			return -1;
		}

		// load the light styles
		for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
		{
			char* Style = GetLine(ReadPos);
			char* Tmp = (char*)Hunk_Alloc(String::Length(Style) + 1);
			String::Cpy(Tmp, Style);
			sv.lightstyles[i] = Tmp;
		}
		ReadPos = SV_LoadEffects(ReadPos);
	}

	// load the edicts out of the savegame file
	const char* start = ReadPos;
	while (start)
	{
		char* token = String::Parse1(&start);
		Log::writeLine("Token %s", token);
		if (!start)
		{
			break;		// end of file
		}
		entnum = String::Atoi(token);
		token = String::Parse1(&start);
		Log::writeLine("Token %s", token);
		if (String::Cmp(token, "{"))
		{
			Sys_Error("First token isn't a brace");
		}

		// parse an edict

		if (entnum == -1)
		{
			start = ED_ParseGlobals(start);
			// Need to restore this
			pr_global_struct->startspot = PR_SetString(sv.startspot);
		}
		else
		{
			ent = EDICT_NUM(entnum);
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
				SV_LinkEdict(ent, false);
				if (ent->v.modelindex && ent->GetModel())
				{
					i = SV_ModelIndex(PR_GetString(ent->GetModel()));
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
		sv.time = time;
		sv.paused = true;

		pr_global_struct->serverflags = svs.serverflags;

		RestoreClients();
	}
	else if (ClientsMode == 2)
	{
		sv.time = time;
	}
	else if (ClientsMode == 3)
	{
		sv.time = time;

		pr_global_struct->serverflags = svs.serverflags;

		RestoreClients();
	}

	if (ClientsMode != 1 && auto_correct)
	{
		Con_DPrintf("*** Auto-corrected model indexes!\n");
	}

//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void Host_Changelevel2_f(void)
{
	char level[MAX_QPATH];
	char _startspot[MAX_QPATH];
	char* startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
	if (!sv.active || clc.demoplaying)
	{
		Con_Printf("Only the server may changelevel\n");
		return;
	}

	String::Cpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms();

	// save the current level's state
	old_time = sv.time;
	SaveGamestate(false);

	// try to restore the new level
	if (LoadGamestate(level, startspot, 0))
	{
		SV_SpawnServer(level, startspot);
		RestoreClients();
	}
}


//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f(void)
{
	char* newName;
	char* pdest;

	if (Cmd_Argc() == 1)
	{
		Con_Printf("\"name\" is \"%s\"\n", clqh_name->string);
		return;
	}
	if (Cmd_Argc() == 2)
	{
		newName = Cmd_Argv(1);
	}
	else
	{
		newName = Cmd_ArgsUnmodified();
	}
	newName[15] = 0;

	//this is for the fuckers who put braces in the name causing loadgame to crash.
	pdest = strchr(newName,'{');
	if (pdest)
	{
		*pdest = 0;	//zap the brace
		Con_Printf("Illegal char in name removed!\n");
	}

	if (cmd_source == src_command)
	{
		if (String::Cmp(clqh_name->string, newName) == 0)
		{
			return;
		}
		Cvar_Set("_cl_name", newName);
		if (cls.state == CA_CONNECTED)
		{
			Cmd_ForwardToServer();
		}
		return;
	}

	if (host_client->name[0] && String::Cmp(host_client->name, "unconnected"))
	{
		if (String::Cmp(host_client->name, newName) != 0)
		{
			Con_Printf("%s renamed to %s\n", host_client->name, newName);
		}
	}
	String::Cpy(host_client->name, newName);
	host_client->edict->SetNetName(PR_SetString(host_client->name));

// send notification to all clients

	sv.reliable_datagram.WriteByte(h2svc_updatename);
	sv.reliable_datagram.WriteByte(host_client - svs.clients);
	sv.reliable_datagram.WriteString2(host_client->name);
}

extern const char* ClassNames[NUM_CLASSES];	//from menu.c
void Host_Class_f(void)
{
	float newClass;

	if (Cmd_Argc() == 1)
	{
		if (!(int)cl_playerclass->value)
		{
			Con_Printf("\"playerclass\" is %d (\"unknown\")\n", (int)cl_playerclass->value);
		}
		else
		{
			Con_Printf("\"playerclass\" is %d (\"%s\")\n", (int)cl_playerclass->value,ClassNames[(int)cl_playerclass->value - 1]);
		}
		return;
	}
	if (Cmd_Argc() == 2)
	{
		newClass = String::Atof(Cmd_Argv(1));
	}
	else
	{
		newClass = String::Atof(Cmd_ArgsUnmodified());
	}

	if (cmd_source == src_command)
	{
		Cvar_SetValue("_cl_playerclass", newClass);

#ifdef MISSIONPACK
		// when classes changes after map load, update cl_playerclass, cl_playerclass should
		// probably only be used in worldspawn, though
		if (pr_global_struct)
		{
			pr_global_struct->cl_playerclass = newClass;
		}
#endif

		if (cls.state == CA_CONNECTED)
		{
			Cmd_ForwardToServer();
		}
		return;
	}

	if (sv.loadgame || host_client->playerclass)
	{
		if (host_client->edict->GetPlayerClass())
		{
			newClass = host_client->edict->GetPlayerClass();
		}
		else if (host_client->playerclass)
		{
			newClass = host_client->playerclass;
		}
	}

	host_client->playerclass = newClass;
	host_client->edict->SetPlayerClass(newClass);

	// Change the weapon model used
	pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
	PR_ExecuteProgram(pr_global_struct->ClassChangeWeapon);

// send notification to all clients

	sv.reliable_datagram.WriteByte(h2svc_updateclass);
	sv.reliable_datagram.WriteByte(host_client - svs.clients);
	sv.reliable_datagram.WriteByte((byte)newClass);
}

//just an easy place to do some profile testing
#if 0
void Host_Version_f(void)
{
	int i;
	int repcount = 10000;
	float time1,time2,r1,r2;


	if (Cmd_Argc() == 2)
	{
		repcount = String::Atof(Cmd_Argv(1));
		if (repcount < 0)
		{
			repcount = 0;
		}
	}
	Con_Printf("looping %d times.\n", repcount);

	time1 = Sys_DoubleTime();
	for (i = repcount; i; i--)
	{
		char buf[2048];
		Com_Memset(buf,i,2048);
	}
	time2 = Sys_DoubleTime();
	r1 = time2 - time1;
	Con_Printf("loop 1 = %f\n", r1);

	time1 = Sys_DoubleTime();
	for (i = repcount; i; i--)
	{
		char buf[2048];
		Com_Memset(buf,i,2048);
	}
	time2 = Sys_DoubleTime();
	r2 = time2 - time1;
	Con_Printf("loop 2 = %f\n", r2);

	if (r2 < r1)
	{
		Con_Printf("loop 2 is faster by %f\n", r1 - r2);
	}
	else
	{
		Con_Printf("loop 1 is faster by %f\n", r2 - r1);
	}
	Con_Printf("Version %4.2f\n", HEXEN2_VERSION);
	Con_Printf("Exe: "__TIME__ " "__DATE__ "\n");
}
#else
void Host_Version_f(void)
{
	Con_Printf("Version %4.2f\n", HEXEN2_VERSION);
	Con_Printf("Exe: "__TIME__ " "__DATE__ "\n");
}
#endif

#ifdef IDGODS
void Host_Please_f(void)
{
	client_t* cl;
	int j;

	if (cmd_source != src_command)
	{
		return;
	}

	if ((Cmd_Argc() == 3) && String::Cmp(Cmd_Argv(1), "#") == 0)
	{
		j = String::Atof(Cmd_Argv(2)) - 1;
		if (j < 0 || j >= svs.maxclients)
		{
			return;
		}
		if (!svs.clients[j].active)
		{
			return;
		}
		cl = &svs.clients[j];
		if (cl->privileged)
		{
			cl->privileged = false;
			cl->edict->v.flags = (int)cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
			cl->edict->v.movetype = QHMOVETYPE_WALK;
		}
		else
		{
			cl->privileged = true;
		}
	}

	if (Cmd_Argc() != 2)
	{
		return;
	}

	for (j = 0, cl = svs.clients; j < svs.maxclients; j++, cl++)
	{
		if (!cl->active)
		{
			continue;
		}
		if (String::ICmp(cl->name, Cmd_Argv(1)) == 0)
		{
			if (cl->privileged)
			{
				cl->privileged = false;
				cl->edict->v.flags = (int)cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
				cl->edict->v.movetype = QHMOVETYPE_WALK;
			}
			else
			{
				cl->privileged = true;
			}
			break;
		}
	}
}
#endif


void Host_Say(qboolean teamonly)
{
	client_t* client;
	client_t* save;
	int j;
	char* p;
	char text[64];
	qboolean fromServer = false;

	if (cmd_source == src_command)
	{
		if (cls.state == CA_DEDICATED)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			Cmd_ForwardToServer();
			return;
		}
	}

	if (Cmd_Argc() < 2)
	{
		return;
	}

	save = host_client;

	p = Cmd_ArgsUnmodified();
// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

// turn on color set 1
	if (!fromServer)
	{
		sprintf(text, "%c%s: ", 1, save->name);
	}
	else
	{
		sprintf(text, "%c<%s> ", 1, hostname->string);
	}

	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
		{
			continue;
		}
		if (teamplay->value && teamonly && client->edict->GetTeam() != save->edict->GetTeam())
		{
			continue;
		}
		host_client = client;
		SV_ClientPrintf("%s", text);
	}
	host_client = save;

	Con_Printf("%s", &text[1]);
}


void Host_Say_f(void)
{
	Host_Say(false);
}


void Host_Say_Team_f(void)
{
	Host_Say(true);
}


void Host_Tell_f(void)
{
	client_t* client;
	client_t* save;
	int j;
	char* p;
	char text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (Cmd_Argc() < 3)
	{
		return;
	}

	String::Cpy(text, host_client->name);
	String::Cat(text, sizeof(text), ": ");

	p = Cmd_ArgsUnmodified();

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
		{
			continue;
		}

		if (String::ICmp(client->name, Cmd_Argv(1)))
		{
			continue;
		}
		host_client = client;
		SV_ClientPrintf("%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f(void)
{
	int top, bottom;
	int playercolor;

	if (Cmd_Argc() == 1)
	{
		Con_Printf("\"color\" is \"%i %i\"\n", ((int)clqh_color->value) >> 4, ((int)clqh_color->value) & 0x0f);
		Con_Printf("color <0-10> [0-10]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	playercolor = top * 16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValue("_cl_color", playercolor);
		if (cls.state == CA_CONNECTED)
		{
			Cmd_ForwardToServer();
		}
		return;
	}

	host_client->colors = playercolor;
	host_client->edict->SetTeam(bottom + 1);

// send notification to all clients
	sv.reliable_datagram.WriteByte(h2svc_updatecolors);
	sv.reliable_datagram.WriteByte(host_client - svs.clients);
	sv.reliable_datagram.WriteByte(host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (sv_player->GetHealth() <= 0)
	{
		SV_ClientPrintf("Can't suicide -- allready dead!\n");
		return;
	}

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram(pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f(void)
{

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}
	if (!pausable->value)
	{
		SV_ClientPrintf("Pause not allowed.\n");
	}
	else
	{
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf("%s paused the game\n", PR_GetString(sv_player->GetNetName()));
		}
		else
		{
			SV_BroadcastPrintf("%s unpaused the game\n", PR_GetString(sv_player->GetNetName()));
		}

		// send notification to all clients
		sv.reliable_datagram.WriteByte(h2svc_setpause);
		sv.reliable_datagram.WriteByte(sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f(void)
{
	if (cmd_source == src_command)
	{
		Con_Printf("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf("prespawn not valid -- allready spawned\n");
		return;
	}

	host_client->message.WriteData(sv.signon._data, sv.signon.cursize);
	host_client->message.WriteByte(h2svc_signonnum);
	host_client->message.WriteByte(2);
	host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f(void)
{
	int i;
	client_t* client;
	qhedict_t* ent;

	if (cmd_source == src_command)
	{
		Con_Printf("spawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf("Spawn not valid -- allready spawned\n");
		return;
	}

// send all current names, colors, and frag counts
	host_client->message.Clear();

// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited allready
		// if this is the last client to be connected, unpause
		sv.paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;
		sv.paused = false;

		if (!ent->GetStatsRestored() || deathmatch->value)
		{
			Com_Memset(&ent->v, 0, progs->entityfields * 4);

			//ent->v.colormap = NUM_FOR_EDICT(ent);
			ent->SetTeam((host_client->colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->playerclass);

			// copy spawn parms out of the client_t

			for (i = 0; i < NUM_SPAWN_PARMS; i++)
				(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

			// call the spawn function

			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram(pr_global_struct->ClientConnect);

			if ((Sys_DoubleTime() - host_client->netconnection->connecttime) <= sv.time)
			{
				Con_Printf("%s entered the game\n", host_client->name);
			}

			PR_ExecuteProgram(pr_global_struct->PutClientInServer);
		}
	}


// send time of update
	host_client->message.WriteByte(h2svc_time);
	host_client->message.WriteFloat(sv.time);

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		host_client->message.WriteByte(h2svc_updatename);
		host_client->message.WriteByte(i);
		host_client->message.WriteString2(client->name);

		host_client->message.WriteByte(h2svc_updateclass);
		host_client->message.WriteByte(i);
		host_client->message.WriteByte(client->playerclass);

		host_client->message.WriteByte(h2svc_updatefrags);
		host_client->message.WriteByte(i);
		host_client->message.WriteShort(client->old_frags);

		host_client->message.WriteByte(h2svc_updatecolors);
		host_client->message.WriteByte(i);
		host_client->message.WriteByte(client->colors);
	}

// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
	{
		host_client->message.WriteByte(h2svc_lightstyle);
		host_client->message.WriteByte((char)i);
		host_client->message.WriteString2(sv.lightstyles[i]);
	}

//
// send some stats
//
	host_client->message.WriteByte(h2svc_updatestat);
	host_client->message.WriteByte(STAT_TOTALSECRETS);
	host_client->message.WriteLong(pr_global_struct->total_secrets);

	host_client->message.WriteByte(h2svc_updatestat);
	host_client->message.WriteByte(STAT_TOTALMONSTERS);
	host_client->message.WriteLong(pr_global_struct->total_monsters);

	host_client->message.WriteByte(h2svc_updatestat);
	host_client->message.WriteByte(STAT_SECRETS);
	host_client->message.WriteLong(pr_global_struct->found_secrets);

	host_client->message.WriteByte(h2svc_updatestat);
	host_client->message.WriteByte(STAT_MONSTERS);
	host_client->message.WriteLong(pr_global_struct->killed_monsters);


	SV_UpdateEffects(&host_client->message);

//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = EDICT_NUM(1 + (host_client - svs.clients));
	host_client->message.WriteByte(h2svc_setangle);
	for (i = 0; i < 2; i++)
		host_client->message.WriteAngle(ent->GetAngles()[i]);
	host_client->message.WriteAngle(0);

	SV_WriteClientdataToMessage(host_client, sv_player, &host_client->message);

	host_client->message.WriteByte(h2svc_signonnum);
	host_client->message.WriteByte(3);
	host_client->sendsignon = true;
}

extern int edit_line;

int strdiff(const char* s1, const char* s2)
{
	int L1,L2,i;

	L1 = String::Length(s1);
	L2 = String::Length(s2);

	for (i = 0; (i < L1 && i < L2); i++)
	{
		if (String::ToLower(s1[i]) != String::ToLower(s2[i]))
		{
			break;
		}
	}

	return i;
}

void Host_Create_f(void)
{
	char* FindName;
	dfunction_t* Search,* func;
	qhedict_t* ent;
	int i,Length,NumFound,Diff,NewDiff;

	if (!sv.active)
	{
		Con_Printf("server is not active!\n");
		return;
	}

	if ((svs.maxclients != 1) || (skill->value > 2))
	{
		Con_Printf("can't cheat anymore!\n");
		return;
	}

	if (Cmd_Argc() == 1)
	{
		Con_Printf("create <quake-ed spawn function>\n");
		return;
	}

	FindName = Cmd_Argv(1);

	func = ED_FindFunctioni(FindName);

	if (!func)
	{
		Length = String::Length(FindName);
		NumFound = 0;

		Diff = 999;

		for (i = 0; i < progs->numfunctions; i++)
		{
			Search = &pr_functions[i];
			if (!String::NICmp(PR_GetString(Search->s_name),FindName,Length))
			{
				if (NumFound == 1)
				{
					Con_Printf("   %s\n", PR_GetString(func->s_name));
				}
				if (NumFound)
				{
					Con_Printf("   %s\n", PR_GetString(Search->s_name));
					NewDiff = strdiff(PR_GetString(Search->s_name), PR_GetString(func->s_name));
					if (NewDiff < Diff)
					{
						Diff = NewDiff;
					}
				}

				func = Search;
				NumFound++;
			}
		}

		if (!NumFound)
		{
			Con_Printf("Could not find spawn function\n");
			return;
		}

		if (NumFound != 1)
		{
			sprintf(g_consoleField.buffer,"create %s",PR_GetString(func->s_name));
			g_consoleField.buffer[Diff + 7] = 0;
			return;
		}
	}

	Con_Printf("Executing %s...\n", PR_GetString(func->s_name));

	ent = ED_Alloc();

	ent->SetClassName(func->s_name);
	VectorCopy(cl.refdef.vieworg, ent->GetOrigin());
	ent->GetOrigin()[0] += cl.refdef.viewaxis[0][0] * 80;
	ent->GetOrigin()[1] += cl.refdef.viewaxis[0][1] * 80;
	ent->GetOrigin()[2] += cl.refdef.viewaxis[0][2] * 80;
	VectorCopy(ent->GetOrigin(),ent->v.absmin);
	VectorCopy(ent->GetOrigin(),ent->v.absmax);
	ent->v.absmin[0] -= 16;
	ent->v.absmin[1] -= 16;
	ent->v.absmin[2] -= 16;
	ent->v.absmax[0] += 16;
	ent->v.absmax[1] += 16;
	ent->v.absmax[2] += 16;

	pr_global_struct->self = EDICT_TO_PROG(ent);
	ignore_precache = true;
	PR_ExecuteProgram(func - pr_functions);
	ignore_precache = false;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f(void)
{
	if (cmd_source == src_command)
	{
		Con_Printf("begin is not valid from the console\n");
		return;
	}

	host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f(void)
{
	const char* who;
	const char* message = NULL;
	client_t* save;
	int i;
	qboolean byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer();
			return;
		}
	}
	else if (pr_global_struct->deathmatch && !host_client->privileged)
	{
		return;
	}

	save = host_client;

	if (Cmd_Argc() > 2 && String::Cmp(Cmd_Argv(1), "#") == 0)
	{
		i = String::Atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.maxclients)
		{
			return;
		}
		if (!svs.clients[i].active)
		{
			return;
		}
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active)
			{
				continue;
			}
			if (String::ICmp(host_client->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
		{
			if (cls.state == CA_DEDICATED)
			{
				who = "Console";
			}
			else
			{
				who = clqh_name->string;
			}
		}
		else
		{
			who = save->name;
		}

		// can't kick yourself!
		if (host_client == save)
		{
			return;
		}

		if (Cmd_Argc() > 2)
		{
			message = Cmd_ArgsUnmodified();
			String::Parse1(&message);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += String::Length(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
		{
			SV_ClientPrintf("Kicked by %s: %s\n", who, message);
		}
		else
		{
			SV_ClientPrintf("Kicked by %s\n", who);
		}
		SV_DropClient(false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f(void)
{
	char* t;
	int v;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if ((pr_global_struct->deathmatch || skill->value > 2) && !host_client->privileged)
	{
		return;
	}

	t = Cmd_Argv(1);
	v = String::Atoi(Cmd_Argv(2));

	switch (t[0])
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// MED 01/04/97 added hipnotic give stuff
/*      if (hipnotic)
      {
         if (t[0] == '6')
         {
            if (t[1] == 'a')
               sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
            else
               sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
         }
         else if (t[0] == '9')
            sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
         else if (t[0] == '0')
            sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
         else if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
      else
*/  {
		if (t[0] >= '2')
		{
			sv_player->SetItems((int)sv_player->GetItems() | (IT_SHOTGUN << (t[0] - '2')));
		}
	}
	break;

	case 's':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_shells1");
            if (val)
                val->_float = v;
        }

        sv_player->v.ammo_shells = v;
        break;		*/
	case 'n':
/*		if (rogue)
        {
        val = GetEdictFieldValue(sv_player, "ammo_nails1");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_nails = v;
            }
        }
        else
        {
            sv_player->v.ammo_nails = v;
        }*/
		break;
	case 'l':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_lava_nails");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_nails = v;
            }
        }*/
		break;
	case 'r':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_rockets1");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_rockets = v;
            }
        }
        else
        {
            sv_player->v.ammo_rockets = v;
        }*/
		break;
	case 'm':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_multi_rockets");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_rockets = v;
            }
        }*/
		break;
	case 'h':
		sv_player->SetHealth(v);
		break;
	case 'c':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_cells1");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_cells = v;
            }
        }
        else
        {
            sv_player->v.ammo_cells = v;
        }*/
		break;
	case 'p':
/*		if (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_plasma");
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_cells = v;
            }
        }*/
		break;
	}
}

qhedict_t* FindViewthing(void)
{
	int i;
	qhedict_t* e;

	for (i = 0; i < sv.num_edicts; i++)
	{
		e = EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "viewthing"))
		{
			return e;
		}
	}
	Con_Printf("No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = R_RegisterModel(Cmd_Argv(1));
	if (!m)
	{
		Con_Printf("Can't load %s\n", Cmd_Argv(1));
		return;
	}

	e->SetFrame(0);
	cl.model_draw[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f(void)
{
	qhedict_t* e;
	int f;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	f = String::Atoi(Cmd_Argv(1));
	if (f >= R_ModelNumFrames(m))
	{
		f = R_ModelNumFrames(m) - 1;
	}

	e->SetFrame(f);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() + 1);
	if (e->GetFrame() >= R_ModelNumFrames(m))
	{
		e->SetFrame(R_ModelNumFrames(m) - 1);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() - 1);
	if (e->GetFrame() < 0)
	{
		e->SetFrame(0);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f(void)
{
	int i, c;

	if (cls.state == CA_DEDICATED)
	{
		if (!sv.active)
		{
			Cbuf_AddText("map start\n");
		}
		return;
	}

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_Printf("%i demo(s) in loop\n", c);

	for (i = 1; i < c + 1; i++)
		String::NCpy(cls.qh_demos[i - 1], Cmd_Argv(i), sizeof(cls.qh_demos[0]) - 1);

	if (!sv.active && cls.qh_demonum != -1 && !clc.demoplaying)
	{
		cls.qh_demonum = 0;
		CL_NextDemo();
	}
	else
	{
		cls.qh_demonum = -1;
	}
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (cls.qh_demonum == -1)
	{
		cls.qh_demonum = 1;
	}
	CL_Disconnect_f();
	CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (!clc.demoplaying)
	{
		return;
	}
	CL_StopPlayback();
	CL_Disconnect();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands(void)
{
	Cmd_AddCommand("status", Host_Status_f);
	Cmd_AddCommand("quit", Host_Quit_f);
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("restart", Host_Restart_f);
	Cmd_AddCommand("changelevel", Host_Changelevel_f);
	Cmd_AddCommand("changelevel2", Host_Changelevel2_f);
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("playerclass", Host_Class_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
	Cmd_AddCommand("version", Host_Version_f);
#ifdef IDGODS
	Cmd_AddCommand("please", Host_Please_f);
#endif
	Cmd_AddCommand("say", Host_Say_f);
	Cmd_AddCommand("say_team", Host_Say_Team_f);
	Cmd_AddCommand("tell", Host_Tell_f);
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
	Cmd_AddCommand("spawn", Host_Spawn_f);
	Cmd_AddCommand("begin", Host_Begin_f);
	Cmd_AddCommand("prespawn", Host_PreSpawn_f);
	Cmd_AddCommand("kick", Host_Kick_f);
	Cmd_AddCommand("ping", Host_Ping_f);
	Cmd_AddCommand("load", Host_Loadgame_f);
	Cmd_AddCommand("save", Host_Savegame_f);
	Cmd_AddCommand("give", Host_Give_f);

	Cmd_AddCommand("startdemos", Host_Startdemos_f);
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);

	Cmd_AddCommand("create", Host_Create_f);
}
