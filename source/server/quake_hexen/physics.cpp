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

/*
pushmove objects do not obey gravity, and do not interact with each other or
trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are QHSOLID_BSP, and QHMOVETYPE_PUSH
bonus items are QHSOLID_TRIGGER touch, and QHMOVETYPE_TOSS
corpses are QHSOLID_NOT and QHMOVETYPE_TOSS
crates are QHSOLID_BBOX and QHMOVETYPE_TOSS
walking monsters are QHSOLID_SLIDEBOX and QHMOVETYPE_STEP
flying/floating monsters are QHSOLID_SLIDEBOX and QHMOVETYPE_FLY

solid_edge items only clip against bsp models.
*/

#include "../server.h"
#include "../progsvm/progsvm.h"
#include "local.h"

Cvar* svqh_gravity;
Cvar* svqh_stopspeed;
Cvar* svqh_maxspeed;
Cvar* svqh_accelerate;
Cvar* svqh_friction;
static Cvar* svqh_maxvelocity;
static Cvar* svqh_spectatormaxspeed;
static Cvar* svqh_airaccelerate;
static Cvar* svqh_wateraccelerate;
static Cvar* svqh_waterfriction;
static Cvar* svqh_nostep;

static Cvar* svqh_mintic;
static Cvar* svqh_maxtic;

void SVQH_RegisterPhysicsCvars()
{
	svqh_stopspeed = Cvar_Get("sv_stopspeed", "100", 0);
	svqh_accelerate = Cvar_Get("sv_accelerate", "10", 0);
	svqh_maxvelocity = Cvar_Get("sv_maxvelocity", "2000", 0);
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		svqh_gravity = Cvar_Get("sv_gravity", "800", 0);
		svqh_friction = Cvar_Get("sv_friction", "4", 0);
		svqh_spectatormaxspeed = Cvar_Get("sv_spectatormaxspeed", "500", 0);
		svqh_airaccelerate = Cvar_Get("sv_airaccelerate", "0.7", 0);
		svqh_wateraccelerate = Cvar_Get("sv_wateraccelerate", "10", 0);
		// bound the size of the physics time tic
		svqh_mintic = Cvar_Get("sv_mintic", "0.03", 0);
		svqh_maxtic = Cvar_Get("sv_maxtic", "0.1", 0);
		if (GGameType & GAME_Hexen2)
		{
			svqh_maxspeed = Cvar_Get("sv_maxspeed", "360", CVAR_SERVERINFO);
			svqh_waterfriction = Cvar_Get("sv_waterfriction", "1", 0);
		}
		else
		{
			svqh_maxspeed = Cvar_Get("sv_maxspeed", "320", 0);
			svqh_waterfriction = Cvar_Get("sv_waterfriction", "4", 0);
		}
	}
	else
	{
		svqh_gravity = Cvar_Get("sv_gravity", "800", CVAR_SERVERINFO);
		svqh_friction = Cvar_Get("sv_friction", "4", CVAR_SERVERINFO);
		svqh_nostep = Cvar_Get("sv_nostep", "0", 0);
		if (GGameType & GAME_Hexen2)
		{
			svqh_maxspeed = Cvar_Get("sv_maxspeed", "640", CVAR_SERVERINFO);
		}
		else
		{
			svqh_maxspeed = Cvar_Get("sv_maxspeed", "320", CVAR_SERVERINFO);
		}
	}

}

void SVQH_SetMoveVars()
{
	movevars.gravity = svqh_gravity->value;
	movevars.stopspeed = svqh_stopspeed->value;
	movevars.maxspeed = svqh_maxspeed->value;
	movevars.accelerate = svqh_accelerate->value;
	movevars.friction = svqh_friction->value;
	movevars.entgravity = 1.0;
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		movevars.spectatormaxspeed = svqh_spectatormaxspeed->value;
		movevars.airaccelerate = svqh_airaccelerate->value;
		movevars.wateraccelerate = svqh_wateraccelerate->value;
		movevars.waterfriction = svqh_waterfriction->value;
	}
}

