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

extern Cvar* pausable;

/*
============================================================

USER STRINGCMD EXECUTION

host_client and sv_player will be valid.
============================================================
*/

//=============================================================================

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
			NET_OutOfBandPrint(NS_SERVER, host_client->qw_snap_from, "%cServer receiving %s from %d...\n", A2C_PRINT, host_client->qw_uploadfn, host_client->qh_userid);
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
			NET_OutOfBandPrint(NS_SERVER, host_client->qw_snap_from, "%c%s upload completed.\nTo download, enter:\ndownload %s\n",
				A2C_PRINT, host_client->qw_uploadfn, p);
		}
	}

}

//=============================================================================

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
	SVQHW_DropClient(host_client);
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
//	SVQHW_FullClientUpdate (host_client, &sv.qh_reliable_datagram);
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
	{"spawn", SVQHW_Spawn_f},
	{"begin", SVQHW_Begin_f},

	{"drop", NULL, SV_Drop_f},
	{"pings", NULL, SV_Pings_f},

// issued by hand at client consoles
	{"kill", NULL, SV_Kill_f},
	{"pause", NULL, SV_Pause_f},
	{"msg", NULL, SV_Msg_f},

	{"say", SVQHW_Say_f},
	{"say_team", SVQHW_Say_Team_f},

	{"setinfo", NULL, SV_SetInfo_f},

	{"serverinfo", NULL, SV_ShowServerinfo_f},

	{"download", SVQHW_BeginDownload_f},
	{"nextdl", SVQHW_NextDownload_f},

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
	else if (qhw_SpectatorThink)
	{
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(qhw_SpectatorThink);
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
			SVQHW_DropClient(cl);
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
			SVQHW_DropClient(cl);
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
	svqhw_spectalk = Cvar_Get("sv_spectalk", "1", 0);
	svqw_mapcheck = Cvar_Get("sv_mapcheck", "1", 0);
}
