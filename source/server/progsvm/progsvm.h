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

#ifndef _PROGSVM_H
#define _PROGSVM_H

#include "../server.h"

#include "progs_file.h"

#define MAX_STACK_DEPTH 32
#define LOCALSTACK_SIZE 2048

union eval_t
{
	string_t string;
	float _float;
	float vector[3];
	func_t function;
	int _int;
	int edict;
};

struct prstack_t
{
	int s;
	dfunction_t* f;
};

extern dprograms_t* progs;
extern dfunction_t* pr_functions;
extern char* pr_strings;
extern ddef_t* pr_globaldefs;
extern ddef_t* pr_fielddefs;
extern dstatement_t* pr_statements;
extern float* pr_globals;			// same as pr_global_struct

extern prstack_t pr_stack[MAX_STACK_DEPTH];
extern int pr_depth;
extern int localstack[LOCALSTACK_SIZE];
extern int localstack_used;
extern bool pr_trace;
extern dfunction_t* pr_xfunction;
extern int pr_xstatement;
extern int pr_argc;
extern const char* pr_opnames[];

void PR_ClearStringMap();
int PR_SetString(const char* string);
const char* PR_GetString(int number);

ddef_t* ED_GlobalAtOfs(int offset);
ddef_t* ED_FieldAtOfs(int offset);
ddef_t* ED_FindField(const char* name);
ddef_t* ED_FindGlobal(const char* name);
dfunction_t* ED_FindFunction(const char* name);
dfunction_t* ED_FindFunctioni(const char* name);

void PR_PrintStatement(dstatement_t* s);

#endif
