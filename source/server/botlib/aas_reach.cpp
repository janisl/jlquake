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

#include "../server.h"
#include "local.h"

//NOTE: all travel times are in hundreth of a second
//number of reachabilities of each type
int reach_swim;			//swim
int reach_equalfloor;	//walk on floors with equal height
int reach_step;			//step up
int reach_walk;			//walk of step
int reach_barrier;		//jump up to a barrier
int reach_waterjump;	//jump out of water
int reach_walkoffledge;	//walk of a ledge
int reach_jump;			//jump
int reach_ladder;		//climb or descent a ladder
int reach_teleport;		//teleport
int reach_elevator;		//use an elevator
int reach_funcbob;		//use a func bob
int reach_grapple;		//grapple hook
int reach_rocketjump;	//rocket jump
int reach_jumppad;		//jump pads
//if true grapple reachabilities are skipped
int calcgrapplereach;

//temporary reachabilities
static aas_lreachability_t* reachabilityheap;	//heap with reachabilities
static aas_lreachability_t* nextreachability;	//next free reachability from the heap
aas_lreachability_t** areareachability;	//reachability links for every area
static int numlreachabilities;

aas_jumplink_t* jumplinks;

void AAS_SetupReachabilityHeap()
{
	reachabilityheap = (aas_lreachability_t*)Mem_ClearedAlloc(
		AAS_MAX_REACHABILITYSIZE * sizeof(aas_lreachability_t));
	for (int i = 0; i < AAS_MAX_REACHABILITYSIZE - 1; i++)
	{
		reachabilityheap[i].next = &reachabilityheap[i + 1];
	}
	reachabilityheap[AAS_MAX_REACHABILITYSIZE - 1].next = NULL;
	nextreachability = reachabilityheap;
	numlreachabilities = 0;
}

void AAS_ShutDownReachabilityHeap()
{
	Mem_Free(reachabilityheap);
	numlreachabilities = 0;
}

aas_lreachability_t* AAS_AllocReachability()
{
	if (!nextreachability)
	{
		return NULL;
	}
	//make sure the error message only shows up once
	if (!nextreachability->next)
	{
		AAS_Error("AAS_MAX_REACHABILITYSIZE");
	}

	aas_lreachability_t* r = nextreachability;
	nextreachability = nextreachability->next;
	numlreachabilities++;
	return r;
}

void AAS_FreeReachability(aas_lreachability_t* lreach)
{
	Com_Memset(lreach, 0, sizeof(aas_lreachability_t));

	lreach->next = nextreachability;
	nextreachability = lreach;
	numlreachabilities--;
}

// returns true if the area has reachability links
int AAS_AreaReachability(int areanum)
{
	if (areanum < 0 || areanum >= aasworld->numareas)
	{
		AAS_Error("AAS_AreaReachability: areanum %d out of range", areanum);
		return 0;
	}
	// RF, if this area is disabled, then fail
	if ((GGameType & (GAME_WolfSP | GAME_ET)) && aasworld->areasettings[areanum].areaflags & AREA_DISABLED)
	{
		return 0;
	}
	return aasworld->areasettings[areanum].numreachableareas;
}

// returns the surface area of the given face
float AAS_FaceArea(aas_face_t* face)
{
	int edgenum = aasworld->edgeindex[face->firstedge];
	int side = edgenum < 0;
	aas_edge_t* edge = &aasworld->edges[abs(edgenum)];
	vec_t* v = aasworld->vertexes[edge->v[side]];

	float total = 0;
	for (int i = 1; i < face->numedges - 1; i++)
	{
		edgenum = aasworld->edgeindex[face->firstedge + i];
		side = edgenum < 0;
		edge = &aasworld->edges[abs(edgenum)];
		vec3_t d1;
		VectorSubtract(aasworld->vertexes[edge->v[side]], v, d1);
		vec3_t d2;
		VectorSubtract(aasworld->vertexes[edge->v[!side]], v, d2);
		vec3_t cross;
		CrossProduct(d1, d2, cross);
		total += 0.5 * VectorLength(cross);
	}
	return total;
}

