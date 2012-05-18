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

bot_debugpoly_t* debugpolygons;
int bot_maxdebugpolys;

void BotImport_Print(int type, const char* fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch (type)
	{
	case PRT_MESSAGE:
		common->Printf("%s", str);
		break;

	case PRT_WARNING:
		common->Printf(S_COLOR_YELLOW "Warning: %s", str);
		break;

	case PRT_ERROR:
		common->Printf(S_COLOR_RED "Error: %s", str);
		break;

	case PRT_FATAL:
		common->Printf(S_COLOR_RED "Fatal: %s", str);
		break;

	case PRT_EXIT:
		common->Error(S_COLOR_RED "Exit: %s", str);
		break;

	default:
		common->Printf("unknown print type\n");
		break;
	}
}

void BotImport_BSPModelMinsMaxsOrigin(int modelnum, const vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin)
{
	clipHandle_t h;
	vec3_t mins, maxs;
	float max;
	int i;

	h = CM_InlineModel(modelnum);
	CM_ModelBounds(h, mins, maxs);
	//if the model is rotated
	if ((angles[0] || angles[1] || angles[2]))
	{
		// expand for rotation

		max = RadiusFromBounds(mins, maxs);
		for (i = 0; i < 3; i++)
		{
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if (outmins)
	{
		VectorCopy(mins, outmins);
	}
	if (outmaxs)
	{
		VectorCopy(maxs, outmaxs);
	}
	if (origin)
	{
		VectorClear(origin);
	}
}

int BotImport_DebugPolygonCreate(int color, int numPoints, const vec3_t* points)
{
	if (!debugpolygons)
	{
		return 0;
	}

	int i;
	for (i = 1; i < bot_maxdebugpolys; i++)
	{
		if (!debugpolygons[i].inuse)
		{
			break;
		}
	}
	if (i >= bot_maxdebugpolys)
	{
		return 0;
	}
	bot_debugpoly_t* poly = &debugpolygons[i];
	poly->inuse = true;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));

	return i;
}

void BotImport_DebugPolygonDelete(int id)
{
	if (!debugpolygons)
	{
		return;
	}
	debugpolygons[id].inuse = false;
}

int BotImport_DebugLineCreate()
{
	vec3_t points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

void BotImport_DebugLineDelete(int line)
{
	BotImport_DebugPolygonDelete(line);
}

static void BotImport_DebugPolygonShow(int id, int color, int numPoints, const vec3_t* points)
{
	bot_debugpoly_t* poly;

	if (!debugpolygons)
	{
		return;
	}
	poly = &debugpolygons[id];
	poly->inuse = true;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
}

void BotImport_DebugLineShow(int line, const vec3_t start, const vec3_t end, int color)
{
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	//points[1][2] -= 2;
	VectorCopy(end, points[2]);
	//points[2][2] -= 2;
	VectorCopy(end, points[3]);


	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	BotImport_DebugPolygonShow(line, color, 4, points);
}
