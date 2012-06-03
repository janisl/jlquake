/*
 * $Header: /H2 Mission Pack/SV_MOVE.C 17    3/18/98 4:46p Mgummelt $
 */

// sv_move.c -- monster movement

#include "quakedef.h"

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
//int c_yes, c_no;//These are never checked!!

qboolean SV_CheckBottom(qhedict_t* ent)
{	//By this point, ent has been moved to it's new position after the
//move, and adjusted for steps
	vec3_t mins, maxs, start, stop;
	q1trace_t trace;
	int x, y;
	float mid, bottom;
	float save_hull;

	VectorAdd(ent->GetOrigin(), ent->GetMins(), mins);
	VectorAdd(ent->GetOrigin(), ent->GetMaxs(), maxs);

/*/ / Make it use the clipping hull 's size, not their bounding box...
	model = sv.models[ (int)sv.qh_edicts->v.modelindex ];
	VectorSubtract (ent->v.maxs, ent->v.mins, size);
	if(ent->v.hull)
	{
		index = ent->v.hull-1;
		wclip_hull = &model->hulls[index];;
		if (!wclip_hull)  // Invalid hull
		{
			Con_Printf ("ERROR: hull %d is null.\n",wclip_hull);
			wclip_hull = &model->hulls[0];
		}
	}
	else  // Using the old way uses size to determine hull to use
	{
		if (size[0] < 3) // Point
			wclip_hull = &model->hulls[0];
		else if (size[0] <= 8)  // Pentacles
			wclip_hull = &model->hulls[4];
		else if (size[0] <= 32 && size[2] <= 28)  // Half Player
			wclip_hull = &model->hulls[3];
		else if (size[0] <= 32)  // Full Player
			wclip_hull = &model->hulls[1];
		else // Golumn
			wclip_hull = &model->hulls[5];
	}
	VectorAdd (ent->v.origin, wclip_hull->clip_mins, mins);
	VectorAdd (ent->v.origin, wclip_hull->clip_maxs, maxs);
*/	
// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for (x = 0; x < 2; x++)
		for (y = 0; y < 2; y++)
		{
			if (x)
			{
				start[0] = maxs[0];
			}
			else
			{
				start[0] = mins[0];
			}
			if (y)
			{
				start[1] = maxs[1];
			}
			else
			{
				start[1] = mins[1];
			}
//			start[0] = x ? maxs[0] : mins[0];
//			start[1] = y ? maxs[1] : mins[1];
			if (SV_PointContents(start) != BSP29CONTENTS_SOLID)
			{
				goto realcheck;
			}
		}

//	c_yes++;
	return true;		// we got out easy

realcheck:	// check it for real...
//	c_no++;

	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
	stop[2] = start[2] - 2 * STEPSIZE;
//do a trace from the bottom center of the ent down
//to 36 below the bottom center of the ent, using a point hull
//the "true" in this function is telling SV_Move to consider
//this ent's movement as MOVE_NOMONSTERS - means it will
//not clip against entities in this move
//NOTE: these don't check for trace_allsolid of trace_startsolid.
//Technically, these can't possibly be a valid result since
//the start point is in the ent, but what about imprecision?

	save_hull = ent->GetHull();	//temp hack so it HullForEntity doesn't calculate the wrong offset
	ent->SetHull(0);
	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);
	ent->SetHull(save_hull);

/*	if((int)ent->v.flags&FL_MONSTER)
    {
        if(trace.allsolid)
            Con_DPrintf("Checkbottom midpoint check was all solid!!!\n");
        else if(trace.startsolid)
            Con_DPrintf("Checkbottom midpoint check started solid!!!\n");
    }
*/
	if (trace.fraction == 1.0)
	{
		return false;
	}