// returns the volume of an area
float AAS_AreaVolume(int areanum)
{
	aas_area_t* area = &aasworld->areas[areanum];
	int facenum = aasworld->faceindex[area->firstface];
	aas_face_t* face = &aasworld->faces[abs(facenum)];
	int edgenum = aasworld->edgeindex[face->firstedge];
	aas_edge_t* edge = &aasworld->edges[abs(edgenum)];

	vec3_t corner;
	VectorCopy(aasworld->vertexes[edge->v[0]], corner);

	//make tetrahedrons to all other faces
	vec_t volume = 0;
	for (int i = 0; i < area->numfaces; i++)
	{
		facenum = abs(aasworld->faceindex[area->firstface + i]);
		face = &aasworld->faces[facenum];
		int side = face->backarea != areanum;
		aas_plane_t* plane = &aasworld->planes[face->planenum ^ side];
		vec_t d = -(DotProduct(corner, plane->normal) - plane->dist);
		vec_t a = AAS_FaceArea(face);
		volume += d * a;
	}

	volume /= 3;
	return volume;
}

// returns the surface area of all ground faces together of the area
float AAS_AreaGroundFaceArea(int areanum)
{
	float total = 0;
	aas_area_t* area = &aasworld->areas[areanum];
	for (int i = 0; i < area->numfaces; i++)
	{
		aas_face_t* face = &aasworld->faces[abs(aasworld->faceindex[area->firstface + i])];
		if (!(face->faceflags & FACE_GROUND))
		{
			continue;
		}

		total += AAS_FaceArea(face);
	}
	return total;
}

// returns the center of a face
void AAS_FaceCenter(int facenum, vec3_t center)
{
	aas_face_t* face = &aasworld->faces[facenum];

	VectorClear(center);
	for (int i = 0; i < face->numedges; i++)
	{
		aas_edge_t* edge = &aasworld->edges[abs(aasworld->edgeindex[face->firstedge + i])];
		VectorAdd(center, aasworld->vertexes[edge->v[0]], center);
		VectorAdd(center, aasworld->vertexes[edge->v[1]], center);
	}
	float scale = 0.5 / face->numedges;
	VectorScale(center, scale, center);
}

// returns the maximum distance a player can fall before being damaged
// damage = deltavelocity*deltavelocity  * 0.0001
int AAS_FallDamageDistance()
{
	float maxzvelocity = sqrt(30.0 * 10000);
	float gravity = aassettings.phys_gravity;
	float t = maxzvelocity / gravity;
	return 0.5 * gravity * t * t;
}

// distance = 0.5 * gravity * t * t
// vel = t * gravity
// damage = vel * vel * 0.0001
float AAS_FallDelta(float distance)
{
	float gravity = aassettings.phys_gravity;
	float t = sqrt(Q_fabs(distance) * 2 / gravity);
	float delta = t * gravity;
	return delta * delta * 0.0001;
}

float AAS_MaxJumpHeight(float phys_jumpvel)
{
	float phys_gravity = aassettings.phys_gravity;
	//maximum height a player can jump with the given initial z velocity
	return 0.5 * phys_gravity * (phys_jumpvel / phys_gravity) * (phys_jumpvel / phys_gravity);
}

// returns true if a player can only crouch in the area
float AAS_MaxJumpDistance(float phys_jumpvel)
{
	float phys_gravity = aassettings.phys_gravity;
	float phys_maxvelocity = aassettings.phys_maxvelocity;
	//time a player takes to fall the height
	float t = sqrt(aassettings.rs_maxjumpfallheight / (0.5 * phys_gravity));
	//maximum distance
	return phys_maxvelocity * (t + phys_jumpvel / phys_gravity);
}

// returns true if a player can only crouch in the area
int AAS_AreaCrouch(int areanum)
{
	return !(aasworld->areasettings[areanum].presencetype & PRESENCE_NORMAL);
}

// returns true if it is possible to swim in the area
int AAS_AreaSwim(int areanum)
{
	return aasworld->areasettings[areanum].areaflags & AREA_LIQUID;
}

int AAS_AreaLava(int areanum)
{
	return aasworld->areasettings[areanum].contents & AREACONTENTS_LAVA;
}

int AAS_AreaSlime(int areanum)
{
	return (aasworld->areasettings[areanum].contents & AREACONTENTS_SLIME);
}

// returns true if the area contains ground faces
int AAS_AreaGrounded(int areanum)
{
	return (aasworld->areasettings[areanum].areaflags & AREA_GROUNDED);
}

// returns true if the area contains ladder faces
int AAS_AreaLadder(int areanum)
{
	return (aasworld->areasettings[areanum].areaflags & AREA_LADDER);
}

int AAS_AreaJumpPad(int areanum)
{
	return (aasworld->areasettings[areanum].contents & AREACONTENTS_JUMPPAD);
}

int AAS_AreaTeleporter(int areanum)
{
	return (aasworld->areasettings[areanum].contents & AREACONTENTS_TELEPORTER);
}

int AAS_AreaClusterPortal(int areanum)
{
	return (aasworld->areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL);
}

