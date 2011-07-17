// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	R_FreeModels();
	R_ModelInit();
}

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	Con_Printf ("Cached models:\n");
	for (int i = 0; i < tr.numModels; i++)
	{
		Con_Printf ("%8p : %s\n", tr.models[i]->q1_cache, tr.models[i]->name);
	}
}
