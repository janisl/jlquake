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
// cvar.c -- dynamic variable tracking

#include "core.h"

#define	MAX_CVARS			2048

#define FILE_HASH_SIZE		512

#define FOREIGN_MSG "Foreign characters are not allowed in userinfo variables.\n"

void Cvar_Changed(Cvar* var);
const char* Cvar_TranslateString(const char* string);

Cvar*		cvar_vars;
int			cvar_modifiedFlags;

static Cvar*		cvar_cheats;

static Cvar*		cvar_indexes[MAX_CVARS];
static int			cvar_numIndexes;

static Cvar*		cvar_hashTable[FILE_HASH_SIZE];

//	Return a hash value for the filename
static long Cvar_GenerateHashValue(const char *fname)
{
	if (!fname)
	{
		common->Error("null name in Cvar_GenerateHashValue");
	}
	long hash = 0;
	for (int i = 0; fname[i] != '\0'; i++)
	{
		char letter = String::ToLower(fname[i]);
		hash += (long)(letter) * (i + 119);
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

char* __CopyString(const char* in)
{
	char* out = (char*)Mem_Alloc(String::Length(in) + 1);
	String::Cpy(out, in);
	return out;
}

Cvar* Cvar_FindVar(const char* VarName)
{
	long hash = Cvar_GenerateHashValue(VarName);

	for (Cvar* var = cvar_hashTable[hash]; var; var = var->hashNext)
	{
		if (!String::ICmp(VarName, var->name))
		{
			return var;
		}
	}

	return NULL;
}

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

//	Some cvar values need to be safe from foreign characters
static char* Cvar_ClearForeignCharacters(const char* value)
{
	static char clean[MAX_CVAR_VALUE_STRING];
	int j = 0;
	for (int i = 0; value[i] != '\0'; i++)
	{
		if ((!(GGameType & GAME_ET) && !(value[i] & 128)) ||
			((GGameType & GAME_ET) && ((byte*)value)[i] != 0xFF && (((byte*)value)[i] <= 127 || ((byte*)value)[i] >= 161)))
		{
			clean[j] = value[i];
			j++;
		}
	}
	clean[j] = '\0';

	return clean;
}

static Cvar* Cvar_Set2(const char *var_name, const char *value, bool force)
{
	Log::develWrite("Cvar_Set2: %s %s\n", var_name, value);

	if (!Cvar_ValidateString(var_name))
	{
		Log::write("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

	Cvar* var = Cvar_FindVar(var_name);
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

	if (!(GGameType & GAME_Tech3) && var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_ValidateString(value))
		{
			Log::write("invalid info cvar value\n");
			return var;
		}
	}

	if (!value)
	{
		value = var->resetString;
	}

	if ((GGameType & (GAME_WolfMP | GAME_ET)) && (var->flags & CVAR_USERINFO))
	{
		char* cleaned = Cvar_ClearForeignCharacters(value);
		if (String::Cmp(value, cleaned))
		{
			common->Printf("%s", Cvar_TranslateString(FOREIGN_MSG));
			common->Printf("Using %s instead of %s\n", cleaned, value);
			return Cvar_Set2(var_name, cleaned, force);
		}
	}

	if (!String::Cmp(value, var->string) && !var->latchedString)
	{
		return var;
	}
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		// ydnar: don't set unsafe variables when com_crashed is set
		if ((var->flags & CVAR_UNSAFE) && com_crashed && com_crashed->integer)
		{
			common->Printf("%s is unsafe. Check com_crashed.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_ROM)
		{
			Log::write("%s is read only.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			Log::write("%s is write protected.\n", var_name);
			return var;
		}

		if ((var->flags & CVAR_CHEAT) && !cvar_cheats->integer)
		{
			Log::write("%s is cheat protected.\n", var_name);
			return var;
		}

		if (var->flags & (CVAR_LATCH | CVAR_LATCH2))
		{
			if (var->latchedString)
			{
				if (String::Cmp(value, var->latchedString) == 0)
				{
					return var;
				}
				Mem_Free(var->latchedString);
			}
			else
			{
				if (String::Cmp(value, var->string) == 0)
				{
					return var;
				}
			}

			Log::write("%s will be changed upon restarting.\n", var_name);
			var->latchedString = __CopyString(value);
			var->modified = true;
			var->modificationCount++;
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

	if (!String::Cmp(value, var->string))
	{
		return var;		// not changed
	}

	var->modified = true;
	var->modificationCount++;

	Mem_Free(var->string);	// free the old value string

	var->string = __CopyString(value);
	var->value = String::Atof(var->string);
	var->integer = String::Atoi(var->string);

	Cvar_Changed(var);
	return var;
}

Cvar* Cvar_Set(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, true);
}

Cvar* Cvar_SetLatched(const char *var_name, const char *value)
{
	return Cvar_Set2(var_name, value, false);
}

void Cvar_SetValue(const char* var_name, float value)
{
	char	val[32];

	if (value == (int)value)
	{
		String::Sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		String::Sprintf(val, sizeof(val), "%f", value);
	}
	Cvar_Set(var_name, val);
}

void Cvar_SetValueLatched(const char* var_name, float value)
{
	char	val[32];

	if (value == (int)value)
	{
		String::Sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		String::Sprintf(val, sizeof(val), "%f", value);
	}
	Cvar_SetLatched(var_name, val);
}

//	If the variable already exists, the value will not be set.
// The flags will be or'ed in if the variable exists.
Cvar* Cvar_Get(const char* VarName, const char* VarValue, int Flags)
{
	if (!VarName || (!(GGameType & GAME_Quake2) && !VarValue))
	{
		throw Exception("Cvar_Get: NULL parameter");
	}

	if (!(GGameType & GAME_Quake2) || (Flags & (CVAR_USERINFO | CVAR_SERVERINFO)))
	{
		if (!Cvar_ValidateString(VarName))
		{
			if (GGameType & GAME_Quake2)
			{
				Log::write("invalid info cvar name\n");
				return NULL;
			}
			Log::write("invalid cvar name string: %s\n", VarName);
			VarName = "BADNAME";
		}
	}

	Cvar* var = Cvar_FindVar(VarName);
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
		else if (VarValue[0] && String::Cmp(var->resetString, VarValue))
		{
			Log::develWrite("Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
				VarName, var->resetString, VarValue);
		}
		// if we have a latched string, take that value now
		//	This is done only for Quake 3 type latched vars.
		if ((var->flags & CVAR_LATCH2) && var->latchedString)
		{
			char* s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2(VarName, s, true);
			Mem_Free(s);
		}

		// TTimo
		// if CVAR_USERINFO was toggled on for an existing cvar, check wether the value needs to be cleaned from foreigh characters
		// (for instance, seta name "name-with-foreign-chars" in the config file, and toggle to CVAR_USERINFO happens later in CL_Init)
		if ((GGameType & (GAME_WolfMP | GAME_ET)) && (Flags & CVAR_USERINFO))
		{
			char *cleaned = Cvar_ClearForeignCharacters(var->string); // NOTE: it is probably harmless to call Cvar_Set2 in all cases, but I don't want to risk it
			if (String::Cmp(var->string, cleaned))
			{
				Cvar_Set2(var->name, var->string, false); // call Cvar_Set2 with the value to be cleaned up for verbosity
			}
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
	if (!(GGameType & GAME_Tech3) && (Flags & (CVAR_USERINFO | CVAR_SERVERINFO)))
	{
		if (!Cvar_ValidateString(VarValue))
		{
			Log::write("invalid info cvar value\n");
			return NULL;
		}
	}

	//
	// allocate a new cvar
	//
	if (cvar_numIndexes >= MAX_CVARS)
	{
		throw Exception("MAX_CVARS" );
	}
	var = new Cvar;
	Com_Memset(var, 0, sizeof(*var));
	cvar_indexes[cvar_numIndexes] = var;
	var->Handle = cvar_numIndexes;
	cvar_numIndexes++;
	var->name = __CopyString(VarName);
	var->string = __CopyString(VarValue);
	var->modified = true;
	var->modificationCount = 1;
	var->value = String::Atof(var->string);
	var->integer = String::Atoi(var->string);
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

float Cvar_VariableValue(const char* var_name)
{
	Cvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->value;
}

int Cvar_VariableIntegerValue(const char* var_name)
{
	Cvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->integer;
}

const char* Cvar_VariableString(const char* var_name)
{
	Cvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		return "";
	}
	return var->string;
}

void Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize)
{
	Cvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		*buffer = 0;
	}
	else
	{
		String::NCpyZ(buffer, var->string, bufsize);
	}
}

void Cvar_LatchedVariableStringBuffer(const char* var_name, char* buffer, int bufsize)
{
	Cvar* var = Cvar_FindVar(var_name);
	if (!var)
	{
		*buffer = 0;
	}
	else
	{
		if (var->latchedString)
		{
			String::NCpyZ(buffer, var->latchedString, bufsize);
		}
		else
		{
			String::NCpyZ(buffer, var->string, bufsize);
		}
	}
}

//	Handles variable inspection and changing from the console
bool Cvar_Command()
{
	// check variables
	Cvar* v = Cvar_FindVar(Cmd_Argv(0));
	if (!v)
	{
		return false;
	}

	// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Log::write("\"%s\" is:\"%s" S_COLOR_WHITE "\" default:\"%s" S_COLOR_WHITE "\"\n",
			v->name, v->string, v->resetString);
		if (v->latchedString)
		{
			Log::write("latched: \"%s\"\n", v->latchedString);
		}
		return true;
	}

	// set the value if forcing isn't required
	Cvar_Set2(v->name, Cmd_Argv(1), false);
	return true;
}

//	Handles large info strings ( Q3CS_SYSTEMINFO )
char* Cvar_InfoString(int bit, int MaxSize, int MaxKeySize, int MaxValSize,
	bool NoHighChars, bool LowerCaseVal)
{
	static char	info[BIG_INFO_STRING];

	info[0] = 0;

	for (Cvar* var = cvar_vars; var; var = var->next)
	{
		if (var->flags & bit)
		{
			Info_SetValueForKey(info, var->name, var->string, MaxSize,
				MaxKeySize, MaxValSize, NoHighChars, LowerCaseVal);
		}
	}
	return info;
}

static int ConvertTech3GameFlags(int flags)
{
	//	For Quake 2 compatibility some flags have been moved around,
	// so map them to new values. Also clear unknown flags.
	int TmpFlags = flags;
	flags &= 0x3fc7;
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
		flags |= CVAR_LATCH2;
	}
	return flags;
}

void Cvar_InfoStringBuffer(int bit, int MaxSize, char* buff, int buffsize)
{
	String::NCpyZ(buff, Cvar_InfoString(ConvertTech3GameFlags(bit), MaxSize), buffsize);
}

//	basically a slightly modified Cvar_Get for the interpreted modules
void Cvar_Register(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags)
{
	flags = ConvertTech3GameFlags(flags);

	Cvar* cv = Cvar_Get(varName, defaultValue, flags);
	if (!vmCvar)
	{
		return;
	}
	vmCvar->handle = cv->Handle;
	vmCvar->modificationCount = -1;
	Cvar_Update(vmCvar);
}

//	updates an interpreted modules' version of a cvar
void Cvar_Update(vmCvar_t* vmCvar)
{
	qassert(vmCvar); // bk

	if ((unsigned)vmCvar->handle >= (unsigned)cvar_numIndexes)
	{
		throw DropException("Cvar_Update: handle out of range" );
	}

	Cvar* cv = cvar_indexes[vmCvar->handle];
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
	if (String::Length(cv->string) + 1 > MAX_CVAR_VALUE_STRING)
	{
		throw DropException(va("Cvar_Update: src %s length %d exceeds MAX_CVAR_VALUE_STRING",
			cv->string, String::Length(cv->string)));
	}
	// bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0 
	// bk001129 - paranoia. Never trust the destination string.
	// bk001129 - beware, sizeof(char*) is always 4 (for cv->string). 
	//            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
	//Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
	String::NCpyZ(vmCvar->string, cv->string,  MAX_CVAR_VALUE_STRING); 

	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}

const char* Cvar_CompleteVariable(const char* partial)
{
	int len = String::Length(partial);

	if (!len)
	{
		return NULL;
	}

	// check exact match
	for (Cvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if (!String::Cmp(partial, cvar->name))
		{
			return cvar->name;
		}
	}

	// check partial match
	for (Cvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if (!String::NCmp(partial,cvar->name, len))
		{
			return cvar->name;
		}
	}

	return NULL;
}

void Cvar_CommandCompletion(void(*callback)(const char* s))
{
	for (Cvar* cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		callback(cvar->name);
	}
}

//	Any testing variables will be reset to the safe values
void Cvar_SetCheatState()
{
	// set all default vars to the safe value
	for (Cvar* var = cvar_vars; var; var = var->next)
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
			if (String::Cmp(var->resetString, var->string))
			{
				Cvar_Set(var->name, var->resetString);
			}
		}
	}
}

