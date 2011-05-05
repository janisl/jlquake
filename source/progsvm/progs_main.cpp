//**************************************************************************
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

#include "progsvm.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

dprograms_t		*progs;
dfunction_t		*pr_functions;
char			*pr_strings;
ddef_t			*pr_fielddefs;
ddef_t			*pr_globaldefs;
dstatement_t	*pr_statements;
float			*pr_globals;			// same as pr_global_struct

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static QArray<const char*>	pr_strtbl;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	PR_ClearStringMap
//
//==========================================================================

void PR_ClearStringMap()
{
	pr_strtbl.Clear();
}

//==========================================================================
//
//	PR_SetString
//
//==========================================================================

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

//==========================================================================
//
//	PR_SetString
//
//==========================================================================

const char* PR_GetString(int Num)
{
	if (Num < 0)
	{
		return pr_strtbl[-Num - 1];
	}
	return pr_strings + Num;
}
