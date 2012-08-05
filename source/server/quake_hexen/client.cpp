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

static void SVQH_Say(client_t* host_client, bool teamonly)
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
	String::Sprintf(text, sizeof(text), "%c%s: ", 1, host_client->name);

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
		if (svqh_teamplay->value && teamonly && client->qh_edict->GetTeam() != host_client->qh_edict->GetTeam())
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
	}
}

void SVQH_Say_f(client_t* host_client)
{
	SVQH_Say(host_client, false);
}

void SVQH_Say_Team_f(client_t* host_client)
{
	SVQH_Say(host_client, true);
}

void SVQH_Tell_f(client_t* host_client)
{
	if (Cmd_Argc() < 3)
	{
		return;
	}

	char text[64];
	String::Cpy(text, host_client->name);
	String::Cat(text, sizeof(text), ": ");

	char* p = Cmd_ArgsUnmodified();

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	// check length & truncate if necessary
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
		if (String::ICmp(client->name, Cmd_Argv(1)))
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
		break;
	}
}

static void SVQHW_Say(client_t* host_client, bool team)
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

	if (host_client->qh_spectator && (!svqhw_spectalk->value || team))
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

	if (qhw_fp_messages)
	{
		if ((!sv.qh_paused || GGameType & GAME_HexenWorld) && svs.realtime * 0.001 < host_client->qh_lockedtill)
		{
			SVQH_ClientPrintf(host_client, PRINT_CHAT,
				"You can't talk for %d more seconds\n",
				(int)(host_client->qh_lockedtill - svs.realtime * 0.001));
			return;
		}
		tmp = host_client->qh_whensaidhead - qhw_fp_messages + 1;
		if (tmp < 0)
		{
			tmp = 10 + tmp;
		}
		if ((!sv.qh_paused || GGameType & GAME_HexenWorld) &&
			host_client->qh_whensaid[tmp] && (svs.realtime * 0.001 - host_client->qh_whensaid[tmp] < qhw_fp_persecond))
		{
			host_client->qh_lockedtill = svs.realtime * 0.001 + qhw_fp_secondsdead;
			if (qhw_fp_msg[0])
			{
				SVQH_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: %s\n", qhw_fp_msg);
			}
			else
			{
				SVQH_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: You can't talk for %d seconds.\n", qhw_fp_secondsdead);
			}
			return;
		}
		host_client->qh_whensaidhead++;
		if (host_client->qh_whensaidhead > 9)
		{
			host_client->qh_whensaidhead = 0;
		}
		host_client->qh_whensaid[host_client->qh_whensaidhead] = svs.realtime * 0.001;
	}

	p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	int speaknum = -1;
	if (GGameType & GAME_HexenWorld && p[0] == '`' && !host_client->qh_spectator && svhw_allowtaunts->value)
	{
		speaknum = atol(&p[1]);
		if (speaknum <= 0 || speaknum > 255 - PRINT_SOUND)
		{
			speaknum = -1;
		}
		else
		{
			text[String::Length(text) - 2] = 0;
			String::Cat(text, sizeof(text)," speaks!\n");
		}
	}

	if (speaknum == -1)
	{
		String::Cat(text, sizeof(text), p);
		String::Cat(text, sizeof(text), "\n");
	}

	common->Printf("%s", text);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		if (host_client->qh_spectator && !svqhw_spectalk->value)
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
				if (GGameType & GAME_HexenWorld && hw_dmMode->value == HWDM_SIEGE)
				{
					if ((host_client->qh_edict->GetSkin() == 102 && client->qh_edict->GetSkin() != 102) || (client->qh_edict->GetSkin() == 102 && host_client->qh_edict->GetSkin() != 102))
					{
						continue;	//noteam players can team chat with each other, cannot recieve team chat of other players

					}
					if (client->hw_siege_team != host_client->hw_siege_team)
					{
						continue;	// on different teams
					}
				}
				else if (String::Cmp(t1, t2) || client->qh_spectator)
				{
					continue;	// on different teams
				}
			}
		}
		if (speaknum == -1)
		{
			SVQH_ClientPrintf(client, PRINT_CHAT, "%s", text);
		}
		else
		{
			SVQH_ClientPrintf(client, PRINT_SOUND + speaknum - 1, "%s", text);
		}
	}
}

void SVQHW_Say_f(client_t* host_client)
{
	SVQHW_Say(host_client, false);
}

void SVQHW_Say_Team_f(client_t* host_client)
{
	SVQHW_Say(host_client, true);
}