static void SVQH_CheckVelocity(qhedict_t* ent)
{
	// bound velocity
	for (int i = 0; i < 3; i++)
	{
		if (IS_NAN(ent->GetVelocity()[i]))
		{
			common->Printf("Got a NaN velocity on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetVelocity()[i] = 0;
		}
		if (IS_NAN(ent->GetOrigin()[i]))
		{
			common->Printf("Got a NaN origin on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetOrigin()[i] = 0;
		}
		if (ent->GetVelocity()[i] > svqh_maxvelocity->value)
		{
			ent->GetVelocity()[i] = svqh_maxvelocity->value;
		}
		else if (ent->GetVelocity()[i] < -svqh_maxvelocity->value)
		{
			ent->GetVelocity()[i] = -svqh_maxvelocity->value;
		}
	}
}

//	Runs thinking code if time.  There is some play in the exact time the think
// function will be called, because it is called before any movement is done
// in a frame.  Not used for pushmove objects, because they must be exact.
// Returns false if the entity removed itself.
bool SVQH_RunThink(qhedict_t* ent, float frametime)
{
	float thinktime;

	do
	{
		thinktime = ent->GetNextThink();
		if (thinktime <= 0)
		{
			return true;
		}
		if (thinktime > sv.qh_time + frametime)
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
	while (GGameType & GAME_QuakeWorld);

	return true;
}

//	Two entities have touched, so run their touch functions
static void SVQH_Impact(qhedict_t* e1, qhedict_t* e2)
{
	int old_self = *pr_globalVars.self;
	int old_other = *pr_globalVars.other;

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

//	The basic solid body movement clip that slides along multiple planes
// Returns the clipflags if the velocity was modified (hit something solid)
// 1 = floor
// 2 = wall / step
// 4 = dead stop
// If steptrace is not NULL, the trace of any vertical wall hit will be stored
static int SVQH_FlyMove(qhedict_t* ent, float time, q1trace_t* steptrace)
{
	int numbumps = 4;

	int blocked = 0;
	vec3_t original_velocity;
	VectorCopy(ent->GetVelocity(), original_velocity);
	vec3_t primal_velocity;
	VectorCopy(ent->GetVelocity(), primal_velocity);
	vec3_t planes[MAX_FLY_MOVE_CLIP_PLANES];
	int numplanes = 0;

	float time_left = time;

	for (int bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (!(GGameType & GAME_QuakeWorld) && !ent->GetVelocity()[0] && !ent->GetVelocity()[1] && !ent->GetVelocity()[2])
		{
			break;
		}

		vec3_t end;
		for (int i = 0; i < 3; i++)
		{
			end[i] = ent->GetOrigin()[i] + time_left* ent->GetVelocity()[i];
		}

		q1trace_t trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

		if (trace.allsolid)
		{
			// entity is trapped in another solid
			ent->SetVelocity(vec3_origin);
			return 3;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, ent->GetOrigin());
			VectorCopy(ent->GetVelocity(), original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
		{
			// moved the entire distance
			break;
		}
		if (trace.entityNum < 0)
		{
			common->Error("SVQH_FlyMove: !trace.ent");
		}

		if (trace.plane.normal[2] > 0.7)
		{
			// floor
			blocked |= 1;
			if (QH_EDICT_NUM(trace.entityNum)->GetSolid() == QHSOLID_BSP)
			{
				ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);
				ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum)));
			}
		}
		if (!trace.plane.normal[2])
		{
			// step
			blocked |= 2;
			if (steptrace)
			{
				// save for player extrafriction
				*steptrace = trace;
			}
		}

		// run the impact function
		SVQH_Impact(ent, QH_EDICT_NUM(trace.entityNum));
		if (ent->free)
		{
			break;		// removed by the impact function
		}
		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_FLY_MOVE_CLIP_PLANES)
		{
			// this shouldn't really happen
			ent->SetVelocity(vec3_origin);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		vec3_t new_velocity;
		int i;
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			int j;
			for (j = 0; j < numplanes; j++)
			{
				if (j != i)
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
					{
						// not ok
						break;
					}
				}
			}
			if (j == numplanes)
			{
				break;
			}
		}

		if (i != numplanes)
		{
			// go along this plane
			ent->SetVelocity(new_velocity);
		}
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
				ent->SetVelocity(vec3_origin);
				return 7;
			}
			vec3_t dir;
			CrossProduct(planes[0], planes[1], dir);
			float d = DotProduct(dir, ent->GetVelocity());
			VectorScale(dir, d, ent->GetVelocity());
		}

		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct(ent->GetVelocity(), primal_velocity) <= 0)
		{
			ent->SetVelocity(vec3_origin);
			return blocked;
		}
	}

	return blocked;
}

static void SVQH_FlyExtras(qhedict_t* ent)
{
	const float hoverinc = 0.4;

	ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);	// Jumping makes you loose this flag so reset it

	if ((ent->GetVelocity()[2] <= 6) && (ent->GetVelocity()[2] >= -6))
	{
		ent->GetVelocity()[2] += ent->GetHoverZ();

		if (ent->GetVelocity()[2] >= 6)
		{
			ent->SetHoverZ(-hoverinc);
			ent->GetVelocity()[2] += ent->GetHoverZ();
		}
		else if (ent->GetVelocity()[2] <= -6)
		{
			ent->SetHoverZ(hoverinc);
			ent->GetVelocity()[2] += ent->GetHoverZ();
		}
	}
	else	// friction for upward or downward progress once key is released
	{
		ent->GetVelocity()[2] -= ent->GetVelocity()[2] * 0.1;
	}
}

static void SVQH_AddGravity(qhedict_t* ent, float frametime)
{
	float ent_gravity = 1.0f;
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		eval_t* val = GetEdictFieldValue(ent, "gravity");
		if (val && val->_float)
		{
			ent_gravity = val->_float;
		}
	}
	ent->GetVelocity()[2] -= ent_gravity * movevars.gravity * frametime;
}

