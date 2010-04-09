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

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

cvar_t*			cvar_vars;
cvar_t		*cvar_cheats;
int			cvar_modifiedFlags;

cvar_t		*cvar_indexes[MAX_CVARS];
int			cvar_numIndexes;

cvar_t*		cvar_hashTable[FILE_HASH_SIZE];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Cvar_GenerateHashValue
//
//	Return a hash value for the filename
//
//==========================================================================

long Cvar_GenerateHashValue(const char *fname)
{
	long hash = 0;
	for (int i = 0; fname[i] != '\0'; i++)
	{
		char letter = QStr::ToLower(fname[i]);
		hash += (long)(letter) * (i + 119);
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

char* __CopyString(const char* in)
{
	char* out = (char*)Mem_Alloc(QStr::Length(in)+1);
	QStr::Cpy(out, in);
	return out;
}

/*
============
Cvar_FindVar
============
*/
cvar_t* Cvar_FindVar(const char *var_name)
{
	cvar_t	*var;
	long hash;

	hash = Cvar_GenerateHashValue(var_name);
	
	for (var=cvar_hashTable[hash] ; var ; var=var->hashNext) {
		if (!QStr::ICmp(var_name, var->name)) {
			return var;
		}
	}

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue( const char *var_name ) {
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}