int AAS_AreaDoNotEnter(int areanum)
{
	return (aasworld->areasettings[areanum].contents & AREACONTENTS_DONOTENTER);
}

int AAS_AreaDoNotEnterLarge(int areanum)
{
	return (aasworld->areasettings[areanum].contents & WOLFAREACONTENTS_DONOTENTER_LARGE);
}

// returns true if there already exists a reachability from area1 to area2
bool AAS_ReachabilityExists(int area1num, int area2num)
{
	for (aas_lreachability_t* r = areareachability[area1num]; r; r = r->next)
	{
		if (r->areanum == area2num)
		{
			return true;
		}
	}
	return false;
}

// searches for reachabilities between adjacent areas with equal floor
// heights
bool AAS_Reachability_EqualFloorHeight(int area1num, int area2num)
{
	if (!AAS_AreaGrounded(area1num) || !AAS_AreaGrounded(area2num))
	{
		return false;
	}

	aas_area_t* area1 = &aasworld->areas[area1num];
	aas_area_t* area2 = &aasworld->areas[area2num];
	//if the areas are not near anough in the x-y direction
	for (int i = 0; i < 2; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + 10)
		{
			return false;
		}
		if (area1->maxs[i] < area2->mins[i] - 10)
		{
			return false;
		}
	}
	//if area 2 is too high above area 1
	if (area2->mins[2] > area1->maxs[2])
	{
		return false;
	}

	vec3_t gravitydirection = {0, 0, -1};
	vec3_t invgravity;
	VectorCopy(gravitydirection, invgravity);
	VectorInverse(invgravity);

	float bestheight = 99999;
	float bestlength = 0;
	bool foundreach = false;
	aas_lreachability_t lr;
	Com_Memset(&lr, 0, sizeof(aas_lreachability_t));//make the compiler happy

	//check if the areas have ground faces with a common edge
	//if existing use the lowest common edge for a reachability link
	for (int i = 0; i < area1->numfaces; i++)
	{
		aas_face_t* face1 = &aasworld->faces[abs(aasworld->faceindex[area1->firstface + i])];
		if (!(face1->faceflags & FACE_GROUND))
		{
			continue;
		}

		for (int j = 0; j < area2->numfaces; j++)
		{
			aas_face_t* face2 = &aasworld->faces[abs(aasworld->faceindex[area2->firstface + j])];
			if (!(face2->faceflags & FACE_GROUND))
			{
				continue;
			}
			//if there is a common edge
			for (int edgenum1 = 0; edgenum1 < face1->numedges; edgenum1++)
			{
				for (int edgenum2 = 0; edgenum2 < face2->numedges; edgenum2++)
				{
					if (abs(aasworld->edgeindex[face1->firstedge + edgenum1]) !=
						abs(aasworld->edgeindex[face2->firstedge + edgenum2]))
					{
						continue;
					}
					int edgenum = aasworld->edgeindex[face1->firstedge + edgenum1];
					int side = edgenum < 0;
					aas_edge_t* edge = &aasworld->edges[abs(edgenum)];
					//get the length of the edge
					vec3_t dir;
					VectorSubtract(aasworld->vertexes[edge->v[1]],
						aasworld->vertexes[edge->v[0]], dir);
					float length = VectorLength(dir);
					//get the start point
					vec3_t start;
					VectorAdd(aasworld->vertexes[edge->v[0]],
						aasworld->vertexes[edge->v[1]], start);
					VectorScale(start, 0.5, start);
					vec3_t end;
					VectorCopy(start, end);
					//get the end point several units inside area2
					//and the start point several units inside area1
					//NOTE: normal is pointing into area2 because the
					//face edges are stored counter clockwise
					vec3_t edgevec;
					VectorSubtract(aasworld->vertexes[edge->v[side]],
						aasworld->vertexes[edge->v[!side]], edgevec);
					aas_plane_t* plane2 = &aasworld->planes[face2->planenum];
					vec3_t normal;
					CrossProduct(edgevec, plane2->normal, normal);
					VectorNormalize(normal);

					VectorMA(end, INSIDEUNITS_WALKEND, normal, end);
					VectorMA(start, INSIDEUNITS_WALKSTART, normal, start);
					end[2] += 0.125;

					float height = DotProduct(invgravity, start);
					//NOTE: if there's nearby solid or a gap area after this area
					//disabled this crap
					//if (AAS_NearbySolidOrGap(start, end)) height += 200;
					//NOTE: disabled because it disables reachabilities to very small areas
					//if (AAS_PointAreaNum(end) != area2num) continue;
					//get the longest lowest edge
					if (height < bestheight ||
						(height < bestheight + 1 && length > bestlength))
					{
						bestheight = height;
						bestlength = length;
						//create a new reachability link
						lr.areanum = area2num;
						lr.facenum = 0;
						lr.edgenum = edgenum;
						VectorCopy(start, lr.start);
						VectorCopy(end, lr.end);
						lr.traveltype = TRAVEL_WALK;
						lr.traveltime = 1;
						foundreach = true;
					}
				}
			}
		}
	}
	if (foundreach)
	{
		//create a new reachability link
		aas_lreachability_t* lreach = AAS_AllocReachability();
		if (!lreach)
		{
			return false;
		}
		lreach->areanum = lr.areanum;
		lreach->facenum = lr.facenum;
		lreach->edgenum = lr.edgenum;
		VectorCopy(lr.start, lreach->start);
		VectorCopy(lr.end, lreach->end);
		lreach->traveltype = lr.traveltype;
		lreach->traveltime = lr.traveltime;
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//if going into a crouch area
		if (!AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num))
		{
			lreach->traveltime += aassettings.rs_startcrouch;
		}
		reach_equalfloor++;
		return true;
	}
	return false;
}