//	Does not change the entities velocity at all
static q1trace_t SVQH_PushEntity(qhedict_t* ent, const vec3_t push)
{
	vec3_t end;
	VectorAdd(ent->GetOrigin(), push, end);

	q1trace_t trace;
	if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE ||
		(GGameType & GAME_Hexen2 && ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE))
	{
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_MISSILE, ent);
	}
	else if (ent->GetSolid() == QHSOLID_TRIGGER || ent->GetSolid() == QHSOLID_NOT)
	{
		// only clip against bmodels
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_NOMONSTERS, ent);
	}
	else if (GGameType & GAME_Hexen2 && ent->GetMoveType() == H2MOVETYPE_SWIM)
	{
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, H2MOVE_WATER, ent);
	}
	else
	{
		trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, QHMOVE_NORMAL, ent);
	}

	if (GGameType & GAME_Hexen2)
	{
		if (ent->GetSolid() != H2SOLID_PHASE)
		{
			if (ent->GetMoveType() != QHMOVETYPE_BOUNCE || (trace.allsolid == 0 && trace.startsolid == 0))
			{
				ent->SetOrigin(trace.endpos);		// Macro - watchout
			}
			else
			{
				trace.fraction = 0;

				return trace;
			}
		}
		else	// Entity is PHASED so bounce off walls and other entities, go through monsters and players
		{
			if (trace.entityNum >= 0)
			{
				// Go through MONSTERS and PLAYERS, can't use QHFL_CLIENT cause rotating brushes do
				if (((int)QH_EDICT_NUM(trace.entityNum)->GetFlags() & QHFL_MONSTER) ||
					(QH_EDICT_NUM(trace.entityNum)->GetMoveType() == QHMOVETYPE_WALK))
				{
					vec3_t impact;
					VectorCopy(trace.endpos, impact);
					qhedict_t* impact_e = QH_EDICT_NUM(trace.entityNum);

					trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, H2MOVE_PHASE, ent);

					VectorCopy(impact, ent->GetOrigin());
					SVQH_Impact(ent, impact_e);

					ent->SetOrigin(trace.endpos);
				}
				else
				{
					ent->SetOrigin(trace.endpos);
				}
			}
			else
			{
				ent->SetOrigin(trace.endpos);
			}
		}
	}
	else
	{
		ent->SetOrigin(trace.endpos);
	}

	SVQH_LinkEdict(ent, true);

	if (trace.entityNum >= 0)
	{
		SVQH_Impact(ent, QH_EDICT_NUM(trace.entityNum));
	}

	return trace;
}

static bool SVQH_Push(qhedict_t* pusher, const vec3_t move)
{
	vec3_t mins, maxs;
	for (int i = 0; i < 3; i++)
	{
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	vec3_t pushorig;
	VectorCopy(pusher->GetOrigin(), pushorig);

	// move the pusher to it's final position
	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	SVQH_LinkEdict(pusher, false);

	// see if any solid entities are inside the final position
	int num_moved = 0;
	qhedict_t* moved_edict[MAX_EDICTS_QH];
	vec3_t moved_from[MAX_EDICTS_QH];
	qhedict_t* check = NEXT_EDICT(sv.qh_edicts);
	for (int e = 1; e < sv.qh_num_edicts; e++, check = NEXT_EDICT(check))
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

		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			pusher->SetSolid(QHSOLID_NOT);
			qhedict_t* block = SVQH_TestEntityPosition(check);
			pusher->SetSolid(QHSOLID_BSP);
			if (block)
			{
				continue;
			}
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

		// remove the onground flag for non-players
		if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) && check->GetMoveType() != QHMOVETYPE_WALK)
		{
			check->SetFlags((int)check->GetFlags() & ~QHFL_ONGROUND);
		}

		vec3_t entorig;
		VectorCopy(check->GetOrigin(), entorig);
		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			VectorAdd(check->GetOrigin(), move, check->GetOrigin());

			qhedict_t* block = SVQH_TestEntityPosition(check);
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
		}
		else
		{
			pusher->SetSolid(QHSOLID_NOT);
			SVQH_PushEntity(check, move);
			pusher->SetSolid(QHSOLID_BSP);

			qhedict_t* block = SVQH_TestEntityPosition(check);
			if (!block)
			{
				continue;
			}
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

		if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
		{
			VectorCopy(entorig, check->GetOrigin());
			SVQH_LinkEdict(check, true);
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
		for (int i = 0; i < num_moved; i++)
		{
			VectorCopy(moved_from[i], moved_edict[i]->GetOrigin());
			SVQH_LinkEdict(moved_edict[i], false);
		}
		return false;
	}

	return true;
}

