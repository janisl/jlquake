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

		if ((int)ent->GetFlags() & QHFL_ONGROUND)		// just hit ground
		{
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
	if (ent->GetLastRunTime() == (float)realtime)
	{
		return;
	}
	ent->SetLastRunTime((float)realtime);

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
