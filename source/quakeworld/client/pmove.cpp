/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

/*
============
PM_FlyMove

The basic solid body movement clip that slides along multiple planes
============
*/
#define	MAX_CLIP_PLANES	5

int PM_FlyMove (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	int			i, j;
	q1trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;
	
	numbumps = 4;
	
	blocked = 0;
	VectorCopy (qh_pmove.velocity, original_velocity);
	VectorCopy (qh_pmove.velocity, primal_velocity);
	numplanes = 0;
	
	time_left = qh_pml.frametime;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		for (i=0 ; i<3 ; i++)
			end[i] = qh_pmove.origin[i] + time_left * qh_pmove.velocity[i];

		trace = PMQH_TestPlayerMove (qh_pmove.origin, end);

		if (trace.startsolid || trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy (vec3_origin, qh_pmove.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, qh_pmove.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

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
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy (vec3_origin, qh_pmove.velocity);
			break;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			PM_ClipVelocity (original_velocity, planes[i], qh_pmove.velocity, 1);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (qh_pmove.velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy (vec3_origin, qh_pmove.velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, qh_pmove.velocity);
			VectorScale (dir, d, qh_pmove.velocity);
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (qh_pmove.velocity, primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, qh_pmove.velocity);
			break;
		}
	}

	if (qh_pmove.waterjumptime)
	{
		VectorCopy (primal_velocity, qh_pmove.velocity);
	}
	return blocked;
}

/*
=============
PM_GroundMove

Player is on ground, with no upwards velocity
=============
*/
void PM_GroundMove (void)
{
	vec3_t	start, dest;
	q1trace_t	trace;
	vec3_t	original, originalvel, down, up, downvel;
	float	downdist, updist;

	qh_pmove.velocity[2] = 0;
	if (!qh_pmove.velocity[0] && !qh_pmove.velocity[1] && !qh_pmove.velocity[2])
		return;

	// first try just moving to the destination	
	dest[0] = qh_pmove.origin[0] + qh_pmove.velocity[0]*qh_pml.frametime;
	dest[1] = qh_pmove.origin[1] + qh_pmove.velocity[1]*qh_pml.frametime;	
	dest[2] = qh_pmove.origin[2];

	// first try moving directly to the next spot
	VectorCopy (dest, start);
	trace = PMQH_TestPlayerMove (qh_pmove.origin, dest);
	if (trace.fraction == 1)
	{
		VectorCopy (trace.endpos, qh_pmove.origin);
		return;
	}

	// try sliding forward both on ground and up 16 pixels
	// take the move that goes farthest
	VectorCopy (qh_pmove.origin, original);
	VectorCopy (qh_pmove.velocity, originalvel);

	// slide move
	PM_FlyMove ();

	VectorCopy (qh_pmove.origin, down);
	VectorCopy (qh_pmove.velocity, downvel);

	VectorCopy (original, qh_pmove.origin);
	VectorCopy (originalvel, qh_pmove.velocity);

// move up a stair height
	VectorCopy (qh_pmove.origin, dest);
	dest[2] += STEPSIZE;
	trace = PMQH_TestPlayerMove (qh_pmove.origin, dest);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, qh_pmove.origin);
	}

// slide move
	PM_FlyMove ();

// press down the stepheight
	VectorCopy (qh_pmove.origin, dest);
	dest[2] -= STEPSIZE;
	trace = PMQH_TestPlayerMove (qh_pmove.origin, dest);
	if ( trace.plane.normal[2] < 0.7)
		goto usedown;
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, qh_pmove.origin);
	}
	VectorCopy (qh_pmove.origin, up);

	// decide which one went farther
	downdist = (down[0] - original[0])*(down[0] - original[0])
		+ (down[1] - original[1])*(down[1] - original[1]);
	updist = (up[0] - original[0])*(up[0] - original[0])
		+ (up[1] - original[1])*(up[1] - original[1]);

	if (downdist > updist)
	{
usedown:
		VectorCopy (down, qh_pmove.origin);
		VectorCopy (downvel, qh_pmove.velocity);
	} else // copy z value from slide move
		qh_pmove.velocity[2] = downvel[2];

// if at a dead stop, retry the move with nudges to get around lips

}



