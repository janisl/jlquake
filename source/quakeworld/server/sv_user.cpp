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
// sv_user.c -- server code for moving users

#include "qwsvdef.h"

qhedict_t* sv_player;

qwusercmd_t cmd;

Cvar* sv_spectalk;

extern int fp_messages, fp_persecond, fp_secondsdead;
extern char fp_msg[];
extern Cvar* pausable;

/*
============================================================

USER STRINGCMD EXECUTION

host_client and sv_player will be valid.
============================================================
*/

void SVQHW_PreSpawn_f(client_t* client)
{
	if (client->state != CS_CONNECTED)
	{
		common->Printf("prespawn not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVQHW_PreSpawn_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	int buf = String::Atoi(Cmd_Argv(2));
	if (buf < 0 || buf >= sv.qh_num_signon_buffers)
	{
		buf = 0;
	}

	if (GGameType & GAME_QuakeWorld)
	{
		if (!buf)
		{
			// should be three numbers following containing checksums
			int check = String::Atoi(Cmd_Argv(3));

			int map_checksum;
			int map_checksum2;
			CM_MapChecksums(map_checksum, map_checksum2);
			if (svqw_mapcheck->value && check != map_checksum && check != map_checksum2)
			{
				SVQH_ClientPrintf(client, PRINT_HIGH,
					"Map model file does not match (%s), %i != %i/%i.\n"
					"You may need a new version of the map, or the proper install files.\n",
					sv.qh_modelname, check, map_checksum, map_checksum2);
				SV_DropClient(client);
				return;
			}
			client->qw_checksum = check;
		}

		//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
		//spawns.  These functions are written to not overflow
		if (client->qw_num_backbuf)
		{
			common->Printf("WARNING %s: [SV_PreSpawn] Back buffered (%d0, clearing", client->name, client->netchan.message.cursize);
			client->qw_num_backbuf = 0;
			client->netchan.message.Clear();
		}
	}

	client->netchan.message.WriteData(
		sv.qh_signon_buffers[buf],
		sv.qh_signon_buffer_size[buf]);

	buf++;
	if (buf == sv.qh_num_signon_buffers)
	{
		// all done prespawning
		if (GGameType & GAME_HexenWorld)
		{
			client->netchan.message.WriteByte(h2svc_stufftext);
			client->netchan.message.WriteString2(va("cmd spawn %i\n",svs.spawncount));
		}
		else
		{
			client->netchan.message.WriteByte(q1svc_stufftext);
			client->netchan.message.WriteString2(va("cmd spawn %i 0\n",svs.spawncount));
		}
	}
	else
	{	// need to prespawn more
		client->netchan.message.WriteByte(GGameType & GAME_HexenWorld ? h2svc_stufftext : q1svc_stufftext);
		client->netchan.message.WriteString2(
			va("cmd prespawn %i %i\n", svs.spawncount, buf));
	}
}

/*
==================
SV_Spawn_f
==================
*/
void SV_Spawn_f(void)
{
	int i;
	client_t* client;
	qhedict_t* ent;
	eval_t* val;
	int n;

	if (host_client->state != CS_CONNECTED)
	{
		common->Printf("Spawn not valid -- allready spawned\n");
		return;
	}

// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SV_Spawn_f from different level\n");
		SVQHW_New_f(host_client);
		return;
	}

	n = String::Atoi(Cmd_Argv(2));

	// make sure n is valid
	if (n < 0 || n > MAX_CLIENTS_QHW)
	{
		common->Printf("SV_Spawn_f invalid client start\n");
		SVQHW_New_f(host_client);
		return;
	}



// send all current names, colors, and frag counts
	// FIXME: is this a good thing?
	host_client->netchan.message.Clear();

// send current status of all other players

	// normally this could overflow, but no need to check due to backbuf
	for (i = n, client = svs.clients + n; i < MAX_CLIENTS_QHW; i++, client++)
		SV_FullClientUpdateToClient(client, host_client);

// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		SVQH_ClientReliableWrite_Begin(host_client, q1svc_lightstyle,
			3 + (sv.qh_lightstyles[i] ? String::Length(sv.qh_lightstyles[i]) : 1));
		SVQH_ClientReliableWrite_Byte(host_client, (char)i);
		SVQH_ClientReliableWrite_String(host_client, sv.qh_lightstyles[i]);
	}

	// set up the edict
	ent = host_client->qh_edict;

	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->SetColorMap(QH_NUM_FOR_EDICT(ent));
	ent->SetTeam(0);	// FIXME
	ent->SetNetName(PR_SetString(host_client->name));

	host_client->qh_entgravity = 1.0;
	val = GetEdictFieldValue(ent, "gravity");
	if (val)
	{
		val->_float = 1.0;
	}
	host_client->qh_maxspeed = svqh_maxspeed->value;
	val = GetEdictFieldValue(ent, "maxspeed");
	if (val)
	{
		val->_float = svqh_maxspeed->value;
	}

