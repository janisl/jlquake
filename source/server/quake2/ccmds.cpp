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
#include <time.h>

client_t* SVQ2_SetPlayer()
{
	if (Cmd_Argc() < 2)
	{
		return NULL;
	}

	const char* s = Cmd_Argv(1);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		int idnum = String::Atoi(Cmd_Argv(1));
		if (idnum < 0 || idnum >= sv_maxclients->value)
		{
			common->Printf("Bad client slot: %i\n", idnum);
			return NULL;
		}

		client_t* client = &svs.clients[idnum];
		if (!client->state)
		{
			common->Printf("Client %i is not active\n", idnum);
			return NULL;
		}
		return client;
	}

	// check for a name match
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (!String::Cmp(cl->name, s))
		{
			return cl;
		}
	}

	common->Printf("Userid %s is not on the server\n", s);
	return NULL;
}

//
//	SAVEGAME FILES
//

//	Delete save/<XXX>/
void SVQ2_WipeSavegame(const char* savename)
{
	common->DPrintf("SV_WipeSaveGame(%s)\n", savename);

	char* name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/server.ssv", savename));
	FS_Remove(name);
	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/game.ssv", savename));
	FS_Remove(name);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s", savename));
	int NumSysFiles;
	char** SysFiles = Sys_ListFiles(name, ".sav", NULL, &NumSysFiles, false);
	for (int i = 0; i < NumSysFiles; i++)
	{
		name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", savename, SysFiles[i]));
		FS_Remove(name);
	}
	Sys_FreeFileList(SysFiles);

	name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s", savename));
	SysFiles = Sys_ListFiles(name, ".sv2", NULL, &NumSysFiles, false);
	for (int i = 0; i < NumSysFiles; i++)
	{
		name = FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/%s/%s", savename, SysFiles[i]));
		FS_Remove(name);
	}
	Sys_FreeFileList(SysFiles);
}

void SVQ2_CopySaveGame(const char* src, const char* dst)
{
	common->DPrintf("SVQ2_CopySaveGame(%s, %s)\n", src, dst);

	SVQ2_WipeSavegame(dst);

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
	for (int i = 0; i < NumSysFiles; i++)
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

void SVQ2_WriteLevelFile()
{
	common->DPrintf("SVQ2_WriteLevelFile()\n");

	char name[MAX_OSPATH];
	String::Sprintf(name, sizeof(name), "save/current/%s.sv2", sv.name);
	fileHandle_t f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("Failed to open %s\n", name);
		return;
	}
	FS_Write(sv.q2_configstrings, sizeof(sv.q2_configstrings), f);
	CM_WritePortalState(f);
	FS_FCloseFile(f);

	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/current/%s.sav", sv.name)));
	ge->WriteLevel(name);
}

void SVQ2_ReadLevelFile()
{
	common->DPrintf("SVQ2_ReadLevelFile()\n");

	char name[MAX_OSPATH];
	String::Sprintf(name, sizeof(name), "save/current/%s.sv2", sv.name);
	fileHandle_t f;
	FS_FOpenFileRead(name, &f, true);
	if (!f)
	{
		common->Printf("Failed to open %s\n", name);
		return;
	}
	FS_Read(sv.q2_configstrings, sizeof(sv.q2_configstrings), f);
	CM_ReadPortalState(f);
	FS_FCloseFile(f);

	String::Cpy(name, FS_BuildOSPath(fs_homepath->string, FS_Gamedir(), va("save/current/%s.sav", sv.name)));
	ge->ReadLevel(name);
}