/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
void PM_Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	vec3_t	start, stop;
	q1trace_t		trace;
	
	if (qh_pmove.waterjumptime)
		return;

	vel = qh_pmove.velocity;
	
	speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	friction = movevars.friction;

// if the leading edge is over a dropoff, increase friction
	if (qh_pmove.onground != -1) {
		start[0] = stop[0] = qh_pmove.origin[0] + vel[0]/speed*16;
		start[1] = stop[1] = qh_pmove.origin[1] + vel[1]/speed*16;
		start[2] = qh_pmove.origin[2] + pmqh_player_mins[2];
		stop[2] = start[2] - 34;

		trace = PMQH_TestPlayerMove (start, stop);

		if (trace.fraction == 1) {
			friction *= 2;
		}
	}

	drop = 0;

	if (qh_pmove.waterlevel >= 2) // apply water friction
		drop += speed*movevars.waterfriction*qh_pmove.waterlevel*qh_pml.frametime;
	else if (qh_pmove.onground != -1) // apply ground friction
	{
		control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop += control*friction*qh_pml.frametime;
	}


// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate
==============
*/
void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	if (qh_pmove.dead)
		return;
	if (qh_pmove.waterjumptime)
		return;

	currentspeed = DotProduct (qh_pmove.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel*qh_pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		qh_pmove.velocity[i] += accelspeed*wishdir[i];	
}

void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	if (qh_pmove.dead)
		return;
	if (qh_pmove.waterjumptime)
		return;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (qh_pmove.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * qh_pml.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		qh_pmove.velocity[i] += accelspeed*wishdir[i];	
}



/*
===================
PM_WaterMove

===================
*/
void PM_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	vec3_t	start, dest;
	q1trace_t	trace;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = qh_pml.forward[i]*qh_pmove.cmd.forwardmove + qh_pml.right[i]*qh_pmove.cmd.sidemove;

	if (!qh_pmove.cmd.forwardmove && !qh_pmove.cmd.sidemove && !qh_pmove.cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += qh_pmove.cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > movevars.maxspeed)
	{
		VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
		wishspeed = movevars.maxspeed;
	}
	wishspeed *= 0.7;

//
// water acceleration
//
//	if (qh_pmove.waterjumptime)
//		Con_Printf ("wm->%f, %f, %f\n", qh_pmove.velocity[0], qh_pmove.velocity[1], qh_pmove.velocity[2]);
	PM_Accelerate (wishdir, wishspeed, movevars.wateraccelerate);

// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (qh_pmove.origin, qh_pml.frametime, qh_pmove.velocity, dest);
	VectorCopy (dest, start);
	start[2] += STEPSIZE + 1;
	trace = PMQH_TestPlayerMove (start, dest);
	if (!trace.startsolid && !trace.allsolid)	// FIXME: check steep slope?
	{	// walked up the step
		VectorCopy (trace.endpos, qh_pmove.origin);
		return;
	}
	
	PM_FlyMove ();
//	if (qh_pmove.waterjumptime)
//		Con_Printf ("<-wm%f, %f, %f\n", qh_pmove.velocity[0], qh_pmove.velocity[1], qh_pmove.velocity[2]);
}


/*
===================
PM_AirMove

===================
*/
void PM_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	fmove = qh_pmove.cmd.forwardmove;
	smove = qh_pmove.cmd.sidemove;
	
	qh_pml.forward[2] = 0;
	qh_pml.right[2] = 0;
	VectorNormalize (qh_pml.forward);
	VectorNormalize (qh_pml.right);

	for (i=0 ; i<2 ; i++)
		wishvel[i] = qh_pml.forward[i]*fmove + qh_pml.right[i]*smove;
	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

//
// clamp to server defined max speed
//
	if (wishspeed > movevars.maxspeed)
	{
		VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
		wishspeed = movevars.maxspeed;
	}
	