static void SVQH_PushMove(qhedict_t* pusher, float movetime)
{
	if (!pusher->GetVelocity()[0] && !pusher->GetVelocity()[1] && !pusher->GetVelocity()[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	vec3_t move;
	for (int i = 0; i < 3; i++)
	{
		move[i] = pusher->GetVelocity()[i] * movetime;
	}

	if (SVQH_Push(pusher, move))
	{
		pusher->v.ltime += movetime;
	}
}

static void SVQH_PushRotate(qhedict_t* pusher, float movetime)
{
	int i, e, t;
	qhedict_t* check, * block;
	vec3_t entorig, pushorig,pushorigangles;
	int num_moved;
	qhedict_t* moved_edict[MAX_EDICTS_QH];
	vec3_t moved_from[MAX_EDICTS_QH];
	vec3_t org, org2, check_center;
	vec3_t forward, right, up;
	qhedict_t* ground;
	qhedict_t* master;
	qhedict_t* slave;
	int slaves_moved;
	qboolean moveit;

	vec3_t move, amove, mins, maxs;
	for (i = 0; i < 3; i++)
	{
		amove[i] = pusher->GetAVelocity()[i] * movetime;
		move[i] = pusher->GetVelocity()[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	vec3_t a;
	VectorSubtract(vec3_origin, amove, a);
	AngleVectors(a, forward, right, up);

	VectorCopy(pusher->GetOrigin(), pushorig);
	VectorCopy(pusher->GetAngles(), pushorigangles);

	// move the pusher to it's final position
	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	VectorAdd(pusher->GetAngles(), amove, pusher->GetAngles());

	pusher->v.ltime += movetime;
	SVQH_LinkEdict(pusher, false);

	master = pusher;
	slaves_moved = 0;

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
			check->GetMoveType() == H2MOVETYPE_FOLLOW ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		moveit = false;
		ground = PROG_TO_EDICT(check->GetGroundEntity());
		if ((int)check->GetFlags() & QHFL_ONGROUND)
		{
			if (ground == pusher)
			{
				moveit = true;
			}
			else
			{
				for (i = 0; i < slaves_moved; i++)
				{
					if (ground == moved_edict[MAX_EDICTS_QH - i - 1])
					{
						moveit = true;
						break;
					}
				}
			}
		}

		if (!moveit)
		{
			if (check->v.absmin[0] >= maxs[0] ||
				check->v.absmin[1] >= maxs[1] ||
				check->v.absmin[2] >= maxs[2] ||
				check->v.absmax[0] <= mins[0] ||
				check->v.absmax[1] <= mins[1] ||
				check->v.absmax[2] <= mins[2])
			{
				for (i = 0; i < slaves_moved; i++)
				{
					slave = moved_edict[MAX_EDICTS_QH - i - 1];
					if (check->v.absmin[0] >= slave->v.absmax[0] ||
						check->v.absmin[1] >= slave->v.absmax[1] ||
						check->v.absmin[2] >= slave->v.absmax[2] ||
						check->v.absmax[0] <= slave->v.absmin[0] ||
						check->v.absmax[1] <= slave->v.absmin[1] ||
						check->v.absmax[2] <= slave->v.absmin[2])
					{
						continue;
					}
				}
				if (i == slaves_moved)
				{
					continue;
				}
			}

			// see if the ent's bbox is inside the pusher's final position
			if (!SVQH_TestEntityPosition(check))
			{
				continue;
			}
		}

		// remove the onground flag for non-players
		if (check->GetMoveType() != QHMOVETYPE_WALK)
		{
			check->SetFlags((int)check->GetFlags() & ~QHFL_ONGROUND);
		}

		VectorCopy(check->GetOrigin(), entorig);
		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		//put check in first move spot
		VectorAdd(check->GetOrigin(), move, check->GetOrigin());
		//Use center of model, like in QUAKE!!!!  Our origins are on the bottom!!!
		for (i = 0; i < 3; i++)
		{
			check_center[i] = (check->v.absmin[i] + check->v.absmax[i]) / 2;
		}
		// calculate destination position
		VectorSubtract(check_center, pusher->GetOrigin(), org);
		//put check back
		VectorSubtract(check->GetOrigin(), move, check->GetOrigin());
		org2[0] = DotProduct(org, forward);
		org2[1] = -DotProduct(org, right);
		org2[2] = DotProduct(org, up);
		vec3_t move2;
		VectorSubtract(org2, org, move2);

		//Add all moves together
		vec3_t move3;
		VectorAdd(move,move2,move3);

		// try moving the contacted entity
		vec3_t testmove;
		for (t = 0; t < 13; t++)
		{
			switch (t)
			{
			case 0:
				//try x, y and z
				VectorCopy(move3,testmove);
				break;
			case 1:
				//Try xy only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = 0;
				break;
			case 2:
				//Try z only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = move3[2];
				break;
			case 3:
				//Try none
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = 0;
				break;
			case 4:
				//Try xy in opposite dir
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2];
				break;
			case 5:
				//Try z in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = move3[2] * -1;
				break;
			case 6:
				//Try xyz in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2] * -1;
				break;
			case 7:
				//Try move3 times 2
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				VectorScale(move3,2,testmove);
				break;
			case 8:
				//Try normalized org
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				VectorScale(org,movetime,org);		//movetime*20?
				VectorCopy(org,testmove);
				break;
			case 9:
				//Try normalized org z * 3 only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = org[2] * 3;	//was: +org[2]*(Q_fabs(org[1])+Q_fabs(org[2]));
				break;
			case 10:
				//Try normalized org xy * 2 only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0] * 2;	//was: +org[0]*Q_fabs(org[2]);
				testmove[1] = org[1] * 2;	//was: +org[1]*Q_fabs(org[2]);
				testmove[2] = 0;
				break;
			case 11:
				//Try xy in opposite org dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0] * -2;
				testmove[1] = org[1] * -2;
				testmove[2] = org[2];
				break;
			case 12:
				//Try z in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0];
				testmove[1] = org[1];
				testmove[2] = org[2] * -3;
				break;
			}

			if (t != 3)
			{
				//THIS IS VERY BAD BAD HACK...
				pusher->SetSolid(QHSOLID_NOT);
				SVQH_PushEntity(check, move3);
				check->GetAngles()[YAW] += amove[YAW];
				pusher->SetSolid(QHSOLID_BSP);
			}
			// if it is still inside the pusher, block
			block = SVQH_TestEntityPosition(check);
			if (!block)
			{
				break;
			}
		}

		if (block)
		{
			// fail the move
			if (check->GetMins()[0] == check->GetMaxs()[0])
			{
				continue;
			}
			if (check->GetSolid() == QHSOLID_NOT || check->GetSolid() == QHSOLID_TRIGGER)
			{
				// corpse
				check->GetMins()[0] = check->GetMins()[1] = 0;
				check->SetMaxs(check->GetMins());
				continue;
			}

			VectorCopy(entorig, check->GetOrigin());
			SVQH_LinkEdict(check, true);

			VectorCopy(pushorig, pusher->GetOrigin());
			pusher->SetAngles(pushorigangles);
			SVQH_LinkEdict(pusher, false);
			pusher->v.ltime -= movetime;

			for (i = 0; i < slaves_moved; i++)
			{
				slave = moved_edict[MAX_EDICTS_QH - i - 1];
				slave->SetAngles(moved_from[MAX_EDICTS_QH - i - 1]);
				SVQH_LinkEdict(slave, false);
				slave->v.ltime -= movetime;
			}

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
				moved_edict[i]->GetAngles()[YAW] -= amove[YAW];

				SVQH_LinkEdict(moved_edict[i], false);
			}
			return;
		}
	}
}

static void SVQH_Physics_Pusher(qhedict_t* ent, float frametime)
{
	float oldltime = ent->v.ltime;

	float thinktime = ent->GetNextThink();
	float movetime;
	if (thinktime < ent->v.ltime + frametime)
	{
		movetime = thinktime - ent->v.ltime;
		if (movetime < 0)
		{
			movetime = 0;
		}
	}
	else
	{
		movetime = frametime;
	}

	if (movetime)
	{
		if (GGameType & GAME_Hexen2 && (ent->GetAVelocity()[0] || ent->GetAVelocity()[1] || ent->GetAVelocity()[2]))
		{
			SVQH_PushRotate(ent, movetime);
		}
		else
		{
			// advances ent->v.ltime if not blocked
			SVQH_PushMove(ent, movetime);
		}
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		vec3_t oldorg;
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

		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			vec3_t move;
			VectorSubtract(ent->GetOrigin(), oldorg, move);

			float l = VectorLength(move);
			if (l > 1.0 / 64)
			{
				VectorCopy(oldorg, ent->GetOrigin());
				SVQH_Push(ent, move);
			}
		}
	}
}

