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

#ifndef _TECH3_LOCAL_H
#define _TECH3_LOCAL_H

//
//	Game
//
idEntity3* SVT3_EntityNum(int number);
bool SVT3_inPVS(const vec3_t p1, const vec3_t p2);
bool SVT3_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2);

//
//	World
//
struct q3moveclip_t
{
	vec3_t boxmins, boxmaxs;	// enclose the test object along entire move
	const float* mins;
	const float* maxs;	// size of the moving object
	const float* start;
	vec3_t end;
	q3trace_t trace;
	int passEntityNum;
	int contentmask;
	int capsule;
};

void SVT3_SectorList_f();
void SVT3_UnlinkSvEntity(q3svEntity_t* ent);
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.
int SVT3_AreaEntities(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount);
void SVT3_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule);
//	mins and maxs are relative
//	if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0
//	if the starting point is in a solid, it will be allowed to move out
// to an open area
//	passEntityNum is explicitly excluded from clipping checks (normally Q3ENTITYNUM_NONE)
void SVT3_Trace(q3trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule);
// returns the CONTENTS_* value from the world and all entities at the given point.
int SVT3_PointContents(const vec3_t p, int passEntityNum);

#endif
