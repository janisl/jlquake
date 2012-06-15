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

#define G_FLOAT(o) (pr_globals[o])
#define G_INT(o) (*(int*)&pr_globals[o])
#define G_EDICT(o) ((qhedict_t*)((byte*)sv.qh_edicts + *(int*)&pr_globals[o]))
#define G_EDICTNUM(o) QH_NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&pr_globals[o])
#define G_STRING(o) (PR_GetString(*(string_t*)&pr_globals[o]))
#define G_FUNCTION(o) (*(func_t*)&pr_globals[o])

#define E_FLOAT(e,o) (((float*)&e->v)[o])
#define E_INT(e,o) (*(int*)&((float*)&e->v)[o])
#define E_VECTOR(e,o) (&((float*)&e->v)[o])
#define E_STRING(e,o) (PR_GetString(*(string_t*)&((float*)&e->v)[o]))

struct prstack_t
{
	int s;
	dfunction_t* f;
};

struct progGlobalVars_t
{
	//	All games
	int* self;
	int* other;
	float* time;
	float* frametime;
	//	QuakeWorld, HexenWorld
	int* newmis;
	//	All games
	float* force_retouch;
	string_t* mapname;
	//	Hexen 2, HexenWorld
	string_t* startspot;
	//	Quake, Hexen 2, HexenWorld
	float* deathmatch;
	//	Hexen 2, HexenWorld
	float* randomclass;
	//	HexenWorld
	float* damageScale;
	float* meleeDamScale;
	float* shyRespawn;
	float* spartanPrint;
	float* manaScale;
	float* tomeMode;
	float* tomeRespawn;
	float* w2Respawn;
	float* altRespawn;
	float* fixedLevel;
	float* autoItems;
	float* dmMode;
	float* easyFourth;
	float* patternRunner;
	//	Quake, Hexen 2, HexenWorld
	float* coop;
	//	Hexen 2 portals
	float* cl_playerclass;
	//	All games
	float* serverflags;
	const float* total_secrets;
	const float* total_monsters;
	const float* found_secrets;
	const float* killed_monsters;
	float* parm1;
	float* v_forward;
	float* v_up;
	float* v_right;
	float* trace_allsolid;
	float* trace_startsolid;
	float* trace_fraction;
	float* trace_endpos;
	float* trace_plane_normal;
	float* trace_plane_dist;
	int* trace_ent;
	float* trace_inopen;
	float* trace_inwater;
	const int* msg_entity;
	//	Hexen 2, HexenWorld
	float* cycle_wrapped;
	//	HexenWorld
	float* max_players;
	const float* defLosses;
	const float* attLosses;
	//	All games
	const func_t* main;
	const func_t* StartFrame;
	const func_t* PlayerPreThink;
	const func_t* PlayerPostThink;
	const func_t* ClientKill;
	const func_t* ClientConnect;
	const func_t* PutClientInServer;
	//	Hexen 2, HexenWorld
	const func_t* ClientReEnter;
	//	All games
	const func_t* ClientDisconnect;
	//	Hexen 2, HexenWorld
	const func_t* ClassChangeWeapon;
	//	Quake, QuakeWorld, HexenWorld
	const func_t* SetNewParms;
	const func_t* SetChangeParms;
	//	HexenWorld
	const func_t* SmitePlayer;
};

typedef void (*builtin_t)();

extern dprograms_t* progs;
extern dfunction_t* pr_functions;
extern char* pr_strings;
extern ddef_t* pr_globaldefs;
extern ddef_t* pr_fielddefs;
extern dstatement_t* pr_statements;
extern float* pr_globals;			// same as pr_global_struct
extern int pr_edict_size;			// in bytes
extern progGlobalVars_t pr_globalVars;

extern builtin_t* pr_builtins;
extern int pr_numbuiltins;
extern prstack_t pr_stack[MAX_STACK_DEPTH];
extern int pr_depth;
extern int localstack[LOCALSTACK_SIZE];
extern int localstack_used;
extern bool pr_trace;
extern dfunction_t* pr_xfunction;
extern int pr_xstatement;
extern int pr_argc;

void PR_ClearStringMap();
int PR_SetString(const char* string);
const char* PR_GetString(int number);
// returns a copy of the string allocated from the server's string heap
const char* ED_NewString(const char* string);

ddef_t* ED_FieldAtOfs(int offset);
ddef_t* ED_FindField(const char* name);
ddef_t* ED_FindGlobal(const char* name);
dfunction_t* ED_FindFunction(const char* name);
dfunction_t* ED_FindFunctioni(const char* name);

const char* PR_ValueString(etype_t type, const eval_t* val);
const char* PR_UglyValueString(etype_t type, const eval_t* val);
const char* PR_GlobalString(int ofs);
const char* PR_GlobalStringNoContents(int ofs);

void ED_WriteGlobals(fileHandle_t f);
bool ED_ParseEpair(void* base, const ddef_t* key, const char* s);
const char* ED_ParseGlobals(const char* data);
void PR_InitGlobals();

void PR_PrintStatement(const dstatement_t* s);
void PR_RunError(const char* error, ...)  id_attribute((format(printf, 1, 2)));
int PR_EnterFunction(dfunction_t* f);
int PR_LeaveFunction();
bool PR_ExecuteProgramCommon(const dstatement_t* st, eval_t* a, eval_t* b, eval_t* c, int& s, int exitdepth);

#endif