void Cvar_Reset(const char* var_name)
{
	Cvar_Set2(var_name, NULL, false);
}

//	Toggles a cvar for easy single key binding
static void Cvar_Toggle_f()
{
	if (Cmd_Argc() != 2)
	{
		Log::write("usage: toggle <variable>\n");
		return;
	}

	int v = Cvar_VariableValue(Cmd_Argv(1));
	v = !v;

	Cvar_Set2(Cmd_Argv(1), va("%i", v), false);
}

//	Cycles a cvar for easy single key binding
static void Cvar_Cycle_f()
{
	if (Cmd_Argc() < 4 || Cmd_Argc() > 5)
	{
		common->Printf("usage: cycle <variable> <start> <end> [step]\n");
		return;
	}

	int value = Cvar_VariableIntegerValue(Cmd_Argv(1));
	int oldvalue = value;
	int start = String::Atoi(Cmd_Argv(2));
	int end = String::Atoi(Cmd_Argv(3));

	int step;
	if (Cmd_Argc() == 5)
	{
		step = abs(String::Atoi(Cmd_Argv(4)));
	}
	else
	{
		step = 1;
	}

	if (abs(end - start) < step)
	{
		step = 1;
	}

	if (end < start)
	{
		value -= step;
		if (value < end)
		{
			value = start - (step - (oldvalue - end + 1));
		}
	}
	else
	{
		value += step;
		if (value > end)
		{
			value = start + (step - (end - oldvalue + 1));
		}
	}

	Cvar_Set2(Cmd_Argv(1), va("%i", value), false);
}

