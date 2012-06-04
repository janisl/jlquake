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
		}
		aasworld->reachabilitysize += areasettings->numreachableareas;
	}
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

bool AAS_GetJumpPadInfo(int ent, vec3_t areastart, vec3_t absmins, vec3_t absmaxs, vec3_t velocity)
{
	float speed;
	AAS_FloatForBSPEpairKey(ent, "speed", &speed);
	if (!speed)
	{
		speed = 1000;
	}
	vec3_t angles;
	VectorClear(angles);
	//get the mins, maxs and origin of the model
	char model[MAX_EPAIRKEY];
	AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
	int modelnum;
	if (model[0])
	{
		modelnum = String::Atoi(model + 1);
	}
	else
	{
		modelnum = 0;
	}
	AAS_BSPModelMinsMaxs(modelnum, angles, absmins, absmaxs);
	vec3_t origin;
	VectorAdd(absmins, absmaxs, origin);
	VectorScale(origin, 0.5, origin);

	//get the start areas
	vec3_t teststart;
	VectorCopy(origin, teststart);
	teststart[2] += 64;
	aas_trace_t trace = AAS_TraceClientBBox(teststart, origin, PRESENCE_CROUCH, -1);
	if (trace.startsolid)
	{
		BotImport_Print(PRT_MESSAGE, "trigger_push start solid\n");
		VectorCopy(origin, areastart);
	}
	else
	{
		VectorCopy(trace.endpos, areastart);
	}
	areastart[2] += 0.125;
	//
	//get the target entity
	char target[MAX_EPAIRKEY];
	AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY);
	int ent2;
	for (ent2 = AAS_NextBSPEntity(0); ent2; ent2 = AAS_NextBSPEntity(ent2))
	{
		char targetname[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent2, "targetname", targetname, MAX_EPAIRKEY))
		{
			continue;
		}
		if (!String::Cmp(targetname, target))
		{
			break;
		}
	}
	if (!ent2)
	{
		BotImport_Print(PRT_MESSAGE, "trigger_push without target entity %s\n", target);
		return false;
	}
	vec3_t ent2origin;
	AAS_VectorForBSPEpairKey(ent2, "origin", ent2origin);

	float height = ent2origin[2] - origin[2];
	float gravity = aassettings.phys_gravity;
	float time = sqrt(height / (0.5 * gravity));
	if (!time)
	{
		BotImport_Print(PRT_MESSAGE, "trigger_push without time\n");
		return false;
	}
	// set s.origin2 to the push velocity
	VectorSubtract(ent2origin, origin, velocity);
	float dist = VectorNormalize(velocity);
	float forward = dist / time;
	//FIXME: why multiply by 1.1
	forward *= 1.1f;
	VectorScale(velocity, forward, velocity);
	velocity[2] = time * gravity;
	return true;
}

int AAS_BestReachableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec3_t goalorigin)
{
	if (!aasworld->loaded)
	{
		BotImport_Print(PRT_ERROR, "AAS_BestReachableArea: aas not loaded\n");
		return 0;
	}
	//find a point in an area
	vec3_t start;
	VectorCopy(origin, start);
	int areanum = AAS_PointAreaNum(start);
	//while no area found fudge around a little
	for (int i = 0; i < 5 && !areanum; i++)
	{
		for (int j = 0; j < 5 && !areanum; j++)
		{
			for (int k = -1; k <= 1 && !areanum; k++)
			{
				for (int l = -1; l <= 1 && !areanum; l++)
				{
					VectorCopy(origin, start);
					start[0] += (float)j * 4 * k;
					start[1] += (float)j * 4 * l;
					start[2] += (float)i * 4;
					areanum = AAS_PointAreaNum(start);
				}
			}
		}
	}
	//if an area was found
	if (areanum)
	{
		//drop client bbox down and try again
		vec3_t end;
		VectorCopy(start, end);
		start[2] += 0.25;
		end[2] -= 50;
		aas_trace_t trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
		if (!trace.startsolid)
		{
			areanum = AAS_PointAreaNum(trace.endpos);
			VectorCopy(trace.endpos, goalorigin);
			//if the origin is in an area with reachability
			if (GGameType & GAME_ET ? AAS_AreaGrounded(areanum) : areanum)
			{
				return areanum;
			}
		}
		else
		{
			//it can very well happen that the AAS_PointAreaNum function tells that
			//a point is in an area and that starting a AAS_TraceClientBBox from that
			//point will return trace.startsolid true
			VectorCopy(start, goalorigin);
			return areanum;
		}
	}

	//NOTE: the goal origin does not have to be in the goal area
	// because the bot will have to move towards the item origin anyway
	VectorCopy(origin, goalorigin);

	vec3_t absmins;
	VectorAdd(origin, mins, absmins);
	vec3_t absmaxs;
	VectorAdd(origin, maxs, absmaxs);
	//link an invalid (-1) entity
	aas_link_t* areas = GGameType & GAME_Quake3 ?
		AAS_LinkEntityClientBBox(absmins, absmaxs, -1, PRESENCE_CROUCH) :
		AAS_AASLinkEntity(absmins, absmaxs, -1);
	//get the reachable link arae
	areanum = AAS_BestReachableLinkArea(areas);
	//unlink the invalid entity
	AAS_UnlinkFromAreas(areas);

	return areanum;
}