//trace did not reach full 36 below, so set mid and bottom
//to whatever distance it did get to below the ent's
//bottom centerpoint (start[2])
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint
	for (x = 0; x < 2; x++)
		for (y = 0; y < 2; y++)
		{	//check 4 corners, in this order:
		//x = 0, y = 0 (NE)
		//x = 0, y = 1 (SE)
		//x = 1, y = 0 (NW)
		//x = 1, y = 1 (SW)
			if (x)
			{
				start[0] = stop[0] = maxs[0];
			}
			else
			{
				start[0] = stop[0] = mins[0];
			}
			if (y)
			{
				start[1] = stop[1] = maxs[1];
			}
			else
			{
				start[1] = stop[1] = mins[1];
			}
//			start[0] = stop[0] = x ? maxs[0] : mins[0];
//			start[1] = stop[1] = y ? maxs[1] : mins[1];

			//same check as above, just from the 4 corners down 36
			save_hull = ent->GetHull();	//temp hack so it HullForEntity doesn't calculate the wrong offset
			ent->SetHull(0);
			trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);
			ent->SetHull(save_hull);

/*			if((int)ent->v.flags&FL_MONSTER)
            {
                if(trace.allsolid)
                    Con_DPrintf("Checkbottom (x=%d,y=%d) check was all solid!!!\n",x,y);
                else if(trace.startsolid)
                    Con_DPrintf("Checkbottom (x=%d,y=%d) check started solid!!!\n",x,y);
            }*/
			//Hit a closer surface than did when checked center,
			//so set the "bottom" to the new, closer z height
			//we hit
			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
			{
				bottom = trace.endpos[2];
			}

			//one of the corners does not have a surface within
			//36 below it, or the surface it did hit is more than
			//54 below this corner.  This is a really stupid check,
			//because if it hits a surface more than 54 below the
			//ent,the trace_fraction will be 1
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
			{
				return false;
			}
		}

//	c_yes++;
	return true;
}


