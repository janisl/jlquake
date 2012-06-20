// sv_edict.c -- entity dictionary

#include "qwsvdef.h"

/*
===============
PR_Init
===============
*/
void PR_Init(void)
{
	PR_InitBuiltins();
	PR_SharedInit();
}
