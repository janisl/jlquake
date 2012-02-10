//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "core.h"

struct qhpml_t
{
	float frametime;

	vec3_t forward;
	vec3_t right;
	vec3_t up;
};

movevars_t movevars;
qhplayermove_t qh_pmove;

vec3_t pmqh_player_mins;
vec3_t pmqh_player_maxs;

vec3_t pmqh_player_maxs_crouch;

static qhpml_t qh_pml;

void PMQH_Init()
{
	if (GGameType & GAME_Quake)
	{
		VectorSet(pmqh_player_mins, -16, -16, -24);
		VectorSet(pmqh_player_maxs, 16, 16, 32);
	}
	else
	{
		VectorSet(pmqh_player_mins, -16, -16, 0);
		VectorSet(pmqh_player_maxs, 16, 16, 56);
		VectorSet(pmqh_player_maxs_crouch, 16, 16, 28);
	}
}

q1trace_t PMQH_TestPlayerMove(const vec3_t start, const vec3_t end)
{
	// fill in a default trace
	q1trace_t total;
	Com_Memset(&total, 0, sizeof(q1trace_t));
	total.fraction = 1;
	total.entityNum = -1;
	VectorCopy(end, total.endpos);

	for (int i = 0; i < qh_pmove.numphysent; i++)
	{
		qhphysent_t* pe = &qh_pmove.physents[i];
		// get the clipping hull
		clipHandle_t hull;
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (pe->model >= 0)
		{
			if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 3, clip_mins, clip_maxs);
			}
			else
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 1, clip_mins, clip_maxs);
			}
		}
		else
		{
			vec3_t mins, maxs;
			if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
			{
				VectorSubtract (pe->mins, pmqh_player_maxs_crouch, mins);
			}
			else
			{
				VectorSubtract(pe->mins, pmqh_player_maxs, mins);
			}
			VectorSubtract(pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
			clip_mins[2] = 0;
		}

		vec3_t offset;
		VectorCopy(pe->origin, offset);

		vec3_t start_l, end_l;
		VectorSubtract(start, offset, start_l);
		VectorSubtract(end, offset, end_l);

		if (GGameType & GAME_Hexen2)
		{
			if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1))
			{
				vec3_t forward, right, up;
				AngleVectors(pe->angles, forward, right, up);

				vec3_t temp;
				VectorCopy(start_l, temp);
				start_l[0] = DotProduct(temp, forward);
				start_l[1] = -DotProduct(temp, right);
				start_l[2] = DotProduct(temp, up);

				VectorCopy(end_l, temp);
				end_l[0] = DotProduct(temp, forward);
				end_l[1] = -DotProduct(temp, right);
				end_l[2] = DotProduct(temp, up);
			}

			// rjr will need to adjust for player when going into different hulls
			start_l[2] -= clip_mins[2];
			end_l[2] -= clip_mins[2];
		}

		// fill in a default trace
		q1trace_t trace;
		Com_Memset(&trace, 0, sizeof(q1trace_t));
		trace.fraction = 1;
		trace.allsolid = true;
		VectorCopy(end, trace.endpos);
		if (GGameType & GAME_Hexen2)
		{
			// rjr will need to adjust for player when going into different hulls
			trace.endpos[2] -= clip_mins[2];
		}

		// trace a line through the apropriate clipping hull
		CM_HullCheckQ1(hull, start_l, end_l, &trace);

		if (GGameType & GAME_Hexen2)
		{
			// rjr will need to adjust for player when going into different hulls
			trace.endpos[2] += clip_mins[2];

			if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1))
			{
				if (trace.fraction != 1)
				{
					vec3_t a;
					VectorSubtract(vec3_origin, pe->angles, a);
					vec3_t forward, right, up;
					AngleVectors(a, forward, right, up);

					vec3_t temp;
					VectorCopy(trace.endpos, temp);
					trace.endpos[0] = DotProduct(temp, forward);
					trace.endpos[1] = -DotProduct(temp, right);
					trace.endpos[2] = DotProduct(temp, up);

					VectorCopy(trace.plane.normal, temp);
					trace.plane.normal[0] = DotProduct(temp, forward);
					trace.plane.normal[1] = -DotProduct(temp, right);
					trace.plane.normal[2] = DotProduct(temp, up);
				}
			}
		}

		if (trace.allsolid)
		{
			trace.startsolid = true;
		}
		if (trace.startsolid)
		{
			trace.fraction = 0;
		}

		// did we clip the move?
		if (trace.fraction < total.fraction)
		{
			// fix trace up by the offset
			VectorAdd(trace.endpos, offset, trace.endpos);
			total = trace;
			total.entityNum = i;
		}
	}

	return total;
}

