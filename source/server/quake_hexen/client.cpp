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
#include "../../common/hexen2strings.h"

#define MAX_FORWARD 6
#define ON_EPSILON      0.1			// point on plane side epsilon

//	Called when the player is getting totally kicked off the host
// if (crash = true), don't bother sending signofs
void SVQH_DropClient(client_t* host_client, bool crash)
{
	int saveSelf;
	int i;
	client_t* client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
		{
			host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_disconnect : q1svc_disconnect);
			NET_SendMessage(host_client->qh_netconnection, &host_client->netchan, &host_client->qh_message);
		}

		if (host_client->qh_edict && host_client->state == CS_ACTIVE)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			saveSelf = *pr_globalVars.self;
			*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
			*pr_globalVars.self = saveSelf;
		}

		common->Printf("Client %s removed\n",host_client->name);
	}

	// break the net connection
	NET_Close(host_client->qh_netconnection, &host_client->netchan);
	host_client->qh_netconnection = NULL;

	// free the client (the body stays around)
	host_client->state = CS_FREE;
	host_client->name[0] = 0;
	host_client->qh_old_frags = -999999;
	if (GGameType & GAME_Hexen2)
	{
		Com_Memset(&host_client->h2_old_v,0,sizeof(host_client->h2_old_v));
		ED_ClearEdict(host_client->qh_edict);
		host_client->h2_send_all_v = true;
	}
	net_activeconnections--;

	// send notification to all clients
	for (i = 0, client = svs.clients; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatename : q1svc_updatename);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteString2("");
		client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatefrags : q1svc_updatefrags);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteShort(0);
		client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatecolors : q1svc_updatecolors);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteByte(0);
	}
}

//	Called when the player is totally leaving the server, either willingly
// or unwillingly.  This is NOT called if the entire server is quiting
// or crashing.
void SVQHW_DropClient(client_t* drop)
{
	// add the disconnect
	drop->netchan.message.WriteByte(GGameType & GAME_HexenWorld ? h2svc_disconnect : q1svc_disconnect);

	if (drop->state == CS_ACTIVE)
	{
		if (!drop->qh_spectator)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
		}
		else if (qhw_SpectatorDisconnect)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(qhw_SpectatorDisconnect);
		}
	}
	else if (GGameType & GAME_HexenWorld && hw_dmMode->value == HWDM_SIEGE)
	{
		if (String::ICmp(PR_GetString(drop->qh_edict->GetPuzzleInv1()),""))
		{
			//this guy has a puzzle piece, call this function anyway
			//to make sure he leaves it behind
			common->Printf("Client in unspawned state had puzzle piece, forcing drop\n");
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
		}
	}

	if (drop->qh_spectator)
	{
		common->Printf("Spectator %s removed\n",drop->name);
	}
	else
	{
		common->Printf("Client %s removed\n",drop->name);
	}

	if (drop->download)
	{
		FS_FCloseFile(drop->download);
		drop->download = 0;
	}
	if (drop->qw_upload)
	{
		FS_FCloseFile(drop->qw_upload);
		drop->qw_upload = 0;
	}
	*drop->qw_uploadfn = 0;

	drop->state = CS_ZOMBIE;		// become free in a few seconds
	drop->qh_connection_started = svs.realtime * 0.001;	// for zombie timeout

	drop->qh_old_frags = 0;
	drop->qh_edict->SetFrags(0);
	drop->name[0] = 0;
	Com_Memset(drop->userinfo, 0, sizeof(drop->userinfo));

	// send notification to all remaining clients
	SVQHW_FullClientUpdate(drop, &sv.qh_reliable_datagram);
}