//	if (qh_pmove.waterjumptime)
//		Con_Printf ("am->%f, %f, %f\n", qh_pmove.velocity[0], qh_pmove.velocity[1], qh_pmove.velocity[2]);

	if ( qh_pmove.onground != -1)
	{
		qh_pmove.velocity[2] = 0;
		PM_Accelerate (wishdir, wishspeed, movevars.accelerate);
		qh_pmove.velocity[2] -= movevars.entgravity * movevars.gravity * qh_pml.frametime;
		PM_GroundMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		PM_AirAccelerate (wishdir, wishspeed, movevars.accelerate);

		// add gravity
		qh_pmove.velocity[2] -= movevars.entgravity * movevars.gravity * qh_pml.frametime;

		PM_FlyMove ();

	}

//Con_Printf("airmove:vec: %4.2f %4.2f %4.2f\n",
//			qh_pmove.velocity[0],
//			qh_pmove.velocity[1],
//			qh_pmove.velocity[2]);
//

//	if (qh_pmove.waterjumptime)
//		Con_Printf ("<-am%f, %f, %f\n", qh_pmove.velocity[0], qh_pmove.velocity[1], qh_pmove.velocity[2]);
}



/*
=============
PM_CatagorizePosition
=============
*/
void PM_CatagorizePosition (void)
{
	vec3_t		point;
	int			cont;
	q1trace_t		tr;

// if the player hull point one unit down is solid, the player
// is on ground

// see if standing on something solid	
	point[0] = qh_pmove.origin[0];
	point[1] = qh_pmove.origin[1];
	point[2] = qh_pmove.origin[2] - 1;
	if (qh_pmove.velocity[2] > 180)
	{
		qh_pmove.onground = -1;
	}
	else
	{
		tr = PMQH_TestPlayerMove (qh_pmove.origin, point);
		if ( tr.plane.normal[2] < 0.7)
			qh_pmove.onground = -1;	// too steep
		else
			qh_pmove.onground = tr.entityNum;
		if (qh_pmove.onground != -1)
		{
			qh_pmove.waterjumptime = 0;
			if (!tr.startsolid && !tr.allsolid)
				VectorCopy (tr.endpos, qh_pmove.origin);
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
	cont = CM_PointContentsQ1(point, 0);

	if (cont <= BSP29CONTENTS_WATER)
	{
		qh_pmove.watertype = cont;
		qh_pmove.waterlevel = 1;
		point[2] = qh_pmove.origin[2] + (pmqh_player_mins[2] + pmqh_player_maxs[2])*0.5;
		cont = CM_PointContentsQ1(point, 0);
		if (cont <= BSP29CONTENTS_WATER)
		{
			qh_pmove.waterlevel = 2;
			point[2] = qh_pmove.origin[2] + 22;
			cont = CM_PointContentsQ1(point, 0);
			if (cont <= BSP29CONTENTS_WATER)
				qh_pmove.waterlevel = 3;
		}
	}
}


/*
=============
JumpButton
=============
*/
void JumpButton (void)
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
			qh_pmove.waterjumptime = 0;
		return;
	}

	if (qh_pmove.waterlevel >= 2)
	{	// swimming, not jumping
		qh_pmove.onground = -1;

		if (qh_pmove.watertype == BSP29CONTENTS_WATER)
			qh_pmove.velocity[2] = 100;
		else if (qh_pmove.watertype == BSP29CONTENTS_SLIME)
			qh_pmove.velocity[2] = 80;
		else
			qh_pmove.velocity[2] = 50;
		return;
	}

	if (qh_pmove.onground == -1)
		return;		// in air, so no effect

	if ( qh_pmove.oldbuttons & QHBUTTON_JUMP )
		return;		// don't pogo stick

	qh_pmove.onground = -1;
	qh_pmove.velocity[2] += 270;

	qh_pmove.oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
}

/*
=============
CheckWaterJump
=============
*/
void CheckWaterJump (void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (qh_pmove.waterjumptime)
		return;

	// ZOID, don't hop out if we just jumped in
	if (qh_pmove.velocity[2] < -180)
		return; // only hop out if we are moving up

	// see if near an edge
	flatforward[0] = qh_pml.forward[0];
	flatforward[1] = qh_pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (qh_pmove.origin, 24, flatforward, spot);
	spot[2] += 8;
	cont = CM_PointContentsQ1(spot, 0);
	if (cont != BSP29CONTENTS_SOLID)
		return;
	spot[2] += 24;
	cont = CM_PointContentsQ1(spot, 0);
	if (cont != BSP29CONTENTS_EMPTY)
		return;
	// jump out of water
	VectorScale (flatforward, 50, qh_pmove.velocity);
	qh_pmove.velocity[2] = 310;
	qh_pmove.waterjumptime = 2;	// safety net
	qh_pmove.oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
}

