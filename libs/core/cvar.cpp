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
// cvar.c -- dynamic variable tracking

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

#define	MAX_CVARS			1024

#define FILE_HASH_SIZE		256

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void Cvar_Changed(QCvar* var);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QCvar*		cvar_vars;
int			cvar_modifiedFlags;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static QCvar*		cvar_cheats;

static QCvar*		cvar_indexes[MAX_CVARS];
static int			cvar_numIndexes;

static QCvar*		cvar_hashTable[FILE_HASH_SIZE];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Cvar_GenerateHashValue
//
//	Return a hash value for the filename
//
//==========================================================================

static long Cvar_GenerateHashValue(const char *fname)
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

//==========================================================================
//
//	__CopyString
//
//==========================================================================

char* __CopyString(const char* in)
{
	char* out = (char*)Mem_Alloc(QStr::Length(in) + 1);
	QStr::Cpy(out, in);
	return out;
}

//==========================================================================
//
//	Cvar_FindVar
//
//==========================================================================

QCvar* Cvar_FindVar(const char* VarName)
{
	long hash = Cvar_GenerateHashValue(VarName);

	for (QCvar* var = cvar_hashTable[hash]; var; var = var->hashNext)
	{
		if (!QStr::ICmp(VarName, var->name))
		{
			return var;
		}
	}

	return NULL;
}

//==========================================================================
//
//	Cvar_ValidateString
//
//==========================================================================

static bool Cvar_ValidateString(const char* S)
{
	if (!S)
	{
		return false;
	}
	if (strchr(S, '\\'))
	{
		return false;
	}
	if (strchr(S, '\"'))
	{
		return false;
	}
	if (strchr(S, ';'))
	{
		return false;
	}
	return true;
}

//==========================================================================
//
//	Cvar_Set2
//
//==========================================================================

static QCvar* Cvar_Set2(const char *var_name, const char *value, bool force)
{
	GLog.DWrite("Cvar_Set2: %s %s\n", var_name, value);

	if (!Cvar_ValidateString(var_name))
	{
		GLog.Write("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

	QCvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		if (!value)
		{
			return NULL;
		}
		// create it
		if (!force)
		{
			return Cvar_Get(var_name, value, CVAR_USER_CREATED);
		}
		else
		{
			return Cvar_Get(var_name, value, 0);
		}
	}

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_ValidateString(value))
		{
			GLog.Write("invalid info cvar value\n");
			return var;
		}
	}

	if (!value)
	{
		value = var->resetString;
	}

	if (!QStr::Cmp(value, var->string) && !var->latchedString)
	{
		return var;
	}
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		if (var->flags & CVAR_ROM)
		{
			GLog.Write("%s is read only.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			GLog.Write("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (QStr::Cmp(value, var->latchedString) == 0)
				{
					return var;
				}
				Mem_Free(var->latchedString);
			}
			else
			{
				if (QStr::Cmp(value, var->string) == 0)
				{
					return var;
				}
			}

			GLog.Write("%s will be changed upon restarting.\n", var_name);
			var->latchedString = __CopyString(value);
			var->modified = true;
			var->modificationCount++;
			return var;
		}

		if ((var->flags & CVAR_CHEAT) && !cvar_cheats->integer)
		{
			GLog.Write("%s is cheat protected.\n", var_name);
			return var;
		}
	}
	else
	{
		if (var->latchedString)
		{
			Mem_Free(var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!QStr::Cmp(value, var->string))
	{
		return var;		// not changed
	}

	var->modified = true;
	var->modificationCount++;

	Mem_Free(var->string);	// free the old value string

	var->string = __CopyString(value);
	var->value = QStr::Atof(var->string);
	var->integer = QStr::Atoi(var->string);

	Cvar_Changed(var);
	return var;
}

//==========================================================================
//
//	Cvar_Set
//
//==========================================================================

QCvar* Cvar_Set(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, true);
}

//==========================================================================
//
//	Cvar_SetLatched
//
//==========================================================================

QCvar* Cvar_SetLatched(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, false);
}

