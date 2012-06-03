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
// sv_phys.c

#include "quakedef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and QHMOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and QHMOVETYPE_TOSS
corpses are SOLID_NOT and QHMOVETYPE_TOSS
crates are SOLID_BBOX and QHMOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and QHMOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and QHMOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

Cvar* sv_friction;
Cvar* sv_stopspeed;
Cvar* sv_gravity;
Cvar* sv_maxvelocity;
Cvar* sv_nostep;

#define MOVE_EPSILON    0.01

void SV_Physics_Toss(qhedict_t* ent);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts(void)
{
	int e;
	qhedict_t* check;

// see if any solid entities are inside the final position
	check = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetMoveType() == QHMOVETYPE_PUSH ||
			check->GetMoveType() == QHMOVETYPE_NONE ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		if (SV_TestEntityPosition(check))
		{
			Con_Printf("entity in invalid position\n");
		}
	}
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(qhedict_t* ent)
{
	int i;

//
// bound velocity
//
	for (i = 0; i < 3; i++)
	{
		if (IS_NAN(ent->GetVelocity()[i]))
		{
			Con_Printf("Got a NaN velocity on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetVelocity()[i] = 0;
		}
		if (IS_NAN(ent->GetOrigin()[i]))
		{
			Con_Printf("Got a NaN origin on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetOrigin()[i] = 0;
		}
		if (ent->GetVelocity()[i] > sv_maxvelocity->value)
		{
			ent->GetVelocity()[i] = sv_maxvelocity->value;
		}
		else if (ent->GetVelocity()[i] < -sv_maxvelocity->value)
		{
			ent->GetVelocity()[i] = -sv_maxvelocity->value;
		}
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
qboolean SV_RunThink(qhedict_t* ent)
{
	float thinktime;

	thinktime = ent->GetNextThink();
	if (thinktime <= 0 || thinktime > sv.qh_time + host_frametime)
	{
		return true;
	}

	if (thinktime < sv.qh_time)
	{
		thinktime = sv.qh_time;	// don't let things stay in the past.
	}
	// it is possible to start that way
	// by a trigger with a local time.
	ent->SetNextThink(0);
	pr_global_struct->time = thinktime;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	pr_global_struct->other = EDICT_TO_PROG(sv.qh_edicts);
	PR_ExecuteProgram(ent->GetThink());
	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(qhedict_t* e1, qhedict_t* e2)
{
	int old_self, old_other;

	old_self = pr_global_struct->self;
	old_other = pr_global_struct->other;

	pr_global_struct->time = sv.qh_time;
	if (e1->GetTouch() && e1->GetSolid() != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e1);
		pr_global_struct->other = EDICT_TO_PROG(e2);
		PR_ExecuteProgram(e1->GetTouch());
	}

	if (e2->GetTouch() && e2->GetSolid() != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e2);
		pr_global_struct->other = EDICT_TO_PROG(e1);
		PR_ExecuteProgram(e2->GetTouch());
	}

	pr_global_struct->self = old_self;
	pr_global_struct->other = old_other;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
int SV_FlyMove(qhedict_t* ent, float time, q1trace_t* steptrace)
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_FLY_MOVE_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;
	int i, j;
	q1trace_t trace;
	vec3_t end;
	float time_left;
	int blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->GetVelocity(), original_velocity);
	VectorCopy(ent->GetVelocity(), primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (!ent->GetVelocity()[0] && !ent->GetVelocity()[1] && !ent->GetVelocity()[2])
		{
			break;
		}

		for (i = 0; i < 3; i++)
			end[i] = ent->GetOrigin()[i] + time_left* ent->GetVelocity()[i];

		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			ent->SetVelocity(vec3_origin);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, ent->GetOrigin());
			VectorCopy(ent->GetVelocity(), original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
		{
			break;		// moved the entire distance

		}
		if (trace.entityNum < 0)
		{
			Sys_Error("SV_FlyMove: !trace.ent");
		}

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (EDICT_NUM(trace.entityNum)->GetSolid() == SOLID_BSP)
			{
				ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
				ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
			{
				*steptrace = trace;	// save for player extrafriction
			}
		}

//
// run the impact function
//
		SV_Impact(ent, EDICT_NUM(trace.entityNum));
		if (ent->free)
		{
			break;		// removed by the impact function


		}
		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_FLY_MOVE_CLIP_PLANES)
		{	// this shouldn't really happen
			ent->SetVelocity(vec3_origin);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
					{
						break;	// not ok
					}
				}
			if (j == numplanes)
			{
				break;
			}
		}

		if (i != numplanes)
		{	// go along this plane
			ent->SetVelocity(new_velocity);
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				ent->SetVelocity(vec3_origin);
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->GetVelocity());
			VectorScale(dir, d, ent->GetVelocity());
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct(ent->GetVelocity(), primal_velocity) <= 0)
		{
			ent->SetVelocity(vec3_origin);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity(qhedict_t* ent)
{
	float ent_gravity;

	eval_t* val;

	val = GetEdictFieldValue(ent, "gravity");
	if (val && val->_float)
	{
		ent_gravity = val->_float;
	}
	else
	{
		ent_gravity = 1.0;
	}
	ent->GetVelocity()[2] -= ent_gravity * sv_gravity->value * host_frametime;
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
q1trace_t SV_PushEntity(qhedict_t* ent, vec3_t push)
{
	q1trace_t trace;
	vec3_t end;

	VectorAdd(ent->GetOrigin(), push, end);

	if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE)
	{
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_MISSILE, ent);
	}
	else if (ent->GetSolid() == SOLID_TRIGGER || ent->GetSolid() == SOLID_NOT)
	{
		// only clip against bmodels
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_NOMONSTERS, ent);
	}
	else
	{
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_NORMAL, ent);
	}

	VectorCopy(trace.endpos, ent->GetOrigin());
	SV_LinkEdict(ent, true);

	if (trace.entityNum >= 0)
	{
		SV_Impact(ent, EDICT_NUM(trace.entityNum));
	}

	return trace;
}


