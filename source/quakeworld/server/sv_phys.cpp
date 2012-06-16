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

#include "qwsvdef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are QHSOLID_BSP, and QHMOVETYPE_PUSH
bonus items are QHSOLID_TRIGGER touch, and QHMOVETYPE_TOSS
corpses are QHSOLID_NOT and QHMOVETYPE_TOSS
crates are QHSOLID_BBOX and QHMOVETYPE_TOSS
walking monsters are QHSOLID_SLIDEBOX and QHMOVETYPE_STEP
flying/floating monsters are QHSOLID_SLIDEBOX and QHMOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

Cvar* sv_maxvelocity;

Cvar* sv_gravity;
Cvar* sv_stopspeed;
Cvar* sv_maxspeed;
Cvar* sv_spectatormaxspeed;
Cvar* sv_accelerate;
Cvar* sv_airaccelerate;
Cvar* sv_wateraccelerate;
Cvar* sv_friction;
Cvar* sv_waterfriction;


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

		if (SVQH_TestEntityPosition(check))
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

	do
	{
		thinktime = ent->GetNextThink();
		if (thinktime <= 0)
		{
			return true;
		}
		if (thinktime > sv.qh_time + host_frametime)
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
		*pr_globalVars.time = thinktime;
		*pr_globalVars.self = EDICT_TO_PROG(ent);
		*pr_globalVars.other = EDICT_TO_PROG(sv.qh_edicts);
		PR_ExecuteProgram(ent->GetThink());

		if (ent->free)
		{
			return false;
		}
	}
	while (1);

	return true;
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

	old_self = *pr_globalVars.self;
	old_other = *pr_globalVars.other;

	*pr_globalVars.time = sv.qh_time;
	if (e1->GetTouch() && e1->GetSolid() != QHSOLID_NOT)
	{
		*pr_globalVars.self = EDICT_TO_PROG(e1);
		*pr_globalVars.other = EDICT_TO_PROG(e2);
		PR_ExecuteProgram(e1->GetTouch());
	}

	if (e2->GetTouch() && e2->GetSolid() != QHSOLID_NOT)
	{
		*pr_globalVars.self = EDICT_TO_PROG(e2);
		*pr_globalVars.other = EDICT_TO_PROG(e1);
		PR_ExecuteProgram(e2->GetTouch());
	}

	*pr_globalVars.self = old_self;
	*pr_globalVars.other = old_other;
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
		for (i = 0; i < 3; i++)
			end[i] = ent->GetOrigin()[i] + time_left* ent->GetVelocity()[i];

		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

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
			SV_Error("SV_FlyMove: !trace.ent");
		}

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (QH_EDICT_NUM(trace.entityNum)->GetSolid() == QHSOLID_BSP)
			{
				ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);
				ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum)));
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
		SV_Impact(ent, QH_EDICT_NUM(trace.entityNum));
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
void SV_AddGravity(qhedict_t* ent, float scale)
{
	ent->GetVelocity()[2] -= scale * movevars.gravity * host_frametime;
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
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_MISSILE, ent);
	}
	else if (ent->GetSolid() == QHSOLID_TRIGGER || ent->GetSolid() == QHSOLID_NOT)
	{
		// only clip against bmodels
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_NOMONSTERS, ent);
	}
	else
	{
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_NORMAL, ent);
	}

	VectorCopy(trace.endpos, ent->GetOrigin());
	SVQH_LinkEdict(ent, true);

	if (trace.entityNum >= 0)
	{
		SV_Impact(ent, QH_EDICT_NUM(trace.entityNum));
	}

	return trace;
}