//	Returns false if the given player position is not valid (in solid)
bool PMQH_TestPlayerPosition(const vec3_t pos)
{
	if (GGameType & GAME_Hexen2)
	{
		q1trace_t trace = PMQH_TestPlayerMove(pos, pos);
		return !(trace.allsolid || trace.startsolid);
	}

	for (int i = 0; i < qh_pmove.numphysent; i++)
	{
		qhphysent_t* pe = &qh_pmove.physents[i];
		// get the clipping hull
		clipHandle_t hull;
		if (pe->model >= 0)
		{
			hull = CM_ModelHull(qh_pmove.physents[i].model, 1);
		}
		else
		{
			vec3_t mins, maxs;
			VectorSubtract(pe->mins, pmqh_player_maxs, mins);
			VectorSubtract(pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
		}

		vec3_t test;
		VectorSubtract(pos, pe->origin, test);

		if (CM_PointContentsQ1(test, hull) == BSP29CONTENTS_SOLID)
		{
			return false;
		}
	}

	return true;
}

//	The basic solid body movement clip that slides along multiple planes
static int PMQH_FlyMove()
{
	int numbumps = 4;
	
	int blocked = 0;
	vec3_t primal_velocity, original_velocity;
	VectorCopy(qh_pmove.velocity, original_velocity);
	VectorCopy(qh_pmove.velocity, primal_velocity);

	vec3_t planes[MAX_FLY_MOVE_CLIP_PLANES];
	int numplanes = 0;

	float time_left = qh_pml.frametime;

	for (int bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		vec3_t end;
		for (int i = 0; i < 3; i++)
		{
			end[i] = qh_pmove.origin[i] + time_left * qh_pmove.velocity[i];
		}

		q1trace_t trace = PMQH_TestPlayerMove (qh_pmove.origin, end);

		if (trace.startsolid || trace.allsolid)
		{
			// entity is trapped in another solid
			VectorCopy(vec3_origin, qh_pmove.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, qh_pmove.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// save entity for contact
		qh_pmove.touchindex[qh_pmove.numtouch] = trace.entityNum;
		qh_pmove.numtouch++;

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
		}

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_FLY_MOVE_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorCopy(vec3_origin, qh_pmove.velocity);
			break;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		int i;
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(original_velocity, planes[i], qh_pmove.velocity, 1);
			int j;
			for (j = 0; j < numplanes; j++)
			{
				if (j != i)
				{
					if (DotProduct(qh_pmove.velocity, planes[j]) < 0)
					{
						break;	// not ok
					}
				}
			}
			if (j == numplanes)
			{
				break;
			}
		}
		
		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				VectorCopy(vec3_origin, qh_pmove.velocity);
				break;
			}
			vec3_t dir;
			CrossProduct(planes[0], planes[1], dir);
			float d = DotProduct(dir, qh_pmove.velocity);
			VectorScale(dir, d, qh_pmove.velocity);
		}

		//
		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(qh_pmove.velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, qh_pmove.velocity);
			break;
		}
	}

	if (qh_pmove.waterjumptime)
	{
		VectorCopy (primal_velocity, qh_pmove.velocity);
	}
	return blocked;
}