// searches for swim reachabilities between adjacent areas
bool AAS_Reachability_Swim(int area1num, int area2num)
{
	if (!AAS_AreaSwim(area1num) || !AAS_AreaSwim(area2num))
	{
		return false;
	}
	//if the second area is crouch only
	if (!(aasworld->areasettings[area2num].presencetype & PRESENCE_NORMAL))
	{
		return false;
	}

	aas_area_t* area1 = &aasworld->areas[area1num];
	aas_area_t* area2 = &aasworld->areas[area2num];

	//if the areas are not near anough
	for (int i = 0; i < 3; i++)
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
	//find a shared face and create a reachability link
	for (int i = 0; i < area1->numfaces; i++)
	{
		int face1num = aasworld->faceindex[area1->firstface + i];
		int side1 = face1num < 0;
		face1num = abs(face1num);

		for (int j = 0; j < area2->numfaces; j++)
		{
			int face2num = abs(aasworld->faceindex[area2->firstface + j]);

			if (face1num == face2num)
			{
				vec3_t start;
				AAS_FaceCenter(face1num, start);

				if (AAS_PointContents(start) & (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER))
				{
					aas_face_t* face1 = &aasworld->faces[face1num];
					//create a new reachability link
					aas_lreachability_t* lreach = AAS_AllocReachability();
					if (!lreach)
					{
						return false;
					}
					lreach->areanum = area2num;
					lreach->facenum = face1num;
					lreach->edgenum = 0;
					VectorCopy(start, lreach->start);
					aas_plane_t* plane = &aasworld->planes[face1->planenum ^ side1];
					VectorMA(lreach->start, GGameType & GAME_Quake3 ? -INSIDEUNITS : INSIDEUNITS, plane->normal, lreach->end);
					lreach->traveltype = TRAVEL_SWIM;
					lreach->traveltime = 1;
					//if the volume of the area is rather small
					if (AAS_AreaVolume(area2num) < 800)
					{
						lreach->traveltime += 200;
					}
					//link the reachability
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					reach_swim++;
					return true;
				}
			}
		}
	}
	return false;
}

