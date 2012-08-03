// sv_user.c -- server code for moving users

#include "qwsvdef.h"
#include "../../common/hexen2strings.h"

qhedict_t* sv_player;

hwusercmd_t cmd;

Cvar* sv_spectalk;
Cvar* sv_allowtaunts;

extern int fp_messages, fp_persecond, fp_secondsdead;
extern char fp_msg[];

/*
============================================================

USER STRINGCMD EXECUTION

host_client and sv_player will be valid.
============================================================
*/

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
	int speaknum = -1;
	qboolean IsRaven = false;

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
		if (realtime < host_client->qh_lockedtill)
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
		if (host_client->qh_whensaid[tmp] && (realtime - host_client->qh_whensaid[tmp] < fp_persecond))
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

	if (host_client->netchan.remoteAddress.ip[0] == 208 &&
		host_client->netchan.remoteAddress.ip[1] == 135 &&
		host_client->netchan.remoteAddress.ip[2] == 137)
	{
		IsRaven = true;
	}

	if (p[0] == '`' && ((!host_client->qh_spectator && sv_allowtaunts->value) || IsRaven))
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
				if (hw_dmMode->value == HWDM_SIEGE)
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
			if (hw_dmMode->value == HWDM_SIEGE && host_client->hw_siege_team != client->hw_siege_team)	//other team speaking
			{
				SVQH_ClientPrintf(client, PRINT_CHAT, "%s", text);//fixme: print biege
			}
			else
			{
				SVQH_ClientPrintf(client, PRINT_CHAT, "%s", text);
			}
		}
		else
		{
			SVQH_ClientPrintf(client, PRINT_SOUND + speaknum - 1, "%s", text);
		}
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

		host_client->netchan.message.WriteByte(hwsvc_updateping);
		host_client->netchan.message.WriteByte(j);
		host_client->netchan.message.WriteShort(SVQH_CalcPing(client));
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

	if (Cmd_Argc() != 2)
	{
		// turn off tracking
		host_client->qh_spec_track = 0;
		return;
	}

	i = String::Atoi(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CLIENTS_QHW || svs.clients[i].state != CS_ACTIVE ||
		svs.clients[i].qh_spectator)
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "Invalid client to track\n");
		host_client->qh_spec_track = 0;
		return;
	}
	host_client->qh_spec_track = i + 1;// now tracking
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
	Info_SetValueForKey(host_client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value, false);
	String::NCpy(host_client->name, Info_ValueForKey(host_client->userinfo, "name"),
		sizeof(host_client->name) - 1);
//	SVQHW_FullClientUpdate (host_client, &sv.qh_reliable_datagram);
	host_client->qh_sendinfo = true;

	// process any changed values
	SV_ExtractFromUserinfo(host_client);
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

typedef struct
{
	const char* name;
	void (* func)(client_t*);
	void (* old_func)(void);
} ucmd_t;

