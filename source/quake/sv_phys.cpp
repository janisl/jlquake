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

	if (!SVQH_TestEntityPosition(ent))
	{
		VectorCopy(ent->GetOrigin(), ent->GetOldOrigin());
		return;
	}

	VectorCopy(ent->GetOrigin(), org);
	VectorCopy(ent->GetOldOrigin(), ent->GetOrigin());
	if (!SVQH_TestEntityPosition(ent))
	{
		Con_DPrintf("Unstuck.\n");
		SVQH_LinkEdict(ent, true);
		return;
	}

	for (z = 0; z < 18; z++)
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
			{
				ent->GetOrigin()[0] = org[0] + i;
				ent->GetOrigin()[1] = org[1] + j;
				ent->GetOrigin()[2] = org[2] + z;
				if (!SVQH_TestEntityPosition(ent))
				{
					Con_DPrintf("Unstuck.\n");
					SVQH_LinkEdict(ent, true);
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
	cont = SVQH_PointContents(point);
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

		SVQH_PushEntity(ent, dir);

// retry the original move
		ent->GetVelocity()[0] = oldvel[0];
		ent->GetVelocity()[1] = oldvel[1];
		ent->GetVelocity()[2] = 0;
		clip = SVQH_FlyMove(ent, 0.1, &steptrace);

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
	oldonground = (int)ent->GetFlags() & QHFL_ONGROUND;
	ent->SetFlags((int)ent->GetFlags() & ~QHFL_ONGROUND);

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(ent->GetVelocity(), oldvel);

	clip = SVQH_FlyMove(ent, host_frametime, &steptrace);

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

	if ((int)sv_player->GetFlags() & QHFL_WATERJUMP)
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
	SVQH_PushEntity(ent, upmove);		// FIXME: don't link?

// move forward
	ent->GetVelocity()[0] = oldvel[0];
	ent->GetVelocity()[1] = oldvel[1];
	ent->GetVelocity()[2] = 0;
	clip = SVQH_FlyMove(ent, host_frametime, &steptrace);

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
		VectorCopy(nosteporg, ent->GetOrigin());
		ent->SetVelocity(nostepvel);
	}
}

void SV_Physics_Toss(qhedict_t* ent);

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
		if (!SVQH_RunThink(ent, host_frametime))
		{
			return;
		}
		break;

	case QHMOVETYPE_WALK:
		if (!SVQH_RunThink(ent, host_frametime))
		{
			return;
		}
		if (!SV_CheckWater(ent) && !((int)ent->GetFlags() & QHFL_WATERJUMP))
		{
			SVQH_AddGravity(ent, host_frametime);
		}
		SV_CheckStuck(ent);
		SV_WalkMove(ent);
		break;

	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
		SV_Physics_Toss(ent);
		break;

	case QHMOVETYPE_FLY:
		if (!SVQH_RunThink(ent, host_frametime))
		{
			return;
		}
		SVQH_FlyMove(ent, host_frametime, NULL);
		break;

	case QHMOVETYPE_NOCLIP:
		if (!SVQH_RunThink(ent, host_frametime))
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
	SVQH_LinkEdict(ent, true);

	*pr_globalVars.time = sv.qh_time;
	*pr_globalVars.self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(*pr_globalVars.PlayerPostThink);
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
	if (!SVQH_RunThink(ent, host_frametime))
	{
		return;
	}

// if onground, return without moving
	if (((int)ent->GetFlags() & QHFL_ONGROUND))
	{
		return;
	}

	SVQH_CheckVelocity(ent);

// add gravity
	if (ent->GetMoveType() != QHMOVETYPE_FLY &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE)
	{
		SVQH_AddGravity(ent, host_frametime);
	}

// move angles
	VectorMA(ent->GetAngles(), host_frametime, ent->GetAVelocity(), ent->GetAngles());

// move origin
	VectorScale(ent->GetVelocity(), host_frametime, move);
	trace = SVQH_PushEntity(ent, move);
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
=============
*/

void SV_Physics_Step(qhedict_t* ent)
{
	qboolean hitsound;

// freefall if not onground
	if (!((int)ent->GetFlags() & (QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM)))
	{
		if (ent->GetVelocity()[2] < svqh_gravity->value * -0.1)
		{
			hitsound = true;
		}
		else
		{
			hitsound = false;
		}

		SVQH_AddGravity(ent, host_frametime);
		SVQH_CheckVelocity(ent);
		SVQH_FlyMove(ent, host_frametime, NULL);
		SVQH_LinkEdict(ent, true);

		if ((int)ent->GetFlags() & QHFL_ONGROUND)		// just hit ground
		{
			if (hitsound)
			{
				SVQH_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
			}
		}
	}

// regular thinking
	SVQH_RunThink(ent, host_frametime);

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

	SVQH_SetMoveVars();

// let the progs know that a new frame has started
	*pr_globalVars.self = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.other = EDICT_TO_PROG(sv.qh_edicts);
	*pr_globalVars.time = sv.qh_time;
	PR_ExecuteProgram(*pr_globalVars.StartFrame);

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

		if (*pr_globalVars.force_retouch)
		{
			SVQH_LinkEdict(ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= svs.qh_maxclients)
		{
			SV_Physics_Client(ent, i);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_PUSH)
		{
			SVQH_Physics_Pusher(ent, host_frametime);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NONE)
		{
			SVQH_Physics_None(ent, host_frametime);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			SVQH_Physics_Noclip(ent, host_frametime);
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

	if (*pr_globalVars.force_retouch)
	{
		(*pr_globalVars.force_retouch)--;
	}

	sv.qh_time += host_frametime;
}
