/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

extern Cvar* pausable;

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
		common->Printf("version: %4.2f\n", VERSION);
	}
	else
	{
		SVQH_ClientPrintf(host_client, 0, "host:    %s\n", Cvar_VariableString("hostname"));
		SVQH_ClientPrintf(host_client, 0, "version: %4.2f\n", VERSION);
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

#ifndef DEDICATED

/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Notarget_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Noclip_f(void)
{
	Cmd_ForwardToServer();
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f(void)
{
	Cmd_ForwardToServer();
}

#endif

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

	svs.qh_serverflags = 0;			// haven't completed an episode yet
	String::Cpy(name, Cmd_Argv(1));
	SV_SpawnServer(name);
	if (sv.state == SS_DEAD)
	{
		return;
	}

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
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

	if (Cmd_Argc() != 2)
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
	SV_SaveSpawnparms();
	String::Cpy(level, Cmd_Argv(1));
	SV_SpawnServer(level);
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
	// in sv_spawnserver
	SV_SpawnServer(mapname);
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

#ifndef DEDICATED
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

	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		text[i] = ' ';
	Com_Memcpy(text, cl.qh_levelname, String::Length(cl.qh_levelname));
	sprintf(kills,"kills:%3i/%3i", cl.qh_stats[Q1STAT_MONSTERS], cl.qh_stats[Q1STAT_TOTALMONSTERS]);
	Com_Memcpy(text + 22, kills, String::Length(kills));
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
	char name[256];
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];

	if (cmd_source != src_command)
	{
		return;
	}

	if (sv.state == SS_DEAD)
	{
		common->Printf("Not playing a local game.\n");
		return;
	}

	if (cl.qh_intermission)
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

	for (i = 0; i < svs.qh_maxclients; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED && (svs.clients[i].qh_edict->GetHealth() <= 0))
		{
			common->Printf("Can't savegame with a dead player\n");
			return;
		}
	}

	String::NCpyZ(name, Cmd_Argv(1), sizeof(name));
	String::DefaultExtension(name, sizeof(name), ".sav");

	common->Printf("Saving game to %s...\n", name);
	fileHandle_t f = FS_FOpenFileWrite(name);
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

// write the light styles

	for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
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
	for (i = 0; i < sv.qh_num_edicts; i++)
	{
		ED_Write(f, QH_EDICT_NUM(i));
		FS_Flush(f);
	}
	FS_FCloseFile(f);
	common->Printf("done.\n");
}
#endif

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
	char name[MAX_OSPATH];
	char mapname[MAX_QPATH];
	float time;
	int i;
	qhedict_t* ent;
	int entnum;
	int version;
	float spawn_parms[NUM_SPAWN_PARMS];

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
#endif

	String::NCpyZ(name, Cmd_Argv(1), sizeof(name));
	String::DefaultExtension(name, sizeof(name), ".sav");

// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

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

	String::Cpy(mapname, GetLine(ReadPos));
	time = String::Atof(GetLine(ReadPos));

#ifndef DEDICATED
	CL_Disconnect_f();
#endif

	SV_SpawnServer(mapname);
	if (sv.state == SS_DEAD)
	{
		common->Printf("Couldn't load map\n");
		return;
	}
	sv.qh_paused = true;		// pause until all clients connect
	sv.loadgame = true;

	// load the light styles

	for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		char* Style = GetLine(ReadPos);
		char* Tmp = (char*)Hunk_Alloc(String::Length(Style) + 1);
		String::Cpy(Tmp, Style);
		sv.qh_lightstyles[i] = Tmp;
	}

	// load the edicts out of the savegame file
	entnum = -1;		// -1 is the globals
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
		{	// parse the global vars
			start = ED_ParseGlobals(start);
		}
		else
		{	// parse an edict

			ent = QH_EDICT_NUM(entnum);
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

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		svs.clients->qh_spawn_parms[i] = spawn_parms[i];

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		CL_EstablishConnection("local");
		Host_Reconnect_f();
	}
#endif
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

	sv.qh_reliable_datagram.WriteByte(q1svc_updatename);
	sv.qh_reliable_datagram.WriteByte(host_client - svs.clients);
	sv.qh_reliable_datagram.WriteString2(host_client->name);
}


void Host_Version_f(void)
{
	common->Printf("Version %4.2f\n", VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

#ifndef DEDICATED

void Host_Say_f()
{
	Cmd_ForwardToServer();
}

void Host_Say_Team_f(void)
{
	Cmd_ForwardToServer();
}

#endif

void Host_ConSay_f()
{
	client_t* client;
	int j;
	char* p;
	char text[64];

	if (Cmd_Argc() < 2)
	{
		return;
	}

	p = Cmd_ArgsUnmodified();
	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	// turn on color set 1
	sprintf(text, "%c<%s> ", 1, sv_hostname->string);

	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
	}
}

void Host_Tell_f(void)
{
	Cmd_ForwardToServer();
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
		common->Printf("color <0-13> [0-13]\n");
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
	sv.qh_reliable_datagram.WriteByte(q1svc_updatecolors);
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
		sv.qh_reliable_datagram.WriteByte(q1svc_setpause);
		sv.qh_reliable_datagram.WriteByte(sv.qh_paused);
	}
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
#ifdef DEDICATED
			who = "Console";
#else
			if (cls.state == CA_DEDICATED)
			{
				who = "Console";
			}
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
		SVQH_DropClient(host_client, false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

#ifndef DEDICATED
/*
==================
Host_Give_f
==================
*/
void Host_Give_f(void)
{
	Cmd_ForwardToServer();
}

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
#ifndef DEDICATED
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
	Cmd_AddCommand("fly", Host_Fly_f);
#endif
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("restart", Host_Restart_f);
	Cmd_AddCommand("changelevel", Host_Changelevel_f);
#ifndef DEDICATED
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
#endif
	Cmd_AddCommand("name", Host_Name_f);
#ifndef DEDICATED
	Cmd_AddCommand("noclip", Host_Noclip_f);
#endif
	Cmd_AddCommand("version", Host_Version_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", Host_ConSay_f);
	}
#ifndef DEDICATED
	else
	{
		Cmd_AddCommand("say", Host_Say_f);
		Cmd_AddCommand("say_team", Host_Say_Team_f);
	}
	Cmd_AddCommand("tell", Host_Tell_f);
#endif
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
	Cmd_AddCommand("kick", Host_Kick_f);
	Cmd_AddCommand("ping", Host_Ping_f);
	Cmd_AddCommand("load", Host_Loadgame_f);
#ifndef DEDICATED
	Cmd_AddCommand("save", Host_Savegame_f);
	Cmd_AddCommand("give", Host_Give_f);
#endif

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