ucmd_t ucmds[] =
{
	{"new", SVQHW_New_f},
	{"modellist", SVHW_Modellist_f},
	{"soundlist", SVHW_Soundlist_f},
	{"prespawn", SVQHW_PreSpawn_f},
	{"spawn", SVQHW_Spawn_f},
	{"begin", SVQHW_Begin_f},

	{"drop", NULL, SV_Drop_f},
	{"pings", NULL, SV_Pings_f},

// issued by hand at client consoles
	{"kill", NULL, SV_Kill_f},
	{"msg", NULL, SV_Msg_f},

	{"say", NULL, SV_Say_f},
	{"say_team", NULL, SV_Say_Team_f},

	{"setinfo", NULL, SV_SetInfo_f},

	{"serverinfo", NULL, SV_ShowServerinfo_f},

	{"download", SVQHW_BeginDownload_f},
	{"nextdl", SVQHW_NextDownload_f},

	{"ptrack", NULL, SV_PTrack_f},//ZOID - used with autocam

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
			VectorCopy(check->GetAngles(), pe->angles);
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
			VectorCopy(check->GetAngles(), pe->angles);
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
void SV_RunCmd(hwusercmd_t* ucmd)
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

	if (ucmd->buttons & 4 || sv_player->GetPlayerClass() == CLASS_DWARF)// crouched?
	{
		sv_player->SetFlags2(((int)sv_player->GetFlags2()) | FL2_CROUCHED);
	}
	else
	{
		sv_player->SetFlags2(((int)sv_player->GetFlags2()) & (~FL2_CROUCHED));
	}

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
	if (host_frametime > HX_FRAME_TIME)
	{
		host_frametime = HX_FRAME_TIME;
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
//	qh_pmove.waterjumptime = sv_player->v.teleport_time;
	qh_pmove.numphysent = 1;
	qh_pmove.physents[0].model = 0;
	qh_pmove.cmd.Set(*ucmd);
	qh_pmove.dead = sv_player->GetHealth() <= 0;
	qh_pmove.oldbuttons = host_client->qh_oldbuttons;
	qh_pmove.hasted = sv_player->GetHasted();
	qh_pmove.movetype = sv_player->GetMoveType();
	qh_pmove.crouched = (sv_player->GetHull() == HULL_CROUCH);
	qh_pmove.teleport_time = (sv_player->GetTeleportTime() - sv.qh_time);

//	movevars.entgravity = host_client->entgravity;
	movevars.entgravity = sv_player->GetGravity();
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
//	sv_player->v.teleport_time = qh_pmove.waterjumptime;
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
	sv_player->SetVelocity(qh_pmove.velocity);
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
//Why not just do an SVQH_Impact here?
//			SVQH_Impact(sv_player,ent);
			if (sv_player->GetTouch())
			{
				*pr_globalVars.self = EDICT_TO_PROG(sv_player);
				*pr_globalVars.other = EDICT_TO_PROG(ent);
				PR_ExecuteProgram(sv_player->GetTouch());
			}
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
	hwusercmd_t oldest, oldcmd, newcmd;
	hwclient_frame_t* frame;
	vec3_t o;

	// calc ping time
	frame = &cl->hw_frames[cl->netchan.incomingAcknowledged & UPDATE_MASK_HW];
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
	cl->hw_frames[cl->netchan.outgoingSequence & UPDATE_MASK_HW].senttime = realtime;
	cl->hw_frames[cl->netchan.outgoingSequence & UPDATE_MASK_HW].ping_time = -1;

	host_client = cl;
	sv_player = host_client->qh_edict;

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

		case h2clc_nop:
			break;

		case hwclc_delta:
			cl->qh_delta_sequence = net_message.ReadByte();
			break;

		case h2clc_move:
		{
			MSGHW_ReadUsercmd(&net_message, &oldest, false);
			MSGHW_ReadUsercmd(&net_message, &oldcmd, false);
			MSGHW_ReadUsercmd(&net_message, &newcmd, true);

			if (cl->state != CS_ACTIVE)
			{
				break;
			}

			SV_PreRunCmd();

			int net_drop = cl->netchan.dropped;
			if (net_drop < 20)
			{
				while (net_drop > 2)
				{
					SV_RunCmd(&cl->hw_lastUsercmd);
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

			cl->hw_lastUsercmd = newcmd;
			cl->hw_lastUsercmd.buttons = 0;// avoid multiple fires on lag
		}
		break;


		case h2clc_stringcmd:
			s = const_cast<char*>(net_message.ReadString2());
			SV_ExecuteClientCommand(host_client, s, true, false);
			break;

		case hwclc_tmove:
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

		case hwclc_inv_select:
			cl->qh_edict->SetInventory(net_message.ReadByte());
			break;

		case hwclc_get_effect:
			c = net_message.ReadByte();
			if (sv.h2_Effects[c].type)
			{
				common->Printf("Getting effect %d\n",(int)c);
				SVHW_SendEffect(&host_client->netchan.message, c);

			}
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
	sv_allowtaunts = Cvar_Get("sv_allowtaunts", "1", 0);
}
