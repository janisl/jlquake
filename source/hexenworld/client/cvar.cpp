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

