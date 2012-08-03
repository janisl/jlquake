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