//	Non moving objects can only think
static void SVQH_Physics_None(qhedict_t* ent, float frametime)
{
	// regular thinking
	SVQH_RunThink(ent, frametime);
}

//	A moving object that doesn't obey physics
static void SVQH_Physics_Noclip(qhedict_t* ent, float frametime)
{
	// regular thinking
	if (!SVQH_RunThink(ent, frametime))
	{
		return;
	}

	VectorMA(ent->GetAngles(), frametime, ent->GetAVelocity(), ent->GetAngles());
	VectorMA(ent->GetOrigin(), frametime, ent->GetVelocity(), ent->GetOrigin());

	SVQH_LinkEdict(ent, false);
}

static void SVQH_CheckWaterTransition(qhedict_t* ent)
{
	int cont = SVQH_PointContents(ent->GetOrigin());
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
			SVQH_StartSound(ent, 0, GGameType & GAME_Hexen2 ? "misc/hith2o.wav" : "misc/h2ohit1.wav", 255, 1);
		}
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
	}
	else
	{
		if (ent->GetWaterType() != BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SVQH_StartSound(ent, 0, GGameType & GAME_Hexen2 ? "misc/hith2o.wav" : "misc/h2ohit1.wav", 255, 1);
		}
		ent->SetWaterType(BSP29CONTENTS_EMPTY);
		ent->SetWaterLevel(cont);
	}
}

//	Toss, bounce, and fly movement.  When onground, do nothing.
static void SVQH_Physics_Toss(qhedict_t* ent, float frametime)
{
	// regular thinking
	if (!SVQH_RunThink(ent, frametime))
	{
		return;
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && ent->GetVelocity()[2] > 0)
	{
		ent->SetFlags((int)ent->GetFlags() & ~QHFL_ONGROUND);
	}

	// if onground, return without moving
	if (((int)ent->GetFlags() & QHFL_ONGROUND))
	{
		return;
	}

	SVQH_CheckVelocity(ent);

	// add gravity
	if (ent->GetMoveType() != QHMOVETYPE_FLY &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE &&
		(!(GGameType & GAME_Hexen2) ||
		(ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE &&
		ent->GetMoveType() != H2MOVETYPE_SWIM)))
	{
		SVQH_AddGravity(ent, frametime);
	}

	// move angles
	VectorMA(ent->GetAngles(), frametime, ent->GetAVelocity(), ent->GetAngles());

	// move origin
	vec3_t move;
	VectorScale(ent->GetVelocity(), frametime, move);
	q1trace_t trace = SVQH_PushEntity(ent, move);
	if (trace.fraction == 1)
	{
		return;
	}
	if (ent->free)
	{
		return;
	}

	float backoff;
	if (ent->GetMoveType() == QHMOVETYPE_BOUNCE)
	{
		backoff = 1.5;
	}
	else if (GGameType & GAME_Hexen2 && ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE)
	{	// Solid phased missiles don't bounce on monsters or players
		if ((ent->GetSolid() == H2SOLID_PHASE) && (((int)QH_EDICT_NUM(trace.entityNum)->GetFlags() & QHFL_MONSTER) || ((int)QH_EDICT_NUM(trace.entityNum)->GetMoveType() == QHMOVETYPE_WALK)))
		{
			return;
		}
		backoff = 2.0;
	}
	else
	{
		backoff = 1;
	}

	PM_ClipVelocity(ent->GetVelocity(), trace.plane.normal, ent->GetVelocity(), backoff);

	// stop if on ground
	if (trace.plane.normal[2] > 0.7 && (!(GGameType & GAME_Hexen2) || ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE))
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
	SVQH_CheckWaterTransition(ent);
}