/*
============
SV_Push

============
*/
qboolean SV_Push(qhedict_t* pusher, vec3_t move)
{
	int i, e;
	qhedict_t* check, * block;
	vec3_t mins, maxs;
	vec3_t pushorig;
	int num_moved;
	qhedict_t* moved_edict[MAX_EDICTS_QH];
	vec3_t moved_from[MAX_EDICTS_QH];

	for (i = 0; i < 3; i++)
	{
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy(pusher->GetOrigin(), pushorig);

// move the pusher to it's final position

	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	SVQH_LinkEdict(pusher, false);

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

		pusher->SetSolid(QHSOLID_NOT);
		block = SVQH_TestEntityPosition(check);
		pusher->SetSolid(QHSOLID_BSP);
		if (block)
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definately be moved
		if (!(((int)check->GetFlags() & QHFL_ONGROUND) &&
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
			if (!SVQH_TestEntityPosition(check))
			{
				continue;
			}
		}

		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		VectorAdd(check->GetOrigin(), move, check->GetOrigin());
		block = SVQH_TestEntityPosition(check);
		if (!block)
		{	// pushed ok
			SVQH_LinkEdict(check, false);
			continue;
		}

		// if it is ok to leave in the old position, do it
		VectorSubtract(check->GetOrigin(), move, check->GetOrigin());
		block = SVQH_TestEntityPosition(check);
		if (!block)
		{
			num_moved--;
			continue;
		}

		// if it is still inside the pusher, block
		if (check->GetMins()[0] == check->GetMaxs()[0])
		{
			SVQH_LinkEdict(check, false);
			continue;
		}
		if (check->GetSolid() == QHSOLID_NOT || check->GetSolid() == QHSOLID_TRIGGER)
		{	// corpse
			check->GetMins()[0] = check->GetMins()[1] = 0;
			check->SetMaxs(check->GetMins());
			SVQH_LinkEdict(check, false);
			continue;
		}

		VectorCopy(pushorig, pusher->GetOrigin());
		SVQH_LinkEdict(pusher, false);

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (pusher->GetBlocked())
		{
			*pr_globalVars.self = EDICT_TO_PROG(pusher);
			*pr_globalVars.other = EDICT_TO_PROG(check);
			PR_ExecuteProgram(pusher->GetBlocked());
		}

		// move back any entities we already moved
		for (i = 0; i < num_moved; i++)
		{
			VectorCopy(moved_from[i], moved_edict[i]->GetOrigin());
			SVQH_LinkEdict(moved_edict[i], false);
		}
		return false;
	}

	return true;
}

/*
============
SV_PushMove

============
*/
void SV_PushMove(qhedict_t* pusher, float movetime)
{
	int i;
	vec3_t move;

	if (!pusher->GetVelocity()[0] && !pusher->GetVelocity()[1] && !pusher->GetVelocity()[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	for (i = 0; i < 3; i++)
		move[i] = pusher->GetVelocity()[i] * movetime;

	if (SV_Push(pusher, move))
	{
		pusher->v.ltime += movetime;
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
	vec3_t oldorg, move;
	float l;

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
		VectorCopy(ent->GetOrigin(), oldorg);
		ent->SetNextThink(0);
		*pr_globalVars.time = sv.qh_time;
		*pr_globalVars.self = EDICT_TO_PROG(ent);
		*pr_globalVars.other = EDICT_TO_PROG(sv.qh_edicts);
		PR_ExecuteProgram(ent->GetThink());
		if (ent->free)
		{
			return;
		}
		VectorSubtract(ent->GetOrigin(), oldorg, move);

		l = VectorLength(move);
		if (l > 1.0 / 64)
		{
//	Con_Printf ("**** snap: %f\n", Length (l));
			VectorCopy(oldorg, ent->GetOrigin());
			SV_Push(ent, move);
		}

	}

}


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

	SVQH_LinkEdict(ent, false);
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

	cont = SVQH_PointContents(ent->GetOrigin());
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

	if (ent->GetVelocity()[2] > 0)
	{
		ent->SetFlags((int)ent->GetFlags() & ~QHFL_ONGROUND);
	}

// if onground, return without moving
	if (((int)ent->GetFlags() & QHFL_ONGROUND))
	{
		return;
	}

	SV_CheckVelocity(ent);

// add gravity
	if (ent->GetMoveType() != QHMOVETYPE_FLY &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE)
	{
		SV_AddGravity(ent, 1.0);
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
			ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum)));
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
FIXME: is this true?
=============
*/
void SV_Physics_Step(qhedict_t* ent)
{
	qboolean hitsound;

// frefall if not onground
	if (!((int)ent->GetFlags() & (QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM)))
	{
		if (ent->GetVelocity()[2] < movevars.gravity * -0.1)
		{
			hitsound = true;
		}
		else
		{
			hitsound = false;
		}

		SV_AddGravity(ent, 1.0);
		SV_CheckVelocity(ent);
		SV_FlyMove(ent, host_frametime, NULL);
		SVQH_LinkEdict(ent, true);

		if ((int)ent->GetFlags() & QHFL_ONGROUND)		// just hit ground
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

void SV_ProgStartFrame(void)
{
// let the progs know that a new frame has started
	*pr_globalVars.self = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.other = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.time = sv.qh_time;
	PR_ExecuteProgram(*pr_globalVars.StartFrame);
}

/*
================
SV_RunEntity

================
*/
void SV_RunEntity(qhedict_t* ent)
{
	if (ent->GetLastRunTime() == (float)realtime)
	{
		return;
	}
	ent->SetLastRunTime((float)realtime);

	switch ((int)ent->GetMoveType())
	{
	case QHMOVETYPE_PUSH:
		SV_Physics_Pusher(ent);
		break;
	case QHMOVETYPE_NONE:
		SV_Physics_None(ent);
		break;
	case QHMOVETYPE_NOCLIP:
		SV_Physics_Noclip(ent);
		break;
	case QHMOVETYPE_STEP:
		SV_Physics_Step(ent);
		break;
	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
	case QHMOVETYPE_FLY:
	case QHMOVETYPE_FLYMISSILE:
		SV_Physics_Toss(ent);
		break;
	default:
		SV_Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
	}
}

/*
================
SV_RunNewmis

================
*/
void SV_RunNewmis(void)
{
	qhedict_t* ent;

	if (!*pr_globalVars.newmis)
	{
		return;
	}
	ent = PROG_TO_EDICT(*pr_globalVars.newmis);
	host_frametime = 0.05;
	*pr_globalVars.newmis = 0;

	SV_RunEntity(ent);
}

/*
================
SV_Physics

================
*/
void SV_Physics(void)
{
	int i;
	qhedict_t* ent;
	static double old_time;

// don't bother running a frame if sys_ticrate seconds haven't passed
	host_frametime = realtime - old_time;
	if (host_frametime < sv_mintic->value)
	{
		return;
	}
	if (host_frametime > sv_maxtic->value)
	{
		host_frametime = sv_maxtic->value;
	}
	old_time = realtime;

	*pr_globalVars.frametime = host_frametime;

	SV_ProgStartFrame();

//
// treat each object in turn
// even the world gets a chance to think
//
	ent = sv.qh_edicts;
	for (i = 0; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}

		if (*pr_globalVars.force_retouch)
		{
			SVQH_LinkEdict(ent, true);	// force retouch even for stationary

		}
		if (i > 0 && i <= MAX_CLIENTS_QW)
		{
			continue;		// clients are run directly from packets

		}
		SV_RunEntity(ent);
		SV_RunNewmis();
	}

	if (*pr_globalVars.force_retouch)
	{
		(*pr_globalVars.force_retouch)--;
	}
}

void SV_SetMoveVars(void)
{
	movevars.gravity            = sv_gravity->value;
	movevars.stopspeed          = sv_stopspeed->value;
	movevars.maxspeed           = sv_maxspeed->value;
	movevars.spectatormaxspeed  = sv_spectatormaxspeed->value;
	movevars.accelerate         = sv_accelerate->value;
	movevars.airaccelerate      = sv_airaccelerate->value;
	movevars.wateraccelerate    = sv_wateraccelerate->value;
	movevars.friction           = sv_friction->value;
	movevars.waterfriction      = sv_waterfriction->value;
	movevars.entgravity         = 1.0;
}
