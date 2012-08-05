// sv_user.c -- server code for moving users

/*
 * $Header: /H2 Mission Pack/SV_USER.C 6     3/13/98 1:51p Mgummelt $
 */

#include "quakedef.h"

qhedict_t* sv_player = NULL;

Cvar* sv_idealrollscale;

/*
===================
SV_FlightMove: this is just the same as SVQH_WaterMove but with a few changes to make it flight

===================
*/
void SV_FlightMove(void)
{
	int i;
	vec3_t wishvel;
	float speed, newspeed, wishspeed, addspeed, accelspeed;

#ifndef DEDICATED
	cl.qh_nodrift = false;
	cl.qh_driftmove = 0;
#endif

	//
	// user intentions
	//
	vec3_t forward, right, up;
	AngleVectors(sv_player->GetVAngle(), forward, right, up);

	for (i = 0; i < 3; i++)
		wishvel[i] = forward[i] * host_client->h2_lastUsercmd.forwardmove + right[i] * host_client->h2_lastUsercmd.sidemove + up[i] * host_client->h2_lastUsercmd.upmove;

	wishspeed = VectorLength(wishvel);
	if (wishspeed > svqh_maxspeed->value)
	{
		VectorScale(wishvel, svqh_maxspeed->value / wishspeed, wishvel);
		wishspeed = svqh_maxspeed->value;
	}

//
// water friction
//
	speed = VectorLength(sv_player->GetVelocity());
	if (speed)
	{
		newspeed = speed - host_frametime * speed * svqh_friction->value;
		if (newspeed < 0)
		{
			newspeed = 0;
		}
		VectorScale(sv_player->GetVelocity(), newspeed / speed, sv_player->GetVelocity());
	}
	else
	{
		newspeed = 0;
	}

//
// water acceleration
//
	if (!wishspeed)
	{
		return;
	}

	addspeed = wishspeed - newspeed;
	if (addspeed <= 0)
	{
		return;
	}

	VectorNormalize(wishvel);
	accelspeed = svqh_accelerate->value * wishspeed * host_frametime;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
		sv_player->GetVelocity()[i] += accelspeed * wishvel[i];
}

/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void SV_ClientThink(void)
{
	vec3_t v_angle;

	if (sv_player->GetMoveType() == QHMOVETYPE_NONE)
	{
		return;
	}

	SVQH_DropPunchAngle(sv_player, host_frametime);

//
// if dead, behave differently
//
	if (sv_player->GetHealth() <= 0)
	{
		return;
	}

//
// angles
// show 1/3 the pitch angle and all the roll angle
	float* angles = sv_player->GetAngles();

	VectorAdd(sv_player->GetVAngle(), sv_player->GetPunchAngle(), v_angle);
	angles[ROLL] = VQH_CalcRoll(sv_player->GetAngles(), sv_player->GetVelocity()) * 4;
	if (!sv_player->GetFixAngle())
	{
		angles[PITCH] = -v_angle[PITCH] / 3;
		angles[YAW] = v_angle[YAW];
	}

	if ((int)sv_player->GetFlags() & QHFL_WATERJUMP)
	{
		SVQH_WaterJump(sv_player);
		return;
	}
//
// walk
//
	if ((sv_player->GetWaterLevel() >= 2) &&
		(sv_player->GetMoveType() != QHMOVETYPE_NOCLIP))
	{
		SVQH_WaterMove(host_client, host_frametime);
		return;
	}
	else if (sv_player->GetMoveType() == QHMOVETYPE_FLY)
	{
		SV_FlightMove();
		return;
	}

	SV_AirMove(host_client, host_frametime);
}