//
// force stats to be updated
//
	Com_Memset(host_client->qh_stats, 0, sizeof(host_client->qh_stats));

	SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_TOTALSECRETS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.total_secrets);

	SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_TOTALMONSTERS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.total_monsters);

	SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_SECRETS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.found_secrets);

	SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_MONSTERS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.killed_monsters);

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	SVQH_SendClientCommand(host_client, "skins\n");

}

/*
==================
SV_SpawnSpectator
==================
*/
void SV_SpawnSpectator(void)
{
	int i;
	qhedict_t* e;

	VectorCopy(vec3_origin, sv_player->GetOrigin());
	sv_player->SetViewOfs(vec3_origin);
	sv_player->GetViewOfs()[2] = 22;

	// search for an info_playerstart to spawn the spectator at
	for (i = MAX_CLIENTS_QHW - 1; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "info_player_start"))
		{
			VectorCopy(e->GetOrigin(), sv_player->GetOrigin());
			return;
		}
	}

}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f(void)
{
	unsigned pmodel = 0, emodel = 0;
	int i;

	if (host_client->state == CS_ACTIVE)
	{
		return;	// don't begin again

	}
	host_client->state = CS_ACTIVE;

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SV_Begin_f from different level\n");
		SVQHW_New_f(host_client);
		return;
	}

	if (host_client->qh_spectator)
	{
		SV_SpawnSpectator();

		if (SpectatorConnect)
		{
			// copy spawn parms out of the client_t
			for (i = 0; i < NUM_SPAWN_PARMS; i++)
				pr_globalVars.parm1[i] = host_client->qh_spawn_parms[i];

			// call the spawn function
			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram(SpectatorConnect);
		}
	}
	else
	{
		// copy spawn parms out of the client_t
		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			pr_globalVars.parm1[i] = host_client->qh_spawn_parms[i];

		// call the spawn function
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(*pr_globalVars.ClientConnect);

		// actually spawn the player
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(*pr_globalVars.PutClientInServer);
	}

	// clear the net statistics, because connecting gives a bogus picture
	host_client->netchan.dropCount = 0;

	//check he's not cheating

	pmodel = String::Atoi(Info_ValueForKey(host_client->userinfo, "pmodel"));
	emodel = String::Atoi(Info_ValueForKey(host_client->userinfo, "emodel"));

	if (pmodel != sv.qw_model_player_checksum ||
		emodel != sv.qw_eyes_player_checksum)
	{
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s WARNING: non standard player/eyes model detected\n", host_client->name);
	}

	// if we are paused, tell the client
	if (sv.qh_paused)
	{
		SVQH_ClientReliableWrite_Begin(host_client, q1svc_setpause, 2);
		SVQH_ClientReliableWrite_Byte(host_client, sv.qh_paused);
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Server is paused.\n");
	}

#if 0
//
// send a fixangle over the reliable channel to make sure it gets there
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = QH_EDICT_NUM(1 + (host_client - svs.clients));
	host_client->netchan.message.WriteByte(q1svc_setangle);
	for (i = 0; i < 2; i++)
		host_client->netchan.message.WriteAngle(ent->v.angles[i]);
	host_client->netchan.message.WriteAngle(0);
#endif
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f(void)
{
	byte buffer[1024];
	int r;
	int percent;
	int size;

	if (!host_client->download)
	{
		return;
	}

	r = host_client->downloadSize - host_client->downloadCount;
	if (r > 768)
	{
		r = 768;
	}
	r = FS_Read(buffer, r, host_client->download);
	SVQH_ClientReliableWrite_Begin(host_client, qwsvc_download, 6 + r);
	SVQH_ClientReliableWrite_Short(host_client, r);

	host_client->downloadCount += r;
	size = host_client->downloadSize;
	if (!size)
	{
		size = 1;
	}
	percent = host_client->downloadCount * 100 / size;
	SVQH_ClientReliableWrite_Byte(host_client, percent);
	SVQH_ClientReliableWrite_SZ(host_client, buffer, r);

	if (host_client->downloadCount != host_client->downloadSize)
	{
		return;
	}

	FS_FCloseFile(host_client->download);
	host_client->download = 0;

}

void OutofBandPrintf(netadr_t where, const char* fmt, ...)
{
	va_list argptr;
	char send[1024];

	send[0] = 0xff;
	send[1] = 0xff;
	send[2] = 0xff;
	send[3] = 0xff;
	send[4] = A2C_PRINT;
	va_start(argptr, fmt);
	Q_vsnprintf(send + 5, 1024 - 5, fmt, argptr);
	va_end(argptr);

	NET_SendPacket(NS_SERVER, String::Length(send) + 1, send, where);
}