/*
=================
NudgePosition

If qh_pmove.origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
void NudgePosition (void)
{
	vec3_t	base;
	int		x, y, z;
	int		i;
	static int		sign[3] = {0, -1, 1};

	VectorCopy (qh_pmove.origin, base);

	for (i=0 ; i<3 ; i++)
		qh_pmove.origin[i] = ((int)(qh_pmove.origin[i]*8)) * 0.125;
//	qh_pmove.origin[2] += 0.124;

//	if (qh_pmove.dead)
//		return;		// might be a squished point, so don'y bother
//	if (PMQH_TestPlayerPosition (qh_pmove.origin) )
//		return;

	for (z=0 ; z<=2 ; z++)
	{
		for (x=0 ; x<=2 ; x++)
		{
			for (y=0 ; y<=2 ; y++)
			{
				qh_pmove.origin[0] = base[0] + (sign[x] * 1.0/8);
				qh_pmove.origin[1] = base[1] + (sign[y] * 1.0/8);
				qh_pmove.origin[2] = base[2] + (sign[z] * 1.0/8);
				if (PMQH_TestPlayerPosition (qh_pmove.origin))
					return;
			}
		}
	}
	VectorCopy (base, qh_pmove.origin);
//	Con_DPrintf ("NudgePosition: stuck\n");
}

/*
===============
SpectatorMove
===============
*/
void SpectatorMove (void)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	// friction

	speed = VectorLength(qh_pmove.velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, qh_pmove.velocity);
	}
	else
	{
		drop = 0;

		friction = movevars.friction*1.5;	// extra friction
		control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop += control*friction*qh_pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (qh_pmove.velocity, newspeed, qh_pmove.velocity);
	}

	// accelerate
	fmove = qh_pmove.cmd.forwardmove;
	smove = qh_pmove.cmd.sidemove;
	
	VectorNormalize (qh_pml.forward);
	VectorNormalize (qh_pml.right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = qh_pml.forward[i]*fmove + qh_pml.right[i]*smove;
	wishvel[2] += qh_pmove.cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > movevars.spectatormaxspeed)
	{
		VectorScale (wishvel, movevars.spectatormaxspeed/wishspeed, wishvel);
		wishspeed = movevars.spectatormaxspeed;
	}

	currentspeed = DotProduct(qh_pmove.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = movevars.accelerate*qh_pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		qh_pmove.velocity[i] += accelspeed*wishdir[i];	


	// move
	VectorMA (qh_pmove.origin, qh_pml.frametime, qh_pmove.velocity, qh_pmove.origin);
}

/*
=============
PlayerMove

Returns with origin, angles, and velocity modified in place.

Numtouch and touchindex[] will be set if any of the physents
were contacted during the move.
=============
*/
void PlayerMove (void)
{
	qh_pml.frametime = qh_pmove.cmd.msec * 0.001;
	qh_pmove.numtouch = 0;

	AngleVectors (qh_pmove.angles, qh_pml.forward, qh_pml.right, qh_pml.up);

	if (qh_pmove.spectator)
	{
		SpectatorMove ();
		return;
	}

	NudgePosition ();

	// take angles directly from command
	VectorCopy (qh_pmove.cmd.angles, qh_pmove.angles);

	// set onground, watertype, and waterlevel
	PM_CatagorizePosition ();

	if (qh_pmove.waterlevel == 2)
		CheckWaterJump ();

	if (qh_pmove.velocity[2] < 0)
		qh_pmove.waterjumptime = 0;

	if (qh_pmove.cmd.buttons & QHBUTTON_JUMP)
		JumpButton ();
	else
		qh_pmove.oldbuttons &= ~QHBUTTON_JUMP;

	PM_Friction ();

	if (qh_pmove.waterlevel >= 2)
		PM_WaterMove ();
	else
		PM_AirMove ();

	// set onground, watertype, and waterlevel for final spot
	PM_CatagorizePosition ();
}