/*
============
SV_PushMove

============
*/
void SV_PushMove(qhedict_t* pusher, float movetime)
{
	int i, e;
	qhedict_t* check, * block;
	vec3_t mins, maxs, move;
	vec3_t entorig, pushorig;
	int num_moved;
	qhedict_t* moved_edict[MAX_EDICTS_Q1];
	vec3_t moved_from[MAX_EDICTS_Q1];

	if (!pusher->GetVelocity()[0] && !pusher->GetVelocity()[1] && !pusher->GetVelocity()[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	for (i = 0; i < 3; i++)
	{
		move[i] = pusher->GetVelocity()[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy(pusher->GetOrigin(), pushorig);

// move the pusher to it's final position

	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	pusher->v.ltime += movetime;
	SV_LinkEdict(pusher, false);


// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetMoveType() == QHMOVETYPE_PUSH ||
			check->GetMoveType() == QHMOVETYPE_NONE ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definately be moved
		if (!(((int)check->GetFlags() & FL_ONGROUND) &&
			  PROG_TO_EDICT(check->GetGroundEntity()) == pusher))
		{
			if (check->v.absmin[0] >= maxs[0] ||
				check->v.absmin[1] >= maxs[1] ||
				check->v.absmin[2] >= maxs[2] ||
				check->v.absmax[0] <= mins[0] ||
				check->v.absmax[1] <= mins[1] ||
				check->v.absmax[2] <= mins[2])
			{
				continue;
			}

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
			{
				continue;
			}
		}

		// remove the onground flag for non-players
		if (check->GetMoveType() != QHMOVETYPE_WALK)
		{
			check->SetFlags((int)check->GetFlags() & ~FL_ONGROUND);
		}

		VectorCopy(check->GetOrigin(), entorig);
		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		pusher->SetSolid(SOLID_NOT);
		SV_PushEntity(check, move);
		pusher->SetSolid(SOLID_BSP);

		// if it is still inside the pusher, block
		block = SV_TestEntityPosition(check);
		if (block)
		{	// fail the move
			if (check->GetMins()[0] == check->GetMaxs()[0])
			{
				continue;
			}
			if (check->GetSolid() == SOLID_NOT || check->GetSolid() == SOLID_TRIGGER)
			{	// corpse
				check->GetMins()[0] = check->GetMins()[1] = 0;
				check->SetMaxs(check->GetMins());
				continue;
			}

			VectorCopy(entorig, check->GetOrigin());
			SV_LinkEdict(check, true);

			VectorCopy(pushorig, pusher->GetOrigin());
			SV_LinkEdict(pusher, false);
			pusher->v.ltime -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->GetBlocked())
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram(pusher->GetBlocked());
			}

			// move back any entities we already moved
			for (i = 0; i < num_moved; i++)
			{
				VectorCopy(moved_from[i], moved_edict[i]->GetOrigin());
				SV_LinkEdict(moved_edict[i], false);
			}
			return;
		}
	}


}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher(qhedict_t* ent)
{
	float thinktime;
	float oldltime;
	float movetime;

	oldltime = ent->v.ltime;

	thinktime = ent->GetNextThink();
	if (thinktime < ent->v.ltime + host_frametime)
	{
		movetime = thinktime - ent->v.ltime;
		if (movetime < 0)
		{
			movetime = 0;
		}
	}
	else
	{
		movetime = host_frametime;
	}

	if (movetime)
	{
		SV_PushMove(ent, movetime);		// advances ent->v.ltime if not blocked
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		ent->SetNextThink(0);
		pr_global_struct->time = sv.qh_time;
		pr_global_struct->self = EDICT_TO_PROG(ent);
		pr_global_struct->other = EDICT_TO_PROG(sv.qh_edicts);
		PR_ExecuteProgram(ent->GetThink());
		if (ent->free)
		{
			return;
		}
	}

}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck(qhedict_t* ent)
{
	int i, j;
	int z;
	vec3_t org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy(ent->GetOrigin(), ent->GetOldOrigin());
		return;
	}

	VectorCopy(ent->GetOrigin(), org);
	VectorCopy(ent->GetOldOrigin(), ent->GetOrigin());
	if (!SV_TestEntityPosition(ent))
	{
		Con_DPrintf("Unstuck.\n");
		SV_LinkEdict(ent, true);
		return;
	}

	for (z = 0; z < 18; z++)
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
			{
				ent->GetOrigin()[0] = org[0] + i;
				ent->GetOrigin()[1] = org[1] + j;
				ent->GetOrigin()[2] = org[2] + z;
				if (!SV_TestEntityPosition(ent))
				{
					Con_DPrintf("Unstuck.\n");
					SV_LinkEdict(ent, true);
					return;
				}
			}

	VectorCopy(org, ent->GetOrigin());
	Con_DPrintf("player is stuck.\n");
}