//	Monsters freefall when they don't have a ground entity, otherwise
// all movement is done with discrete steps.
//	This is also used for objects that have become still on the ground, but
// will fall if the floor is pulled out from under them.
static void SVQH_Physics_Step(qhedict_t* ent, float frametime)
{
	// freefall if not onground
	if (!((int)ent->GetFlags() & (QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM)))
	{
		bool hitsound;
		if (ent->GetVelocity()[2] < movevars.gravity * -0.1)
		{
			hitsound = true;
		}
		else
		{
			hitsound = false;
		}

		SVQH_AddGravity(ent, frametime);
		SVQH_CheckVelocity(ent);
		SVQH_FlyMove(ent, frametime, NULL);
		SVQH_LinkEdict(ent, true);

		if (!(GGameType & GAME_Hexen2) && (int)ent->GetFlags() & QHFL_ONGROUND)
		{
			// just hit ground
			if (hitsound)
			{
				SVQH_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
			}
		}
	}

	// regular thinking
	SVQH_RunThink(ent, frametime);

	SVQH_CheckWaterTransition(ent);
}

//	This is a big hack to try and fix the rare case of getting stuck in the world
// clipping hull.
static void SVQH_CheckStuck(qhedict_t* ent)
{
	if (!SVQH_TestEntityPosition(ent))
	{
		VectorCopy(ent->GetOrigin(), ent->GetOldOrigin());
		return;
	}

	vec3_t org;
	VectorCopy(ent->GetOrigin(), org);
	VectorCopy(ent->GetOldOrigin(), ent->GetOrigin());
	if (!SVQH_TestEntityPosition(ent))
	{
		common->DPrintf("Unstuck.\n");
		SVQH_LinkEdict(ent, true);
		return;
	}

	for (int z = 0; z < 18; z++)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				ent->GetOrigin()[0] = org[0] + i;
				ent->GetOrigin()[1] = org[1] + j;
				ent->GetOrigin()[2] = org[2] + z;
				if (!SVQH_TestEntityPosition(ent))
				{
					common->DPrintf("Unstuck.\n");
					SVQH_LinkEdict(ent, true);
					return;
				}
			}
		}
	}

	VectorCopy(org, ent->GetOrigin());
	common->DPrintf("player is stuck.\n");
}

static bool SVQH_CheckWater(qhedict_t* ent)
{
	vec3_t point;
	point[0] = ent->GetOrigin()[0];
	point[1] = ent->GetOrigin()[1];
	point[2] = ent->GetOrigin()[2] + ent->GetMins()[2] + 1;

	ent->SetWaterLevel(0);
	ent->SetWaterType(BSP29CONTENTS_EMPTY);
	int cont = SVQH_PointContents(point);
	if (cont <= BSP29CONTENTS_WATER)
	{
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
		point[2] = ent->GetOrigin()[2] + (ent->GetMins()[2] + ent->GetMaxs()[2]) * 0.5;
		cont = SVQH_PointContents(point);
		if (cont <= BSP29CONTENTS_WATER)
		{
			ent->SetWaterLevel(2);
			point[2] = ent->GetOrigin()[2] + ent->GetViewOfs()[2];
			cont = SVQH_PointContents(point);
			if (cont <= BSP29CONTENTS_WATER)
			{
				ent->SetWaterLevel(3);
			}
		}
	}

	return ent->GetWaterLevel() > 1;
}

static void SVQH_WallFriction(qhedict_t* ent, const q1trace_t* trace)
{
	vec3_t forward, right, up;
	AngleVectors(ent->GetVAngle(), forward, right, up);
	float d = DotProduct(trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0)
	{
		return;
	}

	// cut the tangential velocity
	float i = DotProduct(trace->plane.normal, ent->GetVelocity());
	vec3_t into;
	VectorScale(trace->plane.normal, i, into);
	vec3_t side;
	VectorSubtract(ent->GetVelocity(), into, side);

	ent->GetVelocity()[0] = side[0] * (1 + d);
	ent->GetVelocity()[1] = side[1] * (1 + d);
}