//	Player is on ground, with no upwards velocity
static void PMQH_GroundMove()
{
	qh_pmove.velocity[2] = 0;
	if (!qh_pmove.velocity[0] && !qh_pmove.velocity[1] && !qh_pmove.velocity[2])
	{
		return;
	}

	// first try just moving to the destination	
	vec3_t dest;
	dest[0] = qh_pmove.origin[0] + qh_pmove.velocity[0] * qh_pml.frametime;
	dest[1] = qh_pmove.origin[1] + qh_pmove.velocity[1] * qh_pml.frametime;	
	dest[2] = qh_pmove.origin[2];

	// first try moving directly to the next spot
	vec3_t start;
	VectorCopy(dest, start);
	q1trace_t trace = PMQH_TestPlayerMove(qh_pmove.origin, dest);
	if (trace.fraction == 1)
	{
		VectorCopy(trace.endpos, qh_pmove.origin);
		return;
	}

	// try sliding forward both on ground and up 16 pixels
	// take the move that goes farthest
	vec3_t original, originalvel;
	VectorCopy(qh_pmove.origin, original);
	VectorCopy(qh_pmove.velocity, originalvel);

	// slide move
	PMQH_FlyMove();

	vec3_t down, downvel;
	VectorCopy(qh_pmove.origin, down);
	VectorCopy(qh_pmove.velocity, downvel);

	VectorCopy(original, qh_pmove.origin);
	VectorCopy(originalvel, qh_pmove.velocity);

	// move up a stair height
	VectorCopy(qh_pmove.origin, dest);
	dest[2] += STEPSIZE;
	trace = PMQH_TestPlayerMove(qh_pmove.origin, dest);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, qh_pmove.origin);
	}

	// slide move
	PMQH_FlyMove();

	// press down the stepheight
	VectorCopy(qh_pmove.origin, dest);
	dest[2] -= STEPSIZE;
	trace = PMQH_TestPlayerMove(qh_pmove.origin, dest);
	bool usedown;
	if (trace.plane.normal[2] < 0.7)
	{
		usedown = true;
	}
	else
	{
		if (!trace.startsolid && !trace.allsolid)
		{
			VectorCopy(trace.endpos, qh_pmove.origin);
		}
		vec3_t up;
		VectorCopy(qh_pmove.origin, up);

		// decide which one went farther
		float downdist = (down[0] - original[0]) * (down[0] - original[0]) +
			(down[1] - original[1]) * (down[1] - original[1]);
		float updist = (up[0] - original[0])*(up[0] - original[0]) +
			(up[1] - original[1]) * (up[1] - original[1]);
		usedown = downdist > updist;
	}

	if (usedown)
	{
		VectorCopy(down, qh_pmove.origin);
		VectorCopy(downvel, qh_pmove.velocity);
	}
	else // copy z value from slide move
	{
		qh_pmove.velocity[2] = downvel[2];
	}

	// if at a dead stop, retry the move with nudges to get around lips
}

