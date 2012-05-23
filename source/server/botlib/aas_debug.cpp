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

#define MAX_DEBUGLINES              1024
#define MAX_DEBUGPOLYGONS           8192

static int debuglines[MAX_DEBUGLINES];
static int debuglinevisible[MAX_DEBUGLINES];
static int numdebuglines;

static int aas_debugpolygons[MAX_DEBUGPOLYGONS];

void AAS_ClearShownPolygons()
{
	for (int i = 0; i < MAX_DEBUGPOLYGONS; i++)
	{
		if (aas_debugpolygons[i])
		{
			BotImport_DebugPolygonDelete(aas_debugpolygons[i]);
		}
		aas_debugpolygons[i] = 0;
	}
}

void AAS_ShowPolygon(int color, int numpoints, const vec3_t* points)
{
	for (int i = 0; i < MAX_DEBUGPOLYGONS; i++)
	{
		if (!aas_debugpolygons[i])
		{
			aas_debugpolygons[i] = BotImport_DebugPolygonCreate(color, numpoints, points);
			break;
		}
	}
}

void AAS_ClearShownDebugLines()
{
	//make all lines invisible
	for (int i = 0; i < MAX_DEBUGLINES; i++)
	{
		if (debuglines[i])
		{
			BotImport_DebugLineDelete(debuglines[i]);
			debuglines[i] = 0;
			debuglinevisible[i] = false;
		}
	}
}

void AAS_DebugLine(const vec3_t start, const vec3_t end, int color)
{
	for (int line = 0; line < MAX_DEBUGLINES; line++)
	{
		if (!debuglines[line])
		{
			debuglines[line] = BotImport_DebugLineCreate();
			debuglinevisible[line] = false;
			numdebuglines++;
		}
		if (!debuglinevisible[line])
		{
			BotImport_DebugLineShow(debuglines[line], start, end, color);
			debuglinevisible[line] = true;
			return;
		}
	}
}

void AAS_PermanentLine(const vec3_t start, const vec3_t end, int color)
{
	int line = BotImport_DebugLineCreate();
	BotImport_DebugLineShow(line, start, end, color);
}

void AAS_DrawPermanentCross(const vec3_t origin, float size, int color)
{
	for (int i = 0; i < 3; i++)
	{
		vec3_t start;
		VectorCopy(origin, start);
		start[i] += size;
		vec3_t end;
		VectorCopy(origin, end);
		end[i] -= size;
		AAS_DebugLine(start, end, color);
		int debugline = BotImport_DebugLineCreate();
		BotImport_DebugLineShow(debugline, start, end, color);
	}
}

static void AAS_ShowFacePolygon(int facenum, int color, int flip)
{
	//check if face number is in range
	if (facenum >= aasworld->numfaces)
	{
		BotImport_Print(PRT_ERROR, "facenum %d out of range\n", facenum);
	}
	aas_face_t* face = &aasworld->faces[facenum];
	//walk through the edges of the face
	vec3_t points[128];
	int numpoints = 0;
	if (flip)
	{
		for (int i = face->numedges - 1; i >= 0; i--)
		{
			//edge number
			int edgenum = aasworld->edgeindex[face->firstedge + i];
			aas_edge_t* edge = &aasworld->edges[abs(edgenum)];
			VectorCopy(aasworld->vertexes[edge->v[edgenum < 0]], points[numpoints]);
			numpoints++;
		}
	}
	else
	{
		for (int i = 0; i < face->numedges; i++)
		{
			//edge number
			int edgenum = aasworld->edgeindex[face->firstedge + i];
			aas_edge_t* edge = &aasworld->edges[abs(edgenum)];
			VectorCopy(aasworld->vertexes[edge->v[edgenum < 0]], points[numpoints]);
			numpoints++;
		}
	}
	AAS_ShowPolygon(color, numpoints, points);
}

