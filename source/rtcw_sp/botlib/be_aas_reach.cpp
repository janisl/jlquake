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
 * name:		be_aas_reach.c
 *
 * desc:		reachability calculations
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "../../common/file_formats/bsp47.h"

//===========================================================================
// creates possible jump reachabilities between the areas
//
// The two closest points on the ground of the areas are calculated
// One of the points will be on an edge of a ground face of area1 and
// one on an edge of a ground face of area2.
// If there is a range of closest points the point in the middle of this range
// is selected.
// Between these two points there must be one or more gaps.
// If the gaps exist a potential jump is predicted.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Jump(int area1num, int area2num)
{
	int i, j, k, l, face1num, face2num, edge1num, edge2num, traveltype;
	int stopevent, areas[10], numareas;
	float phys_jumpvel, maxjumpdistance, maxjumpheight, height, bestdist, speed;
	vec_t* v1, * v2, * v3, * v4;
	vec3_t beststart, beststart2, bestend, bestend2;
	vec3_t teststart, testend, dir, velocity, cmdmove, up = {0, 0, 1}, sidewards;
	aas_area_t* area1, * area2;
	aas_face_t* face1, * face2;
	aas_edge_t* edge1, * edge2;
	aas_plane_t* plane1, * plane2, * plane;
	aas_trace_t trace;
	aas_clientmove_rtcw_t move;
	aas_lreachability_t* lreach;

	if (!AAS_AreaGrounded(area1num) || !AAS_AreaGrounded(area2num))
	{
		return qfalse;
	}
	//cannot jump from or to a crouch area
	if (AAS_AreaCrouch(area1num) || AAS_AreaCrouch(area2num))
	{
		return qfalse;
	}
	//
	area1 = &aasworld->areas[area1num];
	area2 = &aasworld->areas[area2num];

	// RF, check for a forced jump reachability
	if (GGameType & GAME_ET && aasworld->areasettings[area1num].areaflags & AREA_JUMPSRC)
	{
		if (jumplinks[area1num].destarea == area2num)
		{
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return qfalse;
			}
			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = 0;
			VectorCopy(jumplinks[area1num].srcpos, lreach->start);
			VectorCopy(jumplinks[area1num].destpos, lreach->end);
			lreach->traveltype = TRAVEL_JUMP;

			VectorCopy(jumplinks[area1num].srcpos, beststart);
			VectorCopy(jumplinks[area1num].destpos, bestend);
			VectorSubtract(bestend, beststart, dir);
			height = dir[2];
			dir[2] = 0;
			lreach->traveltime = STARTJUMP_TIME + VectorDistance(beststart, bestend) * 240 / aassettings.phys_maxwalkvelocity;
			//
			if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta5)
			{
				lreach->traveltime += aassettings.rs_falldamage5;
			}	//end if
			else if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta10)
			{
				lreach->traveltime += aassettings.rs_falldamage10;
			}	//end if
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_jump++;
			return qtrue;
		}
	}
	//
	phys_jumpvel = aassettings.phys_jumpvel;
	//maximum distance a player can jump
	maxjumpdistance = 2 * AAS_MaxJumpDistance(phys_jumpvel);
	//maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight(phys_jumpvel);

	//if the areas are not near anough in the x-y direction
	for (i = 0; i < 2; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + maxjumpdistance)
		{
			return qfalse;
		}
		if (area1->maxs[i] < area2->mins[i] - maxjumpdistance)
		{
			return qfalse;
		}
	}	//end for
		//if area2 is way to high to jump up to
	if (area2->mins[2] > area1->maxs[2] + maxjumpheight)
	{
		return qfalse;
	}
	//
	bestdist = 999999;
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		face1num = aasworld->faceindex[area1->firstface + i];
		face1 = &aasworld->faces[abs(face1num)];
		//if not a ground face
		if (!(face1->faceflags & FACE_GROUND))
		{
			continue;
		}
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2num = aasworld->faceindex[area2->firstface + j];
			face2 = &aasworld->faces[abs(face2num)];
			//if not a ground face
			if (!(face2->faceflags & FACE_GROUND))
			{
				continue;
			}
			//
			for (k = 0; k < face1->numedges; k++)
			{
				edge1num = abs(aasworld->edgeindex[face1->firstedge + k]);
				edge1 = &aasworld->edges[edge1num];
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = abs(aasworld->edgeindex[face2->firstedge + l]);
					edge2 = &aasworld->edges[edge2num];
					//calculate the minimum distance between the two edges
					v1 = aasworld->vertexes[edge1->v[0]];
					v2 = aasworld->vertexes[edge1->v[1]];
					v3 = aasworld->vertexes[edge2->v[0]];
					v4 = aasworld->vertexes[edge2->v[1]];
					//get the ground planes
					plane1 = &aasworld->planes[face1->planenum];
					plane2 = &aasworld->planes[face2->planenum];
					//
					bestdist = AAS_ClosestEdgePoints(v1, v2, v3, v4, plane1, plane2,
						beststart, bestend,
						beststart2, bestend2, bestdist);
				}	//end for
			}	//end for
		}	//end for
	}	//end for
	VectorMiddle(beststart, beststart2, beststart);
	VectorMiddle(bestend, bestend2, bestend);
	if (bestdist > 4 && bestdist < maxjumpdistance)
	{
		// if very close and almost no height difference then the bot can walk
		if (bestdist <= 48 && Q_fabs(beststart[2] - bestend[2]) < 8)
		{
			speed = 400;
			traveltype = TRAVEL_WALKOFFLEDGE;
		}	//end else if
		else if (AAS_HorizontalVelocityForJump(0, beststart, bestend, &speed))
		{
			//FIXME: why multiply with 1.2???
			speed *= 1.2;
			traveltype = TRAVEL_WALKOFFLEDGE;
		}	//end if
		else
		{
			//get the horizontal speed for the jump, if it isn't possible to calculate this
			//speed (the jump is not possible) then there's no jump reachability created
			if (!AAS_HorizontalVelocityForJump(phys_jumpvel, beststart, bestend, &speed))
			{
				return qfalse;
			}
			if (GGameType & GAME_Quake3)
			{
				speed *= 1.05f;
			}
			traveltype = TRAVEL_JUMP;
			//
			//NOTE: test if the horizontal distance isn't too small
			VectorSubtract(bestend, beststart, dir);
			dir[2] = 0;
			if (VectorLength(dir) < 10)
			{
				return qfalse;
			}
		}	//end if
			//
		VectorSubtract(bestend, beststart, dir);
		VectorNormalize(dir);
		VectorMA(beststart, 1, dir, teststart);
		//
		VectorCopy(teststart, testend);
		testend[2] -= 100;
		trace = AAS_TraceClientBBox(teststart, testend, PRESENCE_NORMAL, -1);
		//
		if (trace.startsolid)
		{
			return qfalse;
		}
		if (trace.fraction < 1)
		{
			plane = &aasworld->planes[trace.planenum];
			// if the bot can stand on the surface
			if (DotProduct(plane->normal, up) >= 0.7)
			{
				// if no lava or slime below
				//----(SA)	modified since slime is no longer deadly
				if (!(AAS_PointContents(trace.endpos) & (GGameType & GAME_Quake3 ?
					(BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME) : BSP46CONTENTS_LAVA)))
				{
					if (teststart[2] - trace.endpos[2] <= aassettings.phys_maxbarrier)
					{
						return qfalse;
					}
				}	//end if
			}	//end if
		}	//end if
			//
		VectorMA(bestend, -1, dir, teststart);
		//
		VectorCopy(teststart, testend);
		testend[2] -= 100;
		trace = AAS_TraceClientBBox(teststart, testend, PRESENCE_NORMAL, -1);
		//
		if (trace.startsolid)
		{
			return qfalse;
		}
		if (trace.fraction < 1)
		{
			plane = &aasworld->planes[trace.planenum];
			// if the bot can stand on the surface
			if (DotProduct(plane->normal, up) >= 0.7)
			{
				// if no lava or slime below
				if (!(AAS_PointContents(trace.endpos) & (GGameType & GAME_ET ?
					BSP46CONTENTS_LAVA : (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME))))
				{
					if (teststart[2] - trace.endpos[2] <= aassettings.phys_maxbarrier)
					{
						return qfalse;
					}
				}	//end if
			}	//end if
		}	//end if
			//
		//get command movement
		VectorClear(cmdmove);
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP)
		{
			cmdmove[2] = aassettings.phys_jumpvel;
		}
		else
		{
			cmdmove[2] = 0;
		}
		//
		VectorSubtract(bestend, beststart, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		if (GGameType & GAME_Quake3)
		{
			CrossProduct(dir, up, sidewards);
			//
			stopevent = SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE;
			if (!AAS_AreaClusterPortal(area1num) && !AAS_AreaClusterPortal(area2num))
			{
				stopevent |= Q3SE_TOUCHCLUSTERPORTAL;
			}
			//
			for (i = 0; i < 3; i++)
			{
				//
				if (i == 1)
				{
					VectorAdd(testend, sidewards, testend);
				}
				else if (i == 2)
				{
					VectorSubtract(bestend, sidewards, testend);
				}
				else
				{
					VectorCopy(bestend, testend);
				}
				VectorSubtract(testend, beststart, dir);
				dir[2] = 0;
				VectorNormalize(dir);
				VectorScale(dir, speed, velocity);
				//
				AAS_PredictClientMovementWolf(&move, -1, beststart, PRESENCE_NORMAL, true,
					velocity, cmdmove, 3, 30, 0.1f,
					stopevent, 0, false);
				// if prediction time wasn't enough to fully predict the movement
				if (move.frames >= 30)
				{
					return false;
				}
				// don't enter slime or lava and don't fall from too high
				if (move.stopevent & (SE_ENTERSLIME | SE_ENTERLAVA))
				{
					return false;
				}
				// never jump or fall through a cluster portal
				if (move.stopevent & Q3SE_TOUCHCLUSTERPORTAL)
				{
					return false;
				}
				//the end position should be in area2, also test a little bit back
				//because the predicted jump could have rushed through the area
				VectorMA(move.endpos, -64, dir, teststart);
				teststart[2] += 1;
				numareas = AAS_TraceAreas(move.endpos, teststart, areas, NULL, sizeof(areas) / sizeof(int));
				for (j = 0; j < numareas; j++)
				{
					if (areas[j] == area2num)
					{
						break;
					}
				}	//end for
				if (j < numareas)
				{
					break;
				}
			}
			if (i >= 3)
			{
				return false;
			}
		}
		else
		{
			VectorScale(dir, speed, velocity);
			AAS_PredictClientMovementWolf(&move, -1, beststart, PRESENCE_NORMAL, qtrue,
				velocity, cmdmove, 3, 30, 0.1,
				SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME |
				SE_ENTERLAVA | SE_HITGROUNDDAMAGE, 0, qfalse);
			//if prediction time wasn't enough to fully predict the movement
			if (move.frames >= 30)
			{
				return qfalse;
			}
			//don't enter slime or lava and don't fall from too high
			if (move.stopevent & SE_ENTERLAVA)
			{
				return qfalse;									//----(SA)	modified since slime is no longer deadly
			}
	//		if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA)) return qfalse;
			//the end position should be in area2, also test a little bit back
			//because the predicted jump could have rushed through the area
			for (i = 0; i <= 32; i += 8)
			{
				VectorMA(move.endpos, -i, dir, teststart);
				teststart[2] += 0.125;
				if (AAS_PointAreaNum(teststart) == area2num)
				{
					break;
				}
			}	//end for
			if (i > 32)
			{
				return qfalse;
			}
		}

		//create a new reachability link
		lreach = AAS_AllocReachability();
		if (!lreach)
		{
			return qfalse;
		}
		lreach->areanum = area2num;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy(beststart, lreach->start);
		VectorCopy(bestend, lreach->end);
		lreach->traveltype = traveltype;

		VectorSubtract(bestend, beststart, dir);
		height = dir[2];
		dir[2] = 0;
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_WALKOFFLEDGE && height > VectorLength(dir))
		{
			lreach->traveltime = aassettings.rs_startwalkoffledge + height * 50 / aassettings.phys_gravity;
		}
		else
		{
			lreach->traveltime = aassettings.rs_startjump + VectorDistance(bestend, beststart) * 240 / aassettings.phys_maxwalkvelocity;
		}	//end if
			//
		if (!AAS_AreaJumpPad(area2num))
		{
			if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta5)
			{
				lreach->traveltime += aassettings.rs_falldamage5;
			}	//end if
			else if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta10)
			{
				lreach->traveltime += aassettings.rs_falldamage10;
			}	//end if
		}	//end if
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP)
		{
			reach_jump++;
		}
		else
		{
			reach_walkoffledge++;
		}

		if (GGameType & GAME_ET)
		{
			return true;
		}
	}	//end if
	return qfalse;
}	//end of the function AAS_Reachability_Jump
//===========================================================================
// create possible teleporter reachabilities
// this is very game dependent.... :(
//
// classname = trigger_multiple or trigger_teleport
// target = "t1"
//
// classname = target_teleporter
// targetname = "t1"
// target = "t2"
//
// classname = misc_teleporter_dest
// targetname = "t2"
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_Teleport(void)
{
	int area1num, area2num;
	char target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];
	char classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	int ent, dest;
	float angle;
	vec3_t destorigin, mins, maxs, end, angles;
	vec3_t mid, velocity, cmdmove;
	aas_lreachability_t* lreach;
	aas_clientmove_rtcw_t move;
	aas_trace_t trace;
	aas_link_t* areas, * link;

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (!String::Cmp(classname, "trigger_multiple"))
		{
			AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
			BotImport_Print(PRT_MESSAGE, "trigger_multiple model = \"%s\"\n", model);
			VectorClear(angles);
			AAS_BSPModelMinsMaxs(String::Atoi(model + 1), angles, mins, maxs);
			//
			if (!AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY))
			{
				BotImport_Print(PRT_ERROR, "trigger_multiple without target\n");
				continue;
			}	//end if
			for (dest = AAS_NextBSPEntity(0); dest; dest = AAS_NextBSPEntity(dest))
			{
				if (!AAS_ValueForBSPEpairKey(dest, "classname", classname, MAX_EPAIRKEY))
				{
					continue;
				}
				if (!String::Cmp(classname, "target_teleporter"))
				{
					if (!AAS_ValueForBSPEpairKey(dest, "targetname", targetname, MAX_EPAIRKEY))
					{
						continue;
					}
					if (!String::Cmp(targetname, target))
					{
						break;
					}	//end if
				}	//end if
			}	//end for
			if (!dest)
			{
				continue;
			}	//end if
			if (!AAS_ValueForBSPEpairKey(dest, "target", target, MAX_EPAIRKEY))
			{
				BotImport_Print(PRT_ERROR, "target_teleporter without target\n");
				continue;
			}	//end if
		}	//end else
		else if (!String::Cmp(classname, "trigger_teleport"))
		{
			AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
			BotImport_Print(PRT_MESSAGE, "trigger_teleport model = \"%s\"\n", model);
			VectorClear(angles);
			AAS_BSPModelMinsMaxs(String::Atoi(model + 1), angles, mins, maxs);
			//
			if (!AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY))
			{
				BotImport_Print(PRT_ERROR, "trigger_teleport without target\n");
				continue;
			}	//end if
		}	//end if
		else
		{
			continue;
		}	//end else
			//
		for (dest = AAS_NextBSPEntity(0); dest; dest = AAS_NextBSPEntity(dest))
		{
			//classname should be misc_teleporter_dest
			//but I've also seen target_position and actually any
			//entity could be used... burp
			if (AAS_ValueForBSPEpairKey(dest, "targetname", targetname, MAX_EPAIRKEY))
			{
				if (!String::Cmp(targetname, target))
				{
					break;
				}	//end if
			}	//end if
		}	//end for
		if (!dest)
		{
			BotImport_Print(PRT_ERROR, "teleporter without misc_teleporter_dest (%s)\n", target);
			continue;
		}	//end if
		if (!AAS_VectorForBSPEpairKey(dest, "origin", destorigin))
		{
			BotImport_Print(PRT_ERROR, "teleporter destination (%s) without origin\n", target);
			continue;
		}	//end if
			//
		area2num = AAS_PointAreaNum(destorigin);
		//if not teleported into a teleporter or into a jumppad
		if (!AAS_AreaTeleporter(area2num) && !AAS_AreaJumpPad(area2num))
		{
			if (!GGameType & GAME_Quake3)
			{
				destorigin[2] += 24;//just for q2e1m2, the dork has put the telepads in the ground
			}
			VectorCopy(destorigin, end);
			if (GGameType & GAME_Quake3)
			{
				end[2] -= 64;
			}
			else
			{
				end[2] -= 100;
			}
			trace = AAS_TraceClientBBox(destorigin, end, PRESENCE_CROUCH, -1);
			if (trace.startsolid)
			{
				BotImport_Print(PRT_ERROR, "teleporter destination (%s) in solid\n", target);
				continue;
			}	//end if
			if (GGameType & GAME_Quake3)
			{
				area2num = AAS_PointAreaNum(trace.endpos);

				//predict where you'll end up
				AAS_FloatForBSPEpairKey(dest, "angle", &angle);
				if (angle)
				{
					VectorSet(angles, 0, angle, 0);
					AngleVectors(angles, velocity, NULL, NULL);
					VectorScale(velocity, 400, velocity);
				}	//end if
				else
				{
					VectorClear(velocity);
				}	//end else
				VectorClear(cmdmove);
				AAS_PredictClientMovementWolf(&move, -1, destorigin, PRESENCE_NORMAL, false,
					velocity, cmdmove, 0, 30, 0.1f,
					SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME |
					SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER, 0, false);				//true);
				area2num = AAS_PointAreaNum(move.endpos);
				if (move.stopevent & (SE_ENTERSLIME | SE_ENTERLAVA))
				{
					BotImport_Print(PRT_WARNING, "teleported into slime or lava at dest %s\n", target);
				}
				VectorCopy(move.endpos, destorigin);
			}
			else
			{
				VectorCopy(trace.endpos, destorigin);
				area2num = AAS_PointAreaNum(destorigin);
			}
		}	//end if
			//
			//BotImport_Print(PRT_MESSAGE, "teleporter brush origin at %f %f %f\n", origin[0], origin[1], origin[2]);
			//BotImport_Print(PRT_MESSAGE, "teleporter brush mins = %f %f %f\n", mins[0], mins[1], mins[2]);
			//BotImport_Print(PRT_MESSAGE, "teleporter brush maxs = %f %f %f\n", maxs[0], maxs[1], maxs[2]);
		//
		VectorAdd(mins, maxs, mid);
		VectorScale(mid, 0.5, mid);
		//link an invalid (-1) entity
		areas = AAS_LinkEntityClientBBox(mins, maxs, -1, PRESENCE_CROUCH);
		if (!areas)
		{
			BotImport_Print(PRT_MESSAGE, "trigger_multiple not in any area\n");
		}
		//
		for (link = areas; link; link = link->next_area)
		{
			//if (!AAS_AreaGrounded(link->areanum)) continue;
			if (!AAS_AreaTeleporter(link->areanum))
			{
				continue;
			}
			//
			area1num = link->areanum;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				break;
			}
			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = 0;
			VectorCopy(mid, lreach->start);
			VectorCopy(destorigin, lreach->end);
			lreach->traveltype = TRAVEL_TELEPORT;
			lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
			lreach->traveltime = aassettings.rs_teleport;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_teleport++;
		}	//end for
			//unlink the invalid entity
		AAS_UnlinkFromAreas(areas);
	}	//end for
}	//end of the function AAS_Reachability_Teleport
//===========================================================================
// create possible elevator (func_plat) reachabilities
// this is very game dependent.... :(
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_Elevator(void)
{
	int area1num, area2num, modelnum, i, j, k, l, n, p;
	float lip, height, speed;
	char model[MAX_EPAIRKEY], classname[MAX_EPAIRKEY];
	int ent;
	vec3_t mins, maxs, origin, angles = {0, 0, 0};
	vec3_t pos1, pos2, mids, platbottom, plattop;
	vec3_t bottomorg, toporg, start, end, dir;
	vec_t xvals[8], yvals[8], xvals_top[8], yvals_top[8];
	aas_lreachability_t* lreach;
	aas_trace_t trace;

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (!String::Cmp(classname, "func_plat"))
		{
			if (!AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY))
			{
				BotImport_Print(PRT_ERROR, "func_plat without model\n");
				continue;
			}	//end if
				//get the model number, and skip the leading *
			modelnum = String::Atoi(model + 1);
			if (modelnum <= 0)
			{
				BotImport_Print(PRT_ERROR, "func_plat with invalid model number\n");
				continue;
			}	//end if
				//get the mins, maxs and origin of the model
				//NOTE: the origin is usually (0,0,0) and the mins and maxs
				//      are the absolute mins and maxs
			AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);
			//
			AAS_VectorForBSPEpairKey(ent, "origin", origin);
			//pos1 is the top position, pos2 is the bottom
			VectorCopy(origin, pos1);
			VectorCopy(origin, pos2);
			//get the lip of the plat
			AAS_FloatForBSPEpairKey(ent, "lip", &lip);
			if (!lip)
			{
				lip = 8;
			}
			//get the movement height of the plat
			AAS_FloatForBSPEpairKey(ent, "height", &height);
			if (!height)
			{
				height = (maxs[2] - mins[2]) - lip;
			}
			//get the speed of the plat
			AAS_FloatForBSPEpairKey(ent, "speed", &speed);
			if (!speed)
			{
				speed = 200;
			}
			//get bottom position below pos1
			pos2[2] -= height;
			//
			BotImport_Print(PRT_MESSAGE, "pos2[2] = %1.1f pos1[2] = %1.1f\n", pos2[2], pos1[2]);
			//get a point just above the plat in the bottom position
			VectorAdd(mins, maxs, mids);
			VectorMA(pos2, 0.5, mids, platbottom);
			platbottom[2] = maxs[2] - (pos1[2] - pos2[2]) + 2;
			//get a point just above the plat in the top position
			VectorAdd(mins, maxs, mids);
			VectorMA(pos2, 0.5, mids, plattop);
			plattop[2] = maxs[2] + 2;
			//
			/*if (!area1num)
			{
			    Log_Write("no grounded area near plat bottom\r\n");
			    continue;
			} //end if*/
			//get the mins and maxs a little larger
			for (i = 0; i < 3; i++)
			{
				mins[i] -= 1;
				maxs[i] += 1;
			}	//end for
				//
			BotImport_Print(PRT_MESSAGE, "platbottom[2] = %1.1f plattop[2] = %1.1f\n", platbottom[2], plattop[2]);
			//
			VectorAdd(mins, maxs, mids);
			VectorScale(mids, 0.5, mids);
			//
			xvals[0] = mins[0]; xvals[1] = mids[0]; xvals[2] = maxs[0]; xvals[3] = mids[0];
			yvals[0] = mids[1]; yvals[1] = maxs[1]; yvals[2] = mids[1]; yvals[3] = mins[1];
			//
			xvals[4] = mins[0]; xvals[5] = maxs[0]; xvals[6] = maxs[0]; xvals[7] = mins[0];
			yvals[4] = maxs[1]; yvals[5] = maxs[1]; yvals[6] = mins[1]; yvals[7] = mins[1];
			//find adjacent areas around the bottom of the plat
			for (i = 0; i < 9; i++)
			{
				if (i < 8)		//check at the sides of the plat
				{
					bottomorg[0] = origin[0] + xvals[i];
					bottomorg[1] = origin[1] + yvals[i];
					bottomorg[2] = platbottom[2] + 16;
					//get a grounded or swim area near the plat in the bottom position
					area1num = AAS_PointAreaNum(bottomorg);
					for (k = 0; k < 16; k++)
					{
						if (area1num)
						{
							if (AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num))
							{
								break;
							}
						}	//end if
						bottomorg[2] += 4;
						area1num = AAS_PointAreaNum(bottomorg);
					}	//end if
						//if in solid
					if (k >= 16)
					{
						continue;
					}	//end if
				}	//end if
				else//at the middle of the plat
				{
					VectorCopy(plattop, bottomorg);
					bottomorg[2] += 24;
					area1num = AAS_PointAreaNum(bottomorg);
					if (!area1num)
					{
						continue;
					}
					VectorCopy(platbottom, bottomorg);
					bottomorg[2] += 24;
				}	//end else
					//look at adjacent areas around the top of the plat
					//make larger steps to outside the plat everytime
				for (n = 0; n < 3; n++)
				{
					for (k = 0; k < 3; k++)
					{
						mins[k] -= 4;
						maxs[k] += 4;
					}	//end for
					xvals_top[0] = mins[0]; xvals_top[1] = mids[0]; xvals_top[2] = maxs[0]; xvals_top[3] = mids[0];
					yvals_top[0] = mids[1]; yvals_top[1] = maxs[1]; yvals_top[2] = mids[1]; yvals_top[3] = mins[1];
					//
					xvals_top[4] = mins[0]; xvals_top[5] = maxs[0]; xvals_top[6] = maxs[0]; xvals_top[7] = mins[0];
					yvals_top[4] = maxs[1]; yvals_top[5] = maxs[1]; yvals_top[6] = mins[1]; yvals_top[7] = mins[1];
					//
					for (j = 0; j < 8; j++)
					{
						toporg[0] = origin[0] + xvals_top[j];
						toporg[1] = origin[1] + yvals_top[j];
						toporg[2] = plattop[2] + 16;
						//get a grounded or swim area near the plat in the top position
						area2num = AAS_PointAreaNum(toporg);
						for (l = 0; l < 16; l++)
						{
							if (area2num)
							{
								if (AAS_AreaGrounded(area2num) || AAS_AreaSwim(area2num))
								{
									VectorCopy(plattop, start);
									start[2] += 32;
									VectorCopy(toporg, end);
									end[2] += 1;
									trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
									if (trace.fraction >= 1)
									{
										break;
									}
								}	//end if
							}	//end if
							toporg[2] += 4;
							area2num = AAS_PointAreaNum(toporg);
						}	//end if
							//if in solid
						if (l >= 16)
						{
							continue;
						}
						//never create a reachability in the same area
						if (area2num == area1num)
						{
							continue;
						}
						//if the area isn't grounded
						if (!AAS_AreaGrounded(area2num))
						{
							continue;
						}
						//if there already exists reachability between the areas
						if (AAS_ReachabilityExists(area1num, area2num))
						{
							continue;
						}
						//if the reachability start is within the elevator bounding box
						VectorSubtract(bottomorg, platbottom, dir);
						VectorNormalize(dir);
						dir[0] = bottomorg[0] + 24 * dir[0];
						dir[1] = bottomorg[1] + 24 * dir[1];
						dir[2] = bottomorg[2];
						//
						for (p = 0; p < 3; p++)
							if (dir[p] < origin[p] + mins[p] || dir[p] > origin[p] + maxs[p])
							{
								break;
							}
						if (p >= 3)
						{
							continue;
						}
						//create a new reachability link
						lreach = AAS_AllocReachability();
						if (!lreach)
						{
							continue;
						}
						lreach->areanum = area2num;
						//the facenum is the model number
						lreach->facenum = modelnum;
						//the edgenum is the height
						lreach->edgenum = (int)height;
						//
						VectorCopy(dir, lreach->start);
						VectorCopy(toporg, lreach->end);
						lreach->traveltype = TRAVEL_ELEVATOR;
						lreach->traveltime = height * 100 / speed;
						if (!lreach->traveltime)
						{
							lreach->traveltime = 50;
						}
						lreach->next = areareachability[area1num];
						areareachability[area1num] = lreach;
						//don't go any further to the outside
						n = 9999;
						//
						//
						reach_elevator++;
					}	//end for
				}	//end for
			}	//end for
		}	//end if
	}	//end for
}	//end of the function AAS_Reachability_Elevator
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_JumpPad(void)
{
	int face2num, i, ret, modelnum, area2num, visualize;
	float speed, zvel, hordist, dist, time, height, gravity, forward;
	aas_face_t* face2;
	aas_area_t* area2;
	aas_lreachability_t* lreach;
	vec3_t areastart, facecenter, dir, cmdmove, teststart;
	vec3_t velocity, origin, ent2origin, angles, absmins, absmaxs;
	aas_clientmove_rtcw_t move;
	aas_trace_t trace;
	int ent, ent2;
	aas_link_t* areas, * link;
	char target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];
	char classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (String::Cmp(classname, "trigger_push"))
		{
			continue;
		}
		//
		AAS_FloatForBSPEpairKey(ent, "speed", &speed);
		if (!speed)
		{
			speed = 1000;
		}
		VectorClear(angles);
		//get the mins, maxs and origin of the model
		AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
		if (model[0])
		{
			modelnum = String::Atoi(model + 1);
		}
		else
		{
			modelnum = 0;
		}
		AAS_BSPModelMinsMaxs(modelnum, angles, absmins, absmaxs);
		//
		VectorAdd(absmins, absmaxs, origin);
		VectorScale(origin, 0.5, origin);

		//get the start areas
		VectorCopy(origin, teststart);
		teststart[2] += 64;
		trace = AAS_TraceClientBBox(teststart, origin, PRESENCE_CROUCH, -1);
		if (trace.startsolid)
		{
			BotImport_Print(PRT_MESSAGE, "trigger_push start solid\n");
			VectorCopy(origin, areastart);
		}	//end if
		else
		{
			VectorCopy(trace.endpos, areastart);
		}	//end else
		areastart[2] += 0.125;
		//
		//get the target entity
		AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY);
		for (ent2 = AAS_NextBSPEntity(0); ent2; ent2 = AAS_NextBSPEntity(ent2))
		{
			if (!AAS_ValueForBSPEpairKey(ent2, "targetname", targetname, MAX_EPAIRKEY))
			{
				continue;
			}
			if (!String::Cmp(targetname, target))
			{
				break;
			}
		}	//end for
		if (!ent2)
		{
			BotImport_Print(PRT_MESSAGE, "trigger_push without target entity %s\n", target);
			continue;
		}	//end if
		AAS_VectorForBSPEpairKey(ent2, "origin", ent2origin);
		//
		height = ent2origin[2] - origin[2];
		gravity = aassettings.phys_gravity;
		time = sqrt(height / (0.5 * gravity));
		if (!time)
		{
			BotImport_Print(PRT_MESSAGE, "trigger_push without time\n");
			continue;
		}	//end if
			// set s.origin2 to the push velocity
		VectorSubtract(ent2origin, origin, velocity);
		dist = VectorNormalize(velocity);
		forward = dist / time;
		//FIXME: why multiply by 1.1
		forward *= 1.1;
		VectorScale(velocity, forward, velocity);
		velocity[2] = time * gravity;
		//get the areas the jump pad brush is in
		areas = AAS_LinkEntityClientBBox(absmins, absmaxs, -1, PRESENCE_CROUCH);
		//*
		for (link = areas; link; link = link->next_area)
		{
			if (link->areanum == 5772)
			{
				ret = qfalse;
			}
		}	//*/
		for (link = areas; link; link = link->next_area)
		{
			if (AAS_AreaJumpPad(link->areanum))
			{
				break;
			}
		}	//end for
		if (!link)
		{
			BotImport_Print(PRT_MESSAGE, "trigger_multiple not in any jump pad area\n");
			AAS_UnlinkFromAreas(areas);
			continue;
		}	//end if
			//
		BotImport_Print(PRT_MESSAGE, "found a trigger_push with velocity %f %f %f\n", velocity[0], velocity[1], velocity[2]);
		//if there is a horizontal velocity check for a reachability without air control
		if (velocity[0] || velocity[1])
		{
			VectorSet(cmdmove, 0, 0, 0);
			//VectorCopy(velocity, cmdmove);
			//cmdmove[2] = 0;
			memset(&move, 0, sizeof(aas_clientmove_rtcw_t));
			area2num = 0;
			for (i = 0; i < 20; i++)
			{
				AAS_PredictClientMovementWolf(&move, -1, areastart, PRESENCE_NORMAL, qfalse,
					velocity, cmdmove, 0, 30, 0.1,
					SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME |
					SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER, 0, qfalse);							//qtrue);
				area2num = AAS_PointAreaNum(move.endpos);
				for (link = areas; link; link = link->next_area)
				{
					if (!AAS_AreaJumpPad(link->areanum))
					{
						continue;
					}
					if (link->areanum == area2num)
					{
						break;
					}
				}	//end if
				if (!link)
				{
					break;
				}
				VectorCopy(move.endpos, areastart);
				VectorCopy(move.velocity, velocity);
			}	//end for
			if (area2num && i < 20)
			{
				for (link = areas; link; link = link->next_area)
				{
					if (!AAS_AreaJumpPad(link->areanum))
					{
						continue;
					}
					if (AAS_ReachabilityExists(link->areanum, area2num))
					{
						continue;
					}
					//create a rocket or bfg jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						AAS_UnlinkFromAreas(areas);
						return;
					}	//end if
					lreach->areanum = area2num;
					//NOTE: the facenum is the Z velocity
					lreach->facenum = velocity[2];
					//NOTE: the edgenum is the horizontal velocity
					lreach->edgenum = sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
					VectorCopy(areastart, lreach->start);
					VectorCopy(move.endpos, lreach->end);
					lreach->traveltype = TRAVEL_JUMPPAD;
					lreach->traveltime = 200;
					lreach->next = areareachability[link->areanum];
					areareachability[link->areanum] = lreach;
					//
					reach_jumppad++;
				}	//end for
			}	//end if
		}	//end if
			//
		if (fabs(velocity[0]) > 100 || fabs(velocity[1]) > 100)
		{
			continue;
		}
		//check for areas we can reach with air control
		for (area2num = 1; area2num < aasworld->numareas; area2num++)
		{
			visualize = qfalse;
			/*
			if (area2num == 3568)
			{
			    for (link = areas; link; link = link->next_area)
			    {
			        if (link->areanum == 3380)
			        {
			            visualize = qtrue;
			            BotImport_Print(PRT_MESSAGE, "bah\n");
			        } //end if
			    } //end for
			} //end if*/
			//never try to go back to one of the original jumppad areas
			//and don't create reachabilities if they already exist
			for (link = areas; link; link = link->next_area)
			{
				if (AAS_ReachabilityExists(link->areanum, area2num))
				{
					break;
				}
				if (AAS_AreaJumpPad(link->areanum))
				{
					if (link->areanum == area2num)
					{
						break;
					}
				}	//end if
			}	//end if
			if (link)
			{
				continue;
			}
			//
			area2 = &aasworld->areas[area2num];
			for (i = 0; i < area2->numfaces; i++)
			{
				face2num = aasworld->faceindex[area2->firstface + i];
				face2 = &aasworld->faces[abs(face2num)];
				//if it is not a ground face
				if (!(face2->faceflags & FACE_GROUND))
				{
					continue;
				}
				//get the center of the face
				AAS_FaceCenter(face2num, facecenter);
				//only go higher up
				if (facecenter[2] < areastart[2])
				{
					continue;
				}
				//get the jumppad jump z velocity
				zvel = velocity[2];
				//get the horizontal speed for the jump, if it isn't possible to calculate this
				//speed (the jump is not possible) then there's no jump reachability created
				ret = AAS_HorizontalVelocityForJump(zvel, areastart, facecenter, &speed);
				if (ret && speed < 150)
				{
					//direction towards the face center
					VectorSubtract(facecenter, areastart, dir);
					dir[2] = 0;
					hordist = VectorNormalize(dir);
					//if (hordist < 1.6 * facecenter[2] - areastart[2])
					{
						//get command movement
						VectorScale(dir, speed, cmdmove);
						//
						AAS_PredictClientMovementWolf(&move, -1, areastart, PRESENCE_NORMAL, qfalse,
							velocity, cmdmove, 30, 30, 0.1,
							SE_ENTERWATER | SE_ENTERSLIME |
							SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
							SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER | SE_HITGROUNDAREA, area2num, visualize);
						//if prediction time wasn't enough to fully predict the movement
						//don't enter slime or lava and don't fall from too high
						if (move.frames < 30 &&
							!(move.stopevent & (SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE)) &&
							(move.stopevent & (SE_HITGROUNDAREA | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER)))
						{
							for (link = areas; link; link = link->next_area)
							{
								if (!AAS_AreaJumpPad(link->areanum))
								{
									continue;
								}
								if (AAS_ReachabilityExists(link->areanum, area2num))
								{
									continue;
								}
								//create a jumppad reachability from area1 to area2
								lreach = AAS_AllocReachability();
								if (!lreach)
								{
									AAS_UnlinkFromAreas(areas);
									return;
								}	//end if
								lreach->areanum = area2num;
								//NOTE: the facenum is the Z velocity
								lreach->facenum = velocity[2];
								//NOTE: the edgenum is the horizontal velocity
								lreach->edgenum = sqrt(cmdmove[0] * cmdmove[0] + cmdmove[1] * cmdmove[1]);
								VectorCopy(areastart, lreach->start);
								VectorCopy(facecenter, lreach->end);
								lreach->traveltype = TRAVEL_JUMPPAD;
								lreach->traveltime = 250;
								lreach->next = areareachability[link->areanum];
								areareachability[link->areanum] = lreach;
								//
								reach_jumppad++;
							}	//end for
						}	//end if
					}	//end if
				}	//end for
			}	//end for
		}	//end for
		AAS_UnlinkFromAreas(areas);
	}	//end for
}	//end of the function AAS_Reachability_JumpPad
//===========================================================================
// never point at ground faces
// always a higher and pretty far area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Grapple(int area1num, int area2num)
{
	int face2num, i, j, areanum, numareas, areas[20];
	float mingrappleangle, z, hordist;
	bsp_trace_t bsptrace;
	aas_trace_t trace;
	aas_face_t* face2;
	aas_area_t* area1, * area2;
	aas_lreachability_t* lreach;
	vec3_t areastart, facecenter, start, end, dir, down = {0, 0, -1};
	vec_t* v;

	//only grapple when on the ground or swimming
	if (!AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num))
	{
		return qfalse;
	}
	//don't grapple from a crouch area
	if (!(AAS_AreaPresenceType(area1num) & PRESENCE_NORMAL))
	{
		return qfalse;
	}
	//NOTE: disabled area swim it doesn't work right
	if (AAS_AreaSwim(area1num))
	{
		return qfalse;
	}
	//
	area1 = &aasworld->areas[area1num];
	area2 = &aasworld->areas[area2num];
	//don't grapple towards way lower areas
	if (area2->maxs[2] < area1->mins[2])
	{
		return qfalse;
	}
	//
	VectorCopy(aasworld->areas[area1num].center, start);
	//if not a swim area
	if (!AAS_AreaSwim(area1num))
	{
		if (!AAS_PointAreaNum(start))
		{
			Log_Write("area %d center %f %f %f in solid?\r\n", area1num,
				start[0], start[1], start[2]);
		}
		VectorCopy(start, end);
		end[2] -= 1000;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
		if (trace.startsolid)
		{
			return qfalse;
		}
		VectorCopy(trace.endpos, areastart);
	}	//end if
	else
	{
		if (!(AAS_PointContents(start) & (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER)))
		{
			return qfalse;
		}
	}	//end else
		//
		//start is now the start point
		//
	for (i = 0; i < area2->numfaces; i++)
	{
		face2num = aasworld->faceindex[area2->firstface + i];
		face2 = &aasworld->faces[abs(face2num)];
		//if it is not a solid face
		if (!(face2->faceflags & FACE_SOLID))
		{
			continue;
		}
		//direction towards the first vertex of the face
		v = aasworld->vertexes[aasworld->edges[abs(aasworld->edgeindex[face2->firstedge])].v[0]];
		VectorSubtract(v, areastart, dir);
		//if the face plane is facing away
		if (DotProduct(aasworld->planes[face2->planenum].normal, dir) > 0)
		{
			continue;
		}
		//get the center of the face
		AAS_FaceCenter(face2num, facecenter);
		//only go higher up with the grapple
		if (facecenter[2] < areastart[2] + 64)
		{
			continue;
		}
		//only use vertical faces or downward facing faces
		if (DotProduct(aasworld->planes[face2->planenum].normal, down) < 0)
		{
			continue;
		}
		//direction towards the face center
		VectorSubtract(facecenter, areastart, dir);
		//
		z = dir[2];
		dir[2] = 0;
		hordist = VectorLength(dir);
		if (!hordist)
		{
			continue;
		}
		//if too far
		if (hordist > 2000)
		{
			continue;
		}
		//check the minimal angle of the movement
		mingrappleangle = 15;	//15 degrees
		if (z / hordist < tan(2 * M_PI * mingrappleangle / 360))
		{
			continue;
		}
		//
		VectorCopy(facecenter, start);
		VectorMA(facecenter, -500, aasworld->planes[face2->planenum].normal, end);
		//
		bsptrace = AAS_Trace(start, NULL, NULL, end, 0, BSP46CONTENTS_SOLID);
		//the grapple won't stick to the sky and the grapple point should be near the AAS wall
		if ((bsptrace.surface.flags & BSP46SURF_SKY) || (bsptrace.fraction * 500 > 32))
		{
			continue;
		}
		//trace a full bounding box from the area center on the ground to
		//the center of the face
		VectorSubtract(facecenter, areastart, dir);
		VectorNormalize(dir);
		VectorMA(areastart, 4, dir, start);
		VectorCopy(bsptrace.endpos, end);
		trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
		VectorSubtract(trace.endpos, facecenter, dir);
		if (VectorLength(dir) > 24)
		{
			continue;
		}
		//
		VectorCopy(trace.endpos, start);
		VectorCopy(trace.endpos, end);
		end[2] -= AAS_FallDamageDistance();
		trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
		if (trace.fraction >= 1)
		{
			continue;
		}
		//area to end in
		areanum = AAS_PointAreaNum(trace.endpos);
		//if not in lava or slime
		if (aasworld->areasettings[areanum].contents & AREACONTENTS_LAVA)			//----(SA)	modified since slime is no longer deadly
		{	//		if (aasworld->areasettings[areanum].contents & (AREACONTENTS_SLIME|AREACONTENTS_LAVA))
			continue;
		}	//end if
			//do not go the the source area
		if (areanum == area1num)
		{
			continue;
		}
		//don't create reachabilities if they already exist
		if (AAS_ReachabilityExists(area1num, areanum))
		{
			continue;
		}
		//only end in areas we can stand
		if (!AAS_AreaGrounded(areanum))
		{
			continue;
		}
		//never go through cluster portals!!
		numareas = AAS_TraceAreas(areastart, bsptrace.endpos, areas, NULL, 20);
		if (numareas >= 20)
		{
			continue;
		}
		for (j = 0; j < numareas; j++)
		{
			if (aasworld->areasettings[areas[j]].contents & AREACONTENTS_CLUSTERPORTAL)
			{
				break;
			}
		}	//end for
		if (j < numareas)
		{
			continue;
		}
		//create a new reachability link
		lreach = AAS_AllocReachability();
		if (!lreach)
		{
			return qfalse;
		}
		lreach->areanum = areanum;
		lreach->facenum = face2num;
		lreach->edgenum = 0;
		VectorCopy(areastart, lreach->start);
		//VectorCopy(facecenter, lreach->end);
		VectorCopy(bsptrace.endpos, lreach->end);
		lreach->traveltype = TRAVEL_GRAPPLEHOOK;
		VectorSubtract(lreach->end, lreach->start, dir);
		lreach->traveltime = STARTGRAPPLE_TIME + VectorLength(dir) * 0.25;
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//
		reach_grapple++;
	}	//end for
		//
	return qfalse;
}	//end of the function AAS_Reachability_Grapple
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetWeaponJumpAreaFlags(void)
{
	int ent, i;
	vec3_t mins = {-15, -15, -15}, maxs = {15, 15, 15};
	vec3_t origin;
	int areanum, weaponjumpareas, spawnflags;
	char classname[MAX_EPAIRKEY];

	weaponjumpareas = 0;
	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (
			!String::Cmp(classname, "item_armor_body") ||
			!String::Cmp(classname, "item_armor_combat") ||
			!String::Cmp(classname, "item_health_mega") ||
			!String::Cmp(classname, "weapon_grenadelauncher") ||
			!String::Cmp(classname, "weapon_rocketlauncher") ||
			!String::Cmp(classname, "weapon_lightning") ||
			!String::Cmp(classname, "weapon_sp5") ||
			!String::Cmp(classname, "weapon_railgun") ||
			!String::Cmp(classname, "weapon_bfg") ||
			!String::Cmp(classname, "item_quad") ||
			!String::Cmp(classname, "item_regen") ||
			!String::Cmp(classname, "item_invulnerability"))
		{
			if (AAS_VectorForBSPEpairKey(ent, "origin", origin))
			{
				spawnflags = 0;
				AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
				//if not a stationary item
				if (!(spawnflags & 1))
				{
					if (!AAS_DropToFloor(origin, mins, maxs))
					{
						BotImport_Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
							classname, origin[0], origin[1], origin[2]);
					}	//end if
				}	//end if
					//areanum = AAS_PointAreaNum(origin);
				areanum = AAS_BestReachableArea(origin, mins, maxs, origin);
				//the bot may rocket jump towards this area
				aasworld->areasettings[areanum].areaflags |= AREA_WEAPONJUMP;
				//
				if (!AAS_AreaGrounded(areanum))
				{
					BotImport_Print(PRT_MESSAGE, "area not grounded\n");
				}
				//
				weaponjumpareas++;
			}	//end if
		}	//end if
	}	//end for
	for (i = 1; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].contents & AREACONTENTS_JUMPPAD)
		{
			aasworld->areasettings[i].areaflags |= AREA_WEAPONJUMP;
			weaponjumpareas++;
		}	//end if
	}	//end for
	BotImport_Print(PRT_MESSAGE, "%d weapon jump areas\n", weaponjumpareas);
}	//end of the function AAS_SetWeaponJumpAreaFlags
//===========================================================================
// create a possible weapon jump reachability from area1 to area2
//
// check if there's a cool item in the second area
// check if area1 is lower than area2
// check if the bot can rocketjump from area1 to area2
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_WeaponJump(int area1num, int area2num)
{
	int face2num, i, n, ret;
	float speed, zvel, hordist;
	aas_face_t* face2;
	aas_area_t* area1, * area2;
	aas_lreachability_t* lreach;
	vec3_t areastart, facecenter, start, end, dir, cmdmove;	// teststart;
	vec3_t velocity;
	aas_clientmove_rtcw_t move;
	aas_trace_t trace;

	if (!AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num))
	{
		return qfalse;
	}
	if (!AAS_AreaGrounded(area2num))
	{
		return qfalse;
	}
	//NOTE: only weapon jump towards areas with an interesting item in it??
	if (!(aasworld->areasettings[area2num].areaflags & AREA_WEAPONJUMP))
	{
		return qfalse;
	}
	//
	area1 = &aasworld->areas[area1num];
	area2 = &aasworld->areas[area2num];
	//don't weapon jump towards way lower areas
	if (area2->maxs[2] < area1->mins[2])
	{
		return qfalse;
	}
	//
	VectorCopy(aasworld->areas[area1num].center, start);
	//if not a swim area
	if (!AAS_PointAreaNum(start))
	{
		Log_Write("area %d center %f %f %f in solid?\r\n", area1num,
			start[0], start[1], start[2]);
	}
	VectorCopy(start, end);
	end[2] -= 1000;
	trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
	if (trace.startsolid)
	{
		return qfalse;
	}
	VectorCopy(trace.endpos, areastart);
	//
	//areastart is now the start point
	//
	for (i = 0; i < area2->numfaces; i++)
	{
		face2num = aasworld->faceindex[area2->firstface + i];
		face2 = &aasworld->faces[abs(face2num)];
		//if it is not a solid face
		if (!(face2->faceflags & FACE_GROUND))
		{
			continue;
		}
		//get the center of the face
		AAS_FaceCenter(face2num, facecenter);
		//only go higher up with weapon jumps
		if (facecenter[2] < areastart[2] + 64)
		{
			continue;
		}
		//NOTE: set to 2 to allow bfg jump reachabilities
		for (n = 0; n < 1 /*2*/; n++)
		{
			//get the rocket jump z velocity
			if (n)
			{
				zvel = AAS_BFGJumpZVelocity(areastart);
			}
			else
			{
				zvel = AAS_RocketJumpZVelocity(areastart);
			}
			//get the horizontal speed for the jump, if it isn't possible to calculate this
			//speed (the jump is not possible) then there's no jump reachability created
			ret = AAS_HorizontalVelocityForJump(zvel, areastart, facecenter, &speed);
			if (ret && speed < 270)
			{
				//direction towards the face center
				VectorSubtract(facecenter, areastart, dir);
				dir[2] = 0;
				hordist = VectorNormalize(dir);
				//if (hordist < 1.6 * (facecenter[2] - areastart[2]))
				{
					//get command movement
					VectorScale(dir, speed, cmdmove);
					VectorSet(velocity, 0, 0, zvel);
					/*
					//get command movement
					VectorScale(dir, speed, velocity);
					velocity[2] = zvel;
					VectorSet(cmdmove, 0, 0, 0);
					*/
					//
					AAS_PredictClientMovementWolf(&move, -1, areastart, PRESENCE_NORMAL, qtrue,
						velocity, cmdmove, 30, 30, 0.1,
						SE_ENTERWATER | SE_ENTERSLIME |
						SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
						SE_TOUCHJUMPPAD | SE_HITGROUNDAREA, area2num, qfalse);
					//if prediction time wasn't enough to fully predict the movement
					//don't enter slime or lava and don't fall from too high
					if (move.frames < 30 &&
						!(move.stopevent & (SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE)) &&
						(move.stopevent & (SE_HITGROUNDAREA | SE_TOUCHJUMPPAD)))
					{
						//create a rocket or bfg jump reachability from area1 to area2
						lreach = AAS_AllocReachability();
						if (!lreach)
						{
							return qfalse;
						}
						lreach->areanum = area2num;
						lreach->facenum = 0;
						lreach->edgenum = 0;
						VectorCopy(areastart, lreach->start);
						VectorCopy(facecenter, lreach->end);
						if (n)
						{
							lreach->traveltype = TRAVEL_BFGJUMP;
						}
						else
						{
							lreach->traveltype = TRAVEL_ROCKETJUMP;
						}
						lreach->traveltime = 300;
						lreach->next = areareachability[area1num];
						areareachability[area1num] = lreach;
						//
						reach_rocketjump++;
						return qtrue;
					}	//end if
				}	//end if
			}	//end if
		}	//end for
	}	//end for
		//
	return qfalse;
}	//end of the function AAS_Reachability_WeaponJump
//===========================================================================
// calculates additional walk off ledge reachabilities for the given area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_WalkOffLedge(int areanum)
{
	int i, j, k, l, m, n;
	int face1num, face2num, face3num, edge1num, edge2num, edge3num;
	int otherareanum, gap, reachareanum, side;
	aas_area_t* area, * area2;
	aas_face_t* face1, * face2, * face3;
	aas_edge_t* edge;
	aas_plane_t* plane;
	vec_t* v1, * v2;
	vec3_t sharededgevec, mid, dir, testend;
	aas_lreachability_t* lreach;
	aas_trace_t trace;

	if (!AAS_AreaGrounded(areanum) || AAS_AreaSwim(areanum))
	{
		return;
	}
	//
	area = &aasworld->areas[areanum];
	//
	for (i = 0; i < area->numfaces; i++)
	{
		face1num = aasworld->faceindex[area->firstface + i];
		face1 = &aasworld->faces[abs(face1num)];
		//face 1 must be a ground face
		if (!(face1->faceflags & FACE_GROUND))
		{
			continue;
		}
		//go through all the edges of this ground face
		for (k = 0; k < face1->numedges; k++)
		{
			edge1num = aasworld->edgeindex[face1->firstedge + k];
			//find another not ground face using this same edge
			for (j = 0; j < area->numfaces; j++)
			{
				face2num = aasworld->faceindex[area->firstface + j];
				face2 = &aasworld->faces[abs(face2num)];
				//face 2 may not be a ground face
				if (face2->faceflags & FACE_GROUND)
				{
					continue;
				}
				//compare all the edges
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = aasworld->edgeindex[face2->firstedge + l];
					if (abs(edge1num) == abs(edge2num))
					{
						//get the area at the other side of the face
						if (face2->frontarea == areanum)
						{
							otherareanum = face2->backarea;
						}
						else
						{
							otherareanum = face2->frontarea;
						}
						//
						area2 = &aasworld->areas[otherareanum];
						//if the other area is grounded!
						if (aasworld->areasettings[otherareanum].areaflags & AREA_GROUNDED)
						{
							//check for a possible gap
							gap = qfalse;
							for (n = 0; n < area2->numfaces; n++)
							{
								face3num = aasworld->faceindex[area2->firstface + n];
								//may not be the shared face of the two areas
								if (abs(face3num) == abs(face2num))
								{
									continue;
								}
								//
								face3 = &aasworld->faces[abs(face3num)];
								//find an edge shared by all three faces
								for (m = 0; m < face3->numedges; m++)
								{
									edge3num = aasworld->edgeindex[face3->firstedge + m];
									//but the edge should be shared by all three faces
									if (abs(edge3num) == abs(edge1num))
									{
										if (!(face3->faceflags & FACE_SOLID))
										{
											gap = qtrue;
											break;
										}	//end if
											//
										if (face3->faceflags & FACE_GROUND)
										{
											gap = qfalse;
											break;
										}	//end if
											//FIXME: there are more situations to be handled
										gap = qtrue;
										break;
									}	//end if
								}	//end for
								if (m < face3->numedges)
								{
									break;
								}
							}	//end for
							if (!gap)
							{
								break;
							}
						}	//end if
							//check for a walk off ledge reachability
						edge = &aasworld->edges[abs(edge1num)];
						side = edge1num < 0;
						//
						v1 = aasworld->vertexes[edge->v[side]];
						v2 = aasworld->vertexes[edge->v[!side]];
						//
						plane = &aasworld->planes[face1->planenum];
						//get the points really into the areas
						VectorSubtract(v2, v1, sharededgevec);
						CrossProduct(plane->normal, sharededgevec, dir);
						VectorNormalize(dir);
						//
						VectorAdd(v1, v2, mid);
						VectorScale(mid, 0.5, mid);
						VectorMA(mid, 8, dir, mid);
						//
						VectorCopy(mid, testend);
						testend[2] -= 1000;
						trace = AAS_TraceClientBBox(mid, testend, PRESENCE_CROUCH, -1);
						//
						if (trace.startsolid)
						{
							//Log_Write("area %d: trace.startsolid\r\n", areanum);
							break;
						}	//end if
						reachareanum = AAS_PointAreaNum(trace.endpos);
						if (reachareanum == areanum)
						{
							//Log_Write("area %d: same area\r\n", areanum);
							break;
						}	//end if
						if (AAS_ReachabilityExists(areanum, reachareanum))
						{
							//Log_Write("area %d: reachability already exists\r\n", areanum);
							break;
						}	//end if
						if (!AAS_AreaGrounded(reachareanum) && !AAS_AreaSwim(reachareanum))
						{
							//Log_Write("area %d, reach area %d: not grounded and not swim\r\n", areanum, reachareanum);
							break;
						}	//end if
							//
						if (aasworld->areasettings[reachareanum].contents & AREACONTENTS_LAVA)		//----(SA)	modified since slime is no longer deadly
						{	//						if (aasworld->areasettings[reachareanum].contents & (AREACONTENTS_SLIME | AREACONTENTS_LAVA))
							//Log_Write("area %d, reach area %d: lava or slime\r\n", areanum, reachareanum);
							break;
						}	//end if
						lreach = AAS_AllocReachability();
						if (!lreach)
						{
							break;
						}
						lreach->areanum = reachareanum;
						lreach->facenum = 0;
						lreach->edgenum = edge1num;
						VectorCopy(mid, lreach->start);
						VectorCopy(trace.endpos, lreach->end);
						lreach->traveltype = TRAVEL_WALKOFFLEDGE;
						lreach->traveltime = STARTWALKOFFLEDGE_TIME + fabs(mid[2] - trace.endpos[2]) * 50 / aassettings.phys_gravity;
						if (!AAS_AreaSwim(reachareanum) && !AAS_AreaJumpPad(reachareanum))
						{
							if (AAS_FallDelta(mid[2] - trace.endpos[2]) > FALLDELTA_5DAMAGE)
							{
								lreach->traveltime += FALLDAMAGE_5_TIME;
							}	//end if
							else if (AAS_FallDelta(mid[2] - trace.endpos[2]) > FALLDELTA_10DAMAGE)
							{
								lreach->traveltime += FALLDAMAGE_10_TIME;
							}	//end if
						}	//end if
						lreach->next = areareachability[areanum];
						areareachability[areanum] = lreach;
						//we've got another walk off ledge reachability
						reach_walkoffledge++;
					}	//end if
				}	//end for
			}	//end for
		}	//end for
	}	//end for
}	//end of the function AAS_Reachability_WalkOffLedge
//===========================================================================
//
// TRAVEL_WALK					100%	equal floor height + steps
// TRAVEL_CROUCH				100%
// TRAVEL_BARRIERJUMP			100%
// TRAVEL_JUMP					 80%
// TRAVEL_LADDER				100%	+ fall down from ladder + jump up to ladder
// TRAVEL_WALKOFFLEDGE			 90%	walk off very steep walls?
// TRAVEL_SWIM					100%
// TRAVEL_WATERJUMP				100%
// TRAVEL_TELEPORT				100%
// TRAVEL_ELEVATOR				100%
// TRAVEL_GRAPPLEHOOK			100%
// TRAVEL_DOUBLEJUMP			  0%
// TRAVEL_RAMPJUMP				  0%
// TRAVEL_STRAFEJUMP			  0%
// TRAVEL_ROCKETJUMP			100%	(currently limited towards areas with items)
// TRAVEL_BFGJUMP				  0%	(currently disabled)
// TRAVEL_JUMPPAD				100%
// TRAVEL_FUNCBOB				100%
//
// Parameter:			-
// Returns:				true if NOT finished
// Changes Globals:		-
//===========================================================================
int AAS_ContinueInitReachability(float time)
{
	int i, j, todo, start_time;
	static float framereachability, reachability_delay;
	static int lastpercentage;

	if (!aasworld->loaded)
	{
		return qfalse;
	}
	//if reachability is calculated for all areas
	if (aasworld->numreachabilityareas >= aasworld->numareas + 2)
	{
		return qfalse;
	}
	//if starting with area 1 (area 0 is a dummy)
	if (aasworld->numreachabilityareas == 1)
	{
		BotImport_Print(PRT_MESSAGE, "calculating reachability...\n");
		lastpercentage = 0;
		framereachability = 2000;
		reachability_delay = 1000;
	}	//end if
		//number of areas to calculate reachability for this cycle
	todo = aasworld->numreachabilityareas + (int)framereachability;
	start_time = Sys_Milliseconds();
	//loop over the areas
	for (i = aasworld->numreachabilityareas; i < aasworld->numareas && i < todo; i++)
	{
		aasworld->numreachabilityareas++;
		//only create jumppad reachabilities from jumppad areas
		if (aasworld->areasettings[i].contents & AREACONTENTS_JUMPPAD)
		{
			continue;
		}	//end if
			//loop over the areas
		for (j = 1; j < aasworld->numareas; j++)
		{
			if (i == j)
			{
				continue;
			}
			//never create reachabilities from teleporter or jumppad areas to regular areas
			if (aasworld->areasettings[i].contents & (AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD))
			{
				if (!(aasworld->areasettings[j].contents & (AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD)))
				{
					continue;
				}	//end if
			}	//end if
				//if there already is a reachability link from area i to j
			if (AAS_ReachabilityExists(i, j))
			{
				continue;
			}
			//check for a swim reachability
			if (AAS_Reachability_Swim(i, j))
			{
				continue;
			}
			//check for a simple walk on equal floor height reachability
			if (AAS_Reachability_EqualFloorHeight(i, j))
			{
				continue;
			}
			//check for step, barrier, waterjump and walk off ledge reachabilities
			if (AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(i, j))
			{
				continue;
			}
			//check for ladder reachabilities
			if (AAS_Reachability_Ladder(i, j))
			{
				continue;
			}
			//check for a jump reachability
			if (AAS_Reachability_Jump(i, j))
			{
				continue;
			}
		}	//end for
			//never create these reachabilities from teleporter or jumppad areas
		if (aasworld->areasettings[i].contents & (AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD))
		{
			continue;
		}	//end if
			//loop over the areas
		for (j = 1; j < aasworld->numareas; j++)
		{
			if (i == j)
			{
				continue;
			}
			//
			if (AAS_ReachabilityExists(i, j))
			{
				continue;
			}
			//check for a grapple hook reachability
// Ridah, no grapple
//			AAS_Reachability_Grapple(i, j);
			//check for a weapon jump reachability
// Ridah, no weapon jumping
//			AAS_Reachability_WeaponJump(i, j);
		}	//end for
			//if the calculation took more time than the max reachability delay
		if (Sys_Milliseconds() - start_time > (int)reachability_delay)
		{
			break;
		}
		//
		if (aasworld->numreachabilityareas * 1000 / aasworld->numareas > lastpercentage)
		{
			break;
		}
	}	//end for
		//
	if (aasworld->numreachabilityareas == aasworld->numareas)
	{
		BotImport_Print(PRT_MESSAGE, "\r%6.1f%%", (float)100.0);
		BotImport_Print(PRT_MESSAGE, "\nplease wait while storing reachability...\n");
		aasworld->numreachabilityareas++;
	}	//end if
		//if this is the last step in the reachability calculations
	else if (aasworld->numreachabilityareas == aasworld->numareas + 1)
	{
		//create additional walk off ledge reachabilities for every area
		for (i = 1; i < aasworld->numareas; i++)
		{
			//only create jumppad reachabilities from jumppad areas
			if (aasworld->areasettings[i].contents & AREACONTENTS_JUMPPAD)
			{
				continue;
			}	//end if
			AAS_Reachability_WalkOffLedge(i);
		}	//end for
			//create jump pad reachabilities
		AAS_Reachability_JumpPad();
		//create teleporter reachabilities
		AAS_Reachability_Teleport();
		//create elevator (func_plat) reachabilities
		AAS_Reachability_Elevator();
		//create func_bobbing reachabilities
		AAS_Reachability_FuncBobbing();
		//
//#ifdef DEBUG
		BotImport_Print(PRT_MESSAGE, "%6d reach swim\n", reach_swim);
		BotImport_Print(PRT_MESSAGE, "%6d reach equal floor\n", reach_equalfloor);
		BotImport_Print(PRT_MESSAGE, "%6d reach step\n", reach_step);
		BotImport_Print(PRT_MESSAGE, "%6d reach barrier\n", reach_barrier);
		BotImport_Print(PRT_MESSAGE, "%6d reach waterjump\n", reach_waterjump);
		BotImport_Print(PRT_MESSAGE, "%6d reach walkoffledge\n", reach_walkoffledge);
		BotImport_Print(PRT_MESSAGE, "%6d reach jump\n", reach_jump);
		BotImport_Print(PRT_MESSAGE, "%6d reach ladder\n", reach_ladder);
		BotImport_Print(PRT_MESSAGE, "%6d reach walk\n", reach_walk);
		BotImport_Print(PRT_MESSAGE, "%6d reach teleport\n", reach_teleport);
		BotImport_Print(PRT_MESSAGE, "%6d reach funcbob\n", reach_funcbob);
		BotImport_Print(PRT_MESSAGE, "%6d reach elevator\n", reach_elevator);
		BotImport_Print(PRT_MESSAGE, "%6d reach grapple\n", reach_grapple);
		BotImport_Print(PRT_MESSAGE, "%6d reach rocketjump\n", reach_rocketjump);
		BotImport_Print(PRT_MESSAGE, "%6d reach jumppad\n", reach_jumppad);
//#endif
		//*/
		//store all the reachabilities
		AAS_StoreReachability();
		//free the reachability link heap
		AAS_ShutDownReachabilityHeap();
		//
		Mem_Free(areareachability);
		//
		aasworld->numreachabilityareas++;
		//
		BotImport_Print(PRT_MESSAGE, "calculating clusters...\n");
	}	//end if
	else
	{
		lastpercentage = aasworld->numreachabilityareas * 1000 / aasworld->numareas;
		BotImport_Print(PRT_MESSAGE, "\r%6.1f%%", (float)lastpercentage / 10);
	}	//end else
		//not yet finished
	return qtrue;
}	//end of the function AAS_ContinueInitReachability
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitReachability(void)
{
	if (!aasworld->loaded)
	{
		return;
	}

	if (aasworld->reachabilitysize)
	{
		if (!((int)LibVarGetValue("forcereachability")))
		{
			aasworld->numreachabilityareas = aasworld->numareas + 2;
			return;
		}	//end if
	}	//end if
	aasworld->savefile = qtrue;
	//start with area 1 because area zero is a dummy
	aasworld->numreachabilityareas = 1;
	//setup the heap with reachability links
	AAS_SetupReachabilityHeap();
	//allocate area reachability link array
	areareachability = (aas_lreachability_t**)Mem_ClearedAlloc(
		aasworld->numareas * sizeof(aas_lreachability_t*));
	//
	AAS_SetWeaponJumpAreaFlags();
}	//end of the function AAS_InitReachable
