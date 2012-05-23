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

#define MAX_DEBUGPOLYGONS           8192

int debuglines[MAX_DEBUGLINES];
int debuglinevisible[MAX_DEBUGLINES];
int numdebuglines;

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
