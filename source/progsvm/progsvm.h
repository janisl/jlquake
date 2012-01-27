//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _PROGSVM_H
#define _PROGSVM_H

#include "../core/core.h"

#include "progs_file.h"
#include "edict.h"

union eval_t
{
	string_t string;
	float _float;
	float vector[3];
	func_t function;
	int _int;
	int edict;
};	

extern dprograms_t* progs;
extern dfunction_t* pr_functions;
extern char* pr_strings;
extern ddef_t* pr_globaldefs;
extern ddef_t* pr_fielddefs;
extern dstatement_t* pr_statements;
extern float* pr_globals;			// same as pr_global_struct

void PR_ClearStringMap();
int PR_SetString(const char* string);
const char* PR_GetString(int number);

ddef_t* ED_GlobalAtOfs(int offset);
ddef_t* ED_FieldAtOfs(int offset);
ddef_t* ED_FindField(const char* name);
ddef_t* ED_FindGlobal(const char* name);
dfunction_t* ED_FindFunction(const char* name);
dfunction_t* ED_FindFunctioni(const char* name);

#endif