//==========================================================================
//
//	Cvar_SetValue
//
//==========================================================================

void Cvar_SetValue(const char* var_name, float value)
{
	char	val[32];

	if (value == (int)value)
	{
		QStr::Sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		QStr::Sprintf(val, sizeof(val), "%f", value);
	}
	Cvar_Set(var_name, val);
}

//==========================================================================
//
//	Cvar_SetValueLatched
//
//==========================================================================

void Cvar_SetValueLatched(const char* var_name, float value)
{
	char	val[32];

	if (value == (int)value)
	{
		QStr::Sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		QStr::Sprintf(val, sizeof(val), "%f", value);
	}
	Cvar_SetLatched(var_name, val);
}

//==========================================================================
//
//	Cvar_Get
//
//	If the variable already exists, the value will not be set.
// The flags will be or'ed in if the variable exists.
//
//==========================================================================

QCvar* Cvar_Get(const char* VarName, const char* VarValue, int Flags)
{
	if (!VarName || (!(GGameType & GAME_Quake2) && !VarValue))
	{
		throw QException("Cvar_Get: NULL parameter");
	}

	if (!(GGameType & GAME_Quake2) || (Flags & (CVAR_USERINFO | CVAR_SERVERINFO)))
	{
		if (!Cvar_ValidateString(VarName))
		{
			if (GGameType & GAME_Quake2)
			{
				GLog.Write("invalid info cvar name\n");
				return NULL;
			}
			GLog.Write("invalid cvar name string: %s\n", VarName);
			VarName = "BADNAME";
		}
	}

	QCvar* var = Cvar_FindVar(VarName);
	if (var)
	{
		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if ((var->flags & CVAR_USER_CREATED) && !(Flags & CVAR_USER_CREATED) &&
			VarValue[0])
		{
			var->flags &= ~CVAR_USER_CREATED;
			Mem_Free(var->resetString);
			var->resetString = __CopyString(VarValue);

			// ZOID--needs to be set so that cvars the game sets as 
			// SERVERINFO get sent to clients
			cvar_modifiedFlags |= Flags;
		}

		var->flags |= Flags;
		// only allow one non-empty reset string without a warning
		if (!var->resetString[0])
		{
			// we don't have a reset string yet
			Mem_Free(var->resetString);
			var->resetString = __CopyString(VarValue);
		}
		else if (VarValue[0] && QStr::Cmp(var->resetString, VarValue))
		{
			GLog.DWrite("Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
				VarName, var->resetString, VarValue);
		}
		// if we have a latched string, take that value now
		if (!(GGameType & GAME_Quake2) && var->latchedString)
		{
			char* s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2(VarName, s, true);
			Mem_Free(s);
		}

		return var;
	}

	//	Quake 2 case, other games check this above.
	if (!VarValue)
	{
		return NULL;
	}

	//  Quake 3 doesn't check this at all. It had commented out check that
	// ignored flags and it was commented out because variables that are not
	// info variables can contain characters that are invalid for info srings.
	// Currently for compatibility it's not done for Quake 3.
	if (!(GGameType & GAME_Quake3) && (Flags & (CVAR_USERINFO | CVAR_SERVERINFO)))
	{
		if (!Cvar_ValidateString(VarValue))
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
	var->name = __CopyString(VarName);
	var->string = __CopyString(VarValue);
	var->modified = true;
	var->modificationCount = 1;
	var->value = QStr::Atof(var->string);
	var->integer = QStr::Atoi(var->string);
	var->resetString = __CopyString(VarValue);

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = Flags;

	long hash = Cvar_GenerateHashValue(VarName);
	var->hashNext = cvar_hashTable[hash];
	cvar_hashTable[hash] = var;

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		Cvar_Changed(var);
	}

	return var;
}

//==========================================================================
//
//	Cvar_VariableValue
//
//==========================================================================

float Cvar_VariableValue(const char* var_name)
{
	QCvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->value;
}

//==========================================================================
//
//	Cvar_VariableIntegerValue
//
//==========================================================================

