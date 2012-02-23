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
//**
//**	Cvar variables are used to hold scalar or string variables that can
//**  be changed or displayed at the console or prog code as well as accessed
//**  directly in C code.
//**
//**	The user can access cvars from the console in three ways:
//**	r_draworder			prints the current value
//**	r_draworder 0		sets the current value to 0
//**	set r_draworder 0	as above, but creates the cvar if not present
//**
//**	Cvars are restricted from having the same names as commands to keep
//**  this interface from being ambiguous.
//**
//**	The are also occasionally used to communicated information between
//**  different modules of the program.
//**
//**	Many variables can be used for cheating purposes, so when cheats is
//**  zero, force all unspecified variables to their default values.
//**
//**************************************************************************

#define CVAR_ARCHIVE		BIT(0)	// set to cause it to be saved to vars.rc
									// used for system variables, not for player
									// specific configurations
#define CVAR_USERINFO		BIT(1)	// sent to server on connect or change
#define CVAR_SERVERINFO		BIT(2)	// sent in response to front end requests
#define CVAR_INIT			BIT(3)	// don't allow change from console at all,
									// but can be set from the command line
#define CVAR_LATCH			BIT(4)	// save changes until server restart
#define CVAR_SYSTEMINFO		BIT(5)	// these cvars will be duplicated on all clients
#define CVAR_ROM			BIT(6)	// display only, cannot be set by user at all
#define CVAR_USER_CREATED	BIT(7)	// created by a set command
#define CVAR_TEMP			BIT(8)	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			BIT(9)	// can not be changed if cheats are disabled
#define CVAR_NORESTART		BIT(10)	// do not clear when a cvar_restart is issued
#define CVAR_WOLFINFO		BIT(11)	// DHM - NERVE :: Like userinfo, but for wolf multiplayer info
#define CVAR_UNSAFE			BIT(12)	// ydnar: unsafe system cvars (renderer,
									// sound settings, anything that might cause a crash)
#define CVAR_SERVERINFO_NOUPDATE	BIT(13)	// gordon: WONT automatically send this
									// to clients, but server browsers will see it
#define CVAR_LATCH2			BIT(14)	// will only change when C code next does
									// a Cvar_Get(), so it can't be changed
									// without proper initialization.  modified
									// will be set, even though the value hasn't
									// changed yet

// nothing outside the Cvar_*() functions should modify these fields!
struct Cvar
{
	//	This class is accessible to Quake 2 game, so the following fields
	// must remain exactly as they are.
	char		*name;
	char		*string;
	char		*latchedString;		// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;			// set each time the cvar is changed
	float		value;				// atof( string )
	Cvar*		next;

	char		*resetString;		// cvar_restart will reset to this value
	int			modificationCount;	// incremented each time the cvar is changed
	int			integer;			// atoi( string )
	Cvar*		hashNext;
	int			Handle;
};

#define MAX_CVAR_VALUE_STRING	256

typedef int	cvarHandle_t;

// the modules that run in the virtual machine can't access the Cvar directly,
// so they must ask for structured updates
struct vmCvar_t
{
	cvarHandle_t	handle;
	int			modificationCount;
	float		value;
	int			integer;
	char		string[MAX_CVAR_VALUE_STRING];
};

Cvar* Cvar_Get(const char* VarName, const char* Value, int Flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

Cvar* Cvar_FindVar(const char* VarName);

float Cvar_VariableValue(const char* VarName);
int	Cvar_VariableIntegerValue(const char* VarName);
// returns 0 if not defined or non numeric

const char* Cvar_VariableString(const char* VarName);
void Cvar_VariableStringBuffer(const char* VarName, char* Buffer, int BufSize);
// returns an empty string if not defined

void Cvar_LatchedVariableStringBuffer(const char* var_name, char* buffer, int bufsize);
// Gordon: returns the latched value if there is one, else the normal one, empty string if not defined as usual

Cvar* Cvar_Set(const char* VarName, const char* Value);
// will create the variable with no flags if it doesn't exist

Cvar* Cvar_SetLatched(const char* VarName, const char* Value);
// don't set the cvar immediately

void Cvar_SetValue(const char* VarName, float Value);
void Cvar_SetValueLatched(const char* VarName, float Value);
// expands value to a string and calls Cvar_Set/Cvar_SetLatched

const char* Cvar_CompleteVariable(const char* Partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

bool Cvar_Command();
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

char* Cvar_InfoString(int Bit, int MaxSize,
	int MaxKeySize = BIG_INFO_KEY, int MaxValSize = BIG_INFO_VALUE,
	bool NoHighChars = false, bool LowerCaseVal = false);
// returns an info string containing all the cvars that have the given bit set
// in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc )
void Cvar_InfoStringBuffer(int Bit, int MaxSize, char* Buff, int BuffSize);

void Cvar_Register(vmCvar_t* VmCvar, const char* VarName, const char* DefaultValue, int Flags);
// basically a slightly modified Cvar_Get for the interpreted modules

void Cvar_Update(vmCvar_t* VmCvar);
// updates an interpreted modules' version of a cvar

void Cvar_CommandCompletion(void(*Callback)(const char* S));
// callback with each valid string

void Cvar_SetCheatState();
// reset all testing vars to a safe value

void Cvar_Reset(const char* VarName);

void Cvar_Init();

#if 0
void Cvar_WriteVariables(fileHandle_t F);
// writes lines containing "set variable value" for all variables
// with the archive flag set to true.

void Cvar_UpdateIfExists(const char* name, const char* value);
// For QuakeWorld's and HexenWorld's server info changes.
#endif

extern Cvar*		cvar_vars;

extern int			cvar_modifiedFlags;
// whenever a cvar is modifed, its flags will be OR'd into this, so
// a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
// etc, variables have been modified since the last check.  The bit
// can then be cleared to allow another change detection.
