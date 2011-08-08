/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "server.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f (void)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!com_dedicated->value)
	{
		Com_Printf ("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_SetLatched("public", "1");

	for (i=1 ; i<MAX_MASTERS ; i++)
		Com_Memset(&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		if (slot == MAX_MASTERS)
			break;

		if (!SOCK_StringToAdr(Cmd_Argv(i), &master_adr[slot], PORT_MASTER))
		{
			Com_Printf("Bad address: %s\n", Cmd_Argv(i));
			continue;
		}

		Com_Printf ("Master server at %s\n", SOCK_AdrToString(master_adr[slot]));

		Com_Printf ("Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_heartbeat = -9999999;
}



/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc() < 2)
		return false;

	s = Cmd_Argv(1);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = String::Atoi(Cmd_Argv(1));
		if (idnum < 0 || idnum >= maxclients->value)
		{
			Com_Printf ("Bad client slot: %i\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			Com_Printf ("Client %i is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (!String::Cmp(cl->name, s))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_Printf ("Userid %s is not on the server\n", s);
	return false;
}


/*
===============================================================================

SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete save/<XXX>/
=====================
*/
void SV_WipeSavegame (const char *savename)
{
	Com_DPrintf("SV_WipeSaveGame(%s)\n", savename);

	char* name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/server.ssv", savename));
	FS_Remove(name);
	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/game.ssv", savename));
	FS_Remove(name);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s", savename));
	int NumSysFiles;
	char** SysFiles = Sys_ListFiles(name, ".sav", NULL, &NumSysFiles, false);
	for (int i = 0 ; i < NumSysFiles; i++)
	{
		name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", savename, SysFiles[i]));
		FS_Remove(name);
	}
	Sys_FreeFileList(SysFiles);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s", savename));
	SysFiles = Sys_ListFiles(name, ".sv2", NULL, &NumSysFiles, false);
	for (int i = 0 ; i < NumSysFiles; i++)
	{
		name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", savename, SysFiles[i]));
		FS_Remove(name);
	}
	Sys_FreeFileList(SysFiles);
}


/*
================
SV_CopySaveGame
================
*/
void SV_CopySaveGame (const char *src, const char *dst)
{
	Com_DPrintf("SV_CopySaveGame(%s, %s)\n", src, dst);

	SV_WipeSavegame (dst);

	// copy the savegame over
	char* name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/server.ssv", src));
	char* name2 = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/server.ssv", dst));
	FS_CopyFile(name, name2);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/game.ssv", src));
	name2 = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/game.ssv", dst));
	FS_CopyFile(name, name2);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s", src));
	int NumSysFiles;
	char** SysFiles = Sys_ListFiles(name, ".sav", NULL, &NumSysFiles, false);
	for (int i = 0 ; i < NumSysFiles; i++)
	{
		name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", src, SysFiles[i]));
		name2 = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", dst, SysFiles[i]));
		FS_CopyFile(name, name2);

		// change sav to sv2
		int l = String::Length(name);
		String::Cpy(name + l - 3, "sv2");
		l = String::Length(name2);
		String::Cpy(name2 + l - 3, "sv2");
		FS_CopyFile(name, name2);
	}
	Sys_FreeFileList(SysFiles);
}


/*
==============
SV_WriteLevelFile

==============
*/
void SV_WriteLevelFile (void)
{
	char	name[MAX_OSPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_WriteLevelFile()\n");

	String::Sprintf(name, sizeof(name), "save/current/%s.sv2", sv.name);
	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Com_Printf ("Failed to open %s\n", name);
		return;
	}
	FS_Write(sv.configstrings, sizeof(sv.configstrings), f);
	CM_WritePortalState(f);
	FS_FCloseFile(f);

	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/current/%s.sav", sv.name)));
	ge->WriteLevel(name);
}

/*
==============
SV_ReadLevelFile

==============
*/
void SV_ReadLevelFile (void)
{
	char	name[MAX_OSPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_ReadLevelFile()\n");

	String::Sprintf (name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_FOpenFileRead(name, &f, true);
	if (!f)
	{
		Com_Printf ("Failed to open %s\n", name);
		return;
	}
	FS_Read(sv.configstrings, sizeof(sv.configstrings), f);
	CM_ReadPortalState (f);
	FS_FCloseFile(f);

	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/current/%s.sav", sv.name)));
	ge->ReadLevel(name);
}

/*
==============
SV_WriteServerFile

==============
*/
void SV_WriteServerFile (qboolean autosave)
{
	fileHandle_t	f;
	Cvar	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf("SV_WriteServerFile(%s)\n", autosave ? "true" : "false");

	String::Sprintf (name, sizeof(name), "save/current/server.ssv");
	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Com_Printf ("Couldn't write %s\n", name);
		return;
	}
	// write the comment field
	Com_Memset(comment, 0, sizeof(comment));

	if (!autosave)
	{
		time (&aclock);
		newtime = localtime (&aclock);
		String::Sprintf (comment,sizeof(comment), "%2i:%i%i %2i/%2i  ", newtime->tm_hour
			, newtime->tm_min/10, newtime->tm_min%10,
			newtime->tm_mon+1, newtime->tm_mday);
		String::Cat(comment, sizeof(comment), sv.configstrings[CS_NAME]);
	}
	else
	{	// autosaved
		String::Sprintf (comment, sizeof(comment), "ENTERING %s", sv.configstrings[CS_NAME]);
	}

	FS_Write(comment, sizeof(comment), f);

	// write the mapcmd
	FS_Write(svs.mapcmd, sizeof(svs.mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (var = cvar_vars ; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;
		if (String::Length(var->name) >= (int)sizeof(name)-1
			|| String::Length(var->string) >= (int)sizeof(string)-1)
		{
			Com_Printf ("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		Com_Memset(name, 0, sizeof(name));
		Com_Memset(string, 0, sizeof(string));
		String::Cpy(name, var->name);
		String::Cpy(string, var->string);
		FS_Write(name, sizeof(name), f);
		FS_Write(string, sizeof(string), f);
	}

	FS_FCloseFile(f);

	// write game state
	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), "save/current/game.ssv"));
	ge->WriteGame (name, autosave);
}

/*
==============
SV_ReadServerFile

==============
*/
void SV_ReadServerFile (void)
{
	fileHandle_t	f;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	char	mapcmd[MAX_TOKEN_CHARS_Q2];

	Com_DPrintf("SV_ReadServerFile()\n");

	FS_FOpenFileRead("save/current/server.ssv", &f, true);
	if (!f)
	{
		Com_Printf ("Couldn't read %s\n", name);
		return;
	}
	// read the comment field
	FS_Read (comment, sizeof(comment), f);

	// read the mapcmd
	FS_Read (mapcmd, sizeof(mapcmd), f);

	// read all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	while (1)
	{
		if (!FS_Read (name, sizeof(name), f))
			break;
		FS_Read (string, sizeof(string), f);
		Com_DPrintf ("Set %s = %s\n", name, string);
		Cvar_Set(name, string);
	}

	FS_FCloseFile (f);

	// start a new game fresh with new cvars
	SV_InitGame ();

	String::Cpy(svs.mapcmd, mapcmd);

	// read game state
	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), "save/current/game.ssv"));
	ge->ReadGame(name);
}


//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void)
{
	SV_Map (true, Cmd_Argv(1), false );
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
void SV_GameMap_f (void)
{
	char		*map;
	int			i;
	client_t	*cl;
	qboolean	*savedInuse;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: gamemap <map>\n");
		return;
	}

	Com_DPrintf("SV_GameMap(%s)\n", Cmd_Argv(1));

	// check for clearing the current savegame
	map = Cmd_Argv(1);
	if (map[0] == '*')
	{
		// wipe all the *.sav files
		SV_WipeSavegame ("current");
	}
	else
	{	// save the map just exited
		if (sv.state == ss_game)
		{
			// clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			savedInuse = (qboolean*)malloc(maxclients->value * sizeof(qboolean));
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
			{
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
				cl->edict->inuse = savedInuse[i];
			free (savedInuse);
		}
	}

	// start up the next map
	SV_Map (false, Cmd_Argv(1), false );

	// archive server state
	String::NCpy(svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd)-1);

	// copy off the level to the autosave slot
	if (!com_dedicated->value)
	{
		SV_WriteServerFile (true);
		SV_CopySaveGame ("current", "save0");
	}
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f (void)
{
	char	*map;
	char	expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr (map, "."))
	{
		String::Sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
		if (FS_ReadFile(expanded, NULL) == -1)
		{
			Com_Printf ("Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame("current");
	SV_GameMap_f ();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/


/*
==============
SV_Loadgame_f

==============
*/
void SV_Loadgame_f (void)
{
	char	name[MAX_OSPATH];
	fileHandle_t	f;
	char	*dir;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: loadgame <directory>\n");
		return;
	}

	Com_Printf ("Loading game...\n");

	dir = Cmd_Argv(1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") )
	{
		Com_Printf ("Bad savedir.\n");
	}

	// make sure the server.ssv file exists
	String::Sprintf (name, sizeof(name), "save/%s/server.ssv", Cmd_Argv(1));
	FS_FOpenFileRead(name, &f, true);
	if (!f)
	{
		Com_Printf ("No such savegame: %s\n", name);
		return;
	}
	FS_FCloseFile(f);

	SV_CopySaveGame (Cmd_Argv(1), "current");

	SV_ReadServerFile ();

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map (false, svs.mapcmd, true);
}



/*
==============
SV_Savegame_f

==============
*/
void SV_Savegame_f (void)
{
	char	*dir;

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		Com_Printf ("Can't savegame in a deathmatch\n");
		return;
	}

	if (!String::Cmp(Cmd_Argv(1), "current"))
	{
		Com_Printf ("Can't save to 'current'\n");
		return;
	}

	if (maxclients->value == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		Com_Printf ("\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") )
	{
		Com_Printf ("Bad savedir.\n");
	}

	Com_Printf ("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile ();

	// save server state
	SV_WriteServerFile (false);

	// copy it off
	SV_CopySaveGame ("current", dir);

	Com_Printf ("Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	if (!svs.initialized)
	{
		Com_Printf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", sv_client->name);
	// print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf (sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (sv_client);
	sv_client->lastmessage = svs.realtime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int			i, j, l;
	client_t	*cl;
	const char*	s;
	int			ping;
	if (!svs.clients)
	{
		Com_Printf ("No server running.\n");
		return;
	}
	Com_Printf ("map              : %s\n", sv.name);

	Com_Printf ("num score ping name            lastmsg address               qport \n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3i ", i);
		Com_Printf ("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
		l = 16 - String::Length(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%7i ", svs.realtime - cl->lastmessage );

		s = SOCK_AdrToString( cl->netchan.remote_address);
		Com_Printf ("%s", s);
		l = 22 - String::Length(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");
		
		Com_Printf ("%5i", cl->netchan.qport);

		Com_Printf ("\n");
	}
	Com_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	String::Cpy(text, "console: ");
	p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p)-1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	for (j = 0, client = svs.clients; j < maxclients->value; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void)
{
	Com_Printf ("Server info settings:\n");
	Info_Print (Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING, MAX_INFO_KEY,
		MAX_INFO_VALUE, true, false));
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf ("userinfo\n");
	Com_Printf ("--------\n");
	Info_Print (sv_client->userinfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
void SV_ServerRecord_f (void)
{
	char	name[MAX_OSPATH];
	byte	buf_data[32768];
	QMsg	buf;
	int		len;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	String::Sprintf (name, sizeof(name), "demos/%s.dm2", Cmd_Argv(1));

	Com_Printf ("recording to %s.\n", name);
	svs.demofile = FS_FOpenFileWrite(name);
	if (!svs.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	svs.demo_multicast.InitOOB(svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

	//
	// write a single giant fake message with all the startup info
	//
	buf.InitOOB(buf_data, sizeof(buf_data));

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	buf.WriteByte(svc_serverdata);
	buf.WriteLong(PROTOCOL_VERSION);
	buf.WriteLong(svs.spawncount);
	// 2 means server demo
	buf.WriteByte(2);	// demos are always attract loops
	buf.WriteString2(Cvar_VariableString ("gamedir"));
	buf.WriteShort(-1);
	// send full levelname
	buf.WriteString2(sv.configstrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
		if (sv.configstrings[i][0])
		{
			buf.WriteByte(svc_configstring);
			buf.WriteShort(i);
			buf.WriteString2(sv.configstrings[i]);
		}

	// write it to the demo file
	Com_DPrintf ("signon message length: %i\n", buf.cursize);
	len = LittleLong (buf.cursize);
	FS_Write(&len, 4, svs.demofile);
	FS_Write(buf._data, buf.cursize, svs.demofile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f (void)
{
	if (!svs.demofile)
	{
		Com_Printf ("Not doing a serverrecord.\n");
		return;
	}
	FS_FCloseFile(svs.demofile);
	svs.demofile = 0;
	Com_Printf ("Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f (void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown ("Server was killed.\n", false);
	NET_Config ( false );	// close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
void SV_ServerCommand_f (void)
{
	if (!ge)
	{
		Com_Printf ("No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("demomap", SV_DemoMap_f);
	Cmd_AddCommand ("gamemap", SV_GameMap_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	if ( com_dedicated->value )
		Cmd_AddCommand ("say", SV_ConSay_f);

	Cmd_AddCommand ("serverrecord", SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop", SV_ServerStop_f);

	Cmd_AddCommand ("save", SV_Savegame_f);
	Cmd_AddCommand ("load", SV_Loadgame_f);

	Cmd_AddCommand ("killserver", SV_KillServer_f);

	Cmd_AddCommand ("sv", SV_ServerCommand_f);
}