//	Handles both ground friction and water friction
static void PMQH_Friction()
{
	if (qh_pmove.waterjumptime)
	{
		return;
	}

	float* vel = qh_pmove.velocity;
	
	float speed = sqrt(vel[0] * vel[0] +vel[1] * vel[1] + vel[2] * vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	float friction = movevars.friction;

	// if the leading edge is over a dropoff, increase friction
	if ((GGameType & GAME_Quake) && qh_pmove.onground != -1)
	{
		vec3_t start, stop;
		start[0] = stop[0] = qh_pmove.origin[0] + vel[0] / speed * 16;
		start[1] = stop[1] = qh_pmove.origin[1] + vel[1] / speed * 16;
		start[2] = qh_pmove.origin[2] + pmqh_player_mins[2];
		stop[2] = start[2] - 34;

		q1trace_t trace = PMQH_TestPlayerMove(start, stop);

		if (trace.fraction == 1)
		{
			friction *= 2;
		}
	}

	float drop = 0;

	if (GGameType & GAME_Quake)
	{
		if (qh_pmove.waterlevel >= 2) // apply water friction
		{
			drop += speed * movevars.waterfriction * qh_pmove.waterlevel * qh_pml.frametime;
		}
		else if (qh_pmove.onground != -1) // apply ground friction
		{
			float control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
			drop += control * friction * qh_pml.frametime;
		}
	}
	else
	{
		// apply ground friction
		if ((qh_pmove.onground != -1) || (qh_pmove.movetype == QHMOVETYPE_FLY))
		{
			float control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
			drop += control * friction * qh_pml.frametime;
		}

		// apply water friction
		if (qh_pmove.waterlevel)
		{
			drop += speed * movevars.waterfriction * qh_pmove.waterlevel * qh_pml.frametime;
		}
	}

	// scale the velocity
	float newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

static void PMQH_Accelerate(const vec3_t wishdir, float wishspeed, float accel)
{
	if (qh_pmove.dead)
	{
		return;
	}
	if (qh_pmove.waterjumptime)
	{
		return;
	}

	float currentspeed = DotProduct(qh_pmove.velocity, wishdir);
	float addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	float accelspeed = accel * qh_pml.frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}
	for (int i = 0; i < 3; i++)
	{
		qh_pmove.velocity[i] += accelspeed * wishdir[i];	
	}
}

static void PMQH_AirAccelerate(const vec3_t wishdir, float wishspeed, float accel)
{
	if (qh_pmove.dead)
	{
		return;
	}
	if (qh_pmove.waterjumptime)
	{
		return;
	}

	float wishspd = wishspeed;
	if (wishspd > 30)
	{
		wishspd = 30;
	}
	float currentspeed = DotProduct(qh_pmove.velocity, wishdir);
	float addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	float accelspeed = accel * wishspeed * qh_pml.frametime;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (int i = 0; i < 3; i++)
	{
		qh_pmove.velocity[i] += accelspeed * wishdir[i];
	}
}

static void PMQH_WaterMove()
{
	//
	// user intentions
	//
	float fmove = qh_pmove.cmd.forwardmove;
	float smove = qh_pmove.cmd.sidemove;
	float maxspeed = movevars.maxspeed;

	if (GGameType & GAME_Hexen2)
	{
		// client 400 (assasin running) is scaled to maxspeed
		// clamp is tested seperately so strafe running works
		maxspeed *= qh_pmove.hasted;
		if (Q_fabs(fmove) > maxspeed)
		{
			if (fmove < 0)
			{
				fmove = -maxspeed;
			}
			else
			{
				fmove = maxspeed;
			}
		}

		if (Q_fabs(smove) > maxspeed)
		{
			if (smove < 0)
			{
				smove = -maxspeed;
			}
			else
			{
				smove = maxspeed;
			}
		}
		
		if (qh_pmove.crouched)
		{
			fmove = fmove / 600 * movevars.maxspeed;
			smove = smove / 600 * movevars.maxspeed;
		}
		else
		{
			fmove = fmove / 400 * movevars.maxspeed;
			smove = smove / 400 * movevars.maxspeed;
		}
	}

	vec3_t wishvel;
	for (int i = 0; i < 3; i++)
	{
		wishvel[i] = qh_pml.forward[i] * fmove + qh_pml.right[i] * smove;
	}

	if (!qh_pmove.cmd.forwardmove && !qh_pmove.cmd.sidemove && !qh_pmove.cmd.upmove)
	{
		if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
		{
			wishvel[2] -= 120;		// drift towards bottom
		}
		else
		{
			wishvel[2] -= 60;		// drift towards bottom
		}
	}
	else
	{
		if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
		{
			wishvel[2] -= 60;		// drift towards bottom
		}
		else
		{
			wishvel[2] += qh_pmove.cmd.upmove;
		}
	}

	vec3_t wishdir;
	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);

	if (wishspeed > maxspeed)
	{
		VectorScale(wishvel, maxspeed / wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	wishspeed *= 0.7;

	//
	// water acceleration
	//
	PMQH_Accelerate(wishdir, wishspeed, movevars.wateraccelerate);

	// assume it is a stair or a slope, so press down from stepheight above
	vec3_t dest;
	VectorMA(qh_pmove.origin, qh_pml.frametime, qh_pmove.velocity, dest);
	vec3_t start;
	VectorCopy(dest, start);
	start[2] += STEPSIZE + 1;
	q1trace_t trace = PMQH_TestPlayerMove(start, dest);
	if (!trace.startsolid && !trace.allsolid)	// FIXME: check steep slope?
	{
		// walked up the step
		VectorCopy(trace.endpos, qh_pmove.origin);
		return;
	}
	
	PMQH_FlyMove();
}

static void PMQH_AirMove()
{
	float fmove, smove;
	if ((GGameType & GAME_Quake) || qh_pmove.teleport_time < 0)
	{
		//no Mario Bros. stop in mid air!
		fmove = qh_pmove.cmd.forwardmove;
		smove = qh_pmove.cmd.sidemove;
	}
	else
	{
		fmove = smove = 0;
	}

	float maxspeed = movevars.maxspeed;

	if (GGameType & GAME_Hexen2)
	{
		// client 400 (assasin running) is scaled to maxspeed
		// clamp is tested seperately so strafe running works
		maxspeed *= qh_pmove.hasted;
		if (Q_fabs(fmove) > maxspeed)
		{
			if (fmove < 0)
			{
				fmove = -maxspeed;
			}
			else
			{
				fmove = maxspeed;
			}
		}

		if (Q_fabs(smove) > maxspeed)
		{
			if (smove < 0)
			{
				smove = -maxspeed;
			}
			else
			{
				smove = maxspeed;
			}
		}
		
		fmove = fmove / 400 * maxspeed;
		smove = smove / 400 * maxspeed;
	}

	qh_pml.forward[2] = 0;
	qh_pml.right[2] = 0;
	VectorNormalize(qh_pml.forward);
	VectorNormalize(qh_pml.right);

	vec3_t wishvel;
	for (int i = 0; i < 2; i++)
	{
		wishvel[i] = qh_pml.forward[i] * fmove + qh_pml.right[i] * smove;
	}
	wishvel[2] = 0;

	vec3_t wishdir;
	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > maxspeed)
	{
		VectorScale(wishvel, maxspeed / wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	
	if (qh_pmove.onground != -1)
	{
		qh_pmove.velocity[2] = 0;
		PMQH_Accelerate(wishdir, wishspeed, movevars.accelerate);
		if (GGameType & GAME_Quake)
		{
			qh_pmove.velocity[2] -= movevars.entgravity * movevars.gravity * qh_pml.frametime;
		}
		PMQH_GroundMove();
	}
	else
	{
		// not on ground, so little effect on velocity
		PMQH_AirAccelerate(wishdir, wishspeed, movevars.accelerate);
		if (GGameType & GAME_Hexen2)
		{
			PMQH_FlyMove();
		}

		// add gravity
		qh_pmove.velocity[2] -= movevars.entgravity * movevars.gravity * qh_pml.frametime;

		if (GGameType & GAME_Quake)
		{
			PMQH_FlyMove();
		}
	}
}

//	for when you are using the ring of flight
static void PMHW_FlyingMove()
{
	float fmove = qh_pmove.cmd.forwardmove;
	float smove = qh_pmove.cmd.sidemove;
	float umove = qh_pmove.cmd.upmove;

	// client 400 (assasin running) is scaled to maxspeed
	// clamp is tested seperately so strafe running works
	float clamp = movevars.maxspeed * qh_pmove.hasted;
	if (Q_fabs(fmove) > clamp)
	{
		if (fmove < 0)
		{
			fmove = -clamp;
		}
		else
		{
			fmove = clamp;
		}
	}

	if (Q_fabs(smove) > clamp)
	{
		if (smove < 0)
		{
			smove = -clamp;
		}
		else
		{
			smove = clamp;
		}
	}
	if (Q_fabs(umove) > clamp)
	{
		if (umove < 0)
		{
			umove = -clamp;
		}
		else
		{
			umove = clamp;
		}
	}
	
	fmove = fmove / 400 * movevars.maxspeed;
	smove = smove / 400 * movevars.maxspeed;
	umove = umove / 400 * movevars.maxspeed;

	VectorNormalize(qh_pml.forward);
	VectorNormalize(qh_pml.right);
	VectorNormalize(qh_pml.up);

	vec3_t wishvel;
	for (int i = 0; i < 3; i++)
	{
		wishvel[i] = qh_pml.forward[i] * fmove + qh_pml.right[i] * smove + qh_pml.up[i] * umove;
	}

	vec3_t wishdir;
	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);

	PMQH_Accelerate(wishdir, wishspeed, movevars.accelerate);
	PMQH_FlyMove();
}

static void PMQH_CatagorizePosition()
{
	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid	
	vec3_t point;
	point[0] = qh_pmove.origin[0];
	point[1] = qh_pmove.origin[1];
	point[2] = qh_pmove.origin[2] - 1;
	if (qh_pmove.velocity[2] > 180)
	{
		qh_pmove.onground = -1;
	}
	else
	{
		q1trace_t tr = PMQH_TestPlayerMove(qh_pmove.origin, point);
		if (tr.plane.normal[2] < 0.7)
		{
			qh_pmove.onground = -1;	// too steep
		}
		else
		{
			qh_pmove.onground = tr.entityNum;
		}
		if (qh_pmove.onground != -1)
		{
			qh_pmove.waterjumptime = 0;
			if (!tr.startsolid && !tr.allsolid)
			{
				VectorCopy(tr.endpos, qh_pmove.origin);
			}
		}

		// standing on an entity other than the world
		if (tr.entityNum > 0)
		{
			qh_pmove.touchindex[qh_pmove.numtouch] = tr.entityNum;
			qh_pmove.numtouch++;
		}
	}

	//
	// get waterlevel
	//
	qh_pmove.waterlevel = 0;
	qh_pmove.watertype = BSP29CONTENTS_EMPTY;

	point[2] = qh_pmove.origin[2] + pmqh_player_mins[2] + 1;
	int cont = CM_PointContentsQ1(point, 0);

	if (cont <= BSP29CONTENTS_WATER)
	{
		qh_pmove.watertype = cont;
		qh_pmove.waterlevel = 1;
		if (GGameType & GAME_Quake)
		{
			point[2] += 26;
		}
		else
		{
			point[2] = qh_pmove.origin[2] + (pmqh_player_mins[2] + pmqh_player_maxs[2]) * 0.5;
		}
		cont = CM_PointContentsQ1(point, 0);
		if (cont <= BSP29CONTENTS_WATER)
		{
			qh_pmove.waterlevel = 2;
			if (GGameType & GAME_Quake)
			{
				point[2] += 22;
			}
			else
			{
				point[2] = qh_pmove.origin[2] + 22;
			}
			cont = CM_PointContentsQ1(point, 0);
			if (cont <= BSP29CONTENTS_WATER)
			{
				qh_pmove.waterlevel = 3;
			}
		}
	}
}

static void PMQH_JumpButton()
{
	if (qh_pmove.dead)
	{
		qh_pmove.oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
		return;
	}

	if (qh_pmove.waterjumptime)
	{
		qh_pmove.waterjumptime -= qh_pml.frametime;
		if (qh_pmove.waterjumptime < 0)
		{
			qh_pmove.waterjumptime = 0;
		}
		return;
	}

	if (qh_pmove.waterlevel >= 2)
	{
		// swimming, not jumping
		qh_pmove.onground = -1;

		if (qh_pmove.watertype == BSP29CONTENTS_WATER)
		{
			qh_pmove.velocity[2] = 100;
		}
		else if (qh_pmove.watertype == BSP29CONTENTS_SLIME)
		{
			qh_pmove.velocity[2] = 80;
		}
		else
		{
			qh_pmove.velocity[2] = 50;
		}
		return;
	}

	if (qh_pmove.onground == -1)
	{
		return;		// in air, so no effect
	}

	if (qh_pmove.oldbuttons & QHBUTTON_JUMP)
	{
		return;		// don't pogo stick
	}

	qh_pmove.onground = -1;
	qh_pmove.velocity[2] += 270;

	qh_pmove.oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
}

static void PMQH_CheckWaterJump()
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (qh_pmove.waterjumptime)
		return;

	// ZOID, don't hop out if we just jumped in
	if (qh_pmove.velocity[2] < -180)
	{
		return; // only hop out if we are moving up
	}

	// see if near an edge
	flatforward[0] = qh_pml.forward[0];
	flatforward[1] = qh_pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (qh_pmove.origin, 24, flatforward, spot);
	if (GGameType & GAME_Quake)
	{
		spot[2] += 8;
	}
	else
	{
		spot[2] += 32;
	}
	cont = CM_PointContentsQ1(spot, 0);
	if (cont != BSP29CONTENTS_SOLID)
	{
		return;
	}
	spot[2] += 24;
	cont = CM_PointContentsQ1(spot, 0);
	if (cont != BSP29CONTENTS_EMPTY)
	{
		return;
	}
	// jump out of water
	if (GGameType & GAME_Quake)
	{
		VectorScale (flatforward, 50, qh_pmove.velocity);
		qh_pmove.velocity[2] = 310;
	}
	else
	{
		VectorScale (qh_pml.forward, 200, qh_pmove.velocity);
		qh_pmove.velocity[2] = 275;
	}
	qh_pmove.waterjumptime = 2;	// safety net
	qh_pmove.oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
}

//	If qh_pmove.origin is in a solid position,
// try nudging slightly on all axis to
// allow for the cut precision of the net coordinates
static void PMQH_NudgePosition()
{
	static int sign[3] = {0, -1, 1};

	vec3_t base;
	VectorCopy(qh_pmove.origin, base);

	for (int z = 0; z <= 2; z++)
	{
		for (int x = 0; x <= 2; x++)
		{
			for (int y = 0; y <= 2; y++)
			{
				qh_pmove.origin[0] = base[0] + (sign[x] * 1.0 / 8);
				qh_pmove.origin[1] = base[1] + (sign[y] * 1.0 / 8);
				qh_pmove.origin[2] = base[2] + (sign[z] * 1.0 / 8);
				if (PMQH_TestPlayerPosition(qh_pmove.origin))
				{
					return;
				}
			}
		}
	}
	VectorCopy(base, qh_pmove.origin);
}

static void PMQH_SpectatorMove()
{
	// friction

	float speed = VectorLength(qh_pmove.velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, qh_pmove.velocity);
	}
	else
	{
		float drop = 0;

		float friction = movevars.friction * 1.5;	// extra friction
		float control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop += control*friction*qh_pml.frametime;

		// scale the velocity
		float newspeed = speed - drop;
		if (newspeed < 0)
		{
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale(qh_pmove.velocity, newspeed, qh_pmove.velocity);
	}

	// accelerate
	float fmove = qh_pmove.cmd.forwardmove;
	float smove = qh_pmove.cmd.sidemove;
	
	VectorNormalize(qh_pml.forward);
	VectorNormalize(qh_pml.right);

	vec3_t wishvel;
	for (int i = 0; i < 3; i++)
	{
		wishvel[i] = qh_pml.forward[i] * fmove + qh_pml.right[i] * smove;
	}
	wishvel[2] += qh_pmove.cmd.upmove;

	vec3_t wishdir;
	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > movevars.spectatormaxspeed)
	{
		VectorScale(wishvel, movevars.spectatormaxspeed / wishspeed, wishvel);
		wishspeed = movevars.spectatormaxspeed;
	}

	float currentspeed = DotProduct(qh_pmove.velocity, wishdir);
	float addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	float accelspeed = movevars.accelerate * qh_pml.frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}
	
	for (int i = 0; i < 3; i++)
	{
		qh_pmove.velocity[i] += accelspeed * wishdir[i];
	}

	// move
	VectorMA(qh_pmove.origin, qh_pml.frametime, qh_pmove.velocity, qh_pmove.origin);
}

