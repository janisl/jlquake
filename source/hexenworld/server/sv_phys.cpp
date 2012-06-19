// sv_phys.c

#include "qwsvdef.h"

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

		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) > 4 ||
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) > 4)
		{
//			Con_DPrintf ("unstuck!\n");
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
//	Con_Printf("Calling pushent\n");
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
		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) < 0.03125 &&
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) < 0.03125)
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


/*
================
SV_Physics_Client

Player character actions
================
*/

void SV_Physics_Client(qhedict_t* ent)	//, int num)
{
	q1trace_t trace;

	trace = SVQH_Move(ent->GetOldOrigin(), ent->GetMins(), ent->GetMaxs(), ent->GetOrigin(), QHMOVE_NOMONSTERS, ent);

	if (trace.fraction < 1.0)
	{
		return;
	}

	trace = SVQH_Move(ent->GetOldOrigin(), ent->GetMins(), ent->GetMaxs(), ent->GetOrigin(), QHMOVE_NORMAL, ent);

	if (ent->GetMoveType() != QHMOVETYPE_BOUNCE || (trace.allsolid == 0 && trace.startsolid == 0))
	{
		VectorCopy(trace.endpos, ent->GetOrigin());


	}
	else
	{
		trace.fraction = 0;
		return;
	}

	if (trace.entityNum >= 0)
	{
		SVQH_Impact(ent, QH_EDICT_NUM(trace.entityNum));
	}
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
			SV_StartSound(ent, 0, GGameType & GAME_Hexen2 ? "misc/hith2o.wav" : "misc/h2ohit1.wav", 255, 1);
		}
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
	}
	else
	{
		if (ent->GetWaterType() != BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound(ent, 0, GGameType & GAME_Hexen2 ? "misc/hith2o.wav" : "misc/h2ohit1.wav", 255, 1);
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

	if (ent->GetVelocity()[2] > 0)
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
		ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE &&
		ent->GetMoveType() != H2MOVETYPE_SWIM)
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
	else if (ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE)
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
	if ((trace.plane.normal[2] > 0.7) && (ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE))
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

		SVQH_AddGravity(ent, host_frametime);
		SVQH_CheckVelocity(ent);
		SVQH_FlyMove(ent, host_frametime, NULL);
		SVQH_LinkEdict(ent, true);

		if (((int)ent->GetFlags() & QHFL_ONGROUND) && (!ent->GetFlags() & QHFL_MONSTER))
		{	// just hit ground
			if (hitsound)
			{
				SV_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
			}
		}
	}

// regular thinking
	SVQH_RunThink(ent, host_frametime);

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
	int c,originMoved;
	qhedict_t* ent2;
	vec3_t oldOrigin,oldAngle;

	if (ent->GetLastRunTime() == (float)realtime)
	{
		return;
	}
	ent->SetLastRunTime((float)realtime);

	ent2 = PROG_TO_EDICT(ent->GetMoveChain());
	if (ent2 != sv.qh_edicts)
	{
		VectorCopy(ent->GetOrigin(),oldOrigin);
		VectorCopy(ent->GetAngles(),oldAngle);
	}

	switch ((int)ent->GetMoveType())
	{
	case QHMOVETYPE_PUSH:
		SVQH_Physics_Pusher(ent, host_frametime);
		break;
	case QHMOVETYPE_NONE:
		SVQH_Physics_None(ent, host_frametime);
		break;
	case QHMOVETYPE_NOCLIP:
		SVQH_Physics_Noclip(ent, host_frametime);
		break;
	case QHMOVETYPE_STEP:
	case H2MOVETYPE_PUSHPULL:
		SV_Physics_Step(ent);
		break;
	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
	case H2MOVETYPE_BOUNCEMISSILE:
	case QHMOVETYPE_FLY:
	case QHMOVETYPE_FLYMISSILE:
	case H2MOVETYPE_SWIM:
		SV_Physics_Toss(ent);
		break;
	case H2MOVETYPE_FOLLOW:
		break;

	case QHMOVETYPE_WALK:
		SVQH_RunThink(ent, host_frametime);
		break;

	default:
		SV_Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
	}

	if (ent2 != sv.qh_edicts)
	{
		originMoved = !VectorCompare(ent->GetOrigin(),oldOrigin);
		if (originMoved || !VectorCompare(ent->GetAngles(),oldAngle))
		{
			VectorSubtract(ent->GetOrigin(),oldOrigin,oldOrigin);
			VectorSubtract(ent->GetAngles(),oldAngle,oldAngle);

			for (c = 0; c < 10; c++)
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
		if (i > 0 && i <= HWMAX_CLIENTS)
		{
//			SV_Physics_Client(ent);
//			VectorCopy (ent->v.origin,ent->v.oldorigin);


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
