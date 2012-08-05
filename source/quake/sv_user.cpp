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

#include "quakedef.h"

qhedict_t* sv_player;

Cvar* sv_edgefriction;

static vec3_t forward, right, up;

vec3_t wishdir;
float wishspeed;

// world
float* angles;
float* origin;
float* velocity;

qboolean onground;

q1usercmd_t cmd;


/*
==================
SV_UserFriction

==================
*/
void SV_UserFriction(void)
{
	float* vel;
	float speed, newspeed, control;
	vec3_t start, stop;
	float friction;
	q1trace_t trace;

	vel = velocity;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	if (!speed)
	{
		return;
	}

// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = origin[0] + vel[0] / speed * 16;
	start[1] = stop[1] = origin[1] + vel[1] / speed * 16;
	start[2] = origin[2] + sv_player->GetMins()[2];
	stop[2] = start[2] - 34;

	trace = SVQH_Move(start, vec3_origin, vec3_origin, stop, true, sv_player);

	if (trace.fraction == 1.0)
	{
		friction = svqh_friction->value * sv_edgefriction->value;
	}
	else
	{
		friction = svqh_friction->value;
	}

// apply friction
	control = speed < svqh_stopspeed->value ? svqh_stopspeed->value : speed;
	newspeed = speed - host_frametime * control * friction;

	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
SV_Accelerate
==============
*/
#if 0
void SV_Accelerate(vec3_t wishvel)
{
	int i;
	float addspeed, accelspeed;
	vec3_t pushvec;

	if (wishspeed == 0)
	{
		return;
	}

	VectorSubtract(wishvel, velocity, pushvec);
	addspeed = VectorNormalize(pushvec);

	accelspeed = svqh_accelerate.value * host_frametime * addspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
		velocity[i] += accelspeed * pushvec[i];
}
#endif
void SV_Accelerate(void)
{
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	accelspeed = svqh_accelerate->value * host_frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
		velocity[i] += accelspeed * wishdir[i];
}

void SV_AirAccelerate(vec3_t wishveloc)
{
	int i;
	float addspeed, wishspd, accelspeed, currentspeed;

	wishspd = VectorNormalize(wishveloc);
	if (wishspd > 30)
	{
		wishspd = 30;
	}
	currentspeed = DotProduct(velocity, wishveloc);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
//	accelspeed = svqh_accelerate.value * host_frametime;
	accelspeed = svqh_accelerate->value * wishspeed * host_frametime;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
		velocity[i] += accelspeed * wishveloc[i];
}


void DropPunchAngle(void)
{
	float len;

	len = VectorNormalize(sv_player->GetPunchAngle());

	len -= 10 * host_frametime;
	if (len < 0)
	{
		len = 0;
	}
	VectorScale(sv_player->GetPunchAngle(), len, sv_player->GetPunchAngle());
}

/*
===================
SV_WaterMove

===================
*/
void SV_WaterMove(void)
{
	int i;
	vec3_t wishvel;
	float speed, newspeed, wishspeed, addspeed, accelspeed;

//
// user intentions
//
	AngleVectors(sv_player->GetVAngle(), forward, right, up);

	for (i = 0; i < 3; i++)
		wishvel[i] = forward[i] * cmd.forwardmove + right[i] * cmd.sidemove;

	if (!cmd.forwardmove && !cmd.sidemove && !cmd.upmove)
	{
		wishvel[2] -= 60;		// drift towards bottom
	}
	else
	{
		wishvel[2] += cmd.upmove;
	}

	wishspeed = VectorLength(wishvel);
	if (wishspeed > svqh_maxspeed->value)
	{
		VectorScale(wishvel, svqh_maxspeed->value / wishspeed, wishvel);
		wishspeed = svqh_maxspeed->value;
	}
	wishspeed *= 0.7;

//
// water friction
//
	speed = VectorLength(velocity);
	if (speed)
	{
		newspeed = speed - host_frametime * speed * svqh_friction->value;
		if (newspeed < 0)
		{
			newspeed = 0;
		}
		VectorScale(velocity, newspeed / speed, velocity);
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
		velocity[i] += accelspeed * wishvel[i];
}

void SV_WaterJump(void)
{
	if (sv.qh_time > sv_player->GetTeleportTime() ||
		!sv_player->GetWaterLevel())
	{
		sv_player->SetFlags((int)sv_player->GetFlags() & ~QHFL_WATERJUMP);
		sv_player->SetTeleportTime(0);
	}
	sv_player->GetVelocity()[0] = sv_player->GetMoveDir()[0];
	sv_player->GetVelocity()[1] = sv_player->GetMoveDir()[1];
}


/*
===================
SV_AirMove

===================
*/
void SV_AirMove(void)
{
	int i;
	vec3_t wishvel;
	float fmove, smove;

	AngleVectors(sv_player->GetAngles(), forward, right, up);

	fmove = cmd.forwardmove;
	smove = cmd.sidemove;

// hack to not let you back into teleporter
	if (sv.qh_time < sv_player->GetTeleportTime() && fmove < 0)
	{
		fmove = 0;
	}

	for (i = 0; i < 3; i++)
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	if ((int)sv_player->GetMoveType() != QHMOVETYPE_WALK)
	{
		wishvel[2] = cmd.upmove;
	}
	else
	{
		wishvel[2] = 0;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	if (wishspeed > svqh_maxspeed->value)
	{
		VectorScale(wishvel, svqh_maxspeed->value / wishspeed, wishvel);
		wishspeed = svqh_maxspeed->value;
	}

	if (sv_player->GetMoveType() == QHMOVETYPE_NOCLIP)
	{	// noclip
		VectorCopy(wishvel, velocity);
	}
	else if (onground)
	{
		SV_UserFriction();
		SV_Accelerate();
	}
	else
	{	// not on ground, so little effect on velocity
		SV_AirAccelerate(wishvel);
	}
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

	onground = (int)sv_player->GetFlags() & QHFL_ONGROUND;

	origin = sv_player->GetOrigin();
	velocity = sv_player->GetVelocity();

	DropPunchAngle();

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
	cmd = host_client->q1_lastUsercmd;
	angles = sv_player->GetAngles();

	VectorAdd(sv_player->GetVAngle(), sv_player->GetPunchAngle(), v_angle);
	angles[ROLL] = VQH_CalcRoll(sv_player->GetAngles(), sv_player->GetVelocity()) * 4;
	if (!sv_player->GetFixAngle())
	{
		angles[PITCH] = -v_angle[PITCH] / 3;
		angles[YAW] = v_angle[YAW];
	}

	if ((int)sv_player->GetFlags() & QHFL_WATERJUMP)
	{
		SV_WaterJump();
		return;
	}
//
// walk
//
	if ((sv_player->GetWaterLevel() >= 2) &&
		(sv_player->GetMoveType() != QHMOVETYPE_NOCLIP))
	{
		SV_WaterMove();
		return;
	}

	SV_AirMove();
}


/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove(q1usercmd_t* move)
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

	i = net_message.ReadByte();
	if (i)
	{
		host_client->qh_edict->SetImpulse(i);
	}
}

struct ucmd_t
{
	const char* name;
	void (*func)(client_t*);
};

ucmd_t ucmds[] =
{
	{ "prespawn", SVQ1_PreSpawn_f },
	{ "spawn", SVQH_Spawn_f },
	{ "begin", SVQH_Begin_f },
	{ "say", SVQH_Say_f },
	{ "say_team", SVQH_Say_Team_f },
	{ "tell", SVQH_Tell_f },
	{ "god", SVQH_God_f },
	{ "notarget", SVQH_Notarget_f },
	{ "noclip", SVQH_Noclip_f },
	{ "fly", SVQ1_Fly_f },
	{ "give", SVQ1_Give_f },
	{ "name", SVQH_Name_f },
	{ "color", SVQH_Color_f },
	{ "kill", SVQH_Kill_f },
	{ "pause", SVQH_Pause_f },
	{ "status", SVQH_Status_f },
	{ "kick", SVQH_Kick_f },
	{ "ping", SVQH_Ping_f },
	{ "ban", SVQH_Ban_f },
	{ NULL, NULL }
};

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s);

	for (ucmd_t* u = ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			return;
		}
	}

	common->DPrintf("%s tried to %s\n", cl->name, s);
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
	const char* s;

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

			case q1clc_nop:
//				common->Printf ("q1clc_nop\n");
				break;

			case q1clc_stringcmd:
				s = net_message.ReadString2();
				SV_ExecuteClientCommand(host_client, s, true, false);
				break;

			case q1clc_disconnect:
//				common->Printf ("SV_ReadClientMessage: client disconnected\n");
				return false;

			case q1clc_move:
				SV_ReadClientMove(&host_client->q1_lastUsercmd);
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
			Com_Memset(&host_client->q1_lastUsercmd, 0, sizeof(host_client->q1_lastUsercmd));
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