//	Sets client to godmode
void SVQH_God_f(client_t* client)
{
	if (*pr_globalVars.deathmatch ||
		(GGameType & GAME_Hexen2 && (*pr_globalVars.coop || qh_skill->value > 2)))
	{
		return;
	}

	client->qh_edict->SetFlags((int)client->qh_edict->GetFlags() ^ QHFL_GODMODE);
	if (!((int)client->qh_edict->GetFlags() & QHFL_GODMODE))
	{
		SVQH_ClientPrintf(client, 0, "godmode OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(client, 0, "godmode ON\n");
	}
}

void SVQH_Notarget_f(client_t* client)
{
	if (*pr_globalVars.deathmatch || (GGameType & GAME_Hexen2 && qh_skill->value > 2))
	{
		return;
	}

	client->qh_edict->SetFlags((int)client->qh_edict->GetFlags() ^ QHFL_NOTARGET);
	if (!((int)client->qh_edict->GetFlags() & QHFL_NOTARGET))
	{
		SVQH_ClientPrintf(client, 0, "notarget OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(client, 0, "notarget ON\n");
	}
}

void SVQH_Noclip_f(client_t* client)
{
	if (*pr_globalVars.deathmatch ||
		(GGameType & GAME_Hexen2 && (*pr_globalVars.coop || qh_skill->value > 2)))
	{
		return;
	}

	if (client->qh_edict->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_NOCLIP);
		SVQH_ClientPrintf(client, 0, "noclip ON\n");
	}
	else
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(client, 0, "noclip OFF\n");
	}
}

//	Sets client to flymode
void SVQ1_Fly_f(client_t* client)
{
	if (*pr_globalVars.deathmatch)
	{
		return;
	}

	if (client->qh_edict->GetMoveType() != QHMOVETYPE_FLY)
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_FLY);
		SVQH_ClientPrintf(client, 0, "flymode ON\n");
	}
	else
	{
		client->qh_edict->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(client, 0, "flymode OFF\n");
	}
}

void SVQ1_Give_f(client_t* host_client)
{
	if (*pr_globalVars.deathmatch)
	{
		return;
	}

	const char* t = Cmd_Argv(1);
	int v = String::Atoi(Cmd_Argv(2));
	qhedict_t* sv_player = host_client->qh_edict;

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
		if (q1_hipnotic)
		{
			if (t[0] == '6')
			{
				if (t[1] == 'a')
				{
					sv_player->SetItems((int)sv_player->GetItems() | Q1HIT_PROXIMITY_GUN);
				}
				else
				{
					sv_player->SetItems((int)sv_player->GetItems() | Q1IT_GRENADE_LAUNCHER);
				}
			}
			else if (t[0] == '9')
			{
				sv_player->SetItems((int)sv_player->GetItems() | Q1HIT_LASER_CANNON);
			}
			else if (t[0] == '0')
			{
				sv_player->SetItems((int)sv_player->GetItems() | Q1HIT_MJOLNIR);
			}
			else if (t[0] >= '2')
			{
				sv_player->SetItems((int)sv_player->GetItems() | (Q1IT_SHOTGUN << (t[0] - '2')));
			}
		}
		else
		{
			if (t[0] >= '2')
			{
				sv_player->SetItems((int)sv_player->GetItems() | (Q1IT_SHOTGUN << (t[0] - '2')));
			}
		}
		break;

	case 's':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_shells1");
			if (val)
			{
				val->_float = v;
			}
		}

		sv_player->SetAmmoShells(v);
		break;
	case 'n':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_nails1");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() <= Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoNails(v);
				}
			}
		}
		else
		{
			sv_player->SetAmmoNails(v);
		}
		break;
	case 'l':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_lava_nails");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() > Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoNails(v);
				}
			}
		}
		break;
	case 'r':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_rockets1");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() <= Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoRockets(v);
				}
			}
		}
		else
		{
			sv_player->SetAmmoRockets(v);
		}
		break;
	case 'm':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_multi_rockets");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() > Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoRockets(v);
				}
			}
		}
		break;
	case 'h':
		sv_player->SetHealth(v);
		break;
	case 'c':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_cells1");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() <= Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoCells(v);
				}
			}
		}
		else
		{
			sv_player->SetAmmoCells(v);
		}
		break;
	case 'p':
		if (q1_rogue)
		{
			eval_t* val = GetEdictFieldValue(sv_player, "ammo_plasma");
			if (val)
			{
				val->_float = v;
				if (sv_player->GetWeapon() > Q1IT_LIGHTNING)
				{
					sv_player->SetAmmoCells(v);
				}
			}
		}
		break;
	}
}

