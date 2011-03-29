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

#include "progsvm.h"

// MACROS ------------------------------------------------------------------

#define MAX_PRSTR			1024

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

static const char*	pr_strtbl[MAX_PRSTR];
static int			num_prstr;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	PR_ClearStringMap
//
//==========================================================================

void PR_ClearStringMap()
{
	num_prstr = 0;
}

//==========================================================================
//
//	PR_SetString
//
//==========================================================================

int PR_SetString(const char* s)
{
	if (s - pr_strings < 0)
	{
		for (int i = 0; i <= num_prstr; i++)
		{
			if (pr_strtbl[i] == s)
			{
				return -i;
			}
		}
		if (num_prstr == MAX_PRSTR - 1)
		{
			throw QException("MAX_PRSTR");
		}
		num_prstr++;
		pr_strtbl[num_prstr] = s;
//Con_DPrintf("SET:%d == %s\n", -num_prstr, s);
		return -num_prstr;
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
//Con_DPrintf("GET:%d == %s\n", num, pr_strtbl[-num]);
		return pr_strtbl[-Num];
	}
	return pr_strings + Num;
}
