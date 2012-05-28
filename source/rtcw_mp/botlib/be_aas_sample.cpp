/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_sample.c
 *
 * desc:		AAS environment sampling
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
qboolean AAS_AreaEntityCollision(int areanum, vec3_t start, vec3_t end,
	int presencetype, int passent, aas_trace_t* trace)
{
	int collision;
	vec3_t boxmins, boxmaxs;
	aas_link_t* link;
	bsp_trace_t bsptrace;

	AAS_PresenceTypeBoundingBox(presencetype, boxmins, boxmaxs);

	Com_Memset(&bsptrace, 0, sizeof(bsp_trace_t));	//make compiler happy
	//assume no collision
	bsptrace.fraction = 1;
	collision = false;
	for (link = aasworld->arealinkedentities[areanum]; link; link = link->next_ent)
	{
		//ignore the pass entity
		if (link->entnum == passent)
		{
			continue;
		}
		//
		if (AAS_EntityCollision(link->entnum, start, boxmins, boxmaxs, end,
				BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP, &bsptrace))
		{
			collision = true;
		}	//end if
	}	//end for
	if (collision)
	{
		trace->startsolid = bsptrace.startsolid;
		trace->ent = bsptrace.ent;
		VectorCopy(bsptrace.endpos, trace->endpos);
		trace->area = 0;
		trace->planenum = 0;
		return true;
	}	//end if
	return false;
}	//end of the function AAS_AreaEntityCollision
//===========================================================================
// recursive subdivision of the line by the BSP tree.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_trace_t AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype,
	int passent)
{
	int side, nodenum, tmpplanenum;
	float front, back, frac;
	vec3_t cur_start, cur_end, cur_mid, v1, v2;
	aas_tracestack_t tracestack[127];
	aas_tracestack_t* tstack_p;
	aas_node_t* aasnode;
	aas_plane_t* plane;
	aas_trace_t trace;

	//clear the trace structure
	Com_Memset(&trace, 0, sizeof(aas_trace_t));
	if (GGameType & GAME_ET)
	{
		trace.ent = Q3ENTITYNUM_NONE;
	}

	if (!aasworld->loaded)
	{
		return trace;
	}

	tstack_p = tracestack;
	//we start with the whole line on the stack
	VectorCopy(start, tstack_p->start);
	VectorCopy(end, tstack_p->end);
	tstack_p->planenum = 0;
	//start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1;		//starting at the root of the tree
	tstack_p++;

	while (1)
	{
		//pop up the stack
		tstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if (tstack_p < tracestack)
		{
			tstack_p++;
			//nothing was hit
			trace.startsolid = false;
			trace.fraction = 1.0;
			//endpos is the end of the line
			VectorCopy(end, trace.endpos);
			//nothing hit
			trace.ent = GGameType & GAME_ET ? Q3ENTITYNUM_NONE : 0;
			trace.area = 0;
			trace.planenum = 0;
			return trace;
		}	//end if
			//number of the current node to test the line against
		nodenum = tstack_p->nodenum;
		//if it is an area
		if (nodenum < 0)
		{
			//if can't enter the area because it hasn't got the right presence type
			if (!(GGameType & GAME_ET) && !(aasworld->areasettings[-nodenum].presencetype & presencetype))
			{
				//if the start point is still the initial start point
				//NOTE: no need for epsilons because the points will be
				//exactly the same when they're both the start point
				if (tstack_p->start[0] == start[0] &&
					tstack_p->start[1] == start[1] &&
					tstack_p->start[2] == start[2])
				{
					trace.startsolid = true;
					trace.fraction = 0.0;
					VectorClear(v1);
				}	//end if
				else
				{
					trace.startsolid = false;
					VectorSubtract(end, start, v1);
					VectorSubtract(tstack_p->start, start, v2);
					trace.fraction = VectorLength(v2) / VectorNormalize(v1);
					VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
				}	//end else
				VectorCopy(tstack_p->start, trace.endpos);
				trace.ent = 0;
				trace.area = -nodenum;
				trace.planenum = tstack_p->planenum;
				//always take the plane with normal facing towards the trace start
				plane = &aasworld->planes[trace.planenum];
				if (DotProduct(v1, plane->normal) > 0)
				{
					trace.planenum ^= 1;
				}
				return trace;
			}	//end if
			else
			{
				if (passent >= 0)
				{
					if (AAS_AreaEntityCollision(-nodenum, tstack_p->start,
							tstack_p->end, presencetype, passent,
							&trace))
					{
						if (!trace.startsolid)
						{
							VectorSubtract(end, start, v1);
							VectorSubtract(trace.endpos, start, v2);
							trace.fraction = VectorLength(v2) / VectorLength(v1);
						}	//end if
						return trace;
					}	//end if
				}	//end if
			}	//end else
			trace.lastarea = -nodenum;
			continue;
		}	//end if
			//if it is a solid leaf
		if (!nodenum)
		{
			//if the start point is still the initial start point
			//NOTE: no need for epsilons because the points will be
			//exactly the same when they're both the start point
			if (tstack_p->start[0] == start[0] &&
				tstack_p->start[1] == start[1] &&
				tstack_p->start[2] == start[2])
			{
				trace.startsolid = true;
				trace.fraction = 0.0;
				VectorClear(v1);
			}	//end if
			else
			{
				trace.startsolid = false;
				VectorSubtract(end, start, v1);
				VectorSubtract(tstack_p->start, start, v2);
				trace.fraction = VectorLength(v2) / VectorNormalize(v1);
				VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
			}	//end else
			VectorCopy(tstack_p->start, trace.endpos);
			trace.ent = GGameType & GAME_ET ? Q3ENTITYNUM_NONE : 0;
			trace.area = 0;	//hit solid leaf
			trace.planenum = tstack_p->planenum;
			//always take the plane with normal facing towards the trace start
			plane = &aasworld->planes[trace.planenum];
			if (DotProduct(v1, plane->normal) > 0)
			{
				trace.planenum ^= 1;
			}
			return trace;
		}	//end if
		//the node to test against
		aasnode = &aasworld->nodes[nodenum];
		//start point of current line to test against node
		VectorCopy(tstack_p->start, cur_start);
		//end point of the current line to test against node
		VectorCopy(tstack_p->end, cur_end);
		//the current node plane
		plane = &aasworld->planes[aasnode->planenum];

		switch (plane->type)
		{	/*FIXME: wtf doesn't this work? obviously the axial node planes aren't always facing positive!!!
			//check for axial planes
			case PLANE_X:
			{
			    front = cur_start[0] - plane->dist;
			    back = cur_end[0] - plane->dist;
			    break;
			} //end case
			case PLANE_Y:
			{
			    front = cur_start[1] - plane->dist;
			    back = cur_end[1] - plane->dist;
			    break;
			} //end case
			case PLANE_Z:
			{
			    front = cur_start[2] - plane->dist;
			    back = cur_end[2] - plane->dist;
			    break;
			} //end case*/
		default:	//gee it's not an axial plane
		{
			front = DotProduct(cur_start, plane->normal) - plane->dist;
			back = DotProduct(cur_end, plane->normal) - plane->dist;
			break;
		}		//end default
		}	//end switch

		//calculate the hitpoint with the node (split point of the line)
		//put the crosspoint TRACEPLANE_EPSILON pixels on the near side
		if (front < 0)
		{
			frac = (front + TRACEPLANE_EPSILON) / (front - back);
		}
		else
		{
			frac = (front - TRACEPLANE_EPSILON) / (front - back);
		}
		//if the whole to be traced line is totally at the front of this node
		//only go down the tree with the front child
		if ((front >= -ON_EPSILON && back >= -ON_EPSILON))
		{
			//keep the current start and end point on the stack
			//and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[0];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				BotImport_Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			}	//end if
		}	//end if
			//if the whole to be traced line is totally at the back of this node
			//only go down the tree with the back child
		else if ((front < ON_EPSILON && back < ON_EPSILON))
		{
			//keep the current start and end point on the stack
			//and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[1];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				BotImport_Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			}	//end if
		}	//end if
			//go down the tree both at the front and back of the node
		else
		{
			tmpplanenum = tstack_p->planenum;
			//
			if (frac < 0)
			{
				frac = 0.001;	//0
			}
			else if (frac > 1)
			{
				frac = 0.999;	//1
			}
			//frac = front / (front-back);
			//
			cur_mid[0] = cur_start[0] + (cur_end[0] - cur_start[0]) * frac;
			cur_mid[1] = cur_start[1] + (cur_end[1] - cur_start[1]) * frac;
			cur_mid[2] = cur_start[2] + (cur_end[2] - cur_start[2]) * frac;

			//side the front part of the line is on
			side = front < 0;
			//first put the end part of the line on the stack (back side)
			VectorCopy(cur_mid, tstack_p->start);
			//not necesary to store because still on stack
			//VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum = aasnode->children[!side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				BotImport_Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			}	//end if
				//now put the part near the start of the line on the stack so we will
				//continue with thats part first. This way we'll find the first
				//hit of the bbox
			VectorCopy(cur_start, tstack_p->start);
			VectorCopy(cur_mid, tstack_p->end);
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum = aasnode->children[side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				BotImport_Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			}	//end if
		}	//end else
	}	//end while
//	return trace;
}	//end of the function AAS_TraceClientBBox
