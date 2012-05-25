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
int reach_doublejump;	//double jump
int reach_rampjump;		//ramp jump
int reach_strafejump;	//strafe jump (just normal jump but further)
int reach_rocketjump;	//rocket jump
int reach_bfgjump;		//bfg jump
int reach_jumppad;		//jump pads
//if true grapple reachabilities are skipped
int calcgrapplereach;

//temporary reachabilities
static aas_lreachability_t* reachabilityheap;	//heap with reachabilities
static aas_lreachability_t* nextreachability;	//next free reachability from the heap
aas_lreachability_t** areareachability;	//reachability links for every area
int numlreachabilities;

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