void set_move_trace(q1trace_t* trace)
{
	pr_global_struct->trace_allsolid = trace->allsolid;
	pr_global_struct->trace_startsolid = trace->startsolid;
	pr_global_struct->trace_fraction = trace->fraction;
	pr_global_struct->trace_inwater = trace->inwater;
	pr_global_struct->trace_inopen = trace->inopen;
	VectorCopy(trace->endpos, pr_global_struct->trace_endpos);
	VectorCopy(trace->plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace->plane.dist;
	if (trace->entityNum >= 0)
	{
		pr_global_struct->trace_ent = EDICT_TO_PROG(EDICT_NUM(trace->entityNum));
	}
	else
	{
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv.qh_edicts);
	}
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
qboolean SV_movestep(qhedict_t* ent, vec3_t move, qboolean relink, qboolean noenemy,
	qboolean set_trace)
{
	float dz;
	vec3_t oldorg, neworg, end;
	q1trace_t trace;
	int i;
	qhedict_t* enemy;

// try the move
	VectorCopy(ent->GetOrigin(), oldorg);
	VectorAdd(ent->GetOrigin(), move, neworg);

// flying monsters don't step up, unless no_z turned on
	if (((int)ent->GetFlags() & (FL_SWIM | FL_FLY)) && !((int)ent->GetFlags() & FL_NOZ) && !((int)ent->GetFlags() & FL_HUNTFACE))
	{
		// try one move with vertical motion, then one without
		for (i = 0; i < 2; i++)
		{
			VectorAdd(ent->GetOrigin(), move, neworg);
			if (!noenemy)
			{
				enemy = PROG_TO_EDICT(ent->GetEnemy());
				if (i == 0 && enemy != sv.qh_edicts)
				{
					if ((int)ent->GetFlags() & FL_HUNTFACE)	//Go for face
					{
						dz = ent->GetOrigin()[2] - PROG_TO_EDICT(ent->GetEnemy())->GetOrigin()[2] + PROG_TO_EDICT(ent->GetEnemy())->GetViewOfs()[2];
					}
					else
					{
						dz = ent->GetOrigin()[2] - PROG_TO_EDICT(ent->GetEnemy())->GetOrigin()[2];
					}

					if (dz > 40)
					{
						neworg[2] -= 8;
					}
					else if (dz < 30)
					{
						neworg[2] += 8;
					}
				}
			}

			if (((int)ent->GetFlags() & FL_SWIM) && SV_PointContents(neworg) == BSP29CONTENTS_EMPTY)
			{	//Would end up out of water, don't do z move
				neworg[2] = ent->GetOrigin()[2];
				trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), neworg, false, ent);
				if (set_trace)
				{
					set_move_trace(&trace);
				}
				if (trace.fraction < 1 || SV_PointContents(trace.endpos) == BSP29CONTENTS_EMPTY)
				{
					return false;	// swim monster left water
				}
			}
			else
			{
				trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), neworg, false, ent);
				if (set_trace)
				{
					set_move_trace(&trace);
				}
			}

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, ent->GetOrigin());
				if (relink)
				{
					SV_LinkEdict(ent, true);
				}

				return true;
			}

			if (noenemy || enemy == sv.qh_edicts)
			{
				break;
			}
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += STEPSIZE;
	VectorCopy(neworg, end);
	end[2] -= STEPSIZE * 2;

	trace = SV_Move(neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent);

	if (set_trace)
	{
		set_move_trace(&trace);
	}

	if (trace.allsolid)
	{
		return false;
	}

	if (trace.startsolid)
	{
		neworg[2] -= STEPSIZE;
		trace = SV_Move(neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent);
		if (set_trace)
		{
			set_move_trace(&trace);
		}
		if (trace.allsolid || trace.startsolid)
		{
			return false;
		}
	}
	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if ((int)ent->GetFlags() & FL_PARTIALGROUND)
		{
			VectorAdd(ent->GetOrigin(), move, ent->GetOrigin());
			if (relink)
			{
				SV_LinkEdict(ent, true);
			}
			ent->SetFlags((int)ent->GetFlags() & ~FL_ONGROUND);
//	Con_Printf ("fall down\n");
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy(trace.endpos, ent->GetOrigin());

	if (!SV_CheckBottom(ent))
	{
		if ((int)ent->GetFlags() & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				SV_LinkEdict(ent, true);
			}
			return true;
		}
		VectorCopy(oldorg, ent->GetOrigin());

		return false;
	}

	if ((int)ent->GetFlags() & FL_PARTIALGROUND)
	{
//		Con_Printf ("back on ground\n");
		ent->SetFlags((int)ent->GetFlags() & ~FL_PARTIALGROUND);
	}
	ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));

// the move is ok
	if (relink)
	{
		SV_LinkEdict(ent, true);
	}
	return true;
}


