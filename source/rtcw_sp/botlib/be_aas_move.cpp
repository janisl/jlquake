/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_move.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs)
{
	vec3_t end;
	bsp_trace_t trace;

	VectorCopy(origin, end);
	end[2] -= 100;
	trace = AAS_Trace(origin, mins, maxs, end, 0, BSP46CONTENTS_SOLID);
	if (trace.startsolid)
	{
		return qfalse;
	}
	VectorCopy(trace.endpos, origin);
	return qtrue;
}	//end of the function AAS_DropToFloor
//===========================================================================
// returns qtrue if the bot is on the ground
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_OnGround(vec3_t origin, int presencetype, int passent)
{
	aas_trace_t trace;
	vec3_t end, up = {0, 0, 1};
	aas_plane_t* plane;

	VectorCopy(origin, end);
	end[2] -= 10;

	trace = AAS_TraceClientBBox(origin, end, presencetype, passent);

	//if in solid
	if (trace.startsolid)
	{
		return qtrue;					//qfalse;
	}
	//if nothing hit at all
	if (trace.fraction >= 1.0)
	{
		return qfalse;
	}
	//if too far from the hit plane
	if (origin[2] - trace.endpos[2] > 10)
	{
		return qfalse;
	}
	//check if the plane isn't too steep
	plane = AAS_PlaneFromNum(trace.planenum);
	if (DotProduct(plane->normal, up) < aassettings.phys_maxsteepness)
	{
		return qfalse;
	}
	//the bot is on the ground
	return qtrue;
}	//end of the function AAS_OnGround
//===========================================================================
// returns qtrue if a bot at the given position is swimming
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Swimming(vec3_t origin)
{
	vec3_t testorg;

	VectorCopy(origin, testorg);
	testorg[2] -= 2;
	if (AAS_PointContents(testorg) & (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER))
	{
		return qtrue;
	}
	return qfalse;
}	//end of the function AAS_Swimming
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_JumpReachRunStart(aas_reachability_t* reach, vec3_t runstart)
{
	vec3_t hordir, start, cmdmove;
	aas_clientmove_t move;

	//
	hordir[0] = reach->start[0] - reach->end[0];
	hordir[1] = reach->start[1] - reach->end[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	//start point
	VectorCopy(reach->start, start);
	start[2] += 1;
	//get command movement
	VectorScale(hordir, 400, cmdmove);
	//
	AAS_PredictClientMovement(&move, -1, start, PRESENCE_NORMAL, qtrue,
		vec3_origin, cmdmove, 1, 2, 0.1,
		SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA |
		SE_HITGROUNDDAMAGE | SE_GAP, 0, qfalse);
	VectorCopy(move.endpos, runstart);
	//don't enter slime or lava and don't fall from too high
	if (move.stopevent & (SE_ENTERLAVA | SE_HITGROUNDDAMAGE))		//----(SA)	modified since slime is no longer deadly
	{	//	if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
		VectorCopy(start, runstart);
	}	//end if
}	//end of the function AAS_JumpReachRunStart
//===========================================================================
// returns the Z velocity when rocket jumping at the origin
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage)
{
	vec3_t kvel, v, start, end, forward, right, viewangles, dir;
	float mass, knockback, points;
	vec3_t rocketoffset = {8, 8, -8};
	vec3_t botmins = {-16, -16, -24};
	vec3_t botmaxs = {16, 16, 32};
	bsp_trace_t bsptrace;

	//look down (90 degrees)
	viewangles[PITCH] = 90;
	viewangles[YAW] = 0;
	viewangles[ROLL] = 0;
	//get the start point shooting from
	VectorCopy(origin, start);
	start[2] += 8;	//view offset Z
	AngleVectors(viewangles, forward, right, NULL);
	start[0] += forward[0] * rocketoffset[0] + right[0] * rocketoffset[1];
	start[1] += forward[1] * rocketoffset[0] + right[1] * rocketoffset[1];
	start[2] += forward[2] * rocketoffset[0] + right[2] * rocketoffset[1] + rocketoffset[2];
	//end point of the trace
	VectorMA(start, 500, forward, end);
	//trace a line to get the impact point
	bsptrace = AAS_Trace(start, NULL, NULL, end, 1, BSP46CONTENTS_SOLID);
	//calculate the damage the bot will get from the rocket impact
	VectorAdd(botmins, botmaxs, v);
	VectorMA(origin, 0.5, v, v);
	VectorSubtract(bsptrace.endpos, v, v);
	//
	points = radiusdamage - 0.5 * VectorLength(v);
	if (points < 0)
	{
		points = 0;
	}
	//the owner of the rocket gets half the damage
	points *= 0.5;
	//mass of the bot (p_client.c: PutClientInServer)
	mass = 200;
	//knockback is the same as the damage points
	knockback = points;
	//direction of the damage (from trace.endpos to bot origin)
	VectorSubtract(origin, bsptrace.endpos, dir);
	VectorNormalize(dir);
	//damage velocity
	VectorScale(dir, 1600.0 * (float)knockback / mass, kvel);		//the rocket jump hack...
	//rocket impact velocity + jump velocity
	return kvel[2] + aassettings.phys_jumpvel;
}	//end of the function AAS_WeaponJumpZVelocity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_RocketJumpZVelocity(vec3_t origin)
{
	//rocket radius damage is 120 (p_weapon.c: Weapon_RocketLauncher_Fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
}	//end of the function AAS_RocketJumpZVelocity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_BFGJumpZVelocity(vec3_t origin)
{
	//bfg radius damage is 1000 (p_weapon.c: weapon_bfg_fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
}	//end of the function AAS_BFGJumpZVelocity
//===========================================================================
// predicts the movement
// assumes regular bounding box sizes
// NOTE: out of water jumping is not included
// NOTE: grappling hook is not included
//
// Parameter:				origin			: origin to start with
//								presencetype	: presence type to start with
//								velocity			: velocity to start with
//								cmdmove			: client command movement
//								cmdframes		: number of frame cmdmove is valid
//								maxframes		: maximum number of predicted frames
//								frametime		: duration of one predicted frame
//								stopevent		: events that stop the prediction
//						stopareanum		: stop as soon as entered this area
// Returns:					aas_clientmove_t
// Changes Globals:		-
//===========================================================================
int AAS_PredictClientMovement(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, int visualize)
{
	float sv_friction, sv_stopspeed, sv_gravity, sv_waterfriction;
	float sv_watergravity;
	float sv_walkaccelerate, sv_airaccelerate, sv_swimaccelerate;
	float sv_maxwalkvelocity, sv_maxcrouchvelocity, sv_maxswimvelocity;
	float sv_maxstep, sv_maxsteepness, sv_jumpvel, friction;
	float gravity, delta, maxvel, wishspeed, accelerate;
	//float velchange, newvel;
	int n, i, j, pc, step, swimming, ax, crouch, event, jump_frame, areanum;
	int areas[20], numareas;
	vec3_t points[20];
	vec3_t org, end, feet, start, stepend, lastorg, wishdir;
	vec3_t frame_test_vel, old_frame_test_vel, left_test_vel;
	vec3_t up = {0, 0, 1};
	aas_plane_t* plane, * plane2;
	aas_trace_t trace, steptrace;

	if (frametime <= 0)
	{
		frametime = 0.1;
	}
	//
	sv_friction = aassettings.phys_friction;
	sv_stopspeed = aassettings.phys_stopspeed;
	sv_gravity = aassettings.phys_gravity;
	sv_waterfriction = aassettings.phys_waterfriction;
	sv_watergravity = aassettings.phys_watergravity;
	sv_maxwalkvelocity = aassettings.phys_maxwalkvelocity;// * frametime;
	sv_maxcrouchvelocity = aassettings.phys_maxcrouchvelocity;// * frametime;
	sv_maxswimvelocity = aassettings.phys_maxswimvelocity;// * frametime;
	sv_walkaccelerate = aassettings.phys_walkaccelerate;
	sv_airaccelerate = aassettings.phys_airaccelerate;
	sv_swimaccelerate = aassettings.phys_swimaccelerate;
	sv_maxstep = aassettings.phys_maxstep;
	sv_maxsteepness = aassettings.phys_maxsteepness;
	sv_jumpvel = aassettings.phys_jumpvel * frametime;
	//
	memset(move, 0, sizeof(aas_clientmove_t));
	memset(&trace, 0, sizeof(aas_trace_t));
	//start at the current origin
	VectorCopy(origin, org);
	org[2] += 0.25;
	//velocity to test for the first frame
	VectorScale(velocity, frametime, frame_test_vel);
	//
	jump_frame = -1;
	//predict a maximum of 'maxframes' ahead
	for (n = 0; n < maxframes; n++)
	{
		swimming = AAS_Swimming(org);
		//get gravity depending on swimming or not
		gravity = swimming ? sv_watergravity : sv_gravity;
		//apply gravity at the START of the frame
		frame_test_vel[2] = frame_test_vel[2] - (gravity * 0.1 * frametime);
		//if on the ground or swimming
		if (onground || swimming)
		{
			friction = swimming ? sv_friction : sv_waterfriction;
			//apply friction
			VectorScale(frame_test_vel, 1 / frametime, frame_test_vel);
			AAS_ApplyFriction(frame_test_vel, friction, sv_stopspeed, frametime);
			VectorScale(frame_test_vel, frametime, frame_test_vel);
		}	//end if
		crouch = qfalse;
		//apply command movement
		if (n < cmdframes)
		{
			ax = 0;
			maxvel = sv_maxwalkvelocity;
			accelerate = sv_airaccelerate;
			VectorCopy(cmdmove, wishdir);
			if (onground)
			{
				if (cmdmove[2] < -300)
				{
					crouch = qtrue;
					maxvel = sv_maxcrouchvelocity;
				}	//end if
					//if not swimming and upmove is positive then jump
				if (!swimming && cmdmove[2] > 1)
				{
					//jump velocity minus the gravity for one frame + 5 for safety
					frame_test_vel[2] = sv_jumpvel - (gravity * 0.1 * frametime) + 5;
					jump_frame = n;
					//jumping so air accelerate
					accelerate = sv_airaccelerate;
				}	//end if
				else
				{
					accelerate = sv_walkaccelerate;
				}	//end else
				ax = 2;
			}	//end if
			if (swimming)
			{
				maxvel = sv_maxswimvelocity;
				accelerate = sv_swimaccelerate;
				ax = 3;
			}	//end if
			else
			{
				wishdir[2] = 0;
			}	//end else
				//
			wishspeed = VectorNormalize(wishdir);
			if (wishspeed > maxvel)
			{
				wishspeed = maxvel;
			}
			VectorScale(frame_test_vel, 1 / frametime, frame_test_vel);
			AAS_Accelerate(frame_test_vel, frametime, wishdir, wishspeed, accelerate);
			VectorScale(frame_test_vel, frametime, frame_test_vel);
			/*
			for (i = 0; i < ax; i++)
			{
			    velchange = (cmdmove[i] * frametime) - frame_test_vel[i];
			    if (velchange > sv_maxacceleration) velchange = sv_maxacceleration;
			    else if (velchange < -sv_maxacceleration) velchange = -sv_maxacceleration;
			    newvel = frame_test_vel[i] + velchange;
			    //
			    if (frame_test_vel[i] <= maxvel && newvel > maxvel) frame_test_vel[i] = maxvel;
			    else if (frame_test_vel[i] >= -maxvel && newvel < -maxvel) frame_test_vel[i] = -maxvel;
			    else frame_test_vel[i] = newvel;
			} //end for
			*/
		}	//end if
		if (crouch)
		{
			presencetype = PRESENCE_CROUCH;
		}	//end if
		else if (presencetype == PRESENCE_CROUCH)
		{
			if (AAS_PointPresenceType(org) & PRESENCE_NORMAL)
			{
				presencetype = PRESENCE_NORMAL;
			}	//end if
		}	//end else
			//save the current origin
		VectorCopy(org, lastorg);
		//move linear during one frame
		VectorCopy(frame_test_vel, left_test_vel);
		j = 0;
		do
		{
			VectorAdd(org, left_test_vel, end);
			//trace a bounding box
			trace = AAS_TraceClientBBox(org, end, presencetype, entnum);
			//
			if (visualize)
			{
				if (trace.startsolid)
				{
					BotImport_Print(PRT_MESSAGE, "PredictMovement: start solid\n");
				}
				AAS_DebugLine(org, trace.endpos, LINECOLOR_RED);
			}	//end if
			//
			if (stopevent & SE_ENTERAREA)
			{
				numareas = AAS_TraceAreas(org, trace.endpos, areas, points, 20);
				for (i = 0; i < numareas; i++)
				{
					if (areas[i] == stopareanum)
					{
						VectorCopy(points[i], move->endpos);
						VectorScale(frame_test_vel, 1 / frametime, move->velocity);
						move->trace = trace;
						move->stopevent = SE_ENTERAREA;
						move->presencetype = presencetype;
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return qtrue;
					}	//end if
				}	//end for
			}	//end if
				//move the entity to the trace end point
			VectorCopy(trace.endpos, org);
			//if there was a collision
			if (trace.fraction < 1.0)
			{
				//get the plane the bounding box collided with
				plane = AAS_PlaneFromNum(trace.planenum);
				//
				if (stopevent & SE_HITGROUNDAREA)
				{
					if (DotProduct(plane->normal, up) > sv_maxsteepness)
					{
						VectorCopy(org, start);
						start[2] += 0.5;
						if (AAS_PointAreaNum(start) == stopareanum)
						{
							VectorCopy(start, move->endpos);
							VectorScale(frame_test_vel, 1 / frametime, move->velocity);
							move->trace = trace;
							move->stopevent = SE_HITGROUNDAREA;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}	//end if
					}	//end if
				}	//end if
					//assume there's no step
				step = qfalse;
				//if it is a vertical plane and the bot didn't jump recently
				if (plane->normal[2] == 0 && (jump_frame < 0 || n - jump_frame > 2))
				{
					//check for a step
					VectorMA(org, -0.25, plane->normal, start);
					VectorCopy(start, stepend);
					start[2] += sv_maxstep;
					steptrace = AAS_TraceClientBBox(start, stepend, presencetype, entnum);
					//
					if (!steptrace.startsolid)
					{
						plane2 = AAS_PlaneFromNum(steptrace.planenum);
						if (DotProduct(plane2->normal, up) > sv_maxsteepness)
						{
							VectorSubtract(end, steptrace.endpos, left_test_vel);
							left_test_vel[2] = 0;
							frame_test_vel[2] = 0;
							if (visualize)
							{
								if (steptrace.endpos[2] - org[2] > 0.125)
								{
									VectorCopy(org, start);
									start[2] = steptrace.endpos[2];
									AAS_DebugLine(org, start, LINECOLOR_BLUE);
								}	//end if
							}	//end if
							org[2] = steptrace.endpos[2];
							step = qtrue;
						}	//end if
					}	//end if
				}	//end if
					//
				if (!step)
				{
					//velocity left to test for this frame is the projection
					//of the current test velocity into the hit plane
					VectorMA(left_test_vel, -DotProduct(left_test_vel, plane->normal),
						plane->normal, left_test_vel);
					//store the old velocity for landing check
					VectorCopy(frame_test_vel, old_frame_test_vel);
					//test velocity for the next frame is the projection
					//of the velocity of the current frame into the hit plane
					VectorMA(frame_test_vel, -DotProduct(frame_test_vel, plane->normal),
						plane->normal, frame_test_vel);
					//check for a landing on an almost horizontal floor
					if (DotProduct(plane->normal, up) > sv_maxsteepness)
					{
						onground = qtrue;
					}	//end if
					if (stopevent & SE_HITGROUNDDAMAGE)
					{
						delta = 0;
						if (old_frame_test_vel[2] < 0 &&
							frame_test_vel[2] > old_frame_test_vel[2] &&
							!onground)
						{
							delta = old_frame_test_vel[2];
						}	//end if
						else if (onground)
						{
							delta = frame_test_vel[2] - old_frame_test_vel[2];
						}	//end else
						if (delta)
						{
							delta = delta * 10;
							delta = delta * delta * 0.0001;
							if (swimming)
							{
								delta = 0;
							}
							// never take falling damage if completely underwater
							/*
							if (ent->waterlevel == 3) return;
							if (ent->waterlevel == 2) delta *= 0.25;
							if (ent->waterlevel == 1) delta *= 0.5;
							*/
							if (delta > 40)
							{
								VectorCopy(org, move->endpos);
								VectorCopy(frame_test_vel, move->velocity);
								move->trace = trace;
								move->stopevent = SE_HITGROUNDDAMAGE;
								move->presencetype = presencetype;
								move->endcontents = 0;
								move->time = n * frametime;
								move->frames = n;
								return qtrue;
							}	//end if
						}	//end if
					}	//end if
				}	//end if
			}	//end if
				//extra check to prevent endless loop
			if (++j > 20)
			{
				return qfalse;
			}
			//while there is a plane hit
		}
		while (trace.fraction < 1.0);
		//if going down
		if (frame_test_vel[2] <= 10)
		{
			//check for a liquid at the feet of the bot
			VectorCopy(org, feet);
			feet[2] -= 22;
			pc = AAS_PointContents(feet);
			//get event from pc
			event = SE_NONE;
			if (pc & BSP46CONTENTS_LAVA)
			{
				event |= SE_ENTERLAVA;
			}
			if (pc & BSP46CONTENTS_SLIME)
			{
				event |= SE_ENTERSLIME;
			}
			if (pc & BSP46CONTENTS_WATER)
			{
				event |= SE_ENTERWATER;
			}
			//
			areanum = AAS_PointAreaNum(org);
			if (aasworld->areasettings[areanum].contents & AREACONTENTS_LAVA)
			{
				event |= SE_ENTERLAVA;
			}
			if (aasworld->areasettings[areanum].contents & AREACONTENTS_SLIME)
			{
				event |= SE_ENTERSLIME;
			}
			if (aasworld->areasettings[areanum].contents & AREACONTENTS_WATER)
			{
				event |= SE_ENTERWATER;
			}
			//if in lava or slime
			if (event & stopevent)
			{
				VectorCopy(org, move->endpos);
				VectorScale(frame_test_vel, 1 / frametime, move->velocity);
				move->stopevent = event & stopevent;
				move->presencetype = presencetype;
				move->endcontents = pc;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}	//end if
		}	//end if
			//
		onground = AAS_OnGround(org, presencetype, entnum);
		//if onground and on the ground for at least one whole frame
		if (onground)
		{
			if (stopevent & SE_HITGROUND)
			{
				VectorCopy(org, move->endpos);
				VectorScale(frame_test_vel, 1 / frametime, move->velocity);
				move->trace = trace;
				move->stopevent = SE_HITGROUND;
				move->presencetype = presencetype;
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}	//end if
		}	//end if
		else if (stopevent & SE_LEAVEGROUND)
		{
			VectorCopy(org, move->endpos);
			VectorScale(frame_test_vel, 1 / frametime, move->velocity);
			move->trace = trace;
			move->stopevent = SE_LEAVEGROUND;
			move->presencetype = presencetype;
			move->endcontents = 0;
			move->time = n * frametime;
			move->frames = n;
			return qtrue;
		}	//end else if
		else if (stopevent & SE_GAP)
		{
			aas_trace_t gaptrace;

			VectorCopy(org, start);
			VectorCopy(start, end);
			end[2] -= 48 + aassettings.phys_maxbarrier;
			gaptrace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
			//if solid is found the bot cannot walk any further and will not fall into a gap
			if (!gaptrace.startsolid)
			{
				//if it is a gap (lower than one step height)
				if (gaptrace.endpos[2] < org[2] - aassettings.phys_maxstep - 1)
				{
					if (!(AAS_PointContents(end) & (BSP46CONTENTS_WATER | BSP46CONTENTS_SLIME)))			//----(SA)	modified since slime is no longer deadly
					{	//					if (!(AAS_PointContents(end) & BSP46CONTENTS_WATER))
						VectorCopy(lastorg, move->endpos);
						VectorScale(frame_test_vel, 1 / frametime, move->velocity);
						move->trace = trace;
						move->stopevent = SE_GAP;
						move->presencetype = presencetype;
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return qtrue;
					}	//end if
				}	//end if
			}	//end if
		}	//end else if
		if (stopevent & SE_TOUCHJUMPPAD)
		{
			if (aasworld->areasettings[AAS_PointAreaNum(org)].contents & AREACONTENTS_JUMPPAD)
			{
				VectorCopy(org, move->endpos);
				VectorScale(frame_test_vel, 1 / frametime, move->velocity);
				move->trace = trace;
				move->stopevent = SE_TOUCHJUMPPAD;
				move->presencetype = presencetype;
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}	//end if
		}	//end if
		if (stopevent & SE_TOUCHTELEPORTER)
		{
			if (aasworld->areasettings[AAS_PointAreaNum(org)].contents & AREACONTENTS_TELEPORTER)
			{
				VectorCopy(org, move->endpos);
				VectorScale(frame_test_vel, 1 / frametime, move->velocity);
				move->trace = trace;
				move->stopevent = SE_TOUCHTELEPORTER;
				move->presencetype = presencetype;
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}	//end if
		}	//end if
	}	//end for
		//
	VectorCopy(org, move->endpos);
	VectorScale(frame_test_vel, 1 / frametime, move->velocity);
	move->stopevent = SE_NONE;
	move->presencetype = presencetype;
	move->endcontents = 0;
	move->time = n * frametime;
	move->frames = n;
	//
	return qtrue;
}	//end of the function AAS_PredictClientMovement