void SVQH_SetIdealPitch(qhedict_t* player)
{
	if (!((int)player->GetFlags() & QHFL_ONGROUND))
	{
		return;
	}

	if (GGameType & GAME_Hexen2 && player->GetMoveType() == QHMOVETYPE_FLY)
	{
		return;
	}

	float angleval = player->GetAngles()[YAW] * M_PI * 2 / 360;
	float sinval = sin(angleval);
	float cosval = cos(angleval);

	float save_hull;
	if (GGameType & GAME_Hexen2)
	{
		save_hull = player->GetHull();
		player->SetHull(0);
	}

	float z[MAX_FORWARD];
	int i;
	for (i = 0; i < MAX_FORWARD; i++)
	{
		vec3_t top;
		top[0] = player->GetOrigin()[0] + cosval * (i + 3) * 12;
		top[1] = player->GetOrigin()[1] + sinval * (i + 3) * 12;
		top[2] = player->GetOrigin()[2] + player->GetViewOfs()[2];

		vec3_t bottom;
		bottom[0] = top[0];
		bottom[1] = top[1];
		bottom[2] = top[2] - 160;

		q1trace_t tr = SVQH_Move(top, vec3_origin, vec3_origin, bottom, 1, player);
		if (tr.allsolid ||		// looking at a wall, leave ideal the way is was
			tr.fraction == 1)	// near a dropoff
		{
			if (GGameType & GAME_Hexen2)
			{
				player->SetHull(save_hull);
			}
			return;
		}
		z[i] = top[2] + tr.fraction * (bottom[2] - top[2]);
	}

	if (GGameType & GAME_Hexen2)
	{
		player->SetHull(save_hull);	//restore
	}

	int dir = 0;
	int steps = 0;
	for (int j = 1; j < i; j++)
	{
		int step = z[j] - z[j - 1];
		if (step > -ON_EPSILON && step < ON_EPSILON)
		{
			continue;
		}

		if (dir && (step - dir > ON_EPSILON || step - dir < -ON_EPSILON))
		{
			return;		// mixed changes

		}
		steps++;
		dir = step;
	}

	if (!dir)
	{
		player->SetIdealPitch(0);
		return;
	}

	if (steps < 2)
	{
		return;
	}

	player->SetIdealPitch(-dir * svqh_idealpitchscale->value);
}

void SVQ1_PreSpawn_f(client_t* client)
{
	if (client->state == CS_ACTIVE)
	{
		common->Printf("prespawn not valid -- allready spawned\n");
		return;
	}

	client->qh_message.WriteData(sv.qh_signon._data, sv.qh_signon.cursize);
	client->qh_message.WriteByte(q1svc_signonnum);
	client->qh_message.WriteByte(2);
	client->qh_sendsignon = true;
}

