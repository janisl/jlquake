// sv_user.c -- server code for moving users

#include "qwsvdef.h"
#include "../../common/hexen2strings.h"

qhedict_t* sv_player;

hwusercmd_t cmd;

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

	for (u = hw_ucmds; u->name; u++)
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			break;
		}

	if (!u->name)
	{
		NET_OutOfBandPrint(NS_SERVER, cl->netchan.remoteAddress, "Bad user command: %s\n", Cmd_Argv(0));
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
	svqhw_spectalk = Cvar_Get("sv_spectalk", "1", 0);
	svhw_allowtaunts = Cvar_Get("sv_allowtaunts", "1", 0);
}
