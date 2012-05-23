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
