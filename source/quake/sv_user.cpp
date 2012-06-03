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

extern Cvar* sv_friction;
Cvar* sv_edgefriction;
extern Cvar* sv_stopspeed;

static vec3_t forward, right, up;

vec3_t wishdir;
float wishspeed;

// world
float* angles;
float* origin;
float* velocity;

qboolean onground;

q1usercmd_t cmd;

Cvar* sv_idealpitchscale;


/*
===============
SV_SetIdealPitch
===============
*/
#define MAX_FORWARD 6
void SV_SetIdealPitch(void)
{
	float angleval, sinval, cosval;
	q1trace_t tr;
	vec3_t top, bottom;
	float z[MAX_FORWARD];
	int i, j;
	int step, dir, steps;

	if (!((int)sv_player->GetFlags() & FL_ONGROUND))
	{
		return;
	}

	angleval = sv_player->GetAngles()[YAW] * M_PI * 2 / 360;
	sinval = sin(angleval);
	cosval = cos(angleval);

	for (i = 0; i < MAX_FORWARD; i++)
	{
		top[0] = sv_player->GetOrigin()[0] + cosval * (i + 3) * 12;
		top[1] = sv_player->GetOrigin()[1] + sinval * (i + 3) * 12;
		top[2] = sv_player->GetOrigin()[2] + sv_player->GetViewOfs()[2];

		bottom[0] = top[0];
		bottom[1] = top[1];
		bottom[2] = top[2] - 160;

		tr = SV_Move(top, vec3_origin, vec3_origin, bottom, 1, sv_player);
		if (tr.allsolid)
		{
			return;	// looking at a wall, leave ideal the way is was

		}
		if (tr.fraction == 1)
		{
			return;	// near a dropoff

		}
		z[i] = top[2] + tr.fraction * (bottom[2] - top[2]);
	}

	dir = 0;
	steps = 0;
	for (j = 1; j < i; j++)
	{
		step = z[j] - z[j - 1];
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
		sv_player->SetIdealPitch(0);
		return;
	}

	if (steps < 2)
	{
		return;
	}
	sv_player->SetIdealPitch(-dir * sv_idealpitchscale->value);
}


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

	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, sv_player);

	if (trace.fraction == 1.0)
	{
		friction = sv_friction->value * sv_edgefriction->value;
	}
	else
	{
		friction = sv_friction->value;
	}

// apply friction
	control = speed < sv_stopspeed->value ? sv_stopspeed->value : speed;
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
Cvar* sv_maxspeed;
Cvar* sv_accelerate;
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

	accelspeed = sv_accelerate.value * host_frametime * addspeed;
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
	accelspeed = sv_accelerate->value * host_frametime * wishspeed;
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
//	accelspeed = sv_accelerate.value * host_frametime;
	accelspeed = sv_accelerate->value * wishspeed * host_frametime;
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
	if (wishspeed > sv_maxspeed->value)
	{
		VectorScale(wishvel, sv_maxspeed->value / wishspeed, wishvel);
		wishspeed = sv_maxspeed->value;
	}
	wishspeed *= 0.7;

//
// water friction
//
	speed = VectorLength(velocity);
	if (speed)
	{
		newspeed = speed - host_frametime * speed * sv_friction->value;
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
	accelspeed = sv_accelerate->value * wishspeed * host_frametime;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
		velocity[i] += accelspeed * wishvel[i];
}

void SV_WaterJump(void)
{
	if (sv.time > sv_player->GetTeleportTime() ||
		!sv_player->GetWaterLevel())
	{
		sv_player->SetFlags((int)sv_player->GetFlags() & ~FL_WATERJUMP);
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
	if (sv.time < sv_player->GetTeleportTime() && fmove < 0)
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
	if (wishspeed > sv_maxspeed->value)
	{
		VectorScale(wishvel, sv_maxspeed->value / wishspeed, wishvel);
		wishspeed = sv_maxspeed->value;
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

	onground = (int)sv_player->GetFlags() & FL_ONGROUND;

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

	if ((int)sv_player->GetFlags() & FL_WATERJUMP)
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
		= sv.time - net_message.ReadFloat();
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
		ret = NET_GetMessage(host_client->qh_netconnection, &host_client->netchan);
		if (ret == -1)
		{
			Con_Printf("SV_ReadClientMessage: NET_GetMessage failed\n");
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
				Con_Printf("SV_ReadClientMessage: badread\n");
				return false;
			}

			cmd = net_message.ReadChar();

			switch (cmd)
			{
			case -1:
				goto nextmsg;		// end of message

			default:
				Con_Printf("SV_ReadClientMessage: unknown command char\n");
				return false;

			case q1clc_nop:
//				Con_Printf ("q1clc_nop\n");
				break;

			case q1clc_stringcmd:
				s = net_message.ReadString2();
				if (host_client->qh_privileged)
				{
					ret = 2;
				}
				else
				{
					ret = 0;
				}
				if (String::NICmp(s, "status", 6) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "god", 3) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "notarget", 8) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "fly", 3) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "name", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "noclip", 6) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "say", 3) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "say_team", 8) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "tell", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "color", 5) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "kill", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "pause", 5) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "spawn", 5) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "begin", 5) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "prespawn", 8) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "kick", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "ping", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "give", 4) == 0)
				{
					ret = 1;
				}
				else if (String::NICmp(s, "ban", 3) == 0)
				{
					ret = 1;
				}
				if (ret == 2)
				{
					Cbuf_InsertText(s);
				}
				else if (ret == 1)
				{
					Cmd_ExecuteString(s, src_client);
				}
				else
				{
					Con_DPrintf("%s tried to %s\n", host_client->name, s);
				}
				break;

			case q1clc_disconnect:
//				Con_Printf ("SV_ReadClientMessage: client disconnected\n");
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

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		sv_player = host_client->qh_edict;

		if (!SV_ReadClientMessage())
		{
			SV_DropClient(false);	// client misbehaved...
			continue;
		}

		if (host_client->state != CS_ACTIVE)
		{
			// clear client movement until a new packet is received
			Com_Memset(&host_client->q1_lastUsercmd, 0, sizeof(host_client->q1_lastUsercmd));
			continue;
		}

// always pause in single player if in console or menus
		if (!sv.paused && (svs.maxclients > 1 || in_keyCatchers == 0))
		{
			SV_ClientThink();
		}
	}
}