/*
==================
SV_NextUpload
==================
*/
void SV_NextUpload(void)
{
	int percent;
	int size;

	if (!*host_client->qw_uploadfn)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Upload denied\n");
		SVQH_SendClientCommand(host_client, "stopul");

		// suck out rest of packet
		size = net_message.ReadShort(); net_message.ReadByte();
		net_message.readcount += size;
		return;
	}

	size = net_message.ReadShort();
	percent = net_message.ReadByte();

	if (!host_client->qw_upload)
	{
		host_client->qw_upload = FS_FOpenFileWrite(host_client->qw_uploadfn);
		if (!host_client->qw_upload)
		{
			common->Printf("Can't create %s\n", host_client->qw_uploadfn);
			SVQH_SendClientCommand(host_client, "stopul");
			*host_client->qw_uploadfn = 0;
			return;
		}
		common->Printf("Receiving %s from %d...\n", host_client->qw_uploadfn, host_client->qh_userid);
		if (host_client->qw_remote_snap)
		{
			OutofBandPrintf(host_client->qw_snap_from, "Server receiving %s from %d...\n", host_client->qw_uploadfn, host_client->qh_userid);
		}
	}

	FS_Write(net_message._data + net_message.readcount, size, host_client->qw_upload);
	net_message.readcount += size;

	common->DPrintf("UPLOAD: %d received\n", size);

	if (percent != 100)
	{
		SVQH_SendClientCommand(host_client, "nextul\n");
	}
	else
	{
		FS_FCloseFile(host_client->qw_upload);
		host_client->qw_upload = 0;

		common->Printf("%s upload completed.\n", host_client->qw_uploadfn);

		if (host_client->qw_remote_snap)
		{
			char* p;

			if ((p = strchr(host_client->qw_uploadfn, '/')) != NULL)
			{
				p++;
			}
			else
			{
				p = host_client->qw_uploadfn;
			}
			OutofBandPrintf(host_client->qw_snap_from, "%s upload completed.\nTo download, enter:\ndownload %s\n",
				host_client->qw_uploadfn, p);
		}
	}

}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char* name;
	extern Cvar* allow_download;
	extern Cvar* allow_download_skins;
	extern Cvar* allow_download_models;
	extern Cvar* allow_download_sounds;
	extern Cvar* allow_download_maps;

	name = Cmd_Argv(1);
// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (strstr(name, "..") || !allow_download->value
		// leading dot is no good
		|| *name == '.'
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| (String::NCmp(name, "skins/", 6) == 0 && !allow_download_skins->value)
		// now models
		|| (String::NCmp(name, "progs/", 6) == 0 && !allow_download_models->value)
		// now sounds
		|| (String::NCmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (String::NCmp(name, "maps/", 6) == 0 && !allow_download_maps->value)
		// MUST be in a subdirectory
		|| !strstr(name, "/"))
	{	// don't allow anything with .. path
		SVQH_ClientReliableWrite_Begin(host_client, qwsvc_download, 4);
		SVQH_ClientReliableWrite_Short(host_client, -1);
		SVQH_ClientReliableWrite_Byte(host_client, 0);
		return;
	}

	if (host_client->download)
	{
		FS_FCloseFile(host_client->download);
		host_client->download = 0;
	}

	// lowercase name (needed for casesen file systems)
	{
		char* p;

		for (p = name; *p; p++)
			*p = (char)String::ToLower(*p);
	}


	host_client->downloadSize = FS_FOpenFileRead(name, &host_client->download, true);
	host_client->downloadCount = 0;

	if (!host_client->download
		// special check for maps, if it came from a pak file, don't allow
		// download  ZOID
		|| (String::NCmp(name, "maps/", 5) == 0 && FS_FileIsInPAK(name, NULL) == 1))
	{
		if (host_client->download)
		{
			FS_FCloseFile(host_client->download);
			host_client->download = 0;
		}

		common->Printf("Couldn't download %s to %s\n", name, host_client->name);
		SVQH_ClientReliableWrite_Begin(host_client, qwsvc_download, 4);
		SVQH_ClientReliableWrite_Short(host_client, -1);
		SVQH_ClientReliableWrite_Byte(host_client, 0);
		return;
	}

	SV_NextDownload_f();
	common->Printf("Downloading %s to %s\n", name, host_client->name);
}

//=============================================================================

