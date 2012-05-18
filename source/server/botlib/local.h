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

#ifndef _BOTLIB_LOCAL_H
#define _BOTLIB_LOCAL_H

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

struct bot_debugpoly_t
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
};

extern bot_debugpoly_t* debugpolygons;
extern int bot_maxdebugpolys;

void BotImport_Print(int type, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, const vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin);
int BotImport_DebugLineCreate();
void BotImport_DebugLineDelete(int line);
void BotImport_DebugLineShow(int line, const vec3_t start, const vec3_t end, int color);

#endif
