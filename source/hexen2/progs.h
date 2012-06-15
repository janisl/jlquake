
#include "../server/progsvm/progsvm.h"	// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,qhedict_t,area)

//============================================================================

extern globalvars_t* pr_global_struct;

extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_Init(void);

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs(void);
void PR_LoadStrings(void);
void PR_LoadInfoStrings(void);

void PR_Profile_f(void);

qhedict_t* ED_Alloc(void);
qhedict_t* ED_Alloc_Temp(void);
void ED_Free(qhedict_t* ed);

void ED_LoadFromFile(const char* data);
void PR_InitBuiltins();

//============================================================================

extern unsigned short pr_crc;

extern Cvar* max_temp_edicts;

extern qboolean ignore_precache;