/*
==================
SV_Say
==================
*/
void SV_Say(qboolean team)
{
	client_t* client;
	int j, tmp;
	char* p;
	char text[2048];
	char t1[32];
	const char* t2;

	if (Cmd_Argc() < 2)
	{
		return;
	}

	if (team)
	{
		String::NCpy(t1, Info_ValueForKey(host_client->userinfo, "team"), 31);
		t1[31] = 0;
	}

	if (host_client->qh_spectator && (!sv_spectalk->value || team))
	{
		sprintf(text, "[SPEC] %s: ", host_client->name);
	}
	else if (team)
	{
		sprintf(text, "(%s): ", host_client->name);
	}
	else
	{
		sprintf(text, "%s: ", host_client->name);
	}

	if (fp_messages)
	{
		if (!sv.qh_paused && realtime < host_client->qh_lockedtill)
		{
			SVQH_ClientPrintf(host_client, PRINT_CHAT,
				"You can't talk for %d more seconds\n",
				(int)(host_client->qh_lockedtill - realtime));
			return;
		}
		tmp = host_client->qh_whensaidhead - fp_messages + 1;
		if (tmp < 0)
		{
			tmp = 10 + tmp;
		}
		if (!sv.qh_paused &&
			host_client->qh_whensaid[tmp] && (realtime - host_client->qh_whensaid[tmp] < fp_persecond))
		{
			host_client->qh_lockedtill = realtime + fp_secondsdead;
			if (fp_msg[0])
			{
				SVQH_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: %s\n", fp_msg);
			}
			else
			{
				SVQH_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: You can't talk for %d seconds.\n", fp_secondsdead);
			}
			return;
		}
		host_client->qh_whensaidhead++;
		if (host_client->qh_whensaidhead > 9)
		{
			host_client->qh_whensaidhead = 0;
		}
		host_client->qh_whensaid[host_client->qh_whensaidhead] = realtime;
	}

	p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	common->Printf("%s", text);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		if (host_client->qh_spectator && !sv_spectalk->value)
		{
			if (!client->qh_spectator)
			{
				continue;
			}
		}

		if (team)
		{
			// the spectator team
			if (host_client->qh_spectator)
			{
				if (!client->qh_spectator)
				{
					continue;
				}
			}
			else
			{
				t2 = Info_ValueForKey(client->userinfo, "team");
				if (String::Cmp(t1, t2) || client->qh_spectator)
				{
					continue;	// on different teams
				}
			}
		}
		SVQH_ClientPrintf(client, PRINT_CHAT, "%s", text);
	}
}


/*
==================
SV_Say_f
==================
*/
void SV_Say_f(void)
{
	SV_Say(false);
}
/*
==================
SV_Say_Team_f
==================
*/
void SV_Say_Team_f(void)
{
	SV_Say(true);
}



//============================================================================

/*
=================
SV_Pings_f

The client is showing the scoreboard, so send new ping times for all
clients
=================
*/
void SV_Pings_f(void)
{
	client_t* client;
	int j;

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updateping, 4);
		SVQH_ClientReliableWrite_Byte(host_client, j);
		SVQH_ClientReliableWrite_Short(host_client, SVQH_CalcPing(client));
		SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatepl, 4);
		SVQH_ClientReliableWrite_Byte(host_client, j);
		SVQH_ClientReliableWrite_Byte(host_client, client->qw_lossage);
	}
}



/*
==================
SV_Kill_f
==================
*/
void SV_Kill_f(void)
{
	if (sv_player->GetHealth() <= 0)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Can't suicide -- allready dead!\n");
		return;
	}

	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram(*pr_globalVars.ClientKill);
}

/*
==================
SV_TogglePause
==================
*/
void SV_TogglePause(const char* msg)
{
	int i;
	client_t* cl;

	sv.qh_paused ^= 1;

	if (msg)
	{
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s", msg);
	}

	// send notification to all clients
	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		SVQH_ClientReliableWrite_Begin(cl, q1svc_setpause, 2);
		SVQH_ClientReliableWrite_Byte(cl, sv.qh_paused);
	}
}


/*
==================
SV_Pause_f
==================
*/
void SV_Pause_f(void)
{
	char st[sizeof(host_client->name) + 32];

	if (!pausable->value)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Pause not allowed.\n");
		return;
	}

	if (host_client->qh_spectator)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Spectators can not pause.\n");
		return;
	}

	if (sv.qh_paused)
	{
		sprintf(st, "%s paused the game\n", host_client->name);
	}
	else
	{
		sprintf(st, "%s unpaused the game\n", host_client->name);
	}

	SV_TogglePause(st);
}