//	Player has come to a dead stop, possibly due to the problem with limited
// float precision at some angle joins in the BSP hull.
//	Try fixing by pushing one pixel in each direction.
//	This is a hack, but in the interest of good gameplay...
static int SVQH_TryUnstick(qhedict_t* ent, const vec3_t oldvel)
{
	vec3_t oldorg;
	VectorCopy(ent->GetOrigin(), oldorg);
	vec3_t dir;
	VectorCopy(vec3_origin, dir);

	for (int i = 0; i < 8; i++)
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

		SVQH_PushEntity(ent, dir);

		// retry the original move
		ent->GetVelocity()[0] = oldvel[0];
		ent->GetVelocity()[1] = oldvel[1];
		ent->GetVelocity()[2] = 0;
		q1trace_t steptrace;
		int clip = SVQH_FlyMove(ent, 0.1, &steptrace);

		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) > 4 ||
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) > 4)
		{
			return clip;
		}

		// go back to the original pos and try again
		VectorCopy(oldorg, ent->GetOrigin());
	}

	ent->SetVelocity(vec3_origin);
	// still not moving
	return 7;
}

//	Only used by players
static void SVQH_WalkMove(qhedict_t* ent, float frametime)
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
	oldonground = (int)ent->GetFlags() & QHFL_ONGROUND;
	ent->SetFlags((int)ent->GetFlags() & ~QHFL_ONGROUND);

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(ent->GetVelocity(), oldvel);

	clip = SVQH_FlyMove(ent, frametime, &steptrace);

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
	if (svqh_nostep->value)
	{
		return;
	}

	if ((int)ent->GetFlags() & QHFL_WATERJUMP)
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
	downmove[2] = -STEPSIZE + oldvel[2] * frametime;

	// move up
	SVQH_PushEntity(ent, upmove);		// FIXME: don't link?

	// move forward
	ent->GetVelocity()[0] = oldvel[0];
	ent->GetVelocity()[1] = oldvel[1];
	ent->GetVelocity()[2] = 0;
	clip = SVQH_FlyMove(ent, frametime, &steptrace);

	// check for stuckness, possibly due to the limited precision of floats
	// in the clipping hulls
	if (clip)
	{
		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) < 0.03125 &&
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) < 0.03125)
		{
			// stepping up didn't make any progress
			clip = SVQH_TryUnstick(ent, oldvel);
		}
	}

	// extra friction based on view angle
	if (clip & 2)
	{
		SVQH_WallFriction(ent, &steptrace);
	}

	// move down
	downtrace = SVQH_PushEntity(ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->GetSolid() == QHSOLID_BSP)
		{
			ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(downtrace.entityNum)));
		}
	}
	else
	{
		// if the push down didn't end up on good ground, use the move without
		// the step up.  This happens near wall / slope combinations, and can
		// cause the player to hop up higher on a slope too steep to climb
		ent->SetOrigin(nosteporg);
		ent->SetVelocity(nostepvel);
	}
}

//	Player character actions
static void SVQH_Physics_Client(qhedict_t* ent, int num, float frametime)
{
	if (svs.clients[num - 1].state < CS_CONNECTED)
	{
		// unconnected slot
		return;
	}

	//
	// call standard client pre-think
	//
	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(*pr_globalVars.PlayerPreThink);

	//
	// do a move
	//
	SVQH_CheckVelocity(ent);

	//
	// decide which move function to call
	//
	switch ((int)ent->GetMoveType())
	{
	case QHMOVETYPE_NONE:
		if (!SVQH_RunThink(ent, frametime))
		{
			return;
		}
		break;

	case QHMOVETYPE_WALK:
		if (!SVQH_RunThink(ent, frametime))
		{
			return;
		}
		if (!SVQH_CheckWater(ent) && !((int)ent->GetFlags() & QHFL_WATERJUMP))
		{
			SVQH_AddGravity(ent, frametime);
		}
		SVQH_CheckStuck(ent);
		SVQH_WalkMove(ent, frametime);
		break;

	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
		SVQH_Physics_Toss(ent, frametime);
		break;

	case H2MOVETYPE_SWIM:
		if (!(GGameType & GAME_Hexen2))
		{
			common->Error("SV_Physics_client: bad movetype %i", (int)ent->GetMoveType());
		}
	case QHMOVETYPE_FLY:
		if (!SVQH_RunThink(ent, frametime))
		{
			return;
		}
		if (GGameType & GAME_Hexen2)
		{
			SVQH_CheckWater(ent);
		}
		SVQH_FlyMove(ent, frametime, NULL);
		if (GGameType & GAME_Hexen2)
		{
			SVQH_FlyExtras(ent);	// Hover & friction
		}
		break;

	case QHMOVETYPE_NOCLIP:
		if (!SVQH_RunThink(ent, frametime))
		{
			return;
		}
		VectorMA(ent->GetOrigin(), frametime, ent->GetVelocity(), ent->GetOrigin());
		break;

	default:
		common->Error("SV_Physics_client: bad movetype %i", (int)ent->GetMoveType());
	}

	//
	// call standard player post-think
	//
	SVQH_LinkEdict(ent, true);

	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(*pr_globalVars.PlayerPostThink);
}

void SVQH_ProgStartFrame()
{
	// let the progs know that a new frame has started
	*pr_globalVars.self = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.other = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.time = sv.qh_time;
	PR_ExecuteProgram(*pr_globalVars.StartFrame);
}

