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

#include "progsvm.h"

dprograms_t* progs;
dfunction_t* pr_functions;
char* pr_strings;
ddef_t* pr_fielddefs;
ddef_t* pr_globaldefs;
dstatement_t* pr_statements;
float* pr_globals;			// same as pr_global_struct

static Array<const char*> pr_strtbl;

void PR_ClearStringMap()
{
	pr_strtbl.Clear();
}

int PR_SetString(const char* s)
{
	if (s < pr_strings || s >= pr_strings + progs->numstrings)
	{
		for (int i = 0; i < pr_strtbl.Num(); i++)
		{
			if (pr_strtbl[i] == s)
			{
				return -i - 1;
			}
		}
		pr_strtbl.Append(s);
		return -pr_strtbl.Num();
	}
	return (int)(s - pr_strings);
}

const char* PR_GetString(int number)
{
	if (number < 0)
	{
		return pr_strtbl[-number - 1];
	}
	return pr_strings + number;
}

ddef_t* ED_GlobalAtOfs(int offset)
{
	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		ddef_t* def = &pr_globaldefs[i];
		if (def->ofs == offset)
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FieldAtOfs(int offset)
{
	for (int i = 0; i < progs->numfielddefs; i++)
	{
		ddef_t* def = &pr_fielddefs[i];
		if (def->ofs == offset)
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FindField(const char* name)
{
	for (int i = 0; i < progs->numfielddefs; i++)
	{
		ddef_t* def = &pr_fielddefs[i];
		if (!String::Cmp(PR_GetString(def->s_name), name))
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FindGlobal(const char* name)
{
	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		ddef_t* def = &pr_globaldefs[i];
		if (!String::Cmp(PR_GetString(def->s_name), name))
		{
			return def;
		}
	}
	return NULL;
}

dfunction_t* ED_FindFunction(const char* name)
{
	for (int i = 0; i < progs->numfunctions; i++)
	{
		dfunction_t* func = &pr_functions[i];
		if (!String::Cmp(PR_GetString(func->s_name), name))
		{
			return func;
		}
	}
	return NULL;
}

dfunction_t* ED_FindFunctioni(const char* name)
{
	for (int i = 0; i < progs->numfunctions; i++)
	{
		dfunction_t* func = &pr_functions[i];
		if (!String::ICmp(PR_GetString(func->s_name), name))
		{
			return func;
		}
	}
	return NULL;
}
