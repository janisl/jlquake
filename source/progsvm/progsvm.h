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

#ifndef _PROGSVM_H
#define _PROGSVM_H

#include "../core/core.h"

#include "progs_file.h"

extern dprograms_t*		progs;
extern dfunction_t*		pr_functions;
extern char*			pr_strings;
extern ddef_t*			pr_globaldefs;
extern ddef_t*			pr_fielddefs;
extern dstatement_t*	pr_statements;
extern float*			pr_globals;			// same as pr_global_struct

void PR_ClearStringMap();
int PR_SetString(const char* String);
const char* PR_GetString(int Num);

#endif