// calculate a range of points closest to each other on both edges
//
// Parameter:			beststart1		start of the range of points on edge v1-v2
//						beststart2		end of the range of points  on edge v1-v2
//						bestend1		start of the range of points on edge v3-v4
//						bestend2		end of the range of points  on edge v3-v4
//						bestdist		best distance so far
float AAS_ClosestEdgePoints(const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec3_t v4,
	const aas_plane_t* plane1, const aas_plane_t* plane2,
	vec3_t beststart1, vec3_t bestend1,
	vec3_t beststart2, vec3_t bestend2, float bestdist)
{
	//edge vectors
	vec3_t dir1, dir2;
	VectorSubtract(v2, v1, dir1);
	VectorSubtract(v4, v3, dir2);
	//get the horizontal directions
	dir1[2] = 0;
	dir2[2] = 0;
	//
	// p1 = point on an edge vector of area2 closest to v1
	// p2 = point on an edge vector of area2 closest to v2
	// p3 = point on an edge vector of area1 closest to v3
	// p4 = point on an edge vector of area1 closest to v4

	vec3_t p1, p2;
	if (dir2[0])
	{
		float a2 = dir2[1] / dir2[0];
		float b2 = v3[1] - a2 * v3[0];
		//point on the edge vector of area2 closest to v1
		p1[0] = (DotProduct(v1, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p1[1] = a2 * p1[0] + b2;
		//point on the edge vector of area2 closest to v2
		p2[0] = (DotProduct(v2, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p2[1] = a2 * p2[0] + b2;
	}
	else
	{
		//point on the edge vector of area2 closest to v1
		p1[0] = v3[0];
		p1[1] = v1[1];
		//point on the edge vector of area2 closest to v2
		p2[0] = v3[0];
		p2[1] = v2[1];
	}

	vec3_t p3, p4;
	if (dir1[0])
	{
		float a1 = dir1[1] / dir1[0];
		float b1 = v1[1] - a1 * v1[0];
		//point on the edge vector of area1 closest to v3
		p3[0] = (DotProduct(v3, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p3[1] = a1 * p3[0] + b1;
		//point on the edge vector of area1 closest to v4
		p4[0] = (DotProduct(v4, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p4[1] = a1 * p4[0] + b1;
	}
	else
	{
		//point on the edge vector of area1 closest to v3
		p3[0] = v1[0];
		p3[1] = v3[1];
		//point on the edge vector of area1 closest to v4
		p4[0] = v1[0];
		p4[1] = v4[1];
	}

	//start with zero z-coordinates
	p1[2] = 0;
	p2[2] = 0;
	p3[2] = 0;
	p4[2] = 0;
	//calculate the z-coordinates from the ground planes
	p1[2] = (plane2->dist - DotProduct(plane2->normal, p1)) / plane2->normal[2];
	p2[2] = (plane2->dist - DotProduct(plane2->normal, p2)) / plane2->normal[2];
	p3[2] = (plane1->dist - DotProduct(plane1->normal, p3)) / plane1->normal[2];
	p4[2] = (plane1->dist - DotProduct(plane1->normal, p4)) / plane1->normal[2];

	bool founddist = false;
	float dist, dist1, dist2;

	if (VectorBetweenVectors(p1, v3, v4))
	{
		dist = VectorDistance(v1, p1);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, v1);
			dist2 = VectorDistance(beststart2, v1);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(v1, beststart2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(v1, beststart1);
				}
			}
			dist1 = VectorDistance(bestend1, p1);
			dist2 = VectorDistance(bestend2, p1);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(p1, bestend2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(p1, bestend1);
				}
			}
		}
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(p1, bestend1);
			VectorCopy(p1, bestend2);
		}
		founddist = true;
	}
	if (VectorBetweenVectors(p2, v3, v4))
	{
		dist = VectorDistance(v2, p2);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, v2);
			dist2 = VectorDistance(beststart2, v2);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(v2, beststart2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(v2, beststart1);
				}
			}
			dist1 = VectorDistance(bestend1, p2);
			dist2 = VectorDistance(bestend2, p2);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(p2, bestend2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(p2, bestend1);
				}
			}
		}
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(p2, bestend1);
			VectorCopy(p2, bestend2);
		}
		founddist = true;
	}
	if (VectorBetweenVectors(p3, v1, v2))
	{
		dist = VectorDistance(v3, p3);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, p3);
			dist2 = VectorDistance(beststart2, p3);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(p3, beststart2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(p3, beststart1);
				}
			}
			dist1 = VectorDistance(bestend1, v3);
			dist2 = VectorDistance(bestend2, v3);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(v3, bestend2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(v3, bestend1);
				}
			}
		}
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p3, beststart1);
			VectorCopy(p3, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		}
		founddist = true;
	}
	if (VectorBetweenVectors(p4, v1, v2))
	{
		dist = VectorDistance(v4, p4);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, p4);
			dist2 = VectorDistance(beststart2, p4);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(p4, beststart2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2))
				{
					VectorCopy(p4, beststart1);
				}
			}
			dist1 = VectorDistance(bestend1, v4);
			dist2 = VectorDistance(bestend2, v4);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(v4, bestend2);
				}
			}
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2))
				{
					VectorCopy(v4, bestend1);
				}
			}
		}
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p4, beststart1);
			VectorCopy(p4, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		}
		founddist = true;
	}

	//if no shortest distance was found the shortest distance
	//is between one of the vertexes of edge1 and one of edge2
	if (!founddist)
	{
		dist = VectorDistance(v1, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		}
		dist = VectorDistance(v1, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		}
		dist = VectorDistance(v2, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		}
		dist = VectorDistance(v2, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		}
	}
	return bestdist;
}