void SVQ2_WriteServerFile(bool autosave)
{
	common->DPrintf("SVQ2_WriteServerFile(%s)\n", autosave ? "true" : "false");

	char name[MAX_OSPATH];
	String::Sprintf(name, sizeof(name), "save/current/server.ssv");
	fileHandle_t f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("Couldn't write %s\n", name);
		return;
	}
	// write the comment field
	char comment[32];
	Com_Memset(comment, 0, sizeof(comment));

	if (!autosave)
	{
		time_t aclock;
		time(&aclock);
		tm* newtime = localtime(&aclock);
		String::Sprintf(comment,sizeof(comment), "%2i:%i%i %2i/%2i  ", newtime->tm_hour,
			newtime->tm_min / 10, newtime->tm_min % 10,
			newtime->tm_mon + 1, newtime->tm_mday);
		String::Cat(comment, sizeof(comment), sv.q2_configstrings[Q2CS_NAME]);
	}
	else
	{	// autosaved
		String::Sprintf(comment, sizeof(comment), "ENTERING %s", sv.q2_configstrings[Q2CS_NAME]);
	}

	FS_Write(comment, sizeof(comment), f);

	// write the mapcmd
	FS_Write(svs.q2_mapcmd, sizeof(svs.q2_mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (Cvar* var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_LATCH))
		{
			continue;
		}
		char string[128];
		if (String::Length(var->name) >= (int)sizeof(name) - 1 ||
			String::Length(var->string) >= (int)sizeof(string) - 1)
		{
			common->Printf("Cvar too long: %s = %s\n", var->name, var->string);
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
	ge->WriteGame(name, autosave);
}

//	Kick a user off of the server
void SVQ2_Kick_f()
{
	if (!svs.initialized)
	{
		common->Printf("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: kick <userid>\n");
		return;
	}

	client_t* client = SVQ2_SetPlayer();
	if (!client)
	{
		return;
	}

	SVQ2_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", client->name);
	// print directly, because the dropped client won't get the
	// SVQ2_BroadcastPrintf message
	SVQ2_ClientPrintf(client, PRINT_HIGH, "You were kicked from the game\n");
	SVQ2_DropClient(client);
	client->q2_lastmessage = svs.q2_realtime;	// min case there is a funny zombie
}

void SVQ2_Status_f()
{
	if (!svs.clients)
	{
		common->Printf("No server running.\n");
		return;
	}
	common->Printf("map              : %s\n", sv.name);

	common->Printf("num score ping name            lastmsg address               qport \n");
	common->Printf("--- ----- ---- --------------- ------- --------------------- ------\n");
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		common->Printf("%3i ", i);
		common->Printf("%5i ", cl->q2_edict->client->ps.stats[Q2STAT_FRAGS]);

		if (cl->state == CS_CONNECTED)
		{
			common->Printf("CNCT ");
		}
		else if (cl->state == CS_ZOMBIE)
		{
			common->Printf("ZMBI ");
		}
		else
		{
			int ping = cl->ping < 9999 ? cl->ping : 9999;
			common->Printf("%4i ", ping);
		}

		common->Printf("%s", cl->name);
		int l = 16 - String::Length(cl->name);
		for (int j = 0; j < l; j++)
		{
			common->Printf(" ");
		}

		common->Printf("%7i ", svs.q2_realtime - cl->q2_lastmessage);

		const char* s = SOCK_AdrToString(cl->netchan.remoteAddress);
		common->Printf("%s", s);
		l = 22 - String::Length(s);
		for (int j = 0; j < l; j++)
		{
			common->Printf(" ");
		}

		common->Printf("%5i", cl->netchan.qport);

		common->Printf("\n");
	}
	common->Printf("\n");
}

void SVQ2_ConSay_f()
{
	if (Cmd_Argc() < 2)
	{
		return;
	}

	char text[1024];
	String::Cpy(text, "console: ");
	char* p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	client_t* client = svs.clients;
	for (int j = 0; j < sv_maxclients->value; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQ2_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}

void SVQ2_Heartbeat_f()
{
	svs.q2_last_heartbeat = -9999999;
}

//	Examine or change the serverinfo string
void SVQ2_Serverinfo_f()
{
	common->Printf("Server info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false));
}

//	Examine all a users info strings
void SVQ2_DumpUser_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: info <userid>\n");
		return;
	}

	client_t* client = SVQ2_SetPlayer();
	if (!client)
	{
		return;
	}

	common->Printf("userinfo\n");
	common->Printf("--------\n");
	Info_Print(client->userinfo);

}

//	Begins server demo recording.  Every entity and every message will be
// recorded, but no playerinfo will be stored.  Primarily for demo merging.
void SVQ2_ServerRecord_f()
{
	char name[MAX_OSPATH];
	byte buf_data[32768];
	QMsg buf;
	int len;
	int i;

	if (Cmd_Argc() != 2)
	{
		common->Printf("serverrecord <demoname>\n");
		return;
	}

	if (svs.q2_demofile)
	{
		common->Printf("Already recording.\n");
		return;
	}

	if (sv.state != SS_GAME)
	{
		common->Printf("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	String::Sprintf(name, sizeof(name), "demos/%s.dm2", Cmd_Argv(1));

	common->Printf("recording to %s.\n", name);
	svs.q2_demofile = FS_FOpenFileWrite(name);
	if (!svs.q2_demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	svs.q2_demo_multicast.InitOOB(svs.q2_demo_multicast_buf, sizeof(svs.q2_demo_multicast_buf));

	//
	// write a single giant fake message with all the startup info
	//
	buf.InitOOB(buf_data, sizeof(buf_data));

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	buf.WriteByte(q2svc_serverdata);
	buf.WriteLong(Q2PROTOCOL_VERSION);
	buf.WriteLong(svs.spawncount);
	// 2 means server demo
	buf.WriteByte(2);	// demos are always attract loops
	buf.WriteString2(Cvar_VariableString("gamedir"));
	buf.WriteShort(-1);
	// send full levelname
	buf.WriteString2(sv.q2_configstrings[Q2CS_NAME]);

	for (i = 0; i < MAX_CONFIGSTRINGS_Q2; i++)
		if (sv.q2_configstrings[i][0])
		{
			buf.WriteByte(q2svc_configstring);
			buf.WriteShort(i);
			buf.WriteString2(sv.q2_configstrings[i]);
		}

	// write it to the demo file
	common->DPrintf("signon message length: %i\n", buf.cursize);
	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, svs.q2_demofile);
	FS_Write(buf._data, buf.cursize, svs.q2_demofile);

	// the rest of the demo file will be individual frames
}

//	Ends server demo recording
void SVQ2_ServerStop_f()
{
	if (!svs.q2_demofile)
	{
		common->Printf("Not doing a serverrecord.\n");
		return;
	}
	FS_FCloseFile(svs.q2_demofile);
	svs.q2_demofile = 0;
	common->Printf("Recording completed.\n");
}

//	Let the game dll handle a command
void SVQ2_ServerCommand_f()
{
	if (!ge)
	{
		common->Printf("No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}