//	Allows setting and defining of arbitrary cvars from console, even if they
// weren't declared in C code.
static void Cvar_Set_f()
{
	int c = Cmd_Argc();
	if (c < 3)
	{
		if (GGameType & GAME_ET)
		{
			common->Printf("usage: set <variable> <value> [unsafe]\n");
		}
		else
		{
			common->Printf("usage: set <variable> <value>\n");
		}
		return;
	}

	// ydnar: handle unsafe vars
	if ((GGameType & GAME_ET) && c >= 4 && !String::Cmp(Cmd_Argv(c - 1), "unsafe"))
	{
		c--;
		if (com_crashed != NULL && com_crashed->integer)
		{
			common->Printf("%s is unsafe. Check com_crashed.\n", Cmd_Argv(1));
			return;
		}
	}

	String combined;
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

//	As Cvar_Set, but also flags it as userinfo
static void Cvar_SetU_f()
{
	if (Cmd_Argc() < 3)
	{
		if (GGameType & GAME_ET)
		{
			common->Printf("usage: setu <variable> <value> [unsafe]\n" );
		}
		else
		{
			common->Printf("usage: setu <variable> <value>\n");
		}
		return;
	}
	Cvar_Set_f();
	Cvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_USERINFO;
}

//	As Cvar_Set, but also flags it as serverinfo
static void Cvar_SetS_f()
{
	if (Cmd_Argc() < 3)
	{
		if (GGameType & GAME_ET)
		{
			common->Printf("usage: sets <variable> <value> [unsafe]\n");
		}
		else
		{
			common->Printf("usage: sets <variable> <value>\n");
		}
		return;
	}
	Cvar_Set_f();
	Cvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_SERVERINFO;
}

//	As Cvar_Set, but also flags it as archived
static void Cvar_SetA_f()
{
	if (Cmd_Argc() < 3)
	{
		if (GGameType & GAME_ET)
		{
			common->Printf("usage: seta <variable> <value> [unsafe]\n");
		}
		else
		{
			common->Printf("usage: seta <variable> <value>\n");
		}
		return;
	}
	Cvar_Set_f();
	Cvar* v = Cvar_FindVar(Cmd_Argv(1));
	if (!v)
	{
		return;
	}
	v->flags |= CVAR_ARCHIVE;
}

static void Cvar_Reset_f()
{
	if (Cmd_Argc() != 2)
	{
		Log::write("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset(Cmd_Argv(1));
}

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
	for (Cvar* var = cvar_vars; var; var = var->next, i++)
	{
		if (match && !String::Filter(match, var->name, false))
		{
			continue;
		}

		if (var->flags & CVAR_SERVERINFO)
		{
			Log::write("S");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & CVAR_USERINFO)
		{
			Log::write("U");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & CVAR_ROM)
		{
			Log::write("R");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & CVAR_INIT)
		{
			Log::write("I");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & CVAR_ARCHIVE)
		{
			Log::write("A");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & (CVAR_LATCH | CVAR_LATCH2))
		{
			Log::write("L");
		}
		else
		{
			Log::write(" ");
		}
		if (var->flags & CVAR_CHEAT)
		{
			Log::write("C");
		}
		else
		{
			Log::write(" ");
		}

		Log::write(" %s \"%s\"\n", var->name, var->string);
	}

	Log::write("\n%i total cvars\n", i);
	Log::write("%i cvar indexes\n", cvar_numIndexes);
}

//	Resets all cvars to their hardcoded values
static void Cvar_Restart_f()
{
	Cvar** prev = &cvar_vars;
	while (1)
	{
		Cvar* var = *prev;
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

void Cvar_Init()
{
	if (GGameType & GAME_WolfSP)
	{
		cvar_cheats = Cvar_Get("sv_cheats", "0", CVAR_ROM | CVAR_SYSTEMINFO);
	}
	else
	{
		cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);
	}

	Cmd_AddCommand("toggle", Cvar_Toggle_f);
	Cmd_AddCommand("cycle", Cvar_Cycle_f);  // ydnar
	Cmd_AddCommand("set", Cvar_Set_f);
	Cmd_AddCommand("sets", Cvar_SetS_f);
	Cmd_AddCommand("setu", Cvar_SetU_f);
	Cmd_AddCommand("seta", Cvar_SetA_f);
	Cmd_AddCommand("reset", Cvar_Reset_f);
	Cmd_AddCommand("cvarlist", Cvar_List_f);
	Cmd_AddCommand("cvar_restart", Cvar_Restart_f);

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// NERVE - SMF - can't rely on autoexec to do this
		Cvar_Get("devdll", "1", CVAR_ROM);
	}
}

//	Appends lines containing "set variable value" for all variables with the
// archive flag set to true.
void Cvar_WriteVariables(fileHandle_t f)
{
	char	buffer[1024];

	for (Cvar* var = cvar_vars; var; var = var->next)
	{
		if (String::ICmp(var->name, "cl_cdkey") == 0)
		{
			continue;
		}
		if (var->flags & CVAR_ARCHIVE)
		{
			// write the latched value, even if it hasn't taken effect yet
			if (var->latchedString)
			{
				if (GGameType & GAME_ET && var->flags & CVAR_UNSAFE)
				{
					String::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\" unsafe\n", var->name, var->latchedString);
				}
				else
				{
					String::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
				}
			}
			else
			{
				if (GGameType & GAME_ET && var->flags & CVAR_UNSAFE)
				{
					String::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\" unsafe\n", var->name, var->string);
				}
				else
				{
					String::Sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
				}
			}
			FS_Printf(f, "%s", buffer);
		}
	}
}

void Cvar_UpdateIfExists(const char* name, const char* value)
{
	// if this is a cvar, change it too	
	Cvar* var = Cvar_FindVar(name);
	if (var)
	{
		var->modified = true;
		var->modificationCount++;

		Mem_Free(var->string);	// free the old value string

		var->string = __CopyString(value);
		var->value = String::Atof(var->string);
		var->integer = String::Atoi(var->string);
	}
}