int AAS_TravelFlagsForTeam(int ent)
{
	if (!(GGameType & GAME_Quake3))
	{
		return 0;
	}
	int notteam;
	if (!AAS_IntForBSPEpairKey(ent, "bot_notteam", &notteam))
	{
		return 0;
	}
	if (notteam == 1)
	{
		return TRAVELFLAG_NOTTEAM1;
	}
	if (notteam == 2)
	{
		return TRAVELFLAG_NOTTEAM2;
	}
	return 0;
}

void AAS_StoreReachability()
{
	int i;
	aas_areasettings_t* areasettings;
	aas_lreachability_t* lreach;
	aas_reachability_t* reach;

	if (aasworld->reachability)
	{
		Mem_Free(aasworld->reachability);
	}
	aasworld->reachability = (aas_reachability_t*)Mem_ClearedAlloc((numlreachabilities + 10) * sizeof(aas_reachability_t));
	aasworld->reachabilitysize = 1;
	for (i = 0; i < aasworld->numareas; i++)
	{
		areasettings = &aasworld->areasettings[i];
		areasettings->firstreachablearea = aasworld->reachabilitysize;
		areasettings->numreachableareas = 0;
		for (lreach = areareachability[i]; lreach; lreach = lreach->next)
		{
			reach = &aasworld->reachability[areasettings->firstreachablearea +
				areasettings->numreachableareas];
			reach->areanum = lreach->areanum;
			reach->facenum = lreach->facenum;
			reach->edgenum = lreach->edgenum;
			VectorCopy(lreach->start, reach->start);
			VectorCopy(lreach->end, reach->end);
			reach->traveltype = lreach->traveltype;
			reach->traveltime = lreach->traveltime;
			// RF, enforce the min reach time
			if (GGameType & (GAME_WolfSP | GAME_ET) && reach->traveltime < REACH_MIN_TIME)
			{
				reach->traveltime = REACH_MIN_TIME;
			}
			//
			areasettings->numreachableareas++;
		}	//end for
		aasworld->reachabilitysize += areasettings->numreachableareas;
	}	//end for
}