/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove(h2usercmd_t* move)
{
	int i;
	vec3_t angle;
	int bits;

// read ping time
	host_client->qh_ping_times[host_client->qh_num_pings % NUM_PING_TIMES]
		= sv.qh_time - net_message.ReadFloat();
	host_client->qh_num_pings++;

// read current angles
	for (i = 0; i < 3; i++)
		angle[i] = net_message.ReadAngle();

	host_client->qh_edict->SetVAngle(angle);

// read movement
	move->forwardmove = net_message.ReadShort();
	move->sidemove = net_message.ReadShort();
	move->upmove = net_message.ReadShort();

// read buttons
	bits = net_message.ReadByte();
	host_client->qh_edict->SetButton0(bits & 1);
	host_client->qh_edict->SetButton2((bits & 2) >> 1);

	if (bits & 4)	// crouched?
	{
		host_client->qh_edict->SetFlags2(((int)host_client->qh_edict->GetFlags2()) | FL2_CROUCHED);
	}
	else
	{
		host_client->qh_edict->SetFlags2(((int)host_client->qh_edict->GetFlags2()) & (~FL2_CROUCHED));
	}

	i = net_message.ReadByte();
	if (i)
	{
		host_client->qh_edict->SetImpulse(i);
	}

// read light level
	host_client->qh_edict->SetLightLevel(net_message.ReadByte());
}

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s);

	for (ucmd_t* u = h2_ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			return;
		}
	}

	common->DPrintf("%s tried to %s\n", host_client->name, s);
}

/*
===================
SV_ReadClientMessage

Returns false if the client should be killed
===================
*/
qboolean SV_ReadClientMessage(void)
{
	int ret;
	int cmd;
	char* s;

	do
	{
nextmsg:
		ret = NET_GetMessage(host_client->qh_netconnection, &host_client->netchan, &net_message);
		if (ret == -1)
		{
			common->Printf("SV_ReadClientMessage: NET_GetMessage failed\n");
			return false;
		}
		if (!ret)
		{
			return true;
		}

		net_message.BeginReadingOOB();

		while (1)
		{
			if (host_client->state < CS_CONNECTED)
			{
				return false;	// a command caused an error

			}
			if (net_message.badread)
			{
				common->Printf("SV_ReadClientMessage: badread\n");
				return false;
			}

			cmd = net_message.ReadChar();

			switch (cmd)
			{
			case -1:
				goto nextmsg;		// end of message

			default:
				common->Printf("SV_ReadClientMessage: unknown command char\n");
				return false;

			case h2clc_nop:
//				common->Printf ("h2clc_nop\n");
				break;

			case h2clc_stringcmd:
				s = const_cast<char*>(net_message.ReadString2());
				SV_ExecuteClientCommand(host_client, s, true, false);
				break;

			case h2clc_disconnect:
//				common->Printf ("SV_ReadClientMessage: client disconnected\n");
				return false;

			case h2clc_move:
				SV_ReadClientMove(&host_client->h2_lastUsercmd);
				break;

			case h2clc_inv_select:
				host_client->qh_edict->SetInventory(net_message.ReadByte());
				break;

			case h2clc_frame:
				host_client->h2_last_frame = net_message.ReadByte();
				host_client->h2_last_sequence = net_message.ReadByte();
				break;
			}
		}
	}
	while (ret == 1);

	return true;
}


/*
==================
SV_RunClients
==================
*/
void SV_RunClients(void)
{
	int i;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		sv_player = host_client->qh_edict;

		if (!SV_ReadClientMessage())
		{
			SVQH_DropClient(host_client, false);	// client misbehaved...
			continue;
		}

		if (host_client->state != CS_ACTIVE)
		{
			// clear client movement until a new packet is received
			Com_Memset(&host_client->h2_lastUsercmd, 0, sizeof(host_client->h2_lastUsercmd));
			continue;
		}

// always pause in single player if in console or menus
#ifdef DEDICATED
		if (!sv.qh_paused)
#else
		if (!sv.qh_paused && (svs.qh_maxclients > 1 || in_keyCatchers == 0))
#endif
		{
			SV_ClientThink();
		}
	}
}