/*
=================
SV_Drop_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Drop_f(void)
{
	SV_EndRedirect();
	if (!host_client->qh_spectator)
	{
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s dropped\n", host_client->name);
	}
	SV_DropClient(host_client);
}

/*
=================
SV_PTrack_f

Change the bandwidth estimate for a client
=================
*/
void SV_PTrack_f(void)
{
	int i;
	qhedict_t* ent, * tent;

	if (!host_client->qh_spectator)
	{
		return;
	}

	if (Cmd_Argc() != 2)
	{
		// turn off tracking
		host_client->qh_spec_track = 0;
		ent = QH_EDICT_NUM(host_client - svs.clients + 1);
		tent = QH_EDICT_NUM(0);
		ent->SetGoalEntity(EDICT_TO_PROG(tent));
		return;
	}

	i = String::Atoi(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CLIENTS_QHW || svs.clients[i].state != CS_ACTIVE ||
		svs.clients[i].qh_spectator)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Invalid client to track\n");
		host_client->qh_spec_track = 0;
		ent = QH_EDICT_NUM(host_client - svs.clients + 1);
		tent = QH_EDICT_NUM(0);
		ent->SetGoalEntity(EDICT_TO_PROG(tent));
		return;
	}
	host_client->qh_spec_track = i + 1;// now tracking

	ent = QH_EDICT_NUM(host_client - svs.clients + 1);
	tent = QH_EDICT_NUM(i + 1);
	ent->SetGoalEntity(EDICT_TO_PROG(tent));
}

/*
=================
SV_Msg_f

Change the message level for a client
=================
*/
void SV_Msg_f(void)
{
	if (Cmd_Argc() != 2)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Current msg level is %i\n",
			host_client->messagelevel);
		return;
	}

	host_client->messagelevel = String::Atoi(Cmd_Argv(1));

	SVQH_ClientPrintf(host_client, PRINT_HIGH, "Msg level set to %i\n", host_client->messagelevel);
}