void SVH2_Give_f(client_t* host_client)
{
	if (*pr_globalVars.deathmatch || qh_skill->value > 2)
	{
		return;
	}

	char* t = Cmd_Argv(1);
	int v = String::Atoi(Cmd_Argv(2));

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
		if (t[0] >= '2')
		{
			host_client->qh_edict->SetItems((int)host_client->qh_edict->GetItems() | (H2IT_WEAPON2 << (t[0] - '2')));
		}
		break;

	case 'h':
		host_client->qh_edict->SetHealth(v);
		break;
	}
}

void SVQH_Name_f(client_t* client)
{
	if (Cmd_Argc() == 1)
	{
		return;
	}
	char* newName;
	if (Cmd_Argc() == 2)
	{
		newName = Cmd_Argv(1);
	}
	else
	{
		newName = Cmd_ArgsUnmodified();
	}
	newName[15] = 0;

	if (GGameType & GAME_Hexen2)
	{
		//this is for the fuckers who put braces in the name causing loadgame to crash.
		char* pdest = strchr(newName,'{');
		if (pdest)
		{
			*pdest = 0;	//zap the brace
			common->Printf("Illegal char in name removed!\n");
		}
	}

	if (client->name[0] && String::Cmp(client->name, "unconnected"))
	{
		if (String::Cmp(client->name, newName) != 0)
		{
			common->Printf("%s renamed to %s\n", client->name, newName);
		}
	}
	String::Cpy(client->name, newName);
	client->qh_edict->SetNetName(PR_SetString(client->name));

	// send notification to all clients
	sv.qh_reliable_datagram.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatename : q1svc_updatename);
	sv.qh_reliable_datagram.WriteByte(client - svs.clients);
	sv.qh_reliable_datagram.WriteString2(client->name);
}

void SVH2_Class_f(client_t* client)
{
	if (Cmd_Argc() == 1)
	{
		return;
	}
	float newClass;
	if (Cmd_Argc() == 2)
	{
		newClass = String::Atof(Cmd_Argv(1));
	}
	else
	{
		newClass = String::Atof(Cmd_ArgsUnmodified());
	}

	if (sv.loadgame || client->h2_playerclass)
	{
		if (client->qh_edict->GetPlayerClass())
		{
			newClass = client->qh_edict->GetPlayerClass();
		}
		else if (client->h2_playerclass)
		{
			newClass = client->h2_playerclass;
		}
	}

	client->h2_playerclass = newClass;
	client->qh_edict->SetPlayerClass(newClass);

	// Change the weapon model used
	*pr_globalVars.self = EDICT_TO_PROG(client->qh_edict);
	PR_ExecuteProgram(*pr_globalVars.ClassChangeWeapon);

	// send notification to all clients
	sv.qh_reliable_datagram.WriteByte(h2svc_updateclass);
	sv.qh_reliable_datagram.WriteByte(client - svs.clients);
	sv.qh_reliable_datagram.WriteByte((byte)newClass);
}

void SVQH_Color_f(client_t* client)
{
	if (Cmd_Argc() == 1)
	{
		return;
	}

	int top, bottom;
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

	int playercolor = top * 16 + bottom;

	client->qh_colors = playercolor;
	client->qh_edict->SetTeam(bottom + 1);

	// send notification to all clients
	sv.qh_reliable_datagram.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatecolors : q1svc_updatecolors);
	sv.qh_reliable_datagram.WriteByte(client - svs.clients);
	sv.qh_reliable_datagram.WriteByte(client->qh_colors);
}

//	Change the message level for a client
void SVQHW_Msg_f(client_t* client)
{
	if (Cmd_Argc() != 2)
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "Current msg level is %i\n",
			client->messagelevel);
		return;
	}

	client->messagelevel = String::Atoi(Cmd_Argv(1));

	SVQH_ClientPrintf(client, PRINT_HIGH, "Msg level set to %i\n", client->messagelevel);
}