void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly)
{
	int i, facenum;
	aas_area_t* area;
	aas_face_t* face;

	if (areanum < 0 || areanum >= aasworld->numareas)
	{
		BotImport_Print(PRT_ERROR, "area %d out of range [0, %d]\n",
			areanum, aasworld->numareas);
		return;
	}

	//pointer to the convex area
	area = &aasworld->areas[areanum];

	//walk through the faces of the area
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = abs(aasworld->faceindex[area->firstface + i]);
		//check if face number is in range
		if (facenum >= aasworld->numfaces)
		{
			BotImport_Print(PRT_ERROR, "facenum %d out of range\n", facenum);
		}	//end if
		face = &aasworld->faces[facenum];
		//ground faces only
		if (groundfacesonly)
		{
			if (!(face->faceflags & (FACE_GROUND | FACE_LADDER)))
			{
				continue;
			}
		}	//end if
		AAS_ShowFacePolygon(facenum, color, face->frontarea != areanum);
	}	//end for
}

void AAS_DrawCross(const vec3_t origin, float size, int color)
{
	for (int i = 0; i < 3; i++)
	{
		vec3_t start;
		VectorCopy(origin, start);
		start[i] += size;
		vec3_t end;
		VectorCopy(origin, end);
		end[i] -= size;
		AAS_DebugLine(start, end, color);
	}
}

void AAS_PrintTravelType(int traveltype)
{
#ifdef DEBUG
	char* str;
	switch (traveltype & TRAVELTYPE_MASK)
	{
	case TRAVEL_INVALID: str = "TRAVEL_INVALID"; break;
	case TRAVEL_WALK: str = "TRAVEL_WALK"; break;
	case TRAVEL_CROUCH: str = "TRAVEL_CROUCH"; break;
	case TRAVEL_BARRIERJUMP: str = "TRAVEL_BARRIERJUMP"; break;
	case TRAVEL_JUMP: str = "TRAVEL_JUMP"; break;
	case TRAVEL_LADDER: str = "TRAVEL_LADDER"; break;
	case TRAVEL_WALKOFFLEDGE: str = "TRAVEL_WALKOFFLEDGE"; break;
	case TRAVEL_SWIM: str = "TRAVEL_SWIM"; break;
	case TRAVEL_WATERJUMP: str = "TRAVEL_WATERJUMP"; break;
	case TRAVEL_TELEPORT: str = "TRAVEL_TELEPORT"; break;
	case TRAVEL_ELEVATOR: str = "TRAVEL_ELEVATOR"; break;
	case TRAVEL_ROCKETJUMP: str = "TRAVEL_ROCKETJUMP"; break;
	case TRAVEL_BFGJUMP: str = "TRAVEL_BFGJUMP"; break;
	case TRAVEL_GRAPPLEHOOK: str = "TRAVEL_GRAPPLEHOOK"; break;
	case TRAVEL_JUMPPAD: str = "TRAVEL_JUMPPAD"; break;
	case TRAVEL_FUNCBOB: str = "TRAVEL_FUNCBOB"; break;
	default: str = "UNKNOWN TRAVEL TYPE"; break;
	}
	BotImport_Print(PRT_MESSAGE, "%s", str);
#endif
}

void AAS_DrawArrow(const vec3_t start, const vec3_t end, int linecolor, int arrowcolor)
{
	vec3_t dir;
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	vec3_t up = {0, 0, 1};
	float dot = DotProduct(dir, up);
	vec3_t cross;
	if (dot > 0.99 || dot < -0.99)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	vec3_t p1;
	VectorMA(end, -6, dir, p1);
	vec3_t p2;
	VectorCopy(p1, p2);
	VectorMA(p1, 6, cross, p1);
	VectorMA(p2, -6, cross, p2);

	AAS_DebugLine(start, end, linecolor);
	AAS_DebugLine(p1, end, arrowcolor);
	AAS_DebugLine(p2, end, arrowcolor);
}

