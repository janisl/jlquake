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

static double old_time;

void RestoreClients(void);

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
#ifndef DEDICATED
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state != CA_DEDICATED)
	{
		M_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
#endif
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

	if (cmd_source == src_command)
	{
		if (sv.state == SS_DEAD)
		{
			Cmd_ForwardToServer();
			return;
		}
	}

	if (cmd_source == src_command)
	{
		common->Printf("host:    %s\n", Cvar_VariableString("hostname"));
		common->Printf("version: %4.2f\n", HEXEN2_VERSION);
	}
	else
	{
		SVQH_ClientPrintf(host_client, 0, "host:    %s\n", Cvar_VariableString("hostname"));
		SVQH_ClientPrintf(host_client, 0, "version: %4.2f\n", HEXEN2_VERSION);
	}
	SOCK_ShowIP();
	if (cmd_source == src_command)
	{
		common->Printf("map:     %s\n", sv.name);
		common->Printf("players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
	}
	else
	{
		SVQH_ClientPrintf(host_client, 0, "map:     %s\n", sv.name);
		SVQH_ClientPrintf(host_client, 0, "players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
	}
	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		seconds = (int)(net_time - client->qh_netconnection->connecttime);
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
		if (cmd_source == src_command)
		{
			common->Printf("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
			common->Printf("   %s\n", client->qh_netconnection->address);
		}
		else
		{
			SVQH_ClientPrintf(host_client, 0, "#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
			SVQH_ClientPrintf(host_client, 0, "   %s\n", client->qh_netconnection->address);
		}
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

	if (*pr_globalVars.deathmatch ||
		*pr_globalVars.coop || skill->value > 2)
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ QHFL_GODMODE);
	if (!((int)sv_player->GetFlags() & QHFL_GODMODE))
	{
		SVQH_ClientPrintf(host_client, 0, "godmode OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(host_client, 0, "godmode ON\n");
	}
}

void Host_Notarget_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (*pr_globalVars.deathmatch || skill->value > 2)
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ QHFL_NOTARGET);
	if (!((int)sv_player->GetFlags() & QHFL_NOTARGET))
	{
		SVQH_ClientPrintf(host_client, 0, "notarget OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(host_client, 0, "notarget ON\n");
	}
}

void Host_Noclip_f(void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (*pr_globalVars.deathmatch ||
		*pr_globalVars.coop || skill->value > 2)
	{
		return;
	}

	if (sv_player->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		sv_player->SetMoveType(QHMOVETYPE_NOCLIP);
		SVQH_ClientPrintf(host_client, 0, "noclip ON\n");
	}
	else
	{
		sv_player->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(host_client, 0, "noclip OFF\n");
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

	SVQH_ClientPrintf(host_client, 0, "Client ping times:\n");
	for (i = 0, client = svs.clients; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		total = 0;
		for (j = 0; j < NUM_PING_TIMES; j++)
			total += client->qh_ping_times[j];
		total /= NUM_PING_TIMES;
		SVQH_ClientPrintf(host_client, 0, "%4i %s\n", (int)(total * 1000), client->name);
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
#ifndef DEDICATED
		common->Printf("map <levelname>: start a new server\nCurrently on: %s\n",cl.qh_levelname);
#endif
		return;
	}

	if (cmd_source != src_command)
	{
		return;
	}

#ifndef DEDICATED
	cls.qh_demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect();
#endif
	Host_ShutdownServer(false);

#ifndef DEDICATED
	in_keyCatchers = 0;			// remove console or menu
	SCR_BeginLoadingPlaque();
#endif

	info_mask = 0;
	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		info_mask2 = 0x80000000;
	}
	else
	{
		info_mask2 = 0;
	}

	svs.qh_serverflags = 0;			// haven't completed an episode yet
	String::Cpy(name, Cmd_Argv(1));
	SV_SpawnServer(name, NULL);
	if (sv.state == SS_DEAD)
	{
		return;
	}

#ifndef DEDICATED
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
#endif
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
		common->Printf("changelevel <levelname> : continue game on a new level\n");
		return;
	}
#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (sv.state == SS_DEAD || clc.demoplaying)
#endif
	{
		common->Printf("Only the server may changelevel\n");
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

#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (clc.demoplaying || sv.state == SS_DEAD)
#endif
	{
		return;
	}

	if (cmd_source != src_command)
	{
		return;
	}

	String::Cpy(mapname, sv.name);	// must copy out, because it gets cleared
	String::Cpy(startspot, sv.h2_startspot);

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

#ifndef DEDICATED
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
#endif

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
#ifndef DEDICATED
	Com_Memcpy(text, cl.qh_levelname, String::Length(cl.qh_levelname));
#endif
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

	if (sv.state == SS_DEAD)
	{
		common->Printf("Not playing a local game.\n");
		return;
	}

#ifndef DEDICATED
	if (cl.qh_intermission)
	{
		common->Printf("Can't save in intermission.\n");
		return;
	}
#endif

#ifndef TESTSAVE
	if (svs.qh_maxclients != 1)
	{
		common->Printf("Can't save multiplayer games.\n");
		return;
	}
#endif

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

	for (i = 0; i < svs.qh_maxclients; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED && (svs.clients[i].qh_edict->GetHealth() <= 0))
		{
			common->Printf("Can't savegame with a dead player\n");
			return;
		}
	}


	SaveGamestate(false);

#ifndef DEDICATED
	CL_RemoveGIPFiles(Cmd_Argv(1));
#endif

	char* netname = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "clients.gip");
	FS_Remove(netname);

	sprintf(dest, "%s/", Cmd_Argv(1));
	common->Printf("Saving game to %s...\n", Cmd_Argv(1));

#ifndef DEDICATED
	CL_CopyFiles("", ".gip", dest);
#endif

	sprintf(dest,"%s/info.dat", Cmd_Argv(1));
	f = FS_FOpenFileWrite(dest);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment(comment);
	FS_Printf(f, "%s\n", comment);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		FS_Printf(f, "%f\n", svs.clients->qh_spawn_parms[i]);
	FS_Printf(f, "%d\n", svqh_current_skill);
	FS_Printf(f, "%s\n", sv.name);
	FS_Printf(f, "%f\n",sv.qh_time);
	FS_Printf(f, "%d\n",svs.qh_maxclients);
	FS_Printf(f, "%f\n",svqh_deathmatch->value);
	FS_Printf(f, "%f\n",svqh_coop->value);
	FS_Printf(f, "%f\n",svqh_teamplay->value);
	FS_Printf(f, "%f\n",randomclass->value);
#ifndef DEDICATED
	FS_Printf(f, "%f\n",clh2_playerclass->value);
#endif
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
		common->Printf("load <savename> : load a game\n");
		return;
	}

#ifndef DEDICATED
	cls.qh_demonum = -1;		// stop demo loop in case this fails
	CL_Disconnect();
	CL_RemoveGIPFiles(NULL);
#endif

	common->Printf("Loading game from %s...\n", Cmd_Argv(1));

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
	version = String::Atoi(GetLine(ReadPos));

	if (version != SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return;
	}
	GetLine(ReadPos);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
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

	String::Cpy(mapname, GetLine(ReadPos));
	time = String::Atof(GetLine(ReadPos));

	tempi = -1;
	tempi = String::Atoi(GetLine(ReadPos));
	if (tempi >= 1)
	{
		svs.qh_maxclients = tempi;
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

#ifndef DEDICATED
	sprintf(dest, "%s/", Cmd_Argv(1));
	CL_CopyFiles(dest, ".gip", "");
#endif

	LoadGamestate(mapname, NULL, 2);

	SV_SaveSpawnparms();

	ent = QH_EDICT_NUM(1);

	Cvar_SetValue("_cl_playerclass", ent->GetPlayerClass());//this better be the same as above...

#ifdef MISSIONPACK
	// this may be rudundant with the setting in PR_LoadProgs, but not sure so its here too
	*pr_globalVars.cl_playerclass = ent->GetPlayerClass();
#endif

	svs.clients->h2_playerclass = ent->GetPlayerClass();

	sv.qh_paused = true;		// pause until all clients connect
	sv.loadgame = true;

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		CL_EstablishConnection("local");
		Host_Reconnect_f();
	}
#endif
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
		end = svs.qh_maxclients + 1;

		sprintf(name, "clients.gip");
	}
	else
	{
		start = 1;
		end = sv.qh_num_edicts;

		sprintf(name, "%s.gip", sv.name);

//		common->Printf ("Saving game to %s...\n", name);
	}

	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", SAVEGAME_VERSION);

	if (!ClientsOnly)
	{
		Host_SavegameComment(comment);
		FS_Printf(f, "%s\n", comment);
		FS_Printf(f, "%f\n", skill->value);
		FS_Printf(f, "%s\n", sv.name);
		FS_Printf(f, "%f\n", sv.qh_time);

		// write the light styles

		for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
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

	host_client = svs.clients;

//	for (i=svs.qh_maxclients+1 ; i<sv.num_edicts ; i++)
//  to save the client states
	for (i = start; i < end; i++)
	{
		ent = QH_EDICT_NUM(i);
		if ((int)ent->GetFlags() & H2FL_ARCHIVE_OVERRIDE)
		{
			continue;
		}
		if (ClientsOnly)
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

void RestoreClients(void)
{
	int i,j;
	qhedict_t* ent;
	double time_diff;

	if (LoadGamestate(NULL,NULL,1))
	{
		return;
	}

	time_diff = sv.qh_time - old_time;

	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			ent = host_client->qh_edict;

			//ent->v.colormap = QH_NUM_FOR_EDICT(ent);
			ent->SetTeam((host_client->qh_colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->h2_playerclass);

			// copy spawn parms out of the client_t

			for (j = 0; j < NUM_SPAWN_PARMS; j++)
				pr_globalVars.parm1[j] = host_client->qh_spawn_parms[j];

			// call the spawn function

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(ent);
			G_FLOAT(OFS_PARM0) = time_diff;
			PR_ExecuteProgram(*pr_globalVars.ClientReEnter);
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

	if (version != SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
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

		if (sv.state == SS_DEAD)
		{
			common->Printf("Couldn't load map\n");
			return -1;
		}

		// load the light styles
		for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
		{
			char* Style = GetLine(ReadPos);
			char* Tmp = (char*)Hunk_Alloc(String::Length(Style) + 1);
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
		sv.qh_time = time;
		sv.qh_paused = true;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		RestoreClients();
	}
	else if (ClientsMode == 2)
	{
		sv.qh_time = time;
	}
	else if (ClientsMode == 3)
	{
		sv.qh_time = time;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		RestoreClients();
	}

	if (ClientsMode != 1 && auto_correct)
	{
		common->DPrintf("*** Auto-corrected model indexes!\n");
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
		common->Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (sv.state == SS_DEAD || clc.demoplaying)
#endif
	{
		common->Printf("Only the server may changelevel\n");
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
	old_time = sv.qh_time;
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
#ifndef DEDICATED
		common->Printf("\"name\" is \"%s\"\n", clqh_name->string);
#endif
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
		common->Printf("Illegal char in name removed!\n");
	}

	if (cmd_source == src_command)
	{
#ifndef DEDICATED
		if (String::Cmp(clqh_name->string, newName) == 0)
		{
			return;
		}
		Cvar_Set("_cl_name", newName);
		if (cls.state == CA_ACTIVE)
		{
			Cmd_ForwardToServer();
		}
#endif
		return;
	}

	if (host_client->name[0] && String::Cmp(host_client->name, "unconnected"))
	{
		if (String::Cmp(host_client->name, newName) != 0)
		{
			common->Printf("%s renamed to %s\n", host_client->name, newName);
		}
	}
	String::Cpy(host_client->name, newName);
	host_client->qh_edict->SetNetName(PR_SetString(host_client->name));

// send notification to all clients

	sv.qh_reliable_datagram.WriteByte(h2svc_updatename);
	sv.qh_reliable_datagram.WriteByte(host_client - svs.clients);
	sv.qh_reliable_datagram.WriteString2(host_client->name);
}

extern const char* ClassNames[NUM_CLASSES];	//from menu.c
void Host_Class_f(void)
{
	float newClass;

	if (Cmd_Argc() == 1)
	{
#ifndef DEDICATED
		if (!(int)clh2_playerclass->value)
		{
			common->Printf("\"playerclass\" is %d (\"unknown\")\n", (int)clh2_playerclass->value);
		}
		else
		{
			common->Printf("\"playerclass\" is %d (\"%s\")\n", (int)clh2_playerclass->value,ClassNames[(int)clh2_playerclass->value - 1]);
		}
#endif
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
#ifndef DEDICATED
		Cvar_SetValue("_cl_playerclass", newClass);

#ifdef MISSIONPACK
		// when classes changes after map load, update cl_playerclass, cl_playerclass should
		// probably only be used in worldspawn, though
		if (pr_globalVars.cl_playerclass)
		{
			*pr_globalVars.cl_playerclass = newClass;
		}
#endif

		if (cls.state == CA_ACTIVE)
		{
			Cmd_ForwardToServer();
		}
#endif
		return;
	}

	if (sv.loadgame || host_client->h2_playerclass)
	{
		if (host_client->qh_edict->GetPlayerClass())
		{
			newClass = host_client->qh_edict->GetPlayerClass();
		}
		else if (host_client->h2_playerclass)
		{
			newClass = host_client->h2_playerclass;
		}
	}

	host_client->h2_playerclass = newClass;
	host_client->qh_edict->SetPlayerClass(newClass);

	// Change the weapon model used
	*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
	PR_ExecuteProgram(*pr_globalVars.ClassChangeWeapon);

// send notification to all clients

	sv.qh_reliable_datagram.WriteByte(h2svc_updateclass);
	sv.qh_reliable_datagram.WriteByte(host_client - svs.clients);
	sv.qh_reliable_datagram.WriteByte((byte)newClass);
}

void Host_Version_f(void)
{
	common->Printf("Version %4.2f\n", HEXEN2_VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

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
#ifndef DEDICATED
		if (cls.state == CA_DEDICATED)
#endif
		{
			fromServer = true;
			teamonly = false;
		}
#ifndef DEDICATED
		else
		{
			Cmd_ForwardToServer();
			return;
		}
#endif
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
		sprintf(text, "%c<%s> ", 1, sv_hostname->string);
	}

	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (!client || client->state != CS_ACTIVE)
		{
			continue;
		}
		if (svqh_teamplay->value && teamonly && client->qh_edict->GetTeam() != save->qh_edict->GetTeam())
		{
			continue;
		}
		host_client = client;
		SVQH_ClientPrintf(host_client, 0, "%s", text);
	}
	host_client = save;

	common->Printf("%s", &text[1]);
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
	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		if (String::ICmp(client->name, Cmd_Argv(1)))
		{
			continue;
		}
		host_client = client;
		SVQH_ClientPrintf(host_client, 0, "%s", text);
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
#ifndef DEDICATED
		common->Printf("\"color\" is \"%i %i\"\n", ((int)clqh_color->value) >> 4, ((int)clqh_color->value) & 0x0f);
		common->Printf("color <0-10> [0-10]\n");
#endif
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
#ifndef DEDICATED
		Cvar_SetValue("_cl_color", playercolor);
		if (cls.state == CA_ACTIVE)
		{
			Cmd_ForwardToServer();
		}
#endif
		return;
	}

	host_client->qh_colors = playercolor;
	host_client->qh_edict->SetTeam(bottom + 1);

// send notification to all clients
	sv.qh_reliable_datagram.WriteByte(h2svc_updatecolors);
	sv.qh_reliable_datagram.WriteByte(host_client - svs.clients);
	sv.qh_reliable_datagram.WriteByte(host_client->qh_colors);
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
		SVQH_ClientPrintf(host_client, 0, "Can't suicide -- allready dead!\n");
		return;
	}

	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram(*pr_globalVars.ClientKill);
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
		SVQH_ClientPrintf(host_client, 0, "Pause not allowed.\n");
	}
	else
	{
		sv.qh_paused ^= 1;

		if (sv.qh_paused)
		{
			SVQH_BroadcastPrintf(0, "%s paused the game\n", PR_GetString(sv_player->GetNetName()));
		}
		else
		{
			SVQH_BroadcastPrintf(0, "%s unpaused the game\n", PR_GetString(sv_player->GetNetName()));
		}

		// send notification to all clients
		sv.qh_reliable_datagram.WriteByte(h2svc_setpause);
		sv.qh_reliable_datagram.WriteByte(sv.qh_paused);
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
		common->Printf("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->state == CS_ACTIVE)
	{
		common->Printf("prespawn not valid -- allready spawned\n");
		return;
	}

	host_client->qh_message.WriteData(sv.qh_signon._data, sv.qh_signon.cursize);
	host_client->qh_message.WriteByte(h2svc_signonnum);
	host_client->qh_message.WriteByte(2);
	host_client->qh_sendsignon = true;
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
		common->Printf("spawn is not valid from the console\n");
		return;
	}

	if (host_client->state == CS_ACTIVE)
	{
		common->Printf("Spawn not valid -- allready spawned\n");
		return;
	}

// send all current names, colors, and frag counts
	host_client->qh_message.Clear();

// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited allready
		// if this is the last client to be connected, unpause
		sv.qh_paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->qh_edict;
		sv.qh_paused = false;

		if (!ent->GetStatsRestored() || svqh_deathmatch->value)
		{
			Com_Memset(&ent->v, 0, progs->entityfields * 4);

			//ent->v.colormap = QH_NUM_FOR_EDICT(ent);
			ent->SetTeam((host_client->qh_colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->h2_playerclass);

			// copy spawn parms out of the client_t

			for (i = 0; i < NUM_SPAWN_PARMS; i++)
				pr_globalVars.parm1[i] = host_client->qh_spawn_parms[i];

			// call the spawn function

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram(*pr_globalVars.ClientConnect);

			if ((Sys_DoubleTime() - host_client->qh_netconnection->connecttime) <= sv.qh_time)
			{
				common->Printf("%s entered the game\n", host_client->name);
			}

			PR_ExecuteProgram(*pr_globalVars.PutClientInServer);
		}
	}


// send time of update
	host_client->qh_message.WriteByte(h2svc_time);
	host_client->qh_message.WriteFloat(sv.qh_time);

	for (i = 0, client = svs.clients; i < svs.qh_maxclients; i++, client++)
	{
		host_client->qh_message.WriteByte(h2svc_updatename);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteString2(client->name);

		host_client->qh_message.WriteByte(h2svc_updateclass);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteByte(client->h2_playerclass);

		host_client->qh_message.WriteByte(h2svc_updatefrags);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteShort(client->qh_old_frags);

		host_client->qh_message.WriteByte(h2svc_updatecolors);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteByte(client->qh_colors);
	}

// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES_H2; i++)
	{
		host_client->qh_message.WriteByte(h2svc_lightstyle);
		host_client->qh_message.WriteByte((char)i);
		host_client->qh_message.WriteString2(sv.qh_lightstyles[i]);
	}

//
// send some stats
//
	host_client->qh_message.WriteByte(h2svc_updatestat);
	host_client->qh_message.WriteByte(STAT_TOTALSECRETS);
	host_client->qh_message.WriteLong(*pr_globalVars.total_secrets);

	host_client->qh_message.WriteByte(h2svc_updatestat);
	host_client->qh_message.WriteByte(STAT_TOTALMONSTERS);
	host_client->qh_message.WriteLong(*pr_globalVars.total_monsters);

	host_client->qh_message.WriteByte(h2svc_updatestat);
	host_client->qh_message.WriteByte(STAT_SECRETS);
	host_client->qh_message.WriteLong(*pr_globalVars.found_secrets);

	host_client->qh_message.WriteByte(h2svc_updatestat);
	host_client->qh_message.WriteByte(STAT_MONSTERS);
	host_client->qh_message.WriteLong(*pr_globalVars.killed_monsters);


	SVH2_UpdateEffects(&host_client->qh_message);

//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = QH_EDICT_NUM(1 + (host_client - svs.clients));
	host_client->qh_message.WriteByte(h2svc_setangle);
	for (i = 0; i < 2; i++)
		host_client->qh_message.WriteAngle(ent->GetAngles()[i]);
	host_client->qh_message.WriteAngle(0);

	SV_WriteClientdataToMessage(host_client, sv_player, &host_client->qh_message);

	host_client->qh_message.WriteByte(h2svc_signonnum);
	host_client->qh_message.WriteByte(3);
	host_client->qh_sendsignon = true;
}

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

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f(void)
{
	if (cmd_source == src_command)
	{
		common->Printf("begin is not valid from the console\n");
		return;
	}

	host_client->state = CS_ACTIVE;
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
		if (sv.state == SS_DEAD)
		{
			Cmd_ForwardToServer();
			return;
		}
	}
	else if (*pr_globalVars.deathmatch)
	{
		return;
	}

	save = host_client;

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
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		{
			if (host_client->state < CS_CONNECTED)
			{
				continue;
			}
			if (String::ICmp(host_client->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.qh_maxclients)
	{
		if (cmd_source == src_command)
		{
#ifndef DEDICATED
			if (cls.state == CA_DEDICATED)
#endif
			{
				who = "Console";
			}
#ifndef DEDICATED
			else
			{
				who = clqh_name->string;
			}
#endif
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
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s: %s\n", who, message);
		}
		else
		{
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s\n", who);
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

	if (*pr_globalVars.deathmatch || skill->value > 2)
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

#ifndef DEDICATED
qhedict_t* FindViewthing(void)
{
	int i;
	qhedict_t* e;

	for (i = 0; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "viewthing"))
		{
			return e;
		}
	}
	common->Printf("No viewthing on map\n");
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
		common->Printf("Can't load %s\n", Cmd_Argv(1));
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
#endif

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
#ifndef DEDICATED
	int i, c;

	if (cls.state == CA_DEDICATED)
#endif
	{
		if (sv.state == SS_DEAD)
		{
			Cbuf_AddText("map start\n");
		}
		return;
	}

#ifndef DEDICATED
	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		common->Printf("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	common->Printf("%i demo(s) in loop\n", c);

	for (i = 1; i < c + 1; i++)
		String::NCpy(cls.qh_demos[i - 1], Cmd_Argv(i), sizeof(cls.qh_demos[0]) - 1);

	if (sv.state == SS_DEAD && cls.qh_demonum != -1 && !clc.demoplaying)
	{
		cls.qh_demonum = 0;
		CL_NextDemo();
	}
	else
	{
		cls.qh_demonum = -1;
	}
#endif
}

#ifndef DEDICATED
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
#endif

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
#ifndef DEDICATED
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
#endif
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("playerclass", Host_Class_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
	Cmd_AddCommand("version", Host_Version_f);
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
#ifndef DEDICATED
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);
#endif
}
