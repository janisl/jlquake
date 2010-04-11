// cvar.c -- dynamic variable tracking

#include "quakedef.h"
#ifdef _WIN32
#include "winquake.h"
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
	qboolean changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return var;
	}

	changed = QStr::Cmp(var->string, value);
	
	Mem_Free (var->string);	// free the old value string
	
	var->string = (char*)Mem_Alloc (QStr::Length(value)+1);
	QStr::Cpy(var->string, value);
	var->value = QStr::Atof(var->string);
	var->integer = QStr::Atoi(var->string);
	if ((var->flags & CVAR_SERVERINFO) && changed)
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
	}
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