void SVQH_Spawn_f(client_t* host_client)
{
	if (host_client->state == CS_ACTIVE)
	{
		common->Printf("Spawn not valid -- allready spawned\n");
		return;
	}

	if (GGameType & GAME_Hexen2)
	{
		// send all current names, colors, and frag counts
		host_client->qh_message.Clear();
	}

	// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited allready
		// if this is the last client to be connected, unpause
		sv.qh_paused = false;
	}
	else
	{
		// set up the edict
		qhedict_t* ent = host_client->qh_edict;
		if (GGameType & GAME_Hexen2)
		{
			sv.qh_paused = false;
		}

		if (!(GGameType & GAME_Hexen2) || !ent->GetStatsRestored() || svqh_deathmatch->value)
		{
			Com_Memset(&ent->v, 0, progs->entityfields * 4);
			if (!(GGameType & GAME_Hexen2))
			{
				ent->SetColorMap(QH_NUM_FOR_EDICT(ent));
			}
			ent->SetTeam((host_client->qh_colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			if (GGameType & GAME_Hexen2)
			{
				ent->SetPlayerClass(host_client->h2_playerclass);
			}

			// copy spawn parms out of the client_t

			for (int i = 0; i < NUM_SPAWN_PARMS; i++)
			{
				pr_globalVars.parm1[i] = host_client->qh_spawn_parms[i];
			}

			// call the spawn function

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(ent);
			PR_ExecuteProgram(*pr_globalVars.ClientConnect);

			if ((Sys_DoubleTime() - host_client->qh_netconnection->connecttime) <= sv.qh_time)
			{
				common->Printf("%s entered the game\n", host_client->name);
			}

			PR_ExecuteProgram(*pr_globalVars.PutClientInServer);
		}
	}

	if (!(GGameType & GAME_Hexen2))
	{
		// send all current names, colors, and frag counts
		host_client->qh_message.Clear();
	}

	// send time of update
	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_time : q1svc_time);
	host_client->qh_message.WriteFloat(sv.qh_time);

	client_t* client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, client++)
	{
		host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatename : q1svc_updatename);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteString2(client->name);

		if (GGameType & GAME_Hexen2)
		{
			host_client->qh_message.WriteByte(h2svc_updateclass);
			host_client->qh_message.WriteByte(i);
			host_client->qh_message.WriteByte(client->h2_playerclass);
		}

		host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatefrags : q1svc_updatefrags);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteShort(client->qh_old_frags);

		host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatecolors : q1svc_updatecolors);
		host_client->qh_message.WriteByte(i);
		host_client->qh_message.WriteByte(client->qh_colors);
	}

	// send all current light styles
	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_lightstyle : q1svc_lightstyle);
		host_client->qh_message.WriteByte((char)i);
		host_client->qh_message.WriteString2(sv.qh_lightstyles[i]);
	}

	//
	// send some stats
	//
	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatestat : q1svc_updatestat);
	host_client->qh_message.WriteByte(Q1STAT_TOTALSECRETS);
	host_client->qh_message.WriteLong(*pr_globalVars.total_secrets);

	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatestat : q1svc_updatestat);
	host_client->qh_message.WriteByte(Q1STAT_TOTALMONSTERS);
	host_client->qh_message.WriteLong(*pr_globalVars.total_monsters);

	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatestat : q1svc_updatestat);
	host_client->qh_message.WriteByte(Q1STAT_SECRETS);
	host_client->qh_message.WriteLong(*pr_globalVars.found_secrets);

	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatestat : q1svc_updatestat);
	host_client->qh_message.WriteByte(Q1STAT_MONSTERS);
	host_client->qh_message.WriteLong(*pr_globalVars.killed_monsters);

	if (GGameType & GAME_Hexen2)
	{
		SVH2_UpdateEffects(&host_client->qh_message);
	}

	//
	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	qhedict_t* ent = QH_EDICT_NUM(1 + (host_client - svs.clients));
	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_setangle : q1svc_setangle);
	for (int i = 0; i < 2; i++)
	{
		host_client->qh_message.WriteAngle(ent->GetAngles()[i]);
	}
	host_client->qh_message.WriteAngle(0);

	SVQH_WriteClientdataToMessage(host_client, &host_client->qh_message);

	host_client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_signonnum : q1svc_signonnum);
	host_client->qh_message.WriteByte(3);
	host_client->qh_sendsignon = true;
}

void SVQH_Begin_f(client_t* client)
{
	client->state = CS_ACTIVE;
}