int AAS_BestReachableLinkArea(aas_link_t* areas)
{
	for (aas_link_t* link = areas; link; link = link->next_area)
	{
		if (AAS_AreaGrounded(link->areanum) || AAS_AreaSwim(link->areanum))
		{
			return link->areanum;
		}
	}

	for (aas_link_t* link = areas; link; link = link->next_area)
	{
		if (link->areanum)
		{
			return link->areanum;
		}
		//FIXME: this is a bad idea when the reachability is not yet
		// calculated when the level items are loaded
		if (GGameType & GAME_Quake3 && AAS_AreaReachability(link->areanum))
		{
			return link->areanum;
		}
	}
	return 0;
}

// returns true if there is a solid just after the end point when going
// from start to end
bool AAS_NearbySolidOrGap(const vec3_t start, const vec3_t end)
{
	vec3_t dir, testpoint;
	int areanum;

	VectorSubtract(end, start, dir);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(end, 48, dir, testpoint);

	areanum = AAS_PointAreaNum(testpoint);
	if (!areanum)
	{
		testpoint[2] += 16;
		areanum = AAS_PointAreaNum(testpoint);
		if (!areanum)
		{
			return true;
		}
	}
	VectorMA(end, 64, dir, testpoint);
	areanum = AAS_PointAreaNum(testpoint);
	if (areanum)
	{
		if (!AAS_AreaSwim(areanum) && !AAS_AreaGrounded(areanum))
		{
			return true;
		}
	}
	return false;
}

static aas_lreachability_t* AAS_FindFaceReachabilities(const vec3_t* facepoints, int numpoints,
	const aas_plane_t* plane, int towardsface)
{
	aas_lreachability_t* lreachabilities = NULL;
	int bestfacenum = 0;
	aas_plane_t* bestfaceplane = NULL;

	for (int i = 1; i < aasworld->numareas; i++)
	{
		aas_area_t* area = &aasworld->areas[i];
		// get the shortest distance between one of the func_bob start edges and
		// one of the face edges of area1
		vec3_t beststart, beststart2, bestend, bestend2;
		float bestdist = 999999;
		for (int j = 0; j < area->numfaces; j++)
		{
			int facenum = aasworld->faceindex[area->firstface + j];
			aas_face_t* face = &aasworld->faces[abs(facenum)];
			//if not a ground face
			if (!(face->faceflags & FACE_GROUND))
			{
				continue;
			}
			//get the ground planes
			aas_plane_t* faceplane = &aasworld->planes[face->planenum];

			for (int k = 0; k < face->numedges; k++)
			{
				int edgenum = abs(aasworld->edgeindex[face->firstedge + k]);
				aas_edge_t* edge = &aasworld->edges[edgenum];
				//calculate the minimum distance between the two edges
				const float* v1 = aasworld->vertexes[edge->v[0]];
				const float* v2 = aasworld->vertexes[edge->v[1]];

				for (int l = 0; l < numpoints; l++)
				{
					const float* v3 = facepoints[l];
					const float* v4 = facepoints[(l + 1) % numpoints];
					float dist = AAS_ClosestEdgePoints(v1, v2, v3, v4, faceplane, plane,
						beststart, bestend,
						beststart2, bestend2, bestdist);
					if (dist < bestdist)
					{
						bestfacenum = facenum;
						bestfaceplane = faceplane;
						bestdist = dist;
					}
				}
			}
		}

		if (bestdist > 192)
		{
			continue;
		}

		VectorMiddle(beststart, beststart2, beststart);
		VectorMiddle(bestend, bestend2, bestend);

		if (!towardsface)
		{
			vec3_t tmp;
			VectorCopy(beststart, tmp);
			VectorCopy(bestend, beststart);
			VectorCopy(tmp, bestend);
		}

		vec3_t hordir;
		VectorSubtract(bestend, beststart, hordir);
		hordir[2] = 0;
		float hordist = VectorLength(hordir);

		if (hordist > 2 * AAS_MaxJumpDistance(aassettings.phys_jumpvel))
		{
			continue;
		}
		//the end point should not be significantly higher than the start point
		if (bestend[2] - 32 > beststart[2])
		{
			continue;
		}
		//don't fall down too far
		if (bestend[2] < beststart[2] - 128)
		{
			continue;
		}
		//the distance should not be too far
		if (hordist > 32)
		{
			//check for walk off ledge
			float speed;
			if (!AAS_HorizontalVelocityForJump(0, beststart, bestend, &speed))
			{
				continue;
			}
		}

		beststart[2] += 1;
		bestend[2] += 1;

		vec3_t testpoint;
		if (towardsface)
		{
			VectorCopy(bestend, testpoint);
		}
		else
		{
			VectorCopy(beststart, testpoint);
		}
		testpoint[2] = 0;
		testpoint[2] = (bestfaceplane->dist - DotProduct(bestfaceplane->normal, testpoint)) / bestfaceplane->normal[2];

		if (!AAS_PointInsideFace(bestfacenum, testpoint, 0.1f))
		{
			//if the faces are not overlapping then only go down
			if (bestend[2] - 16 > beststart[2])
			{
				continue;
			}
		}
		aas_lreachability_t* lreach = AAS_AllocReachability();
		if (!lreach)
		{
			return lreachabilities;
		}
		lreach->areanum = i;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy(beststart, lreach->start);
		VectorCopy(bestend, lreach->end);
		lreach->traveltype = 0;
		lreach->traveltime = 0;
		lreach->next = lreachabilities;
		lreachabilities = lreach;
		if (towardsface)
		{
			AAS_PermanentLine(lreach->start, lreach->end, 1);
		}
		else
		{
			AAS_PermanentLine(lreach->start, lreach->end, 2);
		}
	}
	return lreachabilities;
}

