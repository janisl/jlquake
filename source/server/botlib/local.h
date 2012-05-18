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

//library variable
struct libvar_t
{
	char* name;
	char* string;
	float value;
	libvar_t* next;
};

extern bot_debugpoly_t* debugpolygons;
extern int bot_maxdebugpolys;

void BotImport_Print(int type, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, const vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin);
int BotImport_DebugLineCreate();
void BotImport_DebugLineDelete(int line);
void BotImport_DebugLineShow(int line, const vec3_t start, const vec3_t end, int color);

//removes all library variables
void LibVarDeAllocAll();
//gets the string of the library variable with the given name
const char* LibVarGetString(const char* var_name);
//gets the value of the library variable with the given name
float LibVarGetValue(const char* var_name);
//creates the library variable if not existing already and returns it
libvar_t* LibVar(const char* var_name, const char* value);
//creates the library variable if not existing already and returns the value
float LibVarValue(const char* var_name, const char* value);
//creates the library variable if not existing already and returns the value string
const char* LibVarString(const char* var_name, const char* value);
//sets the library variable
void LibVarSet(const char* var_name, const char* value);

/*#define MAX_LOGFILENAMESIZE     1024

struct logfile_t
{
	char filename[MAX_LOGFILENAMESIZE];
	FILE* fp;
	int numwrites;
};

extern logfile_t logfile;*/

#endif