//	Returns with origin, angles, and velocity modified in place.
//	Numtouch and touchindex[] will be set if any of the physents
// were contacted during the move.
void PMQH_PlayerMove()
{
	qh_pml.frametime = qh_pmove.cmd.msec * 0.001;
	qh_pmove.numtouch = 0;

	if ((GGameType & GAME_Hexen2) && qh_pmove.movetype == QHMOVETYPE_NONE)
	{
		qh_pmove.oldbuttons = 0;
		qh_pmove.cmd.buttons = 0;
		return;
	}
	
	AngleVectors(qh_pmove.angles, qh_pml.forward, qh_pml.right, qh_pml.up);

	if (qh_pmove.spectator)
	{
		PMQH_SpectatorMove();
		return;
	}

	PMQH_NudgePosition();

	// take angles directly from command
	VectorCopy(qh_pmove.cmd.angles, qh_pmove.angles);

	// set onground, watertype, and waterlevel
	PMQH_CatagorizePosition();

	if (qh_pmove.waterlevel == ((GGameType & GAME_Quake) ? 2 : 1))
	{
		PMQH_CheckWaterJump();
	}

	if (qh_pmove.velocity[2] < 0)
	{
		qh_pmove.waterjumptime = 0;
	}

	if (qh_pmove.cmd.buttons & QHBUTTON_JUMP)
	{
		PMQH_JumpButton();
	}
	else
	{
		qh_pmove.oldbuttons &= ~QHBUTTON_JUMP;
	}

	PMQH_Friction();

	if (qh_pmove.waterlevel >= 2)
	{
		PMQH_WaterMove();
	}
	else if ((GGameType & GAME_Hexen2) && qh_pmove.movetype == QHMOVETYPE_FLY)
	{
		PMHW_FlyingMove();
	}
	else
	{
		PMQH_AirMove();
	}

	// set onground, watertype, and waterlevel for final spot
	PMQH_CatagorizePosition();
}