//	Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each server load.
void SVQHW_New_f(client_t* client)
{
	if (client->state == CS_ACTIVE)
	{
		return;
	}

	client->state = CS_CONNECTED;
	client->qh_connection_started = svs.realtime * 0.001;

	if (GGameType & GAME_HexenWorld)
	{
		// send the info about the new client to all connected clients
		client->qh_sendinfo = true;
	}

	const char* gamedir = Info_ValueForKey(svs.qh_info, "*gamedir");
	if (!gamedir[0])
	{
		gamedir = GGameType & GAME_HexenWorld ? "hw" : "qw";
	}

	//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
	//spawns.  These functions are written to not overflow
	if (client->qw_num_backbuf)
	{
		common->Printf("WARNING %s: [SV_New] Back buffered (%d0, clearing", client->name, client->netchan.message.cursize);
		client->qw_num_backbuf = 0;
		client->netchan.message.Clear();
	}

	// send the serverdata
	client->netchan.message.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_serverdata : qwsvc_serverdata);
	client->netchan.message.WriteLong(GGameType & GAME_HexenWorld ? HWPROTOCOL_VERSION : QWPROTOCOL_VERSION);
	client->netchan.message.WriteLong(svs.spawncount);
	client->netchan.message.WriteString2(gamedir);

	int playernum = QH_NUM_FOR_EDICT(client->qh_edict) - 1;
	if (client->qh_spectator)
	{
		playernum |= 128;
	}
	client->netchan.message.WriteByte(playernum);

	// send full levelname
	if (GGameType & GAME_HexenWorld)
	{
		if (sv.qh_edicts->GetMessage() > 0 && sv.qh_edicts->GetMessage() <= prh2_string_count)
		{
			client->netchan.message.WriteString2(&prh2_global_strings[prh2_string_index[(int)sv.qh_edicts->GetMessage() - 1]]);
		}
		else
		{
			//Use netname on map if there is one, so they don't have to edit strings.txt
			client->netchan.message.WriteString2(PR_GetString(sv.qh_edicts->GetNetName()));
		}
	}
	else
	{
		client->netchan.message.WriteString2(PR_GetString(sv.qh_edicts->GetMessage()));
	}

	// send the movevars
	client->netchan.message.WriteFloat(movevars.gravity);
	client->netchan.message.WriteFloat(movevars.stopspeed);
	client->netchan.message.WriteFloat(movevars.maxspeed);
	client->netchan.message.WriteFloat(movevars.spectatormaxspeed);
	client->netchan.message.WriteFloat(movevars.accelerate);
	client->netchan.message.WriteFloat(movevars.airaccelerate);
	client->netchan.message.WriteFloat(movevars.wateraccelerate);
	client->netchan.message.WriteFloat(movevars.friction);
	client->netchan.message.WriteFloat(movevars.waterfriction);
	client->netchan.message.WriteFloat(movevars.entgravity);

	// send music
	if (GGameType & GAME_HexenWorld)
	{
		client->netchan.message.WriteByte(h2svc_cdtrack);
		client->netchan.message.WriteByte(sv.qh_edicts->GetSoundType());

		client->netchan.message.WriteByte(hwsvc_midi_name);
		client->netchan.message.WriteString2(sv.h2_midi_name);
	}
	else
	{
		client->netchan.message.WriteByte(q1svc_cdtrack);
		client->netchan.message.WriteByte(sv.qh_edicts->GetSounds());
	}

	// send server info string
	client->netchan.message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_stufftext : q1svc_stufftext);
	client->netchan.message.WriteString2(va("fullserverinfo \"%s\"\n", svs.qh_info));
}