/*
=============
SV_CheckWater
=============
*/
qboolean SV_CheckWater(qhedict_t* ent)
{
	vec3_t point;
	int cont;

	point[0] = ent->GetOrigin()[0];
	point[1] = ent->GetOrigin()[1];
	point[2] = ent->GetOrigin()[2] + ent->GetMins()[2] + 1;

	ent->SetWaterLevel(0);
	ent->SetWaterType(BSP29CONTENTS_EMPTY);
	cont = SV_PointContents(point);
	if (cont <= BSP29CONTENTS_WATER)
	{
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
		point[2] = ent->GetOrigin()[2] + (ent->GetMins()[2] + ent->GetMaxs()[2]) * 0.5;
		cont = SV_PointContents(point);
		if (cont <= BSP29CONTENTS_WATER)
		{
			ent->SetWaterLevel(2);
			point[2] = ent->GetOrigin()[2] + ent->GetViewOfs()[2];
			cont = SV_PointContents(point);
			if (cont <= BSP29CONTENTS_WATER)
			{
				ent->SetWaterLevel(3);
			}
		}
	}

	return ent->GetWaterLevel() > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction(qhedict_t* ent, q1trace_t* trace)
{
	vec3_t forward, right, up;
	float d, i;
	vec3_t into, side;

	AngleVectors(ent->GetVAngle(), forward, right, up);
	d = DotProduct(trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0)
	{
		return;
	}

// cut the tangential velocity
	i = DotProduct(trace->plane.normal, ent->GetVelocity());
	VectorScale(trace->plane.normal, i, into);
	VectorSubtract(ent->GetVelocity(), into, side);

	ent->GetVelocity()[0] = side[0] * (1 + d);
	ent->GetVelocity()[1] = side[1] * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick(qhedict_t* ent, vec3_t oldvel)
{
	int i;
	vec3_t oldorg;
	vec3_t dir;
	int clip;
	q1trace_t steptrace;

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(vec3_origin, dir);

	for (i = 0; i < 8; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
		case 0: dir[0] = 2; dir[1] = 0; break;
		case 1: dir[0] = 0; dir[1] = 2; break;
		case 2: dir[0] = -2; dir[1] = 0; break;
		case 3: dir[0] = 0; dir[1] = -2; break;
		case 4: dir[0] = 2; dir[1] = 2; break;
		case 5: dir[0] = -2; dir[1] = 2; break;
		case 6: dir[0] = 2; dir[1] = -2; break;
		case 7: dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity(ent, dir);

// retry the original move
		ent->GetVelocity()[0] = oldvel[0];
		ent->GetVelocity()[1] = oldvel[1];
		ent->GetVelocity()[2] = 0;
		clip = SV_FlyMove(ent, 0.1, &steptrace);

		if (fabs(oldorg[1] - ent->GetOrigin()[1]) > 4 ||
			fabs(oldorg[0] - ent->GetOrigin()[0]) > 4)
		{
//Con_DPrintf ("unstuck!\n");
			return clip;
		}

// go back to the original pos and try again
		VectorCopy(oldorg, ent->GetOrigin());
	}

	ent->SetVelocity(vec3_origin);
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
void SV_WalkMove(qhedict_t* ent)
{
	vec3_t upmove, downmove;
	vec3_t oldorg, oldvel;
	vec3_t nosteporg, nostepvel;
	int clip;
	int oldonground;
	q1trace_t steptrace, downtrace;

//
// do a regular slide move unless it looks like you ran into a step
//
	oldonground = (int)ent->GetFlags() & FL_ONGROUND;
	ent->SetFlags((int)ent->GetFlags() & ~FL_ONGROUND);

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(ent->GetVelocity(), oldvel);

	clip = SV_FlyMove(ent, host_frametime, &steptrace);

	if (!(clip & 2))
	{
		return;		// move didn't block on a step

	}
	if (!oldonground && ent->GetWaterLevel() == 0)
	{
		return;		// don't stair up while jumping

	}
	if (ent->GetMoveType() != QHMOVETYPE_WALK)
	{
		return;		// gibbed by a trigger

	}
	if (sv_nostep->value)
	{
		return;
	}

	if ((int)sv_player->GetFlags() & FL_WATERJUMP)
	{
		return;
	}

	VectorCopy(ent->GetOrigin(), nosteporg);
	VectorCopy(ent->GetVelocity(), nostepvel);

//
// try moving up and forward to go up a step
//
	VectorCopy(oldorg, ent->GetOrigin());	// back to start pos

	VectorCopy(vec3_origin, upmove);
	VectorCopy(vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * host_frametime;

// move up
	SV_PushEntity(ent, upmove);		// FIXME: don't link?

// move forward
	ent->GetVelocity()[0] = oldvel[0];
	ent->GetVelocity()[1] = oldvel[1];
	ent->GetVelocity()[2] = 0;
	clip = SV_FlyMove(ent, host_frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if (fabs(oldorg[1] - ent->GetOrigin()[1]) < 0.03125 &&
			fabs(oldorg[0] - ent->GetOrigin()[0]) < 0.03125)
		{	// stepping up didn't make any progress
			clip = SV_TryUnstick(ent, oldvel);
		}
	}

// extra friction based on view angle
	if (clip & 2)
	{
		SV_WallFriction(ent, &steptrace);
	}

// move down
	downtrace = SV_PushEntity(ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->GetSolid() == SOLID_BSP)
		{
			ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(downtrace.entityNum)));
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb
		VectorCopy(nosteporg, ent->GetOrigin());
		ent->SetVelocity(nostepvel);
	}
}


/*
================
SV_Physics_Client

Player character actions
================
*/
void SV_Physics_Client(qhedict_t* ent, int num)
{
	if (svs.clients[num - 1].state < CS_CONNECTED)
	{
		return;		// unconnected slot

	}
//
// call standard client pre-think
//
	pr_global_struct->time = sv.qh_time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(pr_global_struct->PlayerPreThink);

//
// do a move
//
	SV_CheckVelocity(ent);

//
// decide which move function to call
//
	switch ((int)ent->GetMoveType())
	{
	case QHMOVETYPE_NONE:
		if (!SV_RunThink(ent))
		{
			return;
		}
		break;

	case QHMOVETYPE_WALK:
		if (!SV_RunThink(ent))
		{
			return;
		}
		if (!SV_CheckWater(ent) && !((int)ent->GetFlags() & FL_WATERJUMP))
		{
			SV_AddGravity(ent);
		}
		SV_CheckStuck(ent);
		SV_WalkMove(ent);
		break;

	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
		SV_Physics_Toss(ent);
		break;

	case QHMOVETYPE_FLY:
		if (!SV_RunThink(ent))
		{
			return;
		}
		SV_FlyMove(ent, host_frametime, NULL);
		break;

	case QHMOVETYPE_NOCLIP:
		if (!SV_RunThink(ent))
		{
			return;
		}
		VectorMA(ent->GetOrigin(), host_frametime, ent->GetVelocity(), ent->GetOrigin());
		break;

	default:
		Sys_Error("SV_Physics_client: bad movetype %i", (int)ent->GetMoveType());
	}

//
// call standard player post-think
//
	SV_LinkEdict(ent, true);

	pr_global_struct->time = sv.qh_time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(pr_global_struct->PlayerPostThink);
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(qhedict_t* ent)
{
// regular thinking
	SV_RunThink(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(qhedict_t* ent)
{
// regular thinking
	if (!SV_RunThink(ent))
	{
		return;
	}

	VectorMA(ent->GetAngles(), host_frametime, ent->GetAVelocity(), ent->GetAngles());
	VectorMA(ent->GetOrigin(), host_frametime, ent->GetVelocity(), ent->GetOrigin());

	SV_LinkEdict(ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition(qhedict_t* ent)
{
	int cont;
	cont = SV_PointContents(ent->GetOrigin());
	if (!ent->GetWaterType())
	{	// just spawned here
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
		return;
	}

	if (cont <= BSP29CONTENTS_WATER)
	{
		if (ent->GetWaterType() == BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
	}
	else
	{
		if (ent->GetWaterType() != BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->SetWaterType(BSP29CONTENTS_EMPTY);
		ent->SetWaterLevel(cont);
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss(qhedict_t* ent)
{
	q1trace_t trace;
	vec3_t move;
	float backoff;
	// regular thinking
	if (!SV_RunThink(ent))
	{
		return;
	}

// if onground, return without moving
	if (((int)ent->GetFlags() & FL_ONGROUND))
	{
		return;
	}

	SV_CheckVelocity(ent);

// add gravity
	if (ent->GetMoveType() != QHMOVETYPE_FLY &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE)
	{
		SV_AddGravity(ent);
	}

// move angles
	VectorMA(ent->GetAngles(), host_frametime, ent->GetAVelocity(), ent->GetAngles());

// move origin
	VectorScale(ent->GetVelocity(), host_frametime, move);
	trace = SV_PushEntity(ent, move);
	if (trace.fraction == 1)
	{
		return;
	}
	if (ent->free)
	{
		return;
	}

	if (ent->GetMoveType() == QHMOVETYPE_BOUNCE)
	{
		backoff = 1.5;
	}
	else
	{
		backoff = 1;
	}

	PM_ClipVelocity(ent->GetVelocity(), trace.plane.normal, ent->GetVelocity(), backoff);

// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{
		if (ent->GetVelocity()[2] < 60 || ent->GetMoveType() != QHMOVETYPE_BOUNCE)
		{
			ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));
			ent->SetVelocity(vec3_origin);
			ent->SetAVelocity(vec3_origin);
		}
	}

// check for in water
	SV_CheckWaterTransition(ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/

void SV_Physics_Step(qhedict_t* ent)
{
	qboolean hitsound;

// freefall if not onground
	if (!((int)ent->GetFlags() & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		if (ent->GetVelocity()[2] < sv_gravity->value * -0.1)
		{
			hitsound = true;
		}
		else
		{
			hitsound = false;
		}

		SV_AddGravity(ent);
		SV_CheckVelocity(ent);
		SV_FlyMove(ent, host_frametime, NULL);
		SV_LinkEdict(ent, true);

		if ((int)ent->GetFlags() & FL_ONGROUND)		// just hit ground
		{
			if (hitsound)
			{
				SV_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
			}
		}
	}

// regular thinking
	SV_RunThink(ent);

	SV_CheckWaterTransition(ent);
}

//============================================================================

/*
================
SV_Physics

================
*/
void SV_Physics(void)
{
	int i;
	qhedict_t* ent;

// let the progs know that a new frame has started
	pr_global_struct->self = EDICT_TO_PROG(sv.qh_edicts);
	pr_global_struct->other = EDICT_TO_PROG(sv.qh_edicts);
	pr_global_struct->time = sv.qh_time;
	PR_ExecuteProgram(pr_global_struct->StartFrame);

//SV_CheckAllEnts ();

//
// treat each object in turn
//
	ent = sv.qh_edicts;
	for (i = 0; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}

		if (pr_global_struct->force_retouch)
		{
			SV_LinkEdict(ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= svs.maxclients)
		{
			SV_Physics_Client(ent, i);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_PUSH)
		{
			SV_Physics_Pusher(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NONE)
		{
			SV_Physics_None(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			SV_Physics_Noclip(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_STEP)
		{
			SV_Physics_Step(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_TOSS ||
				 ent->GetMoveType() == QHMOVETYPE_BOUNCE ||
				 ent->GetMoveType() == QHMOVETYPE_FLY ||
				 ent->GetMoveType() == QHMOVETYPE_FLYMISSILE)
		{
			SV_Physics_Toss(ent);
		}
		else
		{
			Sys_Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
		}
	}

	if (pr_global_struct->force_retouch)
	{
		pr_global_struct->force_retouch--;
	}

	sv.qh_time += host_frametime;
}