void AAS_Reachability_FuncBobbing()
{
	for (int ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		char classname[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (String::Cmp(classname, "func_bobbing"))
		{
			continue;
		}
		float height;
		AAS_FloatForBSPEpairKey(ent, "height", &height);
		if (!height)
		{
			height = 32;
		}

		char model[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY))
		{
			BotImport_Print(PRT_ERROR, "func_bobbing without model\n");
			continue;
		}
		//get the model number, and skip the leading *
		int modelnum = String::Atoi(model + 1);
		if (modelnum <= 0)
		{
			BotImport_Print(PRT_ERROR, "func_bobbing with invalid model number\n");
			continue;
		}

		vec3_t mins, maxs, angles = {0, 0, 0};
		AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);

		//if the entity has an origin set then use it
		vec3_t origin;
		if (GGameType & GAME_Quake3 && AAS_VectorForBSPEpairKey(ent, "origin", origin))
		{
			VectorAdd(mins, origin, mins);
			VectorAdd(maxs, origin, maxs);
		}

		vec3_t mid;
		VectorAdd(mins, maxs, mid);
		VectorScale(mid, 0.5, mid);
		VectorCopy(mid, origin);

		vec3_t move_end;
		VectorCopy(origin, move_end);
		vec3_t move_start;
		VectorCopy(origin, move_start);

		int spawnflags;
		AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		// set the axis of bobbing
		int axis;
		if (spawnflags & 1)
		{
			axis = 0;
		}
		else if (spawnflags & 2)
		{
			axis = 1;
		}
		else
		{
			axis = 2;
		}

		move_start[axis] -= height;
		move_end[axis] += height;

		Log_Write("funcbob model %d, start = {%1.1f, %1.1f, %1.1f} end = {%1.1f, %1.1f, %1.1f}\n",
			modelnum, move_start[0], move_start[1], move_start[2], move_end[0], move_end[1], move_end[2]);

		vec3_t start_edgeverts[4];
		for (int i = 0; i < 4; i++)
		{
			VectorCopy(move_start, start_edgeverts[i]);
			start_edgeverts[i][2] += maxs[2] - mid[2];	//+ bbox maxs z
			start_edgeverts[i][2] += 24;	//+ player origin to ground dist
		}
		start_edgeverts[0][0] += maxs[0] - mid[0];
		start_edgeverts[0][1] += maxs[1] - mid[1];
		start_edgeverts[1][0] += maxs[0] - mid[0];
		start_edgeverts[1][1] += mins[1] - mid[1];
		start_edgeverts[2][0] += mins[0] - mid[0];
		start_edgeverts[2][1] += mins[1] - mid[1];
		start_edgeverts[3][0] += mins[0] - mid[0];
		start_edgeverts[3][1] += maxs[1] - mid[1];

		aas_plane_t start_plane;
		start_plane.dist = start_edgeverts[0][2];
		VectorSet(start_plane.normal, 0, 0, 1);

		vec3_t end_edgeverts[4];
		for (int i = 0; i < 4; i++)
		{
			VectorCopy(move_end, end_edgeverts[i]);
			end_edgeverts[i][2] += maxs[2] - mid[2];//+ bbox maxs z
			end_edgeverts[i][2] += 24;	//+ player origin to ground dist
		}
		end_edgeverts[0][0] += maxs[0] - mid[0];
		end_edgeverts[0][1] += maxs[1] - mid[1];
		end_edgeverts[1][0] += maxs[0] - mid[0];
		end_edgeverts[1][1] += mins[1] - mid[1];
		end_edgeverts[2][0] += mins[0] - mid[0];
		end_edgeverts[2][1] += mins[1] - mid[1];
		end_edgeverts[3][0] += mins[0] - mid[0];
		end_edgeverts[3][1] += maxs[1] - mid[1];

		aas_plane_t end_plane;
		end_plane.dist = end_edgeverts[0][2];
		VectorSet(end_plane.normal, 0, 0, 1);

		vec3_t move_start_top;
		VectorCopy(move_start, move_start_top);
		move_start_top[2] += maxs[2] - mid[2] + 24;	//+ bbox maxs z
		vec3_t move_end_top;
		VectorCopy(move_end, move_end_top);
		move_end_top[2] += maxs[2] - mid[2] + 24;	//+ bbox maxs z

		if (!AAS_PointAreaNum(move_start_top))
		{
			continue;
		}
		if (!AAS_PointAreaNum(move_end_top))
		{
			continue;
		}

		for (int i = 0; i < 2; i++)
		{
			aas_lreachability_t* firststartreach;
			aas_lreachability_t* firstendreach;

			if (i == 0)
			{
				firststartreach = AAS_FindFaceReachabilities(start_edgeverts, 4, &start_plane, true);
				firstendreach = AAS_FindFaceReachabilities(end_edgeverts, 4, &end_plane, false);
			}
			else
			{
				firststartreach = AAS_FindFaceReachabilities(end_edgeverts, 4, &end_plane, true);
				firstendreach = AAS_FindFaceReachabilities(start_edgeverts, 4, &start_plane, false);
			}

			//create reachabilities from start to end
			aas_lreachability_t* nextstartreach;
			for (aas_lreachability_t* startreach = firststartreach; startreach; startreach = nextstartreach)
			{
				nextstartreach = startreach->next;

				aas_lreachability_t* nextendreach;
				for (aas_lreachability_t* endreach = firstendreach; endreach; endreach = nextendreach)
				{
					nextendreach = endreach->next;

					Log_Write("funcbob reach from area %d to %d\n", startreach->areanum, endreach->areanum);

					vec3_t org;
					if (i == 0)
					{
						VectorCopy(move_start_top, org);
					}
					else
					{
						VectorCopy(move_end_top, org);
					}
					vec3_t dir;
					VectorSubtract(startreach->start, org, dir);
					dir[2] = 0;
					VectorNormalize(dir);
					vec3_t start;
					VectorCopy(startreach->start, start);
					VectorMA(startreach->start, 1, dir, start);
					start[2] += 1;
					vec3_t end;
					VectorMA(startreach->start, 16, dir, end);
					end[2] += 1;

					int areas[10];
					vec3_t points[10];
					int numareas = AAS_TraceAreas(start, end, areas, points, 10);
					if (numareas <= 0)
					{
						continue;
					}
					if (numareas > 1)
					{
						VectorCopy(points[1], startreach->start);
					}
					else
					{
						VectorCopy(end, startreach->start);
					}

					if (!AAS_PointAreaNum(startreach->start))
					{
						continue;
					}
					if (!AAS_PointAreaNum(endreach->end))
					{
						continue;
					}

					aas_lreachability_t* lreach = AAS_AllocReachability();
					lreach->areanum = endreach->areanum;
					if (i == 0)
					{
						lreach->edgenum = ((int)move_start[axis] << 16) | ((int)move_end[axis] & 0x0000ffff);
					}
					else
					{
						lreach->edgenum = ((int)move_end[axis] << 16) | ((int)move_start[axis] & 0x0000ffff);
					}
					lreach->facenum = (spawnflags << 16) | modelnum;
					VectorCopy(startreach->start, lreach->start);
					VectorCopy(endreach->end, lreach->end);
					lreach->traveltype = TRAVEL_FUNCBOB;
					lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
					lreach->traveltime = aassettings.rs_funcbob;
					reach_funcbob++;
					lreach->next = areareachability[startreach->areanum];
					areareachability[startreach->areanum] = lreach;
				}
			}
			for (aas_lreachability_t* startreach = firststartreach; startreach; startreach = nextstartreach)
			{
				nextstartreach = startreach->next;
				AAS_FreeReachability(startreach);
			}
			aas_lreachability_t* nextendreach;
			for (aas_lreachability_t* endreach = firstendreach; endreach; endreach = nextendreach)
			{
				nextendreach = endreach->next;
				AAS_FreeReachability(endreach);
			}
			//only go up with func_bobbing entities that go up and down
			if (!(spawnflags & 1) && !(spawnflags & 2))
			{
				break;
			}
		}
	}
}
