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

#include "public.h"

#define BOTFILESBASEFOLDER      "botfiles"

//
//	Imports
//

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

//
//	Interface
//

extern int bot_developer;					//true if developer is on

//
//	Libvars
//

//library variable
struct libvar_t
{
	char* name;
	char* string;
	float value;
	libvar_t* next;
};

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

//
//	Logging
//

//open a log file
void Log_Open(const char* filename);
//close log file if present
void Log_Shutdown();
//write to the current opened log file
void Log_Write(const char* fmt, ...) id_attribute((format(printf, 1, 2)));

//
//	Struct
//

#define MAX_STRINGFIELD             80
//field types
#define FT_INT                      2		// int
#define FT_FLOAT                    3		// float
#define FT_STRING                   4		// char [MAX_STRINGFIELD]
//type only mask
#define FT_TYPE                     0x00FF	// only type, clear subtype
//sub types
#define FT_ARRAY                    0x0100	// array of type

//structure field definition
struct fielddef_t
{
	const char* name;						//name of the field
	int offset;								//offset in the structure
	int type;								//type of the field
	//type specific fields
	int maxarray;							//maximum array size
};

//structure definition
struct structdef_t
{
	int size;
	fielddef_t* fields;
};

//read a structure from a script
bool ReadStructure(source_t* source, const structdef_t* def, char* structure);

#endif