// searches step, barrier, waterjump and walk off ledge reachabilities
bool AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num)
{
	int i, j, k, l, edge1num, edge2num, areas[10], numareas;
	int ground_bestarea2groundedgenum, ground_foundreach;
	int water_bestarea2groundedgenum, water_foundreach;
	int side1, area1swim, faceside1, groundface1num;
	float dist, dist1, dist2, diff, invgravitydot, ortdot;
	float x1, x2, x3, x4, y1, y2, y3, y4, tmp, y;
	float length, ground_bestlength, water_bestlength, ground_bestdist, water_bestdist;
	vec3_t v1, v2, v3, v4, tmpv, p1area1, p1area2, p2area1, p2area2;
	vec3_t normal, ort, edgevec, start, end, dir;
	vec3_t ground_beststart, ground_bestend, ground_bestnormal;
	vec3_t water_beststart, water_bestend, water_bestnormal;
	vec3_t invgravity = {0, 0, 1};
	vec3_t testpoint;
	aas_plane_t* plane;
	aas_area_t* area1, * area2;
	aas_face_t* groundface1, * groundface2, * ground_bestface1, * water_bestface1;
	aas_edge_t* edge1, * edge2;
	aas_lreachability_t* lreach;
	aas_trace_t trace;

	//	Shut up compiler.
	VectorClear(ground_beststart);
	VectorClear(ground_bestend);
	VectorClear(ground_bestnormal);
	VectorClear(water_beststart);
	VectorClear(water_bestend);
	VectorClear(water_bestnormal);

	//must be able to walk or swim in the first area
	if (!AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num))
	{
		return false;
	}

	if (!AAS_AreaGrounded(area2num) && !AAS_AreaSwim(area2num))
	{
		return false;
	}
	//
	area1 = &aasworld->areas[area1num];
	area2 = &aasworld->areas[area2num];
	//if the first area contains a liquid
	area1swim = AAS_AreaSwim(area1num);
	//if the areas are not near anough in the x-y direction
	for (i = 0; i < 2; i++)
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
		//
	ground_foundreach = false;
	ground_bestdist = 99999;
	ground_bestlength = 0;
	ground_bestarea2groundedgenum = 0;
	//
	water_foundreach = false;
	water_bestdist = 99999;
	water_bestlength = 0;
	water_bestarea2groundedgenum = 0;
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		groundface1num = aasworld->faceindex[area1->firstface + i];
		faceside1 = groundface1num < 0;
		groundface1 = &aasworld->faces[abs(groundface1num)];
		//if this isn't a ground face
		if (!(groundface1->faceflags & FACE_GROUND))
		{
			//if we can swim in the first area
			if (area1swim)
			{
				//face plane must be more or less horizontal
				plane = &aasworld->planes[groundface1->planenum ^ (!faceside1)];
				if (DotProduct(plane->normal, invgravity) < 0.7)
				{
					continue;
				}
			}
			else
			{
				//if we can't swim in the area it must be a ground face
				continue;
			}
		}
			//
		for (k = 0; k < groundface1->numedges; k++)
		{
			edge1num = aasworld->edgeindex[groundface1->firstedge + k];
			side1 = (edge1num < 0);
			//NOTE: for water faces we must take the side area 1 is
			// on into account because the face is shared and doesn't
			// have to be oriented correctly
			if (!(groundface1->faceflags & FACE_GROUND))
			{
				side1 = (side1 == faceside1);
			}
			edge1num = abs(edge1num);
			edge1 = &aasworld->edges[edge1num];
			//vertexes of the edge
			VectorCopy(aasworld->vertexes[edge1->v[!side1]], v1);
			VectorCopy(aasworld->vertexes[edge1->v[side1]], v2);
			//get a vertical plane through the edge
			//NOTE: normal is pointing into area 2 because the
			//face edges are stored counter clockwise
			VectorSubtract(v2, v1, edgevec);
			CrossProduct(edgevec, invgravity, normal);
			VectorNormalize(normal);
			dist = DotProduct(normal, v1);
			//check the faces from the second area
			for (j = 0; j < area2->numfaces; j++)
			{
				groundface2 = &aasworld->faces[abs(aasworld->faceindex[area2->firstface + j])];
				//must be a ground face
				if (!(groundface2->faceflags & FACE_GROUND))
				{
					continue;
				}
				//check the edges of this ground face
				for (l = 0; l < groundface2->numedges; l++)
				{
					edge2num = abs(aasworld->edgeindex[groundface2->firstedge + l]);
					edge2 = &aasworld->edges[edge2num];
					//vertexes of the edge
					VectorCopy(aasworld->vertexes[edge2->v[0]], v3);
					VectorCopy(aasworld->vertexes[edge2->v[1]], v4);
					//check the distance between the two points and the vertical plane
					//through the edge of area1
					diff = DotProduct(normal, v3) - dist;
					if (diff < -0.1 || diff > 0.1)
					{
						continue;
					}
					diff = DotProduct(normal, v4) - dist;
					if (diff < -0.1 || diff > 0.1)
					{
						continue;
					}
					//
					//project the two ground edges into the step side plane
					//and calculate the shortest distance between the two
					//edges if they overlap in the direction orthogonal to
					//the gravity direction
					CrossProduct(invgravity, normal, ort);
					invgravitydot = DotProduct(invgravity, invgravity);
					ortdot = DotProduct(ort, ort);
					//projection into the step plane
					//NOTE: since gravity is vertical this is just the z coordinate
					y1 = v1[2];	//DotProduct(v1, invgravity) / invgravitydot;
					y2 = v2[2];	//DotProduct(v2, invgravity) / invgravitydot;
					y3 = v3[2];	//DotProduct(v3, invgravity) / invgravitydot;
					y4 = v4[2];	//DotProduct(v4, invgravity) / invgravitydot;
					//
					x1 = DotProduct(v1, ort) / ortdot;
					x2 = DotProduct(v2, ort) / ortdot;
					x3 = DotProduct(v3, ort) / ortdot;
					x4 = DotProduct(v4, ort) / ortdot;
					//
					if (x1 > x2)
					{
						tmp = x1; x1 = x2; x2 = tmp;
						tmp = y1; y1 = y2; y2 = tmp;
						VectorCopy(v1, tmpv); VectorCopy(v2, v1); VectorCopy(tmpv, v2);
					}
					if (x3 > x4)
					{
						tmp = x3; x3 = x4; x4 = tmp;
						tmp = y3; y3 = y4; y4 = tmp;
						VectorCopy(v3, tmpv); VectorCopy(v4, v3); VectorCopy(tmpv, v4);
					}
						//if the two projected edge lines have no overlap
					if (x2 <= x3 || x4 <= x1)
					{
//						Log_Write("lines no overlap: from area %d to %d\r\n", area1num, area2num);
						continue;
					}
						//if the two lines fully overlap
					if ((x1 - 0.5 < x3 && x4 < x2 + 0.5) &&
						(x3 - 0.5 < x1 && x2 < x4 + 0.5))
					{
						dist1 = y3 - y1;
						dist2 = y4 - y2;
						VectorCopy(v1, p1area1);
						VectorCopy(v2, p2area1);
						VectorCopy(v3, p1area2);
						VectorCopy(v4, p2area2);
					}
					else
					{
						//if the points are equal
						if (x1 > x3 - 0.1 && x1 < x3 + 0.1)
						{
							dist1 = y3 - y1;
							VectorCopy(v1, p1area1);
							VectorCopy(v3, p1area2);
						}
						else if (x1 < x3)
						{
							y = y1 + (x3 - x1) * (y2 - y1) / (x2 - x1);
							dist1 = y3 - y;
							VectorCopy(v3, p1area1);
							p1area1[2] = y;
							VectorCopy(v3, p1area2);
						}
						else
						{
							y = y3 + (x1 - x3) * (y4 - y3) / (x4 - x3);
							dist1 = y - y1;
							VectorCopy(v1, p1area1);
							VectorCopy(v1, p1area2);
							p1area2[2] = y;
						}
							//if the points are equal
						if (x2 > x4 - 0.1 && x2 < x4 + 0.1)
						{
							dist2 = y4 - y2;
							VectorCopy(v2, p2area1);
							VectorCopy(v4, p2area2);
						}
						else if (x2 < x4)
						{
							y = y3 + (x2 - x3) * (y4 - y3) / (x4 - x3);
							dist2 = y - y2;
							VectorCopy(v2, p2area1);
							VectorCopy(v2, p2area2);
							p2area2[2] = y;
						}
						else
						{
							y = y1 + (x4 - x1) * (y2 - y1) / (x2 - x1);
							dist2 = y4 - y;
							VectorCopy(v4, p2area1);
							p2area1[2] = y;
							VectorCopy(v4, p2area2);
						}
					}
						//if both distances are pretty much equal
						//then we take the middle of the points
					if (dist1 > dist2 - 1 && dist1 < dist2 + 1)
					{
						dist = dist1;
						VectorAdd(p1area1, p2area1, start);
						VectorScale(start, 0.5, start);
						VectorAdd(p1area2, p2area2, end);
						VectorScale(end, 0.5, end);
					}
					else if (dist1 < dist2)
					{
						dist = dist1;
						VectorCopy(p1area1, start);
						VectorCopy(p1area2, end);
					}
					else
					{
						dist = dist2;
						VectorCopy(p2area1, start);
						VectorCopy(p2area2, end);
					}
						//get the length of the overlapping part of the edges of the two areas
					VectorSubtract(p2area2, p1area2, dir);
					length = VectorLength(dir);
					//
					if (groundface1->faceflags & FACE_GROUND)
					{
						//if the vertical distance is smaller
						if (dist < ground_bestdist ||
							//or the vertical distance is pretty much the same
							//but the overlapping part of the edges is longer
							(dist < ground_bestdist + 1 && length > ground_bestlength))
						{
							ground_bestdist = dist;
							ground_bestlength = length;
							ground_foundreach = true;
							ground_bestarea2groundedgenum = edge1num;
							ground_bestface1 = groundface1;
							//best point towards area1
							VectorCopy(start, ground_beststart);
							//normal is pointing into area2
							VectorCopy(normal, ground_bestnormal);
							//best point towards area2
							VectorCopy(end, ground_bestend);
						}
					}
					else
					{
						//if the vertical distance is smaller
						if (dist < water_bestdist ||
							//or the vertical distance is pretty much the same
							//but the overlapping part of the edges is longer
							(dist < water_bestdist + 1 && length > water_bestlength))
						{
							water_bestdist = dist;
							water_bestlength = length;
							water_foundreach = true;
							water_bestarea2groundedgenum = edge1num;
							water_bestface1 = groundface1;
							//best point towards area1
							VectorCopy(start, water_beststart);
							//normal is pointing into area2
							VectorCopy(normal, water_bestnormal);
							//best point towards area2
							VectorCopy(end, water_bestend);
						}
					}
				}
			}
		}
	}
		//
		// NOTE: swim reachabilities are already filtered out
		//
		// Steps
		//
		//        ---------
		//        |          step height -> TRAVEL_WALK
		//--------|
		//
		//        ---------
		//~~~~~~~~|          step height and low water -> TRAVEL_WALK
		//--------|
		//
		//~~~~~~~~~~~~~~~~~~
		//        ---------
		//        |          step height and low water up to the step -> TRAVEL_WALK
		//--------|
		//
		//check for a step reachability
	if (ground_foundreach)
	{
		//if area2 is higher but lower than the maximum step height
		//NOTE: ground_bestdist >= 0 also catches equal floor reachabilities
		if (ground_bestdist >= 0 && ground_bestdist < aassettings.phys_maxstep)
		{
			//create walk reachability from area1 to area2
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return false;
			}
			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = ground_bestarea2groundedgenum;
			VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
			VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
			lreach->traveltype = TRAVEL_WALK;
			lreach->traveltime = 0;	//1;
			//if going into a crouch area
			if (!AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num))
			{
				lreach->traveltime += aassettings.rs_startcrouch;
			}
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//NOTE: if there's nearby solid or a gap area after this area
			/*
			if (!AAS_NearbySolidOrGap(lreach->start, lreach->end))
			{
			    lreach->traveltime += 100;
			} //end if
			*/
			//avoid rather small areas
			//if (AAS_AreaGroundFaceArea(lreach->areanum) < 500) lreach->traveltime += 100;
			//
			reach_step++;
			return true;
		}
	}
		//
		// Water Jumps
		//
		//        ---------
		//        |
		//~~~~~~~~|
		//        |
		//        |          higher than step height and water up to waterjump height -> TRAVEL_WATERJUMP
		//--------|
		//
		//~~~~~~~~~~~~~~~~~~
		//        ---------
		//        |
		//        |
		//        |
		//        |          higher than step height and low water up to the step -> TRAVEL_WATERJUMP
		//--------|
		//
		//check for a waterjump reachability
	if (water_foundreach)
	{
		//get a test point a little bit towards area1
		VectorMA(water_bestend, -INSIDEUNITS, water_bestnormal, testpoint);
		//go down the maximum waterjump height
		testpoint[2] -= aassettings.phys_maxwaterjump;
		//if there IS water the sv_maxwaterjump height below the bestend point
		if (aasworld->areasettings[AAS_PointAreaNum(testpoint)].areaflags & AREA_LIQUID)
		{
			//don't create rediculous water jump reachabilities from areas very far below
			//the water surface
			if (water_bestdist < aassettings.phys_maxwaterjump + 24)
			{
				//waterjumping from or towards a crouch only area is not possible in Quake2
				if ((aasworld->areasettings[area1num].presencetype & PRESENCE_NORMAL) &&
					(aasworld->areasettings[area2num].presencetype & PRESENCE_NORMAL))
				{
					//create water jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						return false;
					}
					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = water_bestarea2groundedgenum;
					VectorCopy(water_beststart, lreach->start);
					VectorMA(water_bestend, INSIDEUNITS_WATERJUMP, water_bestnormal, lreach->end);
					lreach->traveltype = TRAVEL_WATERJUMP;
					lreach->traveltime = aassettings.rs_waterjump;
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//we've got another waterjump reachability
					reach_waterjump++;
					return true;
				}
			}
		}
	}
		//
		// Barrier Jumps
		//
		//        ---------
		//        |
		//        |
		//        |
		//        |         higher than step height lower than barrier height -> TRAVEL_BARRIERJUMP
		//--------|
		//
		//        ---------
		//        |
		//        |
		//        |
		//~~~~~~~~|         higher than step height lower than barrier height
		//--------|         and a thin layer of water in the area to jump from -> TRAVEL_BARRIERJUMP
		//
		//check for a barrier jump reachability
	if (ground_foundreach)
	{
		//if area2 is higher but lower than the maximum barrier jump height
		if (ground_bestdist > 0 && ground_bestdist < aassettings.phys_maxbarrier)
		{
			//if no water in area1 or a very thin layer of water on the ground
			if (!water_foundreach || (ground_bestdist - water_bestdist < 16))
			{
				//cannot perform a barrier jump towards or from a crouch area in Quake2
				if (!AAS_AreaCrouch(area1num) && !AAS_AreaCrouch(area2num))
				{
					//create barrier jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						return false;
					}
					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = ground_bestarea2groundedgenum;
					VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
					VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
					lreach->traveltype = TRAVEL_BARRIERJUMP;
					lreach->traveltime = aassettings.rs_barrierjump;
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//we've got another barrierjump reachability
					reach_barrier++;
					return true;
				}
			}
		}
	}
		//
		// Walk and Walk Off Ledge
		//
		//--------|
		//        |          can walk or step back -> TRAVEL_WALK
		//        ---------
		//
		//--------|
		//        |
		//        |
		//        |
		//        |          cannot walk/step back -> TRAVEL_WALKOFFLEDGE
		//        ---------
		//
		//--------|
		//        |
		//        |~~~~~~~~
		//        |
		//        |          cannot step back but can waterjump back -> TRAVEL_WALKOFFLEDGE
		//        ---------  FIXME: create TRAVEL_WALK reach??
		//
		//check for a walk or walk off ledge reachability
	if (ground_foundreach)
	{
		if (ground_bestdist < 0)
		{
			if (ground_bestdist > -aassettings.phys_maxstep)
			{
				//create walk reachability from area1 to area2
				lreach = AAS_AllocReachability();
				if (!lreach)
				{
					return false;
				}
				lreach->areanum = area2num;
				lreach->facenum = 0;
				lreach->edgenum = ground_bestarea2groundedgenum;
				VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
				if (GGameType & GAME_Quake3)
				{
					VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
				}
				else
				{
					VectorMA(ground_bestend, INSIDEUNITS_WALKOFFLEDGEEND, ground_bestnormal, lreach->end);
				}
				lreach->traveltype = TRAVEL_WALK;
				lreach->traveltime = 1;
				lreach->next = areareachability[area1num];
				areareachability[area1num] = lreach;
				//we've got another walk reachability
				reach_walk++;
				return true;
			}
			// if no maximum fall height set or less than the max
			if (!aassettings.rs_maxfallheight || fabs(ground_bestdist) < aassettings.rs_maxfallheight)
			{
				//trace a bounding box vertically to check for solids
				VectorMA(ground_bestend, INSIDEUNITS, ground_bestnormal, ground_bestend);
				VectorCopy(ground_bestend, start);
				start[2] = ground_beststart[2];
				VectorCopy(ground_bestend, end);
				end[2] += 4;
				trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
				//if no solids were found
				if (!trace.startsolid && trace.fraction >= 1.0)
				{
					//the trace end point must be in the goal area
					trace.endpos[2] += 1;
					if (AAS_PointAreaNum(trace.endpos) == area2num)
					{
						bool createReachability = true;
						if (GGameType & GAME_Quake3)
						{
							//if not going through a cluster portal
							numareas = AAS_TraceAreas(start, end, areas, NULL, sizeof(areas) / sizeof(int));
							for (i = 0; i < numareas; i++)
							{
								if (AAS_AreaClusterPortal(areas[i]))
								{
									createReachability = false;
									break;
								}
							}
						}
						if (createReachability)
						{
							//create a walk off ledge reachability from area1 to area2
							lreach = AAS_AllocReachability();
							if (!lreach)
							{
								return false;
							}
							lreach->areanum = area2num;
							lreach->facenum = 0;
							lreach->edgenum = ground_bestarea2groundedgenum;
							VectorCopy(ground_beststart, lreach->start);
							VectorCopy(ground_bestend, lreach->end);
							lreach->traveltype = TRAVEL_WALKOFFLEDGE;
							lreach->traveltime = aassettings.rs_startwalkoffledge + Q_fabs(ground_bestdist) * 50 / aassettings.phys_gravity;
							//if falling from too high and not falling into water
							if (!AAS_AreaSwim(area2num) && !AAS_AreaJumpPad(area2num))
							{
								if (AAS_FallDelta(ground_bestdist) > aassettings.phys_falldelta5)
								{
									lreach->traveltime += aassettings.rs_falldamage5;
								}
								if (AAS_FallDelta(ground_bestdist) > aassettings.phys_falldelta10)
								{
									lreach->traveltime += aassettings.rs_falldamage10;
								}
							}
							lreach->next = areareachability[area1num];
							areareachability[area1num] = lreach;
							//
							reach_walkoffledge++;
							//NOTE: don't create a weapon (rl, bfg) jump reachability here
							//because it interferes with other reachabilities
							//like the ladder reachability
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

// create a possible ladder reachability from area1 to area2
bool AAS_Reachability_Ladder(int area1num, int area2num)
{
	int i, j, k, l, edge1num, edge2num, sharededgenum, lowestedgenum;
	int face1num, face2num, ladderface1num, ladderface2num;
	int ladderface1vertical, ladderface2vertical, firstv;
	float face1area, face2area, bestface1area, bestface2area;
	float phys_jumpvel, maxjumpheight;
	vec3_t area1point, area2point, v1, v2, up = {0, 0, 1};
	vec3_t mid, lowestpoint, start, end, sharededgevec, dir;
	aas_area_t* area1, * area2;
	aas_face_t* face1, * face2, * ladderface1, * ladderface2;
	aas_plane_t* plane1, * plane2;
	aas_edge_t* sharededge, * edge1;
	aas_lreachability_t* lreach;
	aas_trace_t trace;

	if (!AAS_AreaLadder(area1num) || !AAS_AreaLadder(area2num))
	{
		return false;
	}

	phys_jumpvel = aassettings.phys_jumpvel;
	//maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight(phys_jumpvel);

	area1 = &aasworld->areas[area1num];
	area2 = &aasworld->areas[area2num];
	//
	ladderface1 = NULL;
	ladderface2 = NULL;
	ladderface1num = 0;	//make compiler happy
	ladderface2num = 0;	//make compiler happy
	bestface1area = -9999;
	bestface2area = -9999;
	sharededgenum = 0;	//make compiler happy
	lowestedgenum = 0;	//make compiler happy
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		face1num = aasworld->faceindex[area1->firstface + i];
		face1 = &aasworld->faces[abs(face1num)];
		//if not a ladder face
		if (!(face1->faceflags & FACE_LADDER))
		{
			continue;
		}
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2num = aasworld->faceindex[area2->firstface + j];
			face2 = &aasworld->faces[abs(face2num)];
			//if not a ladder face
			if (!(face2->faceflags & FACE_LADDER))
			{
				continue;
			}
			//check if the faces share an edge
			for (k = 0; k < face1->numedges; k++)
			{
				edge1num = aasworld->edgeindex[face1->firstedge + k];
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = aasworld->edgeindex[face2->firstedge + l];
					if (abs(edge1num) == abs(edge2num))
					{
						//get the face with the largest area
						face1area = AAS_FaceArea(face1);
						face2area = AAS_FaceArea(face2);
						if (face1area > bestface1area && face2area > bestface2area)
						{
							bestface1area = face1area;
							bestface2area = face2area;
							ladderface1 = face1;
							ladderface2 = face2;
							ladderface1num = face1num;
							ladderface2num = face2num;
							sharededgenum = edge1num;
						}
						break;
					}
				}
				if (l != face2->numedges)
				{
					break;
				}
			}
		}
	}

	if (ladderface1 && ladderface2)
	{
		//get the middle of the shared edge
		sharededge = &aasworld->edges[abs(sharededgenum)];
		firstv = sharededgenum < 0;

		VectorCopy(aasworld->vertexes[sharededge->v[firstv]], v1);
		VectorCopy(aasworld->vertexes[sharededge->v[!firstv]], v2);
		VectorAdd(v1, v2, area1point);
		VectorScale(area1point, 0.5, area1point);
		VectorCopy(area1point, area2point);

		//if the face plane in area 1 is pretty much vertical
		plane1 = &aasworld->planes[ladderface1->planenum ^ (ladderface1num < 0)];
		plane2 = &aasworld->planes[ladderface2->planenum ^ (ladderface2num < 0)];

		//get the points really into the areas
		VectorSubtract(v2, v1, sharededgevec);
		CrossProduct(plane1->normal, sharededgevec, dir);
		VectorNormalize(dir);
		//NOTE: 32 because that's larger than 16 (bot bbox x,y)
		VectorMA(area1point, -32, dir, area1point);
		VectorMA(area2point, 32, dir, area2point);

		ladderface1vertical = Q_fabs(DotProduct(plane1->normal, up)) < 0.1;
		ladderface2vertical = Q_fabs(DotProduct(plane2->normal, up)) < 0.1;
		//there's only reachability between vertical ladder faces
		if (!ladderface1vertical && !ladderface2vertical)
		{
			return false;
		}
		//if both vertical ladder faces
		if (ladderface1vertical && ladderface2vertical
			//and the ladder faces do not make a sharp corner
			&& DotProduct(plane1->normal, plane2->normal) > 0.7
			//and the shared edge is not too vertical
			&& Q_fabs(DotProduct(sharededgevec, up)) < 0.7)
		{
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return false;
			}
			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area1point, lreach->start);
			//VectorCopy(area2point, lreach->end);
			VectorMA(area2point, -3, plane1->normal, lreach->end);
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;

			reach_ladder++;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return false;
			}
			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area2point, lreach->start);
			//VectorCopy(area1point, lreach->end);
			if (GGameType & (GAME_WolfSP | GAME_ET))
			{
				VectorMA(area1point, -3, plane2->normal, lreach->end);
			}
			else
			{
				VectorMA(area1point, -3, plane1->normal, lreach->end);
			}
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_ladder++;
			//
			return true;
		}
			//if the second ladder face is also a ground face
			//create ladder end (just ladder) reachability and
			//walk off a ladder (ledge) reachability
		if (ladderface1vertical && (ladderface2->faceflags & FACE_GROUND))
		{
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return false;
			}
			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area1point, lreach->start);
			VectorCopy(area2point, lreach->end);
			lreach->end[2] += 16;
			VectorMA(lreach->end, -15, plane1->normal, lreach->end);
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;

			reach_ladder++;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach)
			{
				return false;
			}
			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area2point, lreach->start);
			VectorCopy(area1point, lreach->end);
			if (GGameType & GAME_ET)
			{
				// RF, this should be a ladder reachability, since we usually climb down ladders in Wolf (falling is possibly dangerous)
				lreach->end[2] -= 16;
				VectorMA(lreach->start, -15, plane1->normal, lreach->start);
				lreach->traveltype = TRAVEL_LADDER;
				lreach->traveltime = 100;	// have to turn around
			}
			else
			{
				lreach->traveltype = TRAVEL_WALKOFFLEDGE;
				lreach->traveltime = 10;
			}
			lreach->next = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_walkoffledge++;
			//
			return true;
		}

		if (ladderface1vertical)
		{
			//find lowest edge of the ladder face
			lowestpoint[2] = 99999;
			//	Shut up compiler
			lowestpoint[0] = 0;
			lowestpoint[1] = 0;
			for (i = 0; i < ladderface1->numedges; i++)
			{
				edge1num = abs(aasworld->edgeindex[ladderface1->firstedge + i]);
				edge1 = &aasworld->edges[edge1num];
				//
				VectorCopy(aasworld->vertexes[edge1->v[0]], v1);
				VectorCopy(aasworld->vertexes[edge1->v[1]], v2);
				//
				VectorAdd(v1, v2, mid);
				VectorScale(mid, 0.5, mid);
				//
				if (mid[2] < lowestpoint[2])
				{
					VectorCopy(mid, lowestpoint);
					lowestedgenum = edge1num;
				}
			}
				//
			plane1 = &aasworld->planes[ladderface1->planenum];
			//trace down in the middle of this edge
			VectorMA(lowestpoint, 5, plane1->normal, start);
			VectorCopy(start, end);
			start[2] += 5;
			end[2] -= 100;
			//trace without entity collision
			trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);

			trace.endpos[2] += 1;
			area2num = AAS_PointAreaNum(trace.endpos);
			//
			area2 = &aasworld->areas[area2num];
			for (i = 0; i < area2->numfaces; i++)
			{
				face2num = aasworld->faceindex[area2->firstface + i];
				face2 = &aasworld->faces[abs(face2num)];
				//
				if (face2->faceflags & FACE_LADDER)
				{
					plane2 = &aasworld->planes[face2->planenum];
					if (Q_fabs(DotProduct(plane2->normal, up)) < 0.1)
					{
						break;
					}
				}
			}
				//if from another area without vertical ladder faces
			if (i >= area2->numfaces && area2num != area1num &&
				//the reachabilities shouldn't exist already
				!AAS_ReachabilityExists(area1num, area2num) &&
				!AAS_ReachabilityExists(area2num, area1num))
			{
				//if the height is jumpable
				if (start[2] - trace.endpos[2] < maxjumpheight)
				{
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						return false;
					}
					lreach->areanum = area2num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy(lowestpoint, lreach->start);
					VectorCopy(trace.endpos, lreach->end);
					lreach->traveltype = TRAVEL_LADDER;
					lreach->traveltime = 10;
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//
					reach_ladder++;
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						return false;
					}
					lreach->areanum = area1num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy(trace.endpos, lreach->start);
					//get the end point a little bit into the ladder
					VectorMA(lowestpoint, -5, plane1->normal, lreach->end);
					//get the end point a little higher
					lreach->end[2] += 10;
					lreach->traveltype = TRAVEL_JUMP;
					lreach->traveltime = 10;
					lreach->next = areareachability[area2num];
					areareachability[area2num] = lreach;

					reach_jump++;

					return true;
				}
			}
		}
	}
	return false;
}