int Cvar_VariableIntegerValue(const char* var_name)
{
	QCvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->integer;
}

//==========================================================================
//
//	Cvar_VariableString
//
//==========================================================================

const char* Cvar_VariableString(const char* var_name)
{
	QCvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return "";
	}
	return var->string;
}


//==========================================================================
//
//	Cvar_VariableStringBuffer
//
//==========================================================================

void Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
{
	QCvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		*buffer = 0;
	}
	else
	{
		QStr::NCpyZ(buffer, var->string, bufsize);
	}
}

//==========================================================================
//
//	Cvar_Command
//
//	Handles variable inspection and changing from the console
//
//==========================================================================

bool Cvar_Command()
{
	// check variables
	QCvar* v = Cvar_FindVar(Cmd_Argv(0));
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

//==========================================================================
//
//	Cvar_InfoString
//
//	Handles large info strings ( CS_SYSTEMINFO )
//
//==========================================================================

char* Cvar_InfoString(int bit, int MaxSize, int MaxKeySize, int MaxValSize,
	bool NoHighChars, bool LowerCaseVal)
{
	static char	info[BIG_INFO_STRING];

	info[0] = 0;

	for (QCvar* var = cvar_vars; var; var = var->next)
	{
		if (var->flags & bit)
		{
			Info_SetValueForKey(info, var->name, var->string, MaxSize,
				MaxKeySize, MaxValSize, NoHighChars, LowerCaseVal);
		}
	}
	return info;
}

//==========================================================================
//
//	Cvar_InfoStringBuffer
//
//==========================================================================

void Cvar_InfoStringBuffer(int bit, int MaxSize, char* buff, int buffsize)
{
	QStr::NCpyZ(buff, Cvar_InfoString(bit, MaxSize), buffsize);
}

//==========================================================================
//
//	Cvar_Register
//
//	basically a slightly modified Cvar_Get for the interpreted modules
//
//==========================================================================

void Cvar_Register(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags)
{
	//	For Quake 2 compatibility some flags have been moved around,
	// so map them to new values. Also clear unknown flags.
	int TmpFlags = flags;
	flags &= 0x07c7;
	if (TmpFlags & 8)
	{
		flags |= CVAR_SYSTEMINFO;
	}
	if (TmpFlags & 16)
	{
		flags |= CVAR_INIT;
	}
	if (TmpFlags & 32)
	{
		flags |= CVAR_LATCH;
	}

	QCvar* cv = Cvar_Get(varName, defaultValue, flags);
	if (!vmCvar)
	{
		return;
	}
	vmCvar->handle = cv->Handle;
	vmCvar->modificationCount = -1;
	Cvar_Update(vmCvar);
}


//==========================================================================
//
//	Cvar_Update
//
//	updates an interpreted modules' version of a cvar
//
//==========================================================================

void Cvar_Update(vmCvar_t* vmCvar)
{
	qassert(vmCvar); // bk

	if ((unsigned)vmCvar->handle >= cvar_numIndexes)
	{
		throw QDropException("Cvar_Update: handle out of range" );
	}

	QCvar* cv = cvar_indexes[vmCvar->handle];
	if (!cv)
	{
		return;
	}

	if (cv->modificationCount == vmCvar->modificationCount)
	{
		return;
	}
	if (!cv->string)
	{
		return;		// variable might have been cleared by a cvar_restart
	}
	vmCvar->modificationCount = cv->modificationCount;
	// bk001129 - mismatches.
	if (QStr::Length(cv->string) + 1 > MAX_CVAR_VALUE_STRING)
	{
		throw QDropException(va("Cvar_Update: src %s length %d exceeds MAX_CVAR_VALUE_STRING",
			cv->string, QStr::Length(cv->string), sizeof(vmCvar->string)));
	}
	// bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0 
	// bk001129 - paranoia. Never trust the destination string.
	// bk001129 - beware, sizeof(char*) is always 4 (for cv->string). 
	//            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
	//Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
	QStr::NCpyZ(vmCvar->string, cv->string,  MAX_CVAR_VALUE_STRING); 

	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}

//==========================================================================
//
//	Cvar_CompleteVariable
//
//==========================================================================

const char* Cvar_CompleteVariable(const char* partial)
{
	int len = QStr::Length(partial);

	if (!len)
	{
		return NULL;
	}

	// check exact match
	for (QCvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if (!QStr::Cmp(partial, cvar->name))
		{
			return cvar->name;
		}
	}

	// check partial match
	for (QCvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if (!QStr::NCmp(partial,cvar->name, len))
		{
			return cvar->name;
		}
	}

	return NULL;
}

//==========================================================================
//
//	Cvar_CommandCompletion
//
//==========================================================================

void Cvar_CommandCompletion(void(*callback)(const char* s))
{
	for (QCvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		callback(cvar->name);
	}
}

//==========================================================================
//
//	Cvar_SetCheatState
//
//	Any testing variables will be reset to the safe values
//
//==========================================================================

void Cvar_SetCheatState()
{
	// set all default vars to the safe value
	for (QCvar* var = cvar_vars; var; var = var->next)
	{
		if (var->flags & CVAR_CHEAT)
		{
			// the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here 
			// because of a different var->latchedString
			if (var->latchedString)
			{
				Mem_Free(var->latchedString);
				var->latchedString = NULL;
			}
			if (QStr::Cmp(var->resetString,var->string))
			{
				Cvar_Set(var->name, var->resetString);
			}
		}
	}
}

//==========================================================================
//
//	Cvar_Reset
//
//==========================================================================

void Cvar_Reset(const char* var_name)
{
	Cvar_Set2(var_name, NULL, false);
}

//==========================================================================
//
//	Cvar_Toggle_f
//
//	Toggles a cvar for easy single key binding
//
//==========================================================================

static void Cvar_Toggle_f()
{
	if (Cmd_Argc() != 2)
	{
		GLog.Write("usage: toggle <variable>\n");
		return;
	}

	int v = Cvar_VariableValue(Cmd_Argv(1));
	v = !v;

	Cvar_Set2(Cmd_Argv(1), va("%i", v), false);
}

//==========================================================================
//
//	Cvar_Set_f
//
//	Allows setting and defining of arbitrary cvars from console, even if they
// weren't declared in C code.
//
//==========================================================================

static void Cvar_Set_f()
{
	int c = Cmd_Argc();
	if (c < 3)
	{
		GLog.Write("usage: set <variable> <value>\n");
		return;
	}

	QStr combined;
	for (int i = 2; i < c ; i++)
	{
		combined += Cmd_Argv(i);
		if (i != c - 1)
		{
			combined += " ";
		}
	}
	Cvar_Set2(Cmd_Argv(1), *combined, false);
}

//==========================================================================
//
//	Cvar_SetU_f
//
//	As Cvar_Set, but also flags it as userinfo
//
//==========================================================================

static void Cvar_SetU_f()
{
	if (Cmd_Argc() != 3)
	{
		GLog.Write("usage: setu <variable> <value>\n");
		return;
	}
	Cvar_Set_f();
	QCvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_USERINFO;
}

//==========================================================================
//
//	Cvar_SetS_f
//
//	As Cvar_Set, but also flags it as userinfo
//
//==========================================================================

static void Cvar_SetS_f()
{
	if (Cmd_Argc() != 3)
	{
		GLog.Write("usage: sets <variable> <value>\n");
		return;
	}
	Cvar_Set_f();
	QCvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_SERVERINFO;
}

//==========================================================================
//
//	Cvar_SetA_f
//
//	As Cvar_Set, but also flags it as archived
//
//==========================================================================

static void Cvar_SetA_f()
{
	if (Cmd_Argc() != 3)
	{
		GLog.Write("usage: seta <variable> <value>\n");
		return;
	}
	Cvar_Set_f();
	QCvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_ARCHIVE;
}

//==========================================================================
//
//	Cvar_Reset_f
//
//==========================================================================

static void Cvar_Reset_f()
{
	if (Cmd_Argc() != 2)
	{
		GLog.Write("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset(Cmd_Argv(1));
}

//==========================================================================
//
//	Cvar_List_f
//
//==========================================================================

static void Cvar_List_f()
{
	const char*		match;

	if (Cmd_Argc() > 1)
	{
		match = Cmd_Argv(1);
	}
	else
	{
		match = NULL;
	}

	int i = 0;
	for (QCvar* var = cvar_vars; var; var = var->next, i++)
	{
		if (match && !QStr::Filter(match, var->name, false))
		{
			continue;
		}

		if (var->flags & CVAR_SERVERINFO)
		{
			GLog.Write("S");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_USERINFO)
		{
			GLog.Write("U");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_ROM)
		{
			GLog.Write("R");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_INIT)
		{
			GLog.Write("I");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_ARCHIVE)
		{
			GLog.Write("A");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_LATCH)
		{
			GLog.Write("L");
		}
		else
		{
			GLog.Write(" ");
		}
		if (var->flags & CVAR_CHEAT)
		{
			GLog.Write("C");
		}
		else
		{
			GLog.Write(" ");
		}

		GLog.Write(" %s \"%s\"\n", var->name, var->string);
	}

	GLog.Write("\n%i total cvars\n", i);
	GLog.Write("%i cvar indexes\n", cvar_numIndexes);
}

//==========================================================================
//
//	Cvar_Restart_f
//
//	Resets all cvars to their hardcoded values
//
//==========================================================================

static void Cvar_Restart_f()
{
	QCvar** prev = &cvar_vars;
	while (1)
	{
		QCvar* var = *prev;
		if (!var)
		{
			break;
		}

		// don't mess with rom values, or some inter-module
		// communication will get broken (com_cl_running, etc)
		if (var->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART))
		{
			prev = &var->next;
			continue;
		}

		// throw out any variables the user created
		if (var->flags & CVAR_USER_CREATED)
		{
			*prev = var->next;
			if (var->name)
			{
				Mem_Free(var->name);
			}
			if (var->string)
			{
				Mem_Free(var->string);
			}
			if (var->latchedString)
			{
				Mem_Free(var->latchedString);
			}
			if (var->resetString)
			{
				Mem_Free(var->resetString);
			}
			// clear the var completely, since we
			// can't remove the index from the list
			//Com_Memset( var, 0, sizeof( var ) );
			cvar_indexes[var->Handle] = NULL;
			delete var;
			continue;
		}

		Cvar_Set(var->name, var->resetString);

		prev = &var->next;
	}
}

//==========================================================================
//
//	Cvar_Init
//
//==========================================================================

void Cvar_Init()
{
	cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

	Cmd_AddCommand("toggle", Cvar_Toggle_f);
	Cmd_AddCommand("set", Cvar_Set_f);
	Cmd_AddCommand("sets", Cvar_SetS_f);
	Cmd_AddCommand("setu", Cvar_SetU_f);
	Cmd_AddCommand("seta", Cvar_SetA_f);
	Cmd_AddCommand("reset", Cvar_Reset_f);
	Cmd_AddCommand("cvarlist", Cvar_List_f);
	Cmd_AddCommand("cvar_restart", Cvar_Restart_f);
}

//==========================================================================
//
//	Cvar_WriteVariables
//
//	Appends lines containing "set variable value" for all variables with the
// archive flag set to true.
//
//==========================================================================

void Cvar_WriteVariables(fileHandle_t f)
{
	char	buffer[1024];

	for (QCvar* var = cvar_vars; var; var = var->next)
	{
		if (QStr::ICmp(var->name, "cl_cdkey") == 0)
		{
			continue;
		}
		if (var->flags & CVAR_ARCHIVE)
		{
			// write the latched value, even if it hasn't taken effect yet
			if (var->latchedString)
			{
				QStr::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
			}
			else
			{
				QStr::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
			}
			FS_Printf(f, "%s", buffer);
		}
	}
}
