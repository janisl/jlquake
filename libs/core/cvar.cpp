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

cvar_t *Cvar_Set2( const char *var_name, const char *value, bool force);

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

/*
============
Cvar_VariableIntegerValue
============
*/
int Cvar_VariableIntegerValue(const char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
const char* Cvar_VariableString( const char *var_name )
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}


/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var) {
		*buffer = 0;
	}
	else {
		QStr::NCpyZ( buffer, var->string, bufsize );
	}
}

/*
============
Cvar_ValidateString
============
*/
bool Cvar_ValidateString( const char *s )
{
	if ( !s ) {
		return false;
	}
	if ( strchr( s, '\\' ) ) {
		return false;
	}
	if ( strchr( s, '\"' ) ) {
		return false;
	}
	if ( strchr( s, ';' ) ) {
		return false;
	}
	return true;
}

/*
============
Cvar_Set
============
*/
QCvar* Cvar_Set(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, true);
}

/*
============
Cvar_SetLatched
============
*/
QCvar* Cvar_SetLatched(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, false);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue( const char *var_name, float value)
{
	char	val[32];

	if ( value == (int)value ) {
		QStr::Sprintf (val, sizeof(val), "%i",(int)value);
	} else {
		QStr::Sprintf (val, sizeof(val), "%f",value);
	}
	Cvar_Set (var_name, val);
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get(const char *var_name, const char *var_value, int flags)
{
	cvar_t	*var;

    if ( !var_name || ! var_value )
    {
		throw QException("Cvar_Get: NULL parameter" );
    }

	if (!Cvar_ValidateString(var_name))
    {
        GLog.Write("invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

	var = Cvar_FindVar (var_name);
	if (var)
    {
		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if ((var->flags & CVAR_USER_CREATED) && !(flags & CVAR_USER_CREATED) &&
            var_value[0])
        {
			var->flags &= ~CVAR_USER_CREATED;
			Mem_Free(var->resetString);
			var->resetString = __CopyString(var_value);

			// ZOID--needs to be set so that cvars the game sets as 
			// SERVERINFO get sent to clients
			cvar_modifiedFlags |= flags;
		}

		var->flags |= flags;
		// only allow one non-empty reset string without a warning
		if (!var->resetString[0])
        {
			// we don't have a reset string yet
			Mem_Free(var->resetString);
			var->resetString = __CopyString(var_value);
		}
        else if (var_value[0] && QStr::Cmp(var->resetString, var_value))
        {
            GLog.DWrite("Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
				var_name, var->resetString, var_value);
		}
		// if we have a latched string, take that value now
		if (var->latchedString)
        {
			char *s;

			s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2( var_name, s, true );
			Mem_Free( s );
		}

		return var;
	}

    //  Quake 3 didin't check this at all. It had commented out check that
    // ignored flags and it was commented out because variables that are not
    // info variables can contain characters that are invalid for info srings.
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_ValidateString(var_value))
		{
            GLog.Write("invalid info cvar value\n");
			return NULL;
		}
	}

	//
	// allocate a new cvar
	//
	if (cvar_numIndexes >= MAX_CVARS)
    {
		throw QException("MAX_CVARS" );
	}
	var = new QCvar;
	Com_Memset(var, 0, sizeof(*var));
	cvar_indexes[cvar_numIndexes] = var;
	var->Handle = cvar_numIndexes;
	cvar_numIndexes++;
	var->name = __CopyString (var_name);
	var->string = __CopyString (var_value);
	var->modified = true;
	var->modificationCount = 1;
	var->value = QStr::Atof(var->string);
	var->integer = QStr::Atoi(var->string);
	var->resetString = __CopyString( var_value );

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	long hash = Cvar_GenerateHashValue(var_name);
	var->hashNext = cvar_hashTable[hash];
	cvar_hashTable[hash] = var;

	return var;
}

/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = QStr::Length(partial);
	
	if (!len)
		return NULL;
		
	// check exact match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!QStr::Cmp(partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!QStr::NCmp(partial,cvar->name, len))
			return cvar->name;

	return NULL;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command()
{
	cvar_t			*v;

	// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
	{
		return false;
	}

	// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		GLog.Write("\"%s\" is:\"%s" S_COLOR_WHITE "\" default:\"%s" S_COLOR_WHITE "\"\n",
			v->name, v->string, v->resetString);
		if (v->latchedString)
		{
			GLog.Write("latched: \"%s\"\n", v->latchedString);
		}
		return true;
	}

	// set the value if forcing isn't required
	Cvar_Set2(v->name, Cmd_Argv(1), false);
	return true;
}