//	Allow clients to change userinfo
void SVQHW_SetInfo_f(client_t* client)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("User info settings:\n");
		Info_Print(client->userinfo);
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
	if (GGameType & GAME_HexenWorld)
	{
		Info_SetValueForKey(client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value, false);
		String::NCpy(client->name, Info_ValueForKey(client->userinfo, "name"),
			sizeof(client->name) - 1);
		client->qh_sendinfo = true;
	}
	else
	{
		char oldval[MAX_INFO_STRING_QW];
		String::Cpy(oldval, Info_ValueForKey(client->userinfo, Cmd_Argv(1)));

		Info_SetValueForKey(client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value, false);

		if (!String::Cmp(Info_ValueForKey(client->userinfo, Cmd_Argv(1)), oldval))
		{
			return;	// key hasn't changed

		}
	}

	// process any changed values
	SVQHW_ExtractFromUserinfo(client);

	if (GGameType & GAME_QuakeWorld)
	{
		int i = client - svs.clients;
		sv.qh_reliable_datagram.WriteByte(qwsvc_setinfo);
		sv.qh_reliable_datagram.WriteByte(i);
		sv.qh_reliable_datagram.WriteString2(Cmd_Argv(1));
		sv.qh_reliable_datagram.WriteString2(Info_ValueForKey(client->userinfo, Cmd_Argv(1)));
	}
}

void SVQH_Kill_f(client_t* client)
{
	if (client->qh_edict->GetHealth() <= 0)
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "Can't suicide -- allready dead!\n");
		return;
	}

	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(client->qh_edict);
	PR_ExecuteProgram(*pr_globalVars.ClientKill);
}

void SVQH_TogglePause(const char* msg)
{
	sv.qh_paused ^= 1;

	if (msg)
	{
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s", msg);
	}

	// send notification to all clients
	if (GGameType & GAME_QuakeWorld)
	{
		client_t* cl = svs.clients;
		for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
		{
			if (!cl->state)
			{
				continue;
			}
			SVQH_ClientReliableWrite_Begin(cl, q1svc_setpause, 2);
			SVQH_ClientReliableWrite_Byte(cl, sv.qh_paused);
		}
	}
	else
	{
		sv.qh_reliable_datagram.WriteByte(GGameType & GAME_Hexen2 ? h2svc_setpause : q1svc_setpause);
		sv.qh_reliable_datagram.WriteByte(sv.qh_paused);
	}
}

void SVQH_Pause_f(client_t* client)
{
	if (!qh_pausable->value)
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "Pause not allowed.\n");
		return;
	}

	char st[sizeof(client->name) + 32];
	if (GGameType & GAME_QuakeWorld)
	{
		if (client->qh_spectator)
		{
			SVQH_ClientPrintf(client, PRINT_HIGH, "Spectators can not pause.\n");
			return;
		}

		if (sv.qh_paused)
		{
			sprintf(st, "%s paused the game\n", client->name);
		}
		else
		{
			sprintf(st, "%s unpaused the game\n", client->name);
		}
	}
	else
	{
		if (sv.qh_paused)
		{
			sprintf(st, "%s paused the game\n", PR_GetString(client->qh_edict->GetNetName()));
		}
		else
		{
			sprintf(st, "%s unpaused the game\n", PR_GetString(client->qh_edict->GetNetName()));
		}
	}

	SVQH_TogglePause(st);
}