/*
==================
SV_SetInfo_f

Allow clients to change userinfo
==================
*/
void SV_SetInfo_f(void)
{
	int i;
	char oldval[MAX_INFO_STRING_QW];


	if (Cmd_Argc() == 1)
	{
		common->Printf("User info settings:\n");
		Info_Print(host_client->userinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: setinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		return;		// don't set priveledged values

	}
	String::Cpy(oldval, Info_ValueForKey(host_client->userinfo, Cmd_Argv(1)));

	Info_SetValueForKey(host_client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value, false);
// name is extracted below in ExtractFromUserInfo
//	String::NCpy(host_client->name, Info_ValueForKey (host_client->userinfo, "name")
//		, sizeof(host_client->name)-1);
//	SV_FullClientUpdate (host_client, &sv.qh_reliable_datagram);
//	host_client->sendinfo = true;

	if (!String::Cmp(Info_ValueForKey(host_client->userinfo, Cmd_Argv(1)), oldval))
	{
		return;	// key hasn't changed

	}
	// process any changed values
	SV_ExtractFromUserinfo(host_client);

	i = host_client - svs.clients;
	sv.qh_reliable_datagram.WriteByte(qwsvc_setinfo);
	sv.qh_reliable_datagram.WriteByte(i);
	sv.qh_reliable_datagram.WriteString2(Cmd_Argv(1));
	sv.qh_reliable_datagram.WriteString2(Info_ValueForKey(host_client->userinfo, Cmd_Argv(1)));
}

/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f(void)
{
	Info_Print(svs.qh_info);
}

void SV_NoSnap_f(void)
{
	if (*host_client->qw_uploadfn)
	{
		*host_client->qw_uploadfn = 0;
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s refused remote screenshot\n", host_client->name);
	}
}

typedef struct
{
	const char* name;
	void (* func)(client_t*);
	void (* old_func)(void);
} ucmd_t;

ucmd_t ucmds[] =
{
	{"new", SVQHW_New_f},
	{"modellist", SVQW_Modellist_f},
	{"soundlist", SVQW_Soundlist_f},
	{"prespawn", SVQHW_PreSpawn_f},
	{"spawn", NULL, SV_Spawn_f},
	{"begin", NULL, SV_Begin_f},

	{"drop", NULL, SV_Drop_f},
	{"pings", NULL, SV_Pings_f},

// issued by hand at client consoles
	{"kill", NULL, SV_Kill_f},
	{"pause", NULL, SV_Pause_f},
	{"msg", NULL, SV_Msg_f},

	{"say", NULL, SV_Say_f},
	{"say_team", NULL, SV_Say_Team_f},

	{"setinfo", NULL, SV_SetInfo_f},

	{"serverinfo", NULL, SV_ShowServerinfo_f},

	{"download", NULL, SV_BeginDownload_f},
	{"nextdl", NULL, SV_NextDownload_f},

	{"ptrack", NULL, SV_PTrack_f},//ZOID - used with autocam

	{"snap", NULL, SV_NoSnap_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	ucmd_t* u;

	Cmd_TokenizeString(s);
	sv_player = host_client->qh_edict;

	for (u = ucmds; u->name; u++)
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			if (u->func)
			{
				u->func(cl);
			}
			else
			{
				SV_BeginRedirect(RD_CLIENT);
				u->old_func();
				SV_EndRedirect();
			}
			break;
		}

	if (!u->name)
	{
		SV_BeginRedirect(RD_CLIENT);
		common->Printf("Bad user command: %s\n", Cmd_Argv(0));
		SV_EndRedirect();
	}
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

vec3_t pmove_mins, pmove_maxs;

/*
====================
AddLinksToPmove

====================
*/
void AddLinksToPmove(worldSector_t* node)
{
	link_t* l, * next;
	qhedict_t* check;
	int pl;
	int i;
	qhphysent_t* pe;

	pl = EDICT_TO_PROG(sv_player);

	// touch linked edicts
	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		check = QHEDICT_FROM_AREA(l);

		if (check->GetOwner() == pl)
		{
			continue;		// player's own missile
		}
		if (check->GetSolid() == QHSOLID_BSP ||
			check->GetSolid() == QHSOLID_BBOX ||
			check->GetSolid() == QHSOLID_SLIDEBOX)
		{
			if (check == sv_player)
			{
				continue;
			}

			for (i = 0; i < 3; i++)
				if (check->v.absmin[i] > pmove_maxs[i] ||
					check->v.absmax[i] < pmove_mins[i])
				{
					break;
				}
			if (i != 3)
			{
				continue;
			}
			if (qh_pmove.numphysent == QHMAX_PHYSENTS)
			{
				return;
			}
			pe = &qh_pmove.physents[qh_pmove.numphysent];
			qh_pmove.numphysent++;

			VectorCopy(check->GetOrigin(), pe->origin);
			pe->info = QH_NUM_FOR_EDICT(check);
			if (check->GetSolid() == QHSOLID_BSP)
			{
				pe->model = sv.models[(int)(check->v.modelindex)];
			}
			else
			{
				pe->model = -1;
				VectorCopy(check->GetMins(), pe->mins);
				VectorCopy(check->GetMaxs(), pe->maxs);
			}
		}
	}

// recurse down both sides
	if (node->axis == -1)
	{
		return;
	}

	if (pmove_maxs[node->axis] > node->dist)
	{
		AddLinksToPmove(node->children[0]);
	}
	if (pmove_mins[node->axis] < node->dist)
	{
		AddLinksToPmove(node->children[1]);
	}
}


/*
================
AddAllEntsToPmove

For debugging
================
*/
void AddAllEntsToPmove(void)
{
	int e;
	qhedict_t* check;
	int i;
	qhphysent_t* pe;
	int pl;

	pl = EDICT_TO_PROG(sv_player);
	check = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetOwner() == pl)
		{
			continue;
		}
		if (check->GetSolid() == QHSOLID_BSP ||
			check->GetSolid() == QHSOLID_BBOX ||
			check->GetSolid() == QHSOLID_SLIDEBOX)
		{
			if (check == sv_player)
			{
				continue;
			}

			for (i = 0; i < 3; i++)
				if (check->v.absmin[i] > pmove_maxs[i] ||
					check->v.absmax[i] < pmove_mins[i])
				{
					break;
				}
			if (i != 3)
			{
				continue;
			}
			pe = &qh_pmove.physents[qh_pmove.numphysent];

			VectorCopy(check->GetOrigin(), pe->origin);
			qh_pmove.physents[qh_pmove.numphysent].info = e;
			if (check->GetSolid() == QHSOLID_BSP)
			{
				pe->model = sv.models[(int)(check->v.modelindex)];
			}
			else
			{
				pe->model = -1;
				VectorCopy(check->GetMins(), pe->mins);
				VectorCopy(check->GetMaxs(), pe->maxs);
			}

			if (++qh_pmove.numphysent == QHMAX_PHYSENTS)
			{
				break;
			}
		}
	}
}

/*
===========
SV_PreRunCmd
===========
Done before running a player command.  Clears the touch array
*/
byte playertouch[(MAX_EDICTS_QH + 7) / 8];

void SV_PreRunCmd(void)
{
	Com_Memset(playertouch, 0, sizeof(playertouch));
}