static void SVQH_RunEntity(qhedict_t* ent, int i, float frametime, float realtime)
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		if (ent->GetLastRunTime() == (float)realtime)
		{
			return;
		}
		ent->SetLastRunTime((float)realtime);
	}

	qhedict_t* ent2 = GGameType & GAME_Hexen2 ? PROG_TO_EDICT(ent->GetMoveChain()) : sv.qh_edicts;
	vec3_t oldOrigin, oldAngle;
	if (ent2 != sv.qh_edicts)
	{
		VectorCopy(ent->GetOrigin(), oldOrigin);
		VectorCopy(ent->GetAngles(), oldAngle);
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) &&
		i > 0 && i <= svs.qh_maxclients)
	{
		SVQH_Physics_Client(ent, i, frametime);
	}
	else
	{
		switch ((int)ent->GetMoveType())
		{
		case QHMOVETYPE_PUSH:
			SVQH_Physics_Pusher(ent, frametime);
			break;
		case QHMOVETYPE_NONE:
			SVQH_Physics_None(ent, frametime);
			break;
		case QHMOVETYPE_NOCLIP:
			SVQH_Physics_Noclip(ent, frametime);
			break;
		case H2MOVETYPE_PUSHPULL:
			if (!(GGameType & GAME_Hexen2))
			{
				common->Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
			}
		case QHMOVETYPE_STEP:
			SVQH_Physics_Step(ent, frametime);
			break;
		case H2MOVETYPE_BOUNCEMISSILE:
		case H2MOVETYPE_SWIM:
			if (!(GGameType & GAME_Hexen2))
			{
				common->Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
			}
		case QHMOVETYPE_TOSS:
		case QHMOVETYPE_BOUNCE:
		case QHMOVETYPE_FLY:
		case QHMOVETYPE_FLYMISSILE:
			SVQH_Physics_Toss(ent, frametime);
			break;
		case H2MOVETYPE_FOLLOW:
			if (!(GGameType & GAME_HexenWorld))
			{
				common->Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
			}
			break;
		case QHMOVETYPE_WALK:
			if (!(GGameType & GAME_HexenWorld))
			{
				common->Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
			}
			SVQH_RunThink(ent, frametime);
			break;
		default:
			common->Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
		}
	}

	if (ent2 != sv.qh_edicts)
	{
		bool originMoved = !VectorCompare(ent->GetOrigin(),oldOrigin);
		if (originMoved || !VectorCompare(ent->GetAngles(),oldAngle))
		{
			VectorSubtract(ent->GetOrigin(),oldOrigin,oldOrigin);
			VectorSubtract(ent->GetAngles(),oldAngle,oldAngle);

			for (int c = 0; c < 10; c++)
			{	// chain a max of 10 objects
				if (ent2->free)
				{
					break;
				}

				VectorAdd(oldOrigin,ent2->GetOrigin(),ent2->GetOrigin());
				if ((int)ent2->GetFlags() & H2FL_MOVECHAIN_ANGLE)
				{
					VectorAdd(oldAngle,ent2->GetAngles(),ent2->GetAngles());
				}

				if (originMoved && ent2->GetChainMoved())
				{	// callback function
					*pr_globalVars.self = EDICT_TO_PROG(ent2);
					*pr_globalVars.other = EDICT_TO_PROG(ent);
					PR_ExecuteProgram(ent2->GetChainMoved());
				}

				ent2 = PROG_TO_EDICT(ent2->GetMoveChain());
				if (ent2 == sv.qh_edicts)
				{
					break;
				}

			}
		}
	}
}

void SVQH_RunNewmis(float realtime)
{
	if (!*pr_globalVars.newmis)
	{
		return;
	}
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.newmis);
	float host_frametime = 0.05;
	*pr_globalVars.newmis = 0;

	SVQH_RunEntity(ent, -1, host_frametime, realtime);
}

static void SVQH_Physics(float frametime, float realtime)
{
	SVQH_ProgStartFrame();

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	qhedict_t* ent = sv.qh_edicts;
	for (int i = 0; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}

		if (*pr_globalVars.force_retouch)
		{
			SVQH_LinkEdict(ent, true);	// force retouch even for stationary
		}

		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && i > 0 && i <= MAX_CLIENTS_QHW)
		{
			continue;		// clients are run directly from packets
		}

		SVQH_RunEntity(ent, i, frametime, realtime);
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			SVQH_RunNewmis(realtime);
		}
	}

	if (*pr_globalVars.force_retouch)
	{
		(*pr_globalVars.force_retouch)--;
	}
}

//	Quake and Hexen 2 behavior.
void SVQH_RunPhysicsAndUpdateTime(float frametime, float realtime)
{
	SVQH_SetMoveVars();

	SVQH_Physics(frametime, realtime);

	sv.qh_time += frametime;
}

void SVQH_RunPhysicsForTime(float realtime)
{
	static double old_time;

	// don't bother running a frame if sys_ticrate seconds haven't passed
	float frametime = realtime - old_time;
	if (frametime < svqh_mintic->value)
	{
		return;
	}
	if (frametime > svqh_maxtic->value)
	{
		frametime = svqh_maxtic->value;
	}
	old_time = realtime;

	*pr_globalVars.frametime = frametime;

	SVQH_Physics(frametime, realtime);
}