void SVQW_Modellist_f(client_t* client)
{
	if (client->state != CS_CONNECTED)
	{
		common->Printf("modellist not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVQW_Modellist_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	int n = String::Atoi(Cmd_Argv(2));

	//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
	//spawns.  These functions are written to not overflow
	if (client->qw_num_backbuf)
	{
		common->Printf("WARNING %s: [SV_Modellist] Back buffered (%d0, clearing", client->name, client->netchan.message.cursize);
		client->qw_num_backbuf = 0;
		client->netchan.message.Clear();
	}

	client->netchan.message.WriteByte(qwsvc_modellist);
	client->netchan.message.WriteByte(n);
	const char** s;
	for (s = sv.qh_model_precache + 1 + n;
		*s && client->netchan.message.cursize < (MAX_MSGLEN_QW / 2);
		s++, n++)
	{
		client->netchan.message.WriteString2(*s);
	}
	client->netchan.message.WriteByte(0);

	// next msg
	if (*s)
	{
		client->netchan.message.WriteByte(n);
	}
	else
	{
		client->netchan.message.WriteByte(0);
	}
}

void SVHW_Modellist_f(client_t* client)
{
	if (client->state != CS_CONNECTED)
	{
		common->Printf("modellist not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVHW_Modellist_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	client->netchan.message.WriteByte(hwsvc_modellist);
	for (const char** s = sv.qh_model_precache + 1; *s; s++)
	{
		client->netchan.message.WriteString2(*s);
	}
	client->netchan.message.WriteByte(0);
}

void SVQW_Soundlist_f(client_t* client)
{
	if (client->state != CS_CONNECTED)
	{
		common->Printf("soundlist not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVQW_Soundlist_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	int n = String::Atoi(Cmd_Argv(2));

	//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
	//spawns.  These functions are written to not overflow
	if (client->qw_num_backbuf)
	{
		common->Printf("WARNING %s: [SV_Soundlist] Back buffered (%d0, clearing", client->name, client->netchan.message.cursize);
		client->qw_num_backbuf = 0;
		client->netchan.message.Clear();
	}

	client->netchan.message.WriteByte(qwsvc_soundlist);
	client->netchan.message.WriteByte(n);
	const char** s;
	for (s = sv.qh_sound_precache + 1 + n;
		 *s && client->netchan.message.cursize < (MAX_MSGLEN_QW / 2);
		 s++, n++)
	{
		client->netchan.message.WriteString2(*s);
	}

	client->netchan.message.WriteByte(0);

	// next msg
	if (*s)
	{
		client->netchan.message.WriteByte(n);
	}
	else
	{
		client->netchan.message.WriteByte(0);
	}
}

void SVHW_Soundlist_f(client_t* client)
{
	if (client->state != CS_CONNECTED)
	{
		common->Printf("soundlist not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVHW_Soundlist_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	client->netchan.message.WriteByte(hwsvc_soundlist);
	for (const char** s = sv.qh_sound_precache + 1; *s; s++)
	{
		client->netchan.message.WriteString2(*s);
	}
	client->netchan.message.WriteByte(0);
}

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
				SVQHW_DropClient(client);
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

void SVQHW_Spawn_f(client_t* host_client)
{
	int i;

	if (host_client->state != CS_CONNECTED)
	{
		common->Printf("Spawn not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVQHW_Spawn_f from different level\n");
		SVQHW_New_f(host_client);
		return;
	}

	if (GGameType & GAME_QuakeWorld)
	{
		int n = String::Atoi(Cmd_Argv(2));

		// make sure n is valid
		if (n < 0 || n > MAX_CLIENTS_QHW)
		{
			common->Printf("SVQHW_Spawn_f invalid client start\n");
			SVQHW_New_f(host_client);
			return;
		}

		// send all current names, colors, and frag counts
		// FIXME: is this a good thing?
		host_client->netchan.message.Clear();

		// send current status of all other players

		// normally this could overflow, but no need to check due to backbuf
		client_t* client = svs.clients + n;
		for (i = n; i < MAX_CLIENTS_QHW; i++, client++)
		{
			SVQHW_FullClientUpdateToClient(client, host_client);
		}

		// send all current light styles
		for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
		{
			SVQH_ClientReliableWrite_Begin(host_client, q1svc_lightstyle,
				3 + (sv.qh_lightstyles[i] ? String::Length(sv.qh_lightstyles[i]) : 1));
			SVQH_ClientReliableWrite_Byte(host_client, (char)i);
			SVQH_ClientReliableWrite_String(host_client, sv.qh_lightstyles[i]);
		}
	}

	// set up the edict
	qhedict_t* ent = host_client->qh_edict;

	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->SetColorMap(QH_NUM_FOR_EDICT(ent));
	if (GGameType & GAME_HexenWorld && hw_dmMode->value == HWDM_SIEGE)
	{
		ent->SetTeam(ent->GetSiegeTeam());	// FIXME
	}
	else
	{
		ent->SetTeam(0);	// FIXME
	}
	ent->SetNetName(PR_SetString(host_client->name));
	if (GGameType & GAME_HexenWorld)
	{
		ent->SetNextPlayerClass(host_client->hw_next_playerclass);
		ent->SetHasPortals(host_client->hw_portals);
	}

	host_client->qh_entgravity = 1.0;
	eval_t* val = GetEdictFieldValue(ent, "gravity");
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

	if (GGameType & GAME_HexenWorld)
	{
		// send all current names, colors, and frag counts
		// FIXME: is this a good thing?
		host_client->netchan.message.Clear();

		// send current status of all other players

		client_t* client = svs.clients;
		for (i = 0; i < MAX_CLIENTS_QHW; i++, client++)
		{
			SVQHW_FullClientUpdate(client, &host_client->netchan.message);
		}

		// send all current light styles
		for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
		{
			host_client->netchan.message.WriteByte(h2svc_lightstyle);
			host_client->netchan.message.WriteByte((char)i);
			host_client->netchan.message.WriteString2(sv.qh_lightstyles[i]);
		}
	}

	//
	// force stats to be updated
	//
	Com_Memset(host_client->qh_stats, 0, sizeof(host_client->qh_stats));

	SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_updatestatlong : qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_TOTALSECRETS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.total_secrets);

	SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_updatestatlong : qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_TOTALMONSTERS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.total_monsters);

	SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_updatestatlong : qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_SECRETS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.found_secrets);

	SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_updatestatlong : qwsvc_updatestatlong, 6);
	SVQH_ClientReliableWrite_Byte(host_client, Q1STAT_MONSTERS);
	SVQH_ClientReliableWrite_Long(host_client, *pr_globalVars.killed_monsters);

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	SVQH_SendClientCommand(host_client, "skins\n");
}

static void SVQHW_SpawnSpectator(qhedict_t* player)
{
	VectorCopy(vec3_origin, player->GetOrigin());
	player->SetViewOfs(vec3_origin);
	player->GetViewOfs()[2] = 22;

	// search for an info_playerstart to spawn the spectator at
	for (int i = MAX_CLIENTS_QHW - 1; i < sv.qh_num_edicts; i++)
	{
		qhedict_t* e = QH_EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "info_player_start"))
		{
			VectorCopy(e->GetOrigin(), player->GetOrigin());
			return;
		}
	}

}

void SVQHW_Begin_f(client_t* client)
{
	if (client->state == CS_ACTIVE)
	{
		return;	// don't begin again
	}
	client->state = CS_ACTIVE;

	// handle the case of a level changing while a client was connecting
	if (String::Atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		common->Printf("SVQHW_Begin_f from different level\n");
		SVQHW_New_f(client);
		return;
	}

	if (client->qh_spectator)
	{
		SVQHW_SpawnSpectator(client->qh_edict);

		if (qhw_SpectatorConnect)
		{
			// copy spawn parms out of the client_t
			for (int i = 0; i < NUM_SPAWN_PARMS; i++)
			{
				pr_globalVars.parm1[i] = client->qh_spawn_parms[i];
			}

			// call the spawn function
			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(client->qh_edict);
			PR_ExecuteProgram(qhw_SpectatorConnect);
		}
	}
	else
	{
		// copy spawn parms out of the client_t
		for (int i = 0; i < NUM_SPAWN_PARMS; i++)
		{
			pr_globalVars.parm1[i] = client->qh_spawn_parms[i];
		}

		if (GGameType & GAME_HexenWorld)
		{
			client->h2_send_all_v = true;
		}

		// call the spawn function
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.ClientConnect);

		// actually spawn the player
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.PutClientInServer);
	}

	// clear the net statistics, because connecting gives a bogus picture
	client->netchan.dropCount = 0;

	if (GGameType & GAME_QuakeWorld)
	{
		//check he's not cheating

		unsigned pmodel = String::Atoi(Info_ValueForKey(client->userinfo, "pmodel"));
		unsigned emodel = String::Atoi(Info_ValueForKey(client->userinfo, "emodel"));

		if (pmodel != sv.qw_model_player_checksum ||
			emodel != sv.qw_eyes_player_checksum)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s WARNING: non standard player/eyes model detected\n", client->name);
		}

		// if we are paused, tell the client
		if (sv.qh_paused)
		{
			SVQH_ClientReliableWrite_Begin(client, q1svc_setpause, 2);
			SVQH_ClientReliableWrite_Byte(client, sv.qh_paused);
			SVQH_ClientPrintf(client, PRINT_HIGH, "Server is paused.\n");
		}
	}
}