//============================================================================

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
void PF_changeyaw(void);
qboolean SV_StepDirection(qhedict_t* ent, float yaw, float dist)
{
	vec3_t move, oldorigin;
	float delta;
	qboolean set_trace_plane;

	ent->SetIdealYaw(yaw);
	PF_changeyaw();

	yaw = yaw * M_PI * 2 / 360;
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;//FIXME: Make wallcrawlers and flying monsters use this!

	VectorCopy(ent->GetOrigin(), oldorigin);
	if ((int)ent->GetFlags() & FL_SET_TRACE)
	{
		set_trace_plane = true;
	}
	else
	{
		set_trace_plane = false;
	}

	if (SV_movestep(ent, move, false, false, set_trace_plane))
	{
		delta = ent->GetAngles()[YAW] - ent->GetIdealYaw();
		if (delta > 45 && delta < 315)
		{		// not turned far enough, so don't take the step
			VectorCopy(oldorigin, ent->GetOrigin());
		}
		SV_LinkEdict(ent, true);
		return true;
	}
	SV_LinkEdict(ent, true);

	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom(qhedict_t* ent)
{
//	Con_Printf ("SV_FixCheckBottom\n");

	ent->SetFlags((int)ent->GetFlags() | FL_PARTIALGROUND);
}



/*
================
SV_NewChaseDir

================
*/
#define DI_NODIR    -1
void SV_NewChaseDir(qhedict_t* actor, qhedict_t* enemy, float dist)
{
	float deltax,deltay,deltaz;
	float d[3];
	float tdir, olddir, turnaround;

	olddir = AngleMod((int)(actor->GetIdealYaw() / 45) * 45);
	turnaround = AngleMod(olddir - 180);

	deltax = enemy->GetOrigin()[0] - actor->GetOrigin()[0];
	deltay = enemy->GetOrigin()[1] - actor->GetOrigin()[1];

	if ((int)actor->GetFlags() & FL_FLY)//Pentacles
	{
		deltaz = enemy->GetOrigin()[2] + enemy->GetViewOfs()[2] - actor->GetOrigin()[2];
	}
	else
	{
		deltaz = enemy->GetOrigin()[2] - actor->GetOrigin()[2];
	}

	if (deltax > 10)
	{
		d[1] = 0;
	}
	else if (deltax < -10)
	{
		d[1] = 180;
	}
	else
	{
		d[1] = DI_NODIR;
	}
	if (deltay < -10)
	{
		d[2] = 270;
	}
	else if (deltay > 10)
	{
		d[2] = 90;
	}
	else
	{
		d[2] = DI_NODIR;
	}

/*	if (deltaz < -10)//Below
        d[0]= ;
    else if (deltaz>10)//above
        d[0]= ;
    else
        d[0]= DI_NODIR;
*/
// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
		{
			tdir = d[2] == 90 ? 45 : 315;
		}
		else
		{
			tdir = d[2] == 90 ? 135 : 215;
		}

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
		{
			return;
		}
	}

// try other directions
	if (((rand() & 3) & 1) ||  abs(deltay) > abs(deltax))	//If north/sounth diff is greater than east/west diff?!!
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround &&
		SV_StepDirection(actor, d[1], dist))
	{
		return;
	}

	if (d[2] != DI_NODIR && d[2] != turnaround &&
		SV_StepDirection(actor, d[2], dist))
	{
		return;
	}

/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, olddir, dist))
	{
		return;
	}

	if (rand() & 1)		/*randomly determine direction of search*/
	{
		for (tdir = 0; tdir <= 315; tdir += 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			{
				return;
			}
	}
	else
	{
		for (tdir = 315; tdir >= 0; tdir -= 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			{
				return;
			}
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist))
	{
		return;
	}

	actor->SetIdealYaw(olddir);		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!SV_CheckBottom(actor))
	{
		SV_FixCheckBottom(actor);
	}

}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough(qhedict_t* ent, qhedict_t* goal, float dist)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (goal->v.absmin[i] > ent->v.absmax[i] + dist)
		{
			return false;
		}
		if (goal->v.absmax[i] < ent->v.absmin[i] - dist)
		{
			return false;
		}
	}
	return true;
}

/*
======================
SV_MoveToGoal

======================
*/
void SV_MoveToGoal(void)
{
	qhedict_t* ent, * goal;
	float dist;

	ent = PROG_TO_EDICT(pr_global_struct->self);//Entity moving
	goal = PROG_TO_EDICT(ent->GetGoalEntity());	//it's goalentity
	dist = G_FLOAT(OFS_PARM0);	//how far to move

//Reset trace_plane_normal
	VectorCopy(vec3_origin,pr_global_struct->trace_plane_normal);
	if (!((int)ent->GetFlags() & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{	//If not onground, flying, or swimming, return 0
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

// if the next step hits the enemy, return immediately
	if (PROG_TO_EDICT(ent->GetEnemy()) != sv.qh_edicts &&  SV_CloseEnough(ent, goal, dist))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

// bump around...
	if (!SV_StepDirection(ent, ent->GetIdealYaw(), dist))	//If can't go in a direction (including step check) or 30% chance...
	{
		SV_NewChaseDir(ent, goal, dist);//Find a new direction to go in instead
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
	{
		if ((rand() & 3) == 1)
		{
			SV_NewChaseDir(ent, goal, dist);//Find a new direction to go in instead
		}
		G_FLOAT(OFS_RETURN) = 1;
	}
	return;
}