/*
===========
SV_RunCmd
===========
*/
void SV_RunCmd(qwusercmd_t* ucmd)
{
	qhedict_t* ent;
	int i, n;
	int oldmsec;

	cmd = *ucmd;

	// chop up very long commands
	if (cmd.msec > 50)
	{
		oldmsec = ucmd->msec;
		cmd.msec = oldmsec / 2;
		SV_RunCmd(&cmd);
		cmd.msec = oldmsec / 2;
		cmd.impulse = 0;
		SV_RunCmd(&cmd);
		return;
	}

	if (!sv_player->GetFixAngle())
	{
		sv_player->SetVAngle(ucmd->angles);
	}

	sv_player->SetButton0(ucmd->buttons & 1);
	sv_player->SetButton2((ucmd->buttons & 2) >> 1);
	if (ucmd->impulse)
	{
		sv_player->SetImpulse(ucmd->impulse);
	}

//
// angles
// show 1/3 the pitch angle and all the roll angle
	if (sv_player->GetHealth() > 0)
	{
		if (!sv_player->GetFixAngle())
		{
			sv_player->GetAngles()[PITCH] = -sv_player->GetVAngle()[PITCH] / 3;
			sv_player->GetAngles()[YAW] = sv_player->GetVAngle()[YAW];
		}
		sv_player->GetAngles()[ROLL] =
			VQH_CalcRoll(sv_player->GetAngles(), sv_player->GetVelocity()) * 4;
	}

	host_frametime = ucmd->msec * 0.001;
	if (host_frametime > 0.1)
	{
		host_frametime = 0.1;
	}

	if (!host_client->qh_spectator)
	{
		*pr_globalVars.frametime = host_frametime;

		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(*pr_globalVars.PlayerPreThink);

		SVQH_RunThink(sv_player, host_frametime);
	}

	for (i = 0; i < 3; i++)
		qh_pmove.origin[i] = sv_player->GetOrigin()[i] + (sv_player->GetMins()[i] - pmqh_player_mins[i]);
	VectorCopy(sv_player->GetVelocity(), qh_pmove.velocity);
	VectorCopy(sv_player->GetVAngle(), qh_pmove.angles);

	qh_pmove.spectator = host_client->qh_spectator;
	qh_pmove.waterjumptime = sv_player->GetTeleportTime();
	qh_pmove.numphysent = 1;
	qh_pmove.physents[0].model = 0;
	qh_pmove.cmd.Set(*ucmd);
	qh_pmove.dead = sv_player->GetHealth() <= 0;
	qh_pmove.oldbuttons = host_client->qh_oldbuttons;

	movevars.entgravity = host_client->qh_entgravity;
	movevars.maxspeed = host_client->qh_maxspeed;

	for (i = 0; i < 3; i++)
	{
		pmove_mins[i] = qh_pmove.origin[i] - 256;
		pmove_maxs[i] = qh_pmove.origin[i] + 256;
	}
#if 1
	AddLinksToPmove(sv_worldSectors);
#else
	AddAllEntsToPmove();
#endif

#if 0
	{
		int before, after;

		before = PMQH_TestPlayerPosition(qh_pmove.origin);
		PMQH_PlayerMove();
		after = PMQH_TestPlayerPosition(qh_pmove.origin);

		if (sv_player->v.health > 0 && before && !after)
		{
			common->Printf("player %s got stuck in playermove!!!!\n", host_client->name);
		}
	}
#else
	PMQH_PlayerMove();
#endif

	host_client->qh_oldbuttons = qh_pmove.oldbuttons;
	sv_player->SetTeleportTime(qh_pmove.waterjumptime);
	sv_player->SetWaterLevel(qh_pmove.waterlevel);
	sv_player->SetWaterType(qh_pmove.watertype);
	if (qh_pmove.onground != -1)
	{
		sv_player->SetFlags((int)sv_player->GetFlags() | QHFL_ONGROUND);
		sv_player->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(qh_pmove.physents[qh_pmove.onground].info)));
	}
	else
	{
		sv_player->SetFlags((int)sv_player->GetFlags() & ~QHFL_ONGROUND);
	}
	for (i = 0; i < 3; i++)
		sv_player->GetOrigin()[i] = qh_pmove.origin[i] - (sv_player->GetMins()[i] - pmqh_player_mins[i]);

#if 0
	// truncate velocity the same way the net protocol will
	for (i = 0; i < 3; i++)
		sv_player->v.velocity[i] = (int)qh_pmove.velocity[i];
#else
	VectorCopy(qh_pmove.velocity, sv_player->GetVelocity());
#endif

	sv_player->SetVAngle(qh_pmove.angles);

	if (!host_client->qh_spectator)
	{
		// link into place and touch triggers
		SVQH_LinkEdict(sv_player, true);

		// touch other objects
		for (i = 0; i < qh_pmove.numtouch; i++)
		{
			n = qh_pmove.physents[qh_pmove.touchindex[i]].info;
			ent = QH_EDICT_NUM(n);
			if (!ent->GetTouch() || (playertouch[n / 8] & (1 << (n % 8))))
			{
				continue;
			}
			*pr_globalVars.self = EDICT_TO_PROG(ent);
			*pr_globalVars.other = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram(ent->GetTouch());
			playertouch[n / 8] |= 1 << (n % 8);
		}
	}
}

/*
===========
SV_PostRunCmd
===========
Done after running a player command.
*/
void SV_PostRunCmd(void)
{
	// run post-think

	if (!host_client->qh_spectator)
	{
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(*pr_globalVars.PlayerPostThink);
		SVQH_RunNewmis(realtime);
	}
	else if (SpectatorThink)
	{
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(SpectatorThink);
	}
}


