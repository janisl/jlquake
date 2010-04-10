//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#define	CVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
								// used for system variables, not for player
								// specific configurations
#define	CVAR_USERINFO		2	// sent to server on connect or change
#define	CVAR_SERVERINFO		4	// sent in response to front end requests
#define	CVAR_INIT			8	// don't allow change from console at all,
								// but can be set from the command line
#define	CVAR_LATCH			16	// will only change when C code next does
								// a Cvar_Get(), so it can't be changed
								// without proper initialization.  modified
								// will be set, even though the value hasn't
								// changed yet
#define	CVAR_SYSTEMINFO		32	// these cvars will be duplicated on all clients
#define	CVAR_ROM			64	// display only, cannot be set by user at all
#define	CVAR_USER_CREATED	128	// created by a set command
#define	CVAR_TEMP			256	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			512	// can not be changed if cheats are disabled
#define CVAR_NORESTART		1024	// do not clear when a cvar_restart is issued

#define	CVAR_NOSET		8	// don't allow change from console at all,
							// but can be set from the command line

// nothing outside the Cvar_*() functions should modify these fields!
struct QCvar
{
	//	This class is accessible to Quake 2 game, so the following fields
	// must remain exactly as they are.
	char		*name;
	char		*string;
	char		*latchedString;		// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;			// set each time the cvar is changed
	float		value;				// atof( string )
	QCvar*		next;

	qboolean	archive;		// set to true to cause it to be saved to vars.rc
	qboolean	info;			// added to serverinfo or userinfo when changed

	char		*resetString;		// cvar_restart will reset to this value
	int			modificationCount;	// incremented each time the cvar is changed
	int			integer;			// atoi( string )
	QCvar*		hashNext;
	int			Handle;
};

QCvar* Cvar_Get(const char* VarName, const char* Value, int flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

long Cvar_GenerateHashValue(const char* VarName);

QCvar* Cvar_FindVar(const char* VarName);

float Cvar_VariableValue(const char* VarName);
int	Cvar_VariableIntegerValue(const char* VarName);
// returns 0 if not defined or non numeric

const char* Cvar_VariableString(const char* VarName);
void Cvar_VariableStringBuffer(const char* VarName, char* Buffer, int BufSize);
// returns an empty string if not defined

QCvar* Cvar_Set(const char* VarName, const char* Value);
// will create the variable with no flags if it doesn't exist

QCvar* Cvar_SetLatched(const char* VarName, const char* Value);
// don't set the cvar immediately

void Cvar_SetValue(const char* VarName, float value);
// expands value to a string and calls Cvar_Set

const char* Cvar_CompleteVariable(const char* Partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

typedef QCvar cvar_t;

extern QCvar*		cvar_vars;
extern QCvar*		cvar_cheats;
extern int			cvar_modifiedFlags;

#define	MAX_CVARS	1024
extern QCvar*		cvar_indexes[MAX_CVARS];
extern int			cvar_numIndexes;

#define FILE_HASH_SIZE		256
extern QCvar*		cvar_hashTable[FILE_HASH_SIZE];