void SVQH_Status_f(client_t* host_client)
{
	client_t* client;
	int seconds;
	int minutes;
	int hours = 0;
	int j;

	SVQH_ClientPrintf(host_client, 0, "host:    %s\n", Cvar_VariableString("hostname"));
	SVQH_ClientPrintf(host_client, 0, "version: " JLQUAKE_VERSION_STRING "\n");
	SVQH_ClientPrintf(host_client, 0, "map:     %s\n", sv.name);
	SVQH_ClientPrintf(host_client, 0, "players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
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
		SVQH_ClientPrintf(host_client, 0, "#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
		SVQH_ClientPrintf(host_client, 0, "   %s\n", client->qh_netconnection->address);
	}
}

//	Kicks a user off of the server
void SVQH_Kick_f(client_t* kicker)
{
	if (*pr_globalVars.deathmatch)
	{
		return;
	}

	int i;
	bool byNumber = false;
	client_t* kicked;
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
		kicked = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, kicked = svs.clients; i < svs.qh_maxclients; i++, kicked++)
		{
			if (kicked->state < CS_CONNECTED)
			{
				continue;
			}
			if (String::ICmp(kicked->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.qh_maxclients)
	{
		const char* who = kicker->name;

		// can't kick yourself!
		if (kicked == kicker)
		{
			return;
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
					message++;
				message += String::Length(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
		{
			SVQH_ClientPrintf(kicked, 0, "Kicked by %s: %s\n", who, message);
		}
		else
		{
			SVQH_ClientPrintf(kicked, 0, "Kicked by %s\n", who);
		}
		SVQH_DropClient(kicked, false);
	}
}

void SVQH_Ping_f(client_t* host_client)
{
	SVQH_ClientPrintf(host_client, 0, "Client ping times:\n");
	client_t* client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		float total = 0;
		for (int j = 0; j < NUM_PING_TIMES; j++)
		{
			total += client->qh_ping_times[j];
		}
		total /= NUM_PING_TIMES;
		SVQH_ClientPrintf(host_client, 0, "%4i %s\n", (int)(total * 1000), client->name);
	}
}

void SVQH_Ban_f(client_t* client)
{
	NET_Ban_f();
}

//	The client is showing the scoreboard, so send new ping times for all clients
void SVQHW_Pings_f(client_t* host_client)
{
	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_updateping : qwsvc_updateping, 4);
		SVQH_ClientReliableWrite_Byte(host_client, j);
		SVQH_ClientReliableWrite_Short(host_client, SVQH_CalcPing(client));
		if (GGameType & GAME_QuakeWorld)
		{
			SVQH_ClientReliableWrite_Begin(host_client, qwsvc_updatepl, 4);
			SVQH_ClientReliableWrite_Byte(host_client, j);
			SVQH_ClientReliableWrite_Byte(host_client, client->qw_lossage);
		}
	}
}

//	The client is going to disconnect, so remove the connection immediately
void SVQHW_Drop_f(client_t* client)
{
	if (!client->qh_spectator)
	{
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s dropped\n", client->name);
	}
	SVQHW_DropClient(client);
}

void SVQHW_PTrack_f(client_t* client)
{
	if (!client->qh_spectator)
	{
		return;
	}

	if (Cmd_Argc() != 2)
	{
		// turn off tracking
		client->qh_spec_track = 0;
		if (GGameType & GAME_QuakeWorld)
		{
			qhedict_t* ent = QH_EDICT_NUM(client - svs.clients + 1);
			qhedict_t* tent = QH_EDICT_NUM(0);
			ent->SetGoalEntity(EDICT_TO_PROG(tent));
		}
		return;
	}

	int i = String::Atoi(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CLIENTS_QHW || svs.clients[i].state != CS_ACTIVE ||
		svs.clients[i].qh_spectator)
	{
		SVQH_ClientPrintf(client, PRINT_HIGH, "Invalid client to track\n");
		client->qh_spec_track = 0;
		if (GGameType & GAME_QuakeWorld)
		{
			qhedict_t* ent = QH_EDICT_NUM(client - svs.clients + 1);
			qhedict_t* tent = QH_EDICT_NUM(0);
			ent->SetGoalEntity(EDICT_TO_PROG(tent));
		}
		return;
	}
	client->qh_spec_track = i + 1;// now tracking

	if (GGameType & GAME_QuakeWorld)
	{
		qhedict_t* ent = QH_EDICT_NUM(client - svs.clients + 1);
		qhedict_t* tent = QH_EDICT_NUM(i + 1);
		ent->SetGoalEntity(EDICT_TO_PROG(tent));
	}
}

//	Dumps the serverinfo info string
void SVQHW_ShowServerinfo_f(client_t* client)
{
	char outputbuf[8000];
	outputbuf[0] = 0;

	const char* s = svs.qh_info;
	if (*s == '\\')
	{
		s++;
	}
	while (*s)
	{
		char key[512];
		char* o = key;
		while (*s && *s != '\\')
		{
			*o++ = *s++;
		}

		int l = o - key;
		if (l < 20)
		{
			Com_Memset(o, ' ', 20 - l);
			key[20] = 0;
		}
		else
		{
			*o = 0;
		}
		String::Cat(outputbuf, sizeof(outputbuf), key);

		if (!*s)
		{
			String::Cat(outputbuf, sizeof(outputbuf), "MISSING VALUE\n");
			break;
		}

		char value[512];
		o = value;
		s++;
		while (*s && *s != '\\')
		{
			*o++ = *s++;
		}
		*o = 0;

		if (*s)
		{
			s++;
		}
		String::Cat(outputbuf, sizeof(outputbuf), value);
		String::Cat(outputbuf, sizeof(outputbuf), "\n");
	}
	NET_OutOfBandPrint(NS_SERVER, client->netchan.remoteAddress, "%s", outputbuf);
}

void SVQW_NoSnap_f(client_t* client)
{
	if (*client->qw_uploadfn)
	{
		*client->qw_uploadfn = 0;
		SVQH_BroadcastPrintf(PRINT_HIGH, "%s refused remote screenshot\n", client->name);
	}
}
