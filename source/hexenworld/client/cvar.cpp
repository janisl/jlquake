// cvar.c -- dynamic variable tracking

#ifdef SERVERONLY 
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif

char	*cvar_null_string = "";

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set2( const char *var_name, const char *value, bool force )
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return var;
	}

#ifdef SERVERONLY
	if (var->flags & CVAR_SERVERINFO)
	{
		Info_SetValueForKey (svs.info, const_cast<char*>(var_name), const_cast<char*>(value), MAX_SERVERINFO_STRING);
		SV_BroadcastCommand ("fullserverinfo \"%s\"\n", svs.info);
	}
#else
	if (var->flags & CVAR_USERINFO)
	{
		Info_SetValueForKey (cls.userinfo, const_cast<char*>(var_name), const_cast<char*>(value), MAX_INFO_STRING);
		if (cls.state >= ca_connected)
		{
			cls.netchan.message.WriteByte(clc_stringcmd);
			cls.netchan.message.WriteString2(va("setinfo \"%s\" \"%s\"\n", var_name, value));
		}
	}
#endif
	
	Mem_Free (var->string);	// free the old value string
	
	var->string = (char*)Mem_Alloc (QStr::Length(value)+1);
	QStr::Cpy(var->string, value);
	var->value = QStr::Atof(var->string);
	var->integer = QStr::Atoi(var->string);
    return var;
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	char	value[512];

// first check to see if it has allready been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, allready defined\n", variable->name);
		return;
	}
	
// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
		
// link the variable in
	variable->next = cvar_vars;
	cvar_vars = variable;

	if (variable->archive)
		variable->flags |= CVAR_ARCHIVE;
#ifdef SERVERONLY
	if (variable->info)
		variable->flags |= CVAR_SERVERINFO;
#else
	if (variable->info)
		variable->flags |= CVAR_USERINFO;
#endif

// copy the value off, because future sets will Z_Free it
	QStr::Cpy(value, variable->string);
	variable->string = (char*)Mem_Alloc (1);	

	long hash = Cvar_GenerateHashValue(variable->name);
	variable->hashNext = cvar_hashTable[hash];
	cvar_hashTable[hash] = variable;

// set it through the function to be consistant
	Cvar_Set (variable->name, value);
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = var->next)
		if (var->flags & CVAR_ARCHIVE)
			fprintf (f, "%s \"%s\"\n", var->name, var->string);
}