void SVQHW_NextDownload_f(client_t* client)
{
	if (!client->download)
	{
		return;
	}

	int r = client->downloadSize - client->downloadCount;
	if (r > (GGameType & GAME_HexenWorld ? 1024 : 768))
	{
		r = (GGameType & GAME_HexenWorld ? 1024 : 768);
	}
	byte buffer[1024];
	r = FS_Read(buffer, r, client->download);
	SVQH_ClientReliableWrite_Begin(client, GGameType & GAME_HexenWorld ? hwsvc_download : qwsvc_download, 6 + r);
	SVQH_ClientReliableWrite_Short(client, r);

	client->downloadCount += r;
	int size = client->downloadSize;
	if (!size)
	{
		size = 1;
	}
	int percent = client->downloadCount * 100 / size;
	SVQH_ClientReliableWrite_Byte(client, percent);
	SVQH_ClientReliableWrite_SZ(client, buffer, r);

	if (client->downloadCount != client->downloadSize)
	{
		return;
	}

	FS_FCloseFile(client->download);
	client->download = 0;

}

void SVQHW_BeginDownload_f(client_t* client)
{
	char* name = Cmd_Argv(1);
	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (strstr(name, "..") || !qhw_allow_download->value
		// leading dot is no good
		|| *name == '.'
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| (String::NCmp(name, "skins/", 6) == 0 && !qhw_allow_download_skins->value)
		// now models
		|| (String::NCmp(name, "progs/", 6) == 0 && !qhw_allow_download_models->value)
		// now sounds
		|| (String::NCmp(name, "sound/", 6) == 0 && !qhw_allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (String::NCmp(name, "maps/", 6) == 0 && !qhw_allow_download_maps->value)
		// MUST be in a subdirectory
		|| !strstr(name, "/"))
	{	// don't allow anything with .. path
		SVQH_ClientReliableWrite_Begin(client, GGameType & GAME_HexenWorld ? hwsvc_download : qwsvc_download, 4);
		SVQH_ClientReliableWrite_Short(client, -1);
		SVQH_ClientReliableWrite_Byte(client, 0);
		return;
	}

	if (client->download)
	{
		FS_FCloseFile(client->download);
		client->download = 0;
	}

	// lowercase name (needed for casesen file systems)
	{
		char* p;

		for (p = name; *p; p++)
			*p = (char)String::ToLower(*p);
	}


	client->downloadSize = FS_FOpenFileRead(name, &client->download, true);
	client->downloadCount = 0;

	if (!client->download
		// special check for maps, if it came from a pak file, don't allow
		// download  ZOID
		|| (String::NCmp(name, "maps/", 5) == 0 && FS_FileIsInPAK(name, NULL) == 1))
	{
		if (client->download)
		{
			FS_FCloseFile(client->download);
			client->download = 0;
		}

		common->Printf("Couldn't download %s to %s\n", name, client->name);
		SVQH_ClientReliableWrite_Begin(client, GGameType & GAME_HexenWorld ? hwsvc_download : qwsvc_download, 4);
		SVQH_ClientReliableWrite_Short(client, -1);
		SVQH_ClientReliableWrite_Byte(client, 0);
		return;
	}

	SVQHW_NextDownload_f(client);
	common->Printf("Downloading %s to %s\n", name, client->name);
}