/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage(client_t* cl)
{
	int c;
	char* s;
	qwusercmd_t oldest, oldcmd, newcmd;
	qwclient_frame_t* frame;
	vec3_t o;
	qboolean move_issued = false;	//only allow one move command
	int checksumIndex;
	byte checksum, calculatedChecksum;
	int seq_hash;

	// calc ping time
	frame = &cl->qw_frames[cl->netchan.incomingAcknowledged & UPDATE_MASK_QW];
	frame->ping_time = realtime - frame->senttime;

	// make sure the reply sequence number matches the incoming
	// sequence number
	if (cl->netchan.incomingSequence >= cl->netchan.outgoingSequence)
	{
		cl->netchan.outgoingSequence = cl->netchan.incomingSequence;
	}
	else
	{
		cl->qh_send_message = false;	// don't reply, sequences have slipped

	}
	// save time for ping calculations
	cl->qw_frames[cl->netchan.outgoingSequence & UPDATE_MASK_QW].senttime = realtime;
	cl->qw_frames[cl->netchan.outgoingSequence & UPDATE_MASK_QW].ping_time = -1;

	host_client = cl;
	sv_player = host_client->qh_edict;

//	seq_hash = (cl->netchan.incoming_sequence & 0xffff) ; // ^ QW_CHECK_HASH;
	seq_hash = cl->netchan.incomingSequence;

	// mark time so clients will know how much to predict
	// other players
	cl->qh_localtime = sv.qh_time;
	cl->qh_delta_sequence = -1;	// no delta unless requested
	while (1)
	{
		if (net_message.badread)
		{
			common->Printf("SV_ReadClientMessage: badread\n");
			SV_DropClient(cl);
			return;
		}

		c = net_message.ReadByte();
		if (c == -1)
		{
			break;
		}

		switch (c)
		{
		default:
			common->Printf("SV_ReadClientMessage: unknown command char\n");
			SV_DropClient(cl);
			return;

		case q1clc_nop:
			break;

		case qwclc_delta:
			cl->qh_delta_sequence = net_message.ReadByte();
			break;

		case q1clc_move:
			if (move_issued)
			{
				return;		// someone is trying to cheat...

			}
			move_issued = true;

			checksumIndex = net_message.GetReadCount();
			checksum = (byte)net_message.ReadByte();

			// read loss percentage
			cl->qw_lossage = net_message.ReadByte();

			MSGQW_ReadDeltaUsercmd(&net_message, &nullcmd, &oldest);
			MSGQW_ReadDeltaUsercmd(&net_message, &oldest, &oldcmd);
			MSGQW_ReadDeltaUsercmd(&net_message, &oldcmd, &newcmd);

			if (cl->state != CS_ACTIVE)
			{
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte(
				net_message._data + checksumIndex + 1,
				net_message.GetReadCount() - checksumIndex - 1,
				seq_hash);

			if (calculatedChecksum != checksum)
			{
				common->DPrintf("Failed command checksum for %s(%d) (%d != %d)\n",
					cl->name, cl->netchan.incomingSequence, checksum, calculatedChecksum);
				return;
			}

			if (!sv.qh_paused)
			{
				SV_PreRunCmd();

				int net_drop = cl->netchan.dropped;
				if (net_drop < 20)
				{
					while (net_drop > 2)
					{
						SV_RunCmd(&cl->qw_lastUsercmd);
						net_drop--;
					}
					if (net_drop > 1)
					{
						SV_RunCmd(&oldest);
					}
					if (net_drop > 0)
					{
						SV_RunCmd(&oldcmd);
					}
				}
				SV_RunCmd(&newcmd);

				SV_PostRunCmd();
			}

			cl->qw_lastUsercmd = newcmd;
			cl->qw_lastUsercmd.buttons = 0;// avoid multiple fires on lag
			break;


		case q1clc_stringcmd:
			s = const_cast<char*>(net_message.ReadString2());
			SV_ExecuteClientCommand(host_client, s, true, false);
			break;

		case qwclc_tmove:
			o[0] = net_message.ReadCoord();
			o[1] = net_message.ReadCoord();
			o[2] = net_message.ReadCoord();
			// only allowed by spectators
			if (host_client->qh_spectator)
			{
				VectorCopy(o, sv_player->GetOrigin());
				SVQH_LinkEdict(sv_player, false);
			}
			break;

		case qwclc_upload:
			SV_NextUpload();
			break;

		}
	}
}

/*
==============
SV_UserInit
==============
*/
void SV_UserInit(void)
{
	VQH_InitRollCvars();
	sv_spectalk = Cvar_Get("sv_spectalk", "1", 0);
	svqw_mapcheck = Cvar_Get("sv_mapcheck", "1", 0);
}
